// Doc Encode : UTF-8 BOM, Unix(LF)
// pfs_ctx: init/deinit 与 mount/remount/umount (经 pfs_fs_ops 封装分派); 外部 FS 注册
#include "vfs/pfs_ctx.h"
#include "pfs_compiler.h"
#include "base/pfs_mem.h"

#include <string.h>
#include <stdbool.h>

static bool pfs_ctx_mode_ok(int mode)
{
    if ((PFS_MNT_RDONLY == mode) || (PFS_MNT_RDWR == mode))
    {
        return true;
    }
    return false;
}

static bool pfs_ctx_is_mounted(const pfs_ctx_t *p_ctx)
{
    if (pfs_unlikely(NULL == p_ctx))
    {
        return false;
    }
    return (PFS_NONE != p_ctx->fs_type && NULL != p_ctx->p_fs_ops && NULL != p_ctx->p_sbi);
}

// 库内建类型: 由本文件硬编码探测序处理, 禁止经 register 覆盖
static bool pfs_ctx_fs_type_is_builtin(pfs_fs_type_e fs_type)
{
    switch (fs_type)
    {
    case PFS_FAT12:
    case PFS_FAT16:
    case PFS_FAT32:
    case PFS_EXFAT:
    case PFS_NTFS:
    case PFS_ISOFS:
    case PFS_UDF:
    case PFS_EXT2:
    case PFS_EXT3:
    case PFS_EXT4:
        return true;
    default:
        break;
    }
    return false;
}

static void pfs_ctx_clear_mount(pfs_ctx_t *p_ctx)
{
    if (p_ctx->sbi_owned && NULL != p_ctx->p_sbi)
    {
        pfs_free(p_ctx->p_sbi);
    }
    p_ctx->fs_type = PFS_NONE;
    p_ctx->p_blkio = NULL;
    p_ctx->part_offset = 0;
    p_ctx->p_fs_ops = NULL;
    p_ctx->p_sbi = NULL;
    p_ctx->sbi_owned = 0;
}

static const pfs_fs_ops_t *pfs_ctx_fat_ops_by_bits(uint8_t fat_bits)
{
    if (32 == fat_bits)
    {
        return &g_pfs_fat32_fs_ops;
    }
    if (16 == fat_bits)
    {
        return &g_pfs_fat16_fs_ops;
    }
    if (12 == fat_bits)
    {
        return &g_pfs_fat12_fs_ops;
    }
    return NULL;
}

// extents -> ext4; journal(无 extents) -> ext3; 否则 ext2
static const pfs_fs_ops_t *pfs_ctx_ext_ops_by_features(const pfs_ext4_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return NULL;
    }
    if (0u != (p_sbi->feature_incompat & PFS_EXT4_FEATURE_INCOMPAT_EXTENTS))
    {
        return &g_pfs_ext4_fs_ops;
    }
    if (0u != (p_sbi->feature_compat & PFS_EXT4_FEATURE_COMPAT_HAS_JOURNAL))
    {
        return &g_pfs_ext3_fs_ops;
    }
    return &g_pfs_ext2_fs_ops;
}

static int pfs_ctx_bind_mounted(pfs_ctx_t *p_ctx, const pfs_fs_ops_t *p_ops, void *p_sbi,
        pfs_blkio_t *p_blkio, int64_t part_offset, int sbi_owned)
{
    p_ctx->p_fs_ops = p_ops;
    p_ctx->p_sbi = p_sbi;
    p_ctx->sbi_owned = sbi_owned ? 1 : 0;
    p_ctx->fs_type = p_ops->fs_type;
    p_ctx->p_blkio = p_blkio;
    p_ctx->part_offset = part_offset;
    return PFS_OK;
}

// 尝试已注册外部表: 探测成功则用 fs 内存 (过大则堆) 挂载; 探测均失败返回 PFS_ENOEXEC
static int pfs_ctx_mount_registered(pfs_ctx_t *p_ctx, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    for (int i = 0; i < p_ctx->fs_ops_count; i++)
    {
        const pfs_fs_ops_t *p_ops = p_ctx->fs_ops[i];
        if (NULL == p_ops)
        {
            continue;
        }
        if (PFS_OK != pfs_fs_ctrl_probe(p_ops, p_blkio, part_offset))
        {
            continue;
        }

        int sbi_size = p_ops->super.sbi_size;
        if (sbi_size <= 0)
        {
            return PFS_EINVAL;
        }

        void *p_sbi;
        int sbi_owned;

        // 优先复用 ctx.fs (含 sbi_buf); 超过 sizeof(fs) 才堆分配
        if (sbi_size <= (int)sizeof(p_ctx->fs))
        {
            memset(&p_ctx->fs, 0, sizeof(p_ctx->fs));
            p_sbi = &p_ctx->fs;
            sbi_owned = 0;
        }
        else
        {
            p_sbi = pfs_mallocz((size_t)sbi_size);
            if (NULL == p_sbi)
            {
                return PFS_ENOMEM;
            }
            sbi_owned = 1;
        }

        int rc = pfs_fs_super_init(p_ops, p_sbi);
        if (PFS_OK != rc)
        {
            if (sbi_owned)
            {
                pfs_free(p_sbi);
            }
            return rc;
        }
        rc = pfs_fs_super_mount(p_ops, p_sbi, p_blkio, part_offset, mode);
        if (PFS_OK != rc)
        {
            pfs_fs_super_deinit(p_ops, p_sbi);
            if (sbi_owned)
            {
                pfs_free(p_sbi);
            }
            // 探测命中但挂载失败: 继续下一注册表 / 内建序 (勿堵死; 如压缩不支持)
            // OOM 仍 fail-fast
            if (PFS_ENOMEM == rc)
            {
                return rc;
            }
            continue;
        }
        return pfs_ctx_bind_mounted(p_ctx, p_ops, p_sbi, p_blkio, part_offset, sbi_owned);
    }
    return PFS_ENOEXEC;
}

int pfs_ctx_init(pfs_ctx_t *p_ctx)
{
    if (pfs_unlikely(NULL == p_ctx))
    {
        return PFS_EINVAL;
    }

    memset(p_ctx, 0, sizeof(*p_ctx));
    pfs_ctx_clear_mount(p_ctx);
    return PFS_OK;
}

void pfs_ctx_deinit(pfs_ctx_t *p_ctx)
{
    if (pfs_unlikely(NULL == p_ctx))
    {
        return;
    }

    if (pfs_ctx_is_mounted(p_ctx))
    {
        (void)pfs_ctx_umount(p_ctx);
    }

    memset(p_ctx, 0, sizeof(*p_ctx));
    pfs_ctx_clear_mount(p_ctx);
}

int pfs_ctx_register_fs(pfs_ctx_t *p_ctx, pfs_fs_type_e fs_type, const pfs_fs_ops_t *p_fs_ops)
{
    if (pfs_unlikely(NULL == p_ctx || NULL == p_fs_ops))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(PFS_NONE == fs_type || fs_type > PFS_FS_TYPE_MAX))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(fs_type != p_fs_ops->fs_type))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(pfs_ctx_fs_type_is_builtin(fs_type)))
    {
        return PFS_EPERM;
    }
    if (pfs_unlikely(p_fs_ops->super.sbi_size <= 0))
    {
        return PFS_EINVAL;
    }

    for (int i = 0; i < p_ctx->fs_ops_count; i++)
    {
        if (NULL != p_ctx->fs_ops[i] && fs_type == p_ctx->fs_ops[i]->fs_type)
        {
            return PFS_EEXIST;
        }
    }
    if (pfs_unlikely(p_ctx->fs_ops_count >= PFS_CTX_FS_OPS_MAX))
    {
        return PFS_ERANGE;
    }

    p_ctx->fs_ops[p_ctx->fs_ops_count] = p_fs_ops;
    p_ctx->fs_ops_count++;
    return PFS_OK;
}

int pfs_ctx_mount(pfs_ctx_t *p_ctx, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    const pfs_fs_ops_t *p_fat_ops;
    const pfs_fs_ops_t *p_ext_ops;

    if (pfs_unlikely((NULL == p_ctx) || (NULL == p_blkio) || (part_offset < 0) ||
            !pfs_ctx_mode_ok(mode)))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(pfs_ctx_is_mounted(p_ctx)))
    {
        return PFS_EBUSY;
    }

    // 先试本 ctx 注册的外部 FS (注册序)
    int rc = pfs_ctx_mount_registered(p_ctx, p_blkio, part_offset, mode);
    if (PFS_ENOEXEC != rc)
    {
        return rc;
    }

    // 先试 exFAT
    if (PFS_OK == pfs_fs_ctrl_probe(&g_pfs_exfat_fs_ops, p_blkio, part_offset))
    {
        rc = pfs_fs_super_init(&g_pfs_exfat_fs_ops, &p_ctx->fs.exfat);
        if (PFS_OK != rc)
        {
            return rc;
        }
        rc = pfs_fs_super_mount(&g_pfs_exfat_fs_ops, &p_ctx->fs.exfat, p_blkio, part_offset, mode);
        if (PFS_OK == rc)
        {
            return pfs_ctx_bind_mounted(p_ctx, &g_pfs_exfat_fs_ops, &p_ctx->fs.exfat,
                    p_blkio, part_offset, 0);
        }
        pfs_fs_super_deinit(&g_pfs_exfat_fs_ops, &p_ctx->fs.exfat);
        return rc;
    }

    // 再试 NTFS (OEM 明确; 须先于 FAT, 避免 BPB 误判)
    if (PFS_OK == pfs_fs_ctrl_probe(&g_pfs_ntfs_fs_ops, p_blkio, part_offset))
    {
        rc = pfs_fs_super_init(&g_pfs_ntfs_fs_ops, &p_ctx->fs.ntfs);
        if (PFS_OK != rc)
        {
            return rc;
        }
        rc = pfs_fs_super_mount(&g_pfs_ntfs_fs_ops, &p_ctx->fs.ntfs, p_blkio, part_offset, mode);
        if (PFS_OK == rc)
        {
            return pfs_ctx_bind_mounted(p_ctx, &g_pfs_ntfs_fs_ops, &p_ctx->fs.ntfs,
                    p_blkio, part_offset, 0);
        }
        pfs_fs_super_deinit(&g_pfs_ntfs_fs_ops, &p_ctx->fs.ntfs);
        return rc;
    }

    // 再试 UDF (AVDP; 先于 isofs 避免 CD001 误判)
    if (PFS_OK == pfs_fs_ctrl_probe(&g_pfs_udf_fs_ops, p_blkio, part_offset))
    {
        rc = pfs_fs_super_init(&g_pfs_udf_fs_ops, &p_ctx->fs.udf);
        if (PFS_OK != rc)
        {
            return rc;
        }
        rc = pfs_fs_super_mount(&g_pfs_udf_fs_ops, &p_ctx->fs.udf, p_blkio, part_offset, mode);
        if (PFS_OK == rc)
        {
            return pfs_ctx_bind_mounted(p_ctx, &g_pfs_udf_fs_ops, &p_ctx->fs.udf,
                    p_blkio, part_offset, 0);
        }
        pfs_fs_super_deinit(&g_pfs_udf_fs_ops, &p_ctx->fs.udf);
        return rc;
    }

    // 再试 ISO 9660 (CD001; 先于 FAT 避免误判)
    if (PFS_OK == pfs_fs_ctrl_probe(&g_pfs_isofs_fs_ops, p_blkio, part_offset))
    {
        rc = pfs_fs_super_init(&g_pfs_isofs_fs_ops, &p_ctx->fs.isofs);
        if (PFS_OK != rc)
        {
            return rc;
        }
        rc = pfs_fs_super_mount(&g_pfs_isofs_fs_ops, &p_ctx->fs.isofs, p_blkio, part_offset, mode);
        if (PFS_OK == rc)
        {
            return pfs_ctx_bind_mounted(p_ctx, &g_pfs_isofs_fs_ops, &p_ctx->fs.isofs,
                    p_blkio, part_offset, 0);
        }
        pfs_fs_super_deinit(&g_pfs_isofs_fs_ops, &p_ctx->fs.isofs);
        return rc;
    }

    // 再试 ext2/3/4 (magic 0xEF53; 先于 FAT; 成功后再按 feature 换表)
    if (PFS_OK == pfs_fs_ctrl_probe(&g_pfs_ext4_fs_ops, p_blkio, part_offset))
    {
        rc = pfs_fs_super_init(&g_pfs_ext4_fs_ops, &p_ctx->fs.ext);
        if (PFS_OK != rc)
        {
            return rc;
        }
        rc = pfs_fs_super_mount(&g_pfs_ext4_fs_ops, &p_ctx->fs.ext, p_blkio, part_offset, mode);
        if (PFS_OK != rc)
        {
            pfs_fs_super_deinit(&g_pfs_ext4_fs_ops, &p_ctx->fs.ext);
            return rc;
        }
        p_ext_ops = pfs_ctx_ext_ops_by_features(&p_ctx->fs.ext);
        if (NULL == p_ext_ops)
        {
            (void)pfs_fs_super_umount(&g_pfs_ext4_fs_ops, &p_ctx->fs.ext);
            pfs_fs_super_deinit(&g_pfs_ext4_fs_ops, &p_ctx->fs.ext);
            return PFS_EINVAL;
        }
        return pfs_ctx_bind_mounted(p_ctx, p_ext_ops, &p_ctx->fs.ext, p_blkio, part_offset, 0);
    }

    // 再试 FAT (入口用 fat32 表共享 probe/mount; 成功后再按 fat_bits 换 12/16/32 表)
    if (PFS_OK == pfs_fs_ctrl_probe(&g_pfs_fat32_fs_ops, p_blkio, part_offset))
    {
        rc = pfs_fs_super_init(&g_pfs_fat32_fs_ops, &p_ctx->fs.fat);
        if (PFS_OK != rc)
        {
            return rc;
        }
        rc = pfs_fs_super_mount(&g_pfs_fat32_fs_ops, &p_ctx->fs.fat, p_blkio, part_offset, mode);
        if (PFS_OK != rc)
        {
            pfs_fs_super_deinit(&g_pfs_fat32_fs_ops, &p_ctx->fs.fat);
            return rc;
        }
        p_fat_ops = pfs_ctx_fat_ops_by_bits(p_ctx->fs.fat.bs.fat_bits);
        if (NULL == p_fat_ops)
        {
            (void)pfs_fs_super_umount(&g_pfs_fat32_fs_ops, &p_ctx->fs.fat);
            pfs_fs_super_deinit(&g_pfs_fat32_fs_ops, &p_ctx->fs.fat);
            return PFS_EOPNOTSUPP;
        }
        return pfs_ctx_bind_mounted(p_ctx, p_fat_ops, &p_ctx->fs.fat, p_blkio, part_offset, 0);
    }

    return PFS_ENOEXEC;
}

int pfs_ctx_remount(pfs_ctx_t *p_ctx, int mode)
{
    if (pfs_unlikely((NULL == p_ctx) || !pfs_ctx_mode_ok(mode)))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(!pfs_ctx_is_mounted(p_ctx)))
    {
        return PFS_EBUSY;
    }
    return pfs_fs_super_remount(p_ctx->p_fs_ops, p_ctx->p_sbi, mode);
}

int pfs_ctx_umount(pfs_ctx_t *p_ctx)
{

    if (pfs_unlikely(NULL == p_ctx))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(!pfs_ctx_is_mounted(p_ctx)))
    {
        return PFS_EBUSY;
    }

    int rc = pfs_fs_super_umount(p_ctx->p_fs_ops, p_ctx->p_sbi);
    pfs_fs_super_deinit(p_ctx->p_fs_ops, p_ctx->p_sbi);
    pfs_ctx_clear_mount(p_ctx);
    return rc;
}

// end
