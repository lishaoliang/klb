// Doc Encode : UTF-8 BOM, Unix(LF)
// pfs 对外 API: strerror + ctx 堆分配 / 释放 + mkfs (mkfs 经 pfs_fs_ops; mkvol 见 pfs_mkvol.c)
#include "pfs.h"
#include "pfs_compiler.h"
#include "vfs/pfs_ctx.h"
#include "base/pfs_mem.h"
#include "fat/pfs_fat.h"
#include "exfat/pfs_exfat.h"
#include "ntfs/pfs_ntfs.h"
#include "isofs/pfs_isofs.h"
#include "udf/pfs_udf.h"

const char *pfs_version(void)
{
    return PFS_VERSION;
}

int pfs_version_number(void)
{
    return PFS_VERSION_NUMBER;
}

int pfs_version_date(void)
{
    return PFS_VERSION_DATE;
}

const char *pfs_strerror(int errnum)
{
    switch (errnum)
    {
    case PFS_OK:
        return "[pfs] Success";
    case PFS_EPERM:
        return "[pfs] Operation not permitted";
    case PFS_ENOENT:
        return "[pfs] No such file or directory";
    case PFS_EIO:
        return "[pfs] Input/output error";
    case PFS_ENOEXEC:
        return "[pfs] Exec format error";
    case PFS_ENOMEM:
        return "[pfs] Cannot allocate memory";
    case PFS_EBUSY:
        return "[pfs] Device or resource busy";
    case PFS_EEXIST:
        return "[pfs] File exists";
    case PFS_ENOTDIR:
        return "[pfs] Not a directory";
    case PFS_EISDIR:
        return "[pfs] Is a directory";
    case PFS_EINVAL:
        return "[pfs] Invalid argument";
    case PFS_ENOSPC:
        return "[pfs] No space left on device";
    case PFS_EROFS:
        return "[pfs] Read-only file system";
    case PFS_ERANGE:
        return "[pfs] Numerical result out of range";
    case PFS_ENAMETOOLONG:
        return "[pfs] File name too long";
    case PFS_ENOTEMPTY:
        return "[pfs] Directory not empty";
    case PFS_EBADMSG:
        return "[pfs] Bad message";
    case PFS_EILSEQ:
        return "[pfs] Invalid or incomplete multibyte or wide character";
    case PFS_EOPNOTSUPP:
        return "[pfs] Operation not supported";
    default:
        break;
    }
    return "[pfs] Unknown error";
}

pfs_ctx_t *pfs_ctx_create(void)
{
    pfs_ctx_t *p_ctx = (pfs_ctx_t *)pfs_mallocz(sizeof(*p_ctx));
    if (pfs_unlikely(NULL == p_ctx))
    {
        return NULL;
    }

    if (pfs_unlikely(PFS_OK != pfs_ctx_init(p_ctx)))
    {
        pfs_free(p_ctx);
        return NULL;
    }
    return p_ctx;
}

void pfs_ctx_destroy(pfs_ctx_t *p_ctx)
{
    if (pfs_unlikely(NULL == p_ctx))
    {
        return;
    }

    pfs_ctx_deinit(p_ctx);
    pfs_free(p_ctx);
}

pfs_fs_type_e pfs_ctx_fs_type(const pfs_ctx_t *p_ctx)
{
    if (pfs_unlikely(NULL == p_ctx))
    {
        return PFS_NONE;
    }
    return p_ctx->fs_type;
}

const char *pfs_ctx_fs_name(const pfs_ctx_t *p_ctx)
{
    if (pfs_unlikely(NULL == p_ctx))
    {
        return NULL;
    }
    if (PFS_NONE == p_ctx->fs_type || NULL == p_ctx->p_fs_ops)
    {
        return NULL;
    }
    return p_ctx->p_fs_ops->name;
}

static const pfs_fs_ops_t *pfs_mkfs_ops_by_type(pfs_fs_type_e fs_type)
{
    switch (fs_type)
    {
    case PFS_FAT12:
        return &g_pfs_fat12_fs_ops;
    case PFS_FAT16:
        return &g_pfs_fat16_fs_ops;
    case PFS_FAT32:
        return &g_pfs_fat32_fs_ops;
    case PFS_EXFAT:
        return &g_pfs_exfat_fs_ops;
    case PFS_NTFS:
        return &g_pfs_ntfs_fs_ops;
    case PFS_ISOFS:
        return &g_pfs_isofs_fs_ops;
    case PFS_UDF:
        return &g_pfs_udf_fs_ops;
    case PFS_EXT2:
        return &g_pfs_ext2_fs_ops;
    case PFS_EXT3:
        return &g_pfs_ext3_fs_ops;
    case PFS_EXT4:
        return &g_pfs_ext4_fs_ops;
    default:
        break;
    }
    return NULL;
}

static uint64_t pfs_mkfs_clamp_vol_bytes(pfs_fs_type_e fs_type, uint64_t vol_bytes)
{
    if (PFS_FAT12 == fs_type && vol_bytes > PFS_FAT12_VOL_BYTES_MAX)
    {
        return PFS_FAT12_VOL_BYTES_MAX;
    }
    if (PFS_FAT16 == fs_type && vol_bytes > PFS_FAT16_VOL_BYTES_MAX)
    {
        return PFS_FAT16_VOL_BYTES_MAX;
    }
    if (PFS_FAT32 == fs_type && vol_bytes > PFS_FAT32_VOL_BYTES_MAX)
    {
        return PFS_FAT32_VOL_BYTES_MAX;
    }
    if (PFS_EXFAT == fs_type && vol_bytes > PFS_EXFAT_VOL_BYTES_MAX)
    {
        return PFS_EXFAT_VOL_BYTES_MAX;
    }
    if (PFS_NTFS == fs_type && vol_bytes > PFS_NTFS_VOL_BYTES_MAX)
    {
        return PFS_NTFS_VOL_BYTES_MAX;
    }
    if (PFS_ISOFS == fs_type && vol_bytes > PFS_ISOFS_VOL_BYTES_MAX)
    {
        return PFS_ISOFS_VOL_BYTES_MAX;
    }
    if (PFS_UDF == fs_type && vol_bytes > PFS_UDF_VOL_BYTES_MAX)
    {
        return PFS_UDF_VOL_BYTES_MAX;
    }
    if ((PFS_EXT2 == fs_type || PFS_EXT3 == fs_type || PFS_EXT4 == fs_type) &&
            vol_bytes > PFS_EXT4_VOL_BYTES_MAX)
    {
        return PFS_EXT4_VOL_BYTES_MAX;
    }
    return vol_bytes;
}

int pfs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, pfs_fs_type_e fs_type)
{
    const pfs_fs_ops_t *p_ops;

    if (pfs_unlikely(NULL == p_blkio || part_offset < 0))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(!pfs_blkio_is_open(p_blkio)))
    {
        return PFS_EINVAL;
    }

    p_ops = pfs_mkfs_ops_by_type(fs_type);
    if (NULL == p_ops)
    {
        return PFS_EINVAL;
    }
    vol_bytes = pfs_mkfs_clamp_vol_bytes(fs_type, vol_bytes);
    return pfs_fs_ctrl_mkfs(p_ops, p_blkio, part_offset, vol_bytes);
}

// end
