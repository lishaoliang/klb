// Doc Encode : UTF-8 BOM, Unix(LF)
// ubifs 空盘 mkfs (SB+MST+CS+根 INO; 扫描写子集)
#include "ubifs/pfs_ubifs_internal.h"
#include "pfsmtd.h"
#include "base/pfsmtd_mem.h"
#include "base/pfsmtd_dirwalk.h"

#include <string.h>
#include <stdbool.h>

static int pfs_ubifs_mk_resolve_geom(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes,
        int *p_leb_size, int *p_leb_cnt)
{
    int mtd_ls = pfsmtd_ubi_leb_size(p_blkio);
    int mtd_lc = pfsmtd_ubi_leb_count(p_blkio);
    int leb_size;
    int leb_cnt;

    if (0 < mtd_ls && 0 < mtd_lc)
    {
        leb_size = mtd_ls;
        leb_cnt = mtd_lc;
    }
    else
    {
        int64_t avail = pfs_blkio_get_size(p_blkio);
        if (0 > avail)
        {
            return PFS_EIO;
        }
        if (0 < part_offset)
        {
            if (part_offset >= avail)
            {
                return PFS_EINVAL;
            }
            avail -= part_offset;
        }
        if (0 != vol_bytes && (uint64_t)avail > vol_bytes)
        {
            avail = (int64_t)vol_bytes;
        }
        leb_size = 128 * 1024;
        if (PFS_UBIFS_MIN_LEB_SZ > leb_size)
        {
            leb_size = PFS_UBIFS_MIN_LEB_SZ;
        }
        leb_cnt = (int)(avail / (int64_t)leb_size);
    }

    if (PFS_UBIFS_MIN_LEB_SZ > leb_size || PFS_UBIFS_MIN_LEB_CNT > leb_cnt)
    {
        return PFS_EINVAL;
    }
    *p_leb_size = leb_size;
    *p_leb_cnt = leb_cnt;
    return PFS_OK;
}

static void pfs_ubifs_mk_fill_ino(uint8_t *ino, uint64_t inum, uint32_t mode, uint32_t nlink)
{
    pfs_ubifs_key_t key;

    memset(ino, 0, PFS_UBIFS_INO_NODE_SZ);
    pfs_ubifs_ino_key_init(&key, inum);
    pfs_ubifs_key_write_flash(ino + PFS_UBIFS_OFF_KEY, &key);
    pfs_ubifs_put_le64(ino + PFS_UBIFS_INO_OFF_CREAT_SQ, 1);
    pfs_ubifs_put_le64(ino + PFS_UBIFS_INO_OFF_SIZE, 0);
    pfs_ubifs_put_le32(ino + PFS_UBIFS_INO_OFF_NLINK, nlink);
    pfs_ubifs_put_le32(ino + PFS_UBIFS_INO_OFF_MODE, mode);
    pfs_ubifs_put_le32(ino + PFS_UBIFS_INO_OFF_DATA_LEN, 0);
    pfs_ubifs_put_le16(ino + PFS_UBIFS_INO_OFF_COMPR, PFS_UBIFS_COMPR_NONE);
    ino[20] = PFS_UBIFS_INO_NODE;
}

int pfs_ubifs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    if (NULL == p_blkio || !pfs_blkio_is_open(p_blkio) || 0 > part_offset)
    {
        return PFS_EINVAL;
    }

    int leb_size;
    int leb_cnt;
    int rc = pfs_ubifs_mk_resolve_geom(p_blkio, part_offset, vol_bytes, &leb_size, &leb_cnt);
    if (PFS_OK != rc)
    {
        return rc;
    }

    int log_lebs = PFS_UBIFS_MIN_LOG_LEBS;
    int lpt_lebs = PFS_UBIFS_MIN_LPT_LEBS;
    int orph_lebs = PFS_UBIFS_MIN_ORPH_LEBS;
    int main_first = PFS_UBIFS_SB_LEBS + PFS_UBIFS_MST_LEBS + log_lebs + lpt_lebs + orph_lebs;
    int main_lebs = leb_cnt - main_first;
    if (PFS_UBIFS_MIN_MAIN_LEBS > main_lebs)
    {
        return PFS_EINVAL;
    }

    pfs_ubifs_sbi_t sbi;
    memset(&sbi, 0, sizeof(sbi));
    sbi.p_blkio = p_blkio;
    sbi.part_offset = part_offset;
    sbi.leb_size = leb_size;
    sbi.leb_cnt = leb_cnt;
    sbi.main_lebs = main_lebs;
    sbi.main_first = main_first;

    uint8_t *sb = (uint8_t *)pfsmtd_mallocz((size_t)PFS_UBIFS_SB_NODE_SZ);
    uint8_t *mst = (uint8_t *)pfsmtd_mallocz((size_t)PFS_UBIFS_MST_NODE_SZ);
    uint8_t *cs = (uint8_t *)pfsmtd_mallocz((size_t)PFS_UBIFS_CS_NODE_SZ);
    uint8_t *ino = (uint8_t *)pfsmtd_mallocz((size_t)PFS_UBIFS_INO_NODE_SZ);
    if (NULL == sb || NULL == mst || NULL == cs || NULL == ino)
    {
        pfsmtd_free(sb);
        pfsmtd_free(mst);
        pfsmtd_free(cs);
        pfsmtd_free(ino);
        return PFS_ENOMEM;
    }

    sb[24 + 2] = PFS_UBIFS_KEY_HASH_R5;
    sb[24 + 3] = PFS_UBIFS_SIMPLE_KEY_FMT;
    pfs_ubifs_put_le32(sb + 28, 0);
    pfs_ubifs_put_le32(sb + 32, 8);
    pfs_ubifs_put_le32(sb + 36, (uint32_t)leb_size);
    pfs_ubifs_put_le32(sb + 40, (uint32_t)leb_cnt);
    pfs_ubifs_put_le32(sb + 44, (uint32_t)leb_cnt);
    pfs_ubifs_put_le64(sb + 48, (uint64_t)leb_size * (uint64_t)(main_lebs / 2));
    pfs_ubifs_put_le32(sb + 56, (uint32_t)log_lebs);
    pfs_ubifs_put_le32(sb + 60, (uint32_t)lpt_lebs);
    pfs_ubifs_put_le32(sb + 64, (uint32_t)orph_lebs);
    pfs_ubifs_put_le32(sb + 68, 1);
    pfs_ubifs_put_le32(sb + 72, 8);
    pfs_ubifs_put_le32(sb + 76, 0);
    pfs_ubifs_put_le32(sb + 80, PFS_UBIFS_FORMAT_VERSION);
    pfs_ubifs_put_le16(sb + 84, PFS_UBIFS_COMPR_NONE);
    pfs_ubifs_put_le32(sb + 104, 1000000000u);
    for (int i = 0; i < 16; i++)
    {
        sb[108 + i] = (uint8_t)(0x10u + i);
    }
    pfs_ubifs_put_le32(sb + 124, PFS_UBIFS_RO_COMPAT_VERSION);
    pfs_ubifs_prepare_node(sb, PFS_UBIFS_SB_NODE_SZ, PFS_UBIFS_SB_NODE, 1);
    rc = pfs_ubifs_write_node(&sbi, PFS_UBIFS_SB_LNUM, 0, sb, PFS_UBIFS_SB_NODE_SZ);
    if (PFS_OK != rc)
    {
        goto out;
    }

    // 根 INO @ main_first
    pfs_ubifs_mk_fill_ino(ino, PFS_UBIFS_ROOT_INO, PFS_UBIFS_S_IFDIR | 0755, 2);
    pfs_ubifs_prepare_node(ino, PFS_UBIFS_INO_NODE_SZ, PFS_UBIFS_INO_NODE, 4);
    rc = pfs_ubifs_write_node(&sbi, main_first, 0, ino, PFS_UBIFS_INO_NODE_SZ);
    if (PFS_OK != rc)
    {
        goto out;
    }

    pfs_ubifs_put_le64(mst + 24, PFS_UBIFS_FIRST_INO);
    pfs_ubifs_put_le64(mst + 32, 0);
    pfs_ubifs_put_le32(mst + 40, PFS_UBIFS_MST_NO_ORPHS);
    pfs_ubifs_put_le32(mst + 44, PFS_UBIFS_LOG_LNUM);
    pfs_ubifs_put_le32(mst + 48, (uint32_t)main_first);
    pfs_ubifs_put_le32(mst + 52, 0);
    pfs_ubifs_put_le32(mst + 56, PFS_UBIFS_INO_NODE_SZ);
    pfs_ubifs_put_le32(mst + 60, (uint32_t)-1);
    pfs_ubifs_put_le32(mst + 156, (uint32_t)(main_lebs - 1));
    pfs_ubifs_put_le32(mst + 160, 0);
    pfs_ubifs_put_le32(mst + 164, (uint32_t)leb_cnt);
    pfs_ubifs_prepare_node(mst, PFS_UBIFS_MST_NODE_SZ, PFS_UBIFS_MST_NODE, 2);
    rc = pfs_ubifs_write_node(&sbi, PFS_UBIFS_MST_LNUM, 0, mst, PFS_UBIFS_MST_NODE_SZ);
    if (PFS_OK != rc)
    {
        goto out;
    }
    rc = pfs_ubifs_write_node(&sbi, PFS_UBIFS_MST_LNUM + 1, 0, mst, PFS_UBIFS_MST_NODE_SZ);
    if (PFS_OK != rc)
    {
        goto out;
    }

    pfs_ubifs_put_le64(cs + 24, 0);
    pfs_ubifs_prepare_node(cs, PFS_UBIFS_CS_NODE_SZ, PFS_UBIFS_CS_NODE, 3);
    rc = pfs_ubifs_write_node(&sbi, PFS_UBIFS_LOG_LNUM, 0, cs, PFS_UBIFS_CS_NODE_SZ);

out:
    pfsmtd_free(sb);
    pfsmtd_free(mst);
    pfsmtd_free(cs);
    pfsmtd_free(ino);
    return rc;
}

typedef struct _pfs_ubifs_mkvol_ctx_t {
    pfs_ubifs_sbi_t        *p_sbi;
} pfs_ubifs_mkvol_ctx_t;

static int pfs_ubifs_mkvol_on_entry(void *ctx, const char *vfs_rel, const char *host_path, bool is_dir)
{
    pfs_ubifs_mkvol_ctx_t *p_ctx = (pfs_ubifs_mkvol_ctx_t *)ctx;
    if (is_dir)
    {
        return pfs_ubifs_mkdir(p_ctx->p_sbi, vfs_rel, 0755);
    }

    uint8_t *data;
    uint32_t size;
    int rc = pfsmtd_host_read_file(host_path, &data, &size);
    if (PFS_OK != rc)
    {
        return rc;
    }
    pfs_ubifs_file_meta_t fm;
    rc = pfs_ubifs_file_open(p_ctx->p_sbi, vfs_rel, PFS_O_RDWR | PFS_O_CREAT, &fm);
    if (PFS_OK != rc)
    {
        pfsmtd_free(data);
        return rc;
    }
    int64_t pos = 0;
    while ((uint32_t)pos < size)
    {
        int chunk = (int)(size - (uint32_t)pos);
        if (chunk > PFS_UBIFS_BLOCK_SIZE)
        {
            chunk = PFS_UBIFS_BLOCK_SIZE;
        }
        int put;
        rc = pfs_ubifs_file_write(p_ctx->p_sbi, &fm, pos, data + pos, chunk, &put);
        if (PFS_OK != rc || put != chunk)
        {
            pfs_ubifs_file_close(p_ctx->p_sbi, &fm);
            pfsmtd_free(data);
            return (PFS_OK != rc) ? rc : PFS_EIO;
        }
        pos += chunk;
    }
    pfs_ubifs_file_close(p_ctx->p_sbi, &fm);
    pfsmtd_free(data);
    return PFS_OK;
}

int pfs_ubifs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id)
{
    (void)volume_id;
    if (NULL == src_dir || '\0' == src_dir[0])
    {
        return PFS_EINVAL;
    }

    pfs_ubifs_sbi_t sbi;
    pfs_ubifs_mkvol_ctx_t ctx;
    int rc = pfs_ubifs_mkfs(p_blkio, part_offset, vol_bytes);
    if (PFS_OK != rc)
    {
        return rc;
    }
    rc = pfs_ubifs_init(&sbi);
    if (PFS_OK != rc)
    {
        return rc;
    }
    rc = pfs_ubifs_mount(&sbi, p_blkio, part_offset, PFS_MNT_RDWR);
    if (PFS_OK != rc)
    {
        pfs_ubifs_deinit(&sbi);
        return rc;
    }
    ctx.p_sbi = &sbi;
    rc = pfsmtd_dirwalk(src_dir, pfs_ubifs_mkvol_on_entry, &ctx);
    (void)pfs_ubifs_umount(&sbi);
    pfs_ubifs_deinit(&sbi);
    return rc;
}

// end
