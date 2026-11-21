// Doc Encode : UTF-8 BOM, Unix(LF)
// ubifs 卷生命周期 (只读/读写; 扫描重建缓存)
#include "ubifs/pfs_ubifs_internal.h"
#include "pfsmtd.h"
#include "base/pfsmtd_mem.h"

#include <string.h>

int pfs_ubifs_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_OK;
    }
    if (PFS_BLKIO_RET_PARAM == rc)
    {
        return PFS_EINVAL;
    }
    if (PFS_BLKIO_RET_IO == rc)
    {
        return PFS_EIO;
    }
    if (PFS_BLKIO_RET_STATE == rc)
    {
        return PFS_EBUSY;
    }
    return PFS_EIO;
}

int pfs_ubifs_init(pfs_ubifs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_UBIFS_UNMOUNTED;
    return PFS_OK;
}

void pfs_ubifs_deinit(pfs_ubifs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    pfs_ubifs_cache_free(p_sbi);
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_UBIFS_UNMOUNTED;
}

static int pfs_ubifs_read_sb_raw(pfs_blkio_t *p_blkio, int64_t part_offset, int leb_size,
        uint8_t *sb, int sb_len)
{
    int mtd_ls = pfsmtd_ubi_leb_size(p_blkio);

    if (mtd_ls > 0)
    {
        if (leb_size > 0 && leb_size != mtd_ls)
        {
            return PFS_EINVAL;
        }

        int rc = pfsmtd_ubi_leb_read(p_blkio, PFS_UBIFS_SB_LNUM, 0, sb, sb_len);
        if (rc != sb_len)
        {
            return (rc < 0) ? pfs_ubifs_map_blkio_rc(rc) : PFS_EIO;
        }
        return pfs_ubifs_check_node(sb, sb_len);
    }

    if (leb_size <= 0)
    {
        leb_size = PFS_UBIFS_SB_NODE_SZ;
    }

    int64_t off = part_offset + (int64_t)PFS_UBIFS_SB_LNUM * (int64_t)leb_size;
    int rc = pfs_blkio_pread(p_blkio, sb, sb_len, off);
    if (rc != sb_len)
    {
        return (rc < 0) ? pfs_ubifs_map_blkio_rc(rc) : PFS_EIO;
    }
    return pfs_ubifs_check_node(sb, sb_len);
}

int pfs_ubifs_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    if (NULL == p_blkio || 0 > part_offset)
    {
        return PFS_EINVAL;
    }

    uint8_t *sb = (uint8_t *)pfsmtd_mallocz(PFS_UBIFS_SB_NODE_SZ);

    if (NULL == sb)
    {
        return PFS_ENOMEM;
    }

    int mtd_ls = pfsmtd_ubi_leb_size(p_blkio);
    int rc = pfs_ubifs_read_sb_raw(p_blkio, part_offset, mtd_ls > 0 ? mtd_ls : 0, sb, PFS_UBIFS_SB_NODE_SZ);
    if (PFS_OK != rc)
    {
        pfsmtd_free(sb);
        return (PFS_EIO == rc || PFS_EINVAL == rc) ? PFS_ENOEXEC : rc;
    }

    uint32_t ntype = sb[20];
    if (PFS_UBIFS_SB_NODE != ntype)
    {
        pfsmtd_free(sb);
        return PFS_ENOEXEC;
    }
    if (PFS_UBIFS_FORMAT_VERSION != pfs_ubifs_le32(sb + 80))
    {
        pfsmtd_free(sb);
        return PFS_ENOEXEC;
    }
    pfsmtd_free(sb);
    return PFS_OK;
}

int pfs_ubifs_mount(pfs_ubifs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    if (NULL == p_sbi || NULL == p_blkio || 0 > part_offset)
    {
        return PFS_EINVAL;
    }
    if (PFS_UBIFS_UNMOUNTED != p_sbi->mount_state)
    {
        return PFS_EBUSY;
    }

    int want_rw = (0 != (mode & PFS_MNT_RDWR)) ? 1 : 0;

    if (!want_rw && 0 == (mode & PFS_MNT_RDONLY))
    {
        return PFS_EINVAL;
    }

    uint8_t *sb = (uint8_t *)pfsmtd_mallocz(PFS_UBIFS_SB_NODE_SZ);
    uint8_t *mst = (uint8_t *)pfsmtd_mallocz(PFS_UBIFS_MST_NODE_SZ);

    if (NULL == sb || NULL == mst)
    {
        pfsmtd_free(sb);
        pfsmtd_free(mst);
        return PFS_ENOMEM;
    }

    int mtd_ls = pfsmtd_ubi_leb_size(p_blkio);
    int rc = pfs_ubifs_read_sb_raw(p_blkio, part_offset, mtd_ls > 0 ? mtd_ls : 0, sb, PFS_UBIFS_SB_NODE_SZ);
    if (PFS_OK != rc || PFS_UBIFS_SB_NODE != sb[20])
    {
        rc = PFS_ENOEXEC;
        goto out;
    }

    int leb_size = (int)pfs_ubifs_le32(sb + 36);
    int leb_cnt = (int)pfs_ubifs_le32(sb + 40);
    int log_lebs = (int)pfs_ubifs_le32(sb + 56);
    int lpt_lebs = (int)pfs_ubifs_le32(sb + 60);
    int orph_lebs = (int)pfs_ubifs_le32(sb + 64);
    if (leb_size < PFS_UBIFS_MIN_LEB_SZ || leb_cnt < PFS_UBIFS_MIN_LEB_CNT)
    {
        rc = PFS_EINVAL;
        goto out;
    }

    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    p_sbi->leb_size = leb_size;
    p_sbi->leb_cnt = leb_cnt;
    p_sbi->log_lebs = log_lebs;
    p_sbi->lpt_lebs = lpt_lebs;
    p_sbi->orph_lebs = orph_lebs;
    p_sbi->main_first = PFS_UBIFS_SB_LEBS + PFS_UBIFS_MST_LEBS + log_lebs + lpt_lebs + orph_lebs;
    p_sbi->main_lebs = leb_cnt - p_sbi->main_first;
    if (p_sbi->main_lebs < 1)
    {
        rc = PFS_EINVAL;
        goto out;
    }

    rc = pfs_ubifs_read_node(p_sbi, PFS_UBIFS_MST_LNUM, 0, mst, PFS_UBIFS_MST_NODE_SZ);
    if (PFS_OK != rc || PFS_UBIFS_MST_NODE != mst[20])
    {
        rc = PFS_EIO;
        goto out;
    }
    p_sbi->highest_inum = pfs_ubifs_le64(mst + 24);
    if (p_sbi->highest_inum < PFS_UBIFS_FIRST_INO)
    {
        p_sbi->highest_inum = PFS_UBIFS_FIRST_INO;
    }

    rc = pfs_ubifs_cache_alloc(p_sbi);
    if (PFS_OK != rc)
    {
        goto out;
    }
    rc = pfs_ubifs_scan_rebuild(p_sbi);
    if (PFS_OK != rc)
    {
        pfs_ubifs_cache_free(p_sbi);
        goto out;
    }

    p_sbi->mount_state = want_rw ? PFS_UBIFS_MOUNT_RDWR : PFS_UBIFS_MOUNT_RDONLY;
    rc = PFS_OK;

out:
    if (PFS_OK != rc)
    {
        p_sbi->p_blkio = NULL;
        p_sbi->leb_size = 0;
        p_sbi->leb_cnt = 0;
        p_sbi->main_lebs = 0;
        p_sbi->mount_state = PFS_UBIFS_UNMOUNTED;
    }
    pfsmtd_free(sb);
    pfsmtd_free(mst);
    return rc;
}

int pfs_ubifs_remount(pfs_ubifs_sbi_t *p_sbi, int mode)
{
    if (NULL == p_sbi || !pfs_ubifs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 != (mode & PFS_MNT_RDWR))
    {
        p_sbi->mount_state = PFS_UBIFS_MOUNT_RDWR;
        return PFS_OK;
    }
    if (0 == (mode & PFS_MNT_RDONLY))
    {
        return PFS_EINVAL;
    }
    p_sbi->mount_state = PFS_UBIFS_MOUNT_RDONLY;
    return PFS_OK;
}

int pfs_ubifs_umount(pfs_ubifs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (PFS_UBIFS_UNMOUNTED == p_sbi->mount_state)
    {
        return PFS_OK;
    }
    if (PFS_UBIFS_MOUNT_RDWR == p_sbi->mount_state)
    {
        (void)pfs_ubifs_flush_mst(p_sbi);
    }
    pfs_ubifs_cache_free(p_sbi);
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_UBIFS_UNMOUNTED;
    return PFS_OK;
}

int pfs_ubifs_sbi_is_mounted(const pfs_ubifs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_UBIFS_UNMOUNTED != p_sbi->mount_state) ? 1 : 0;
}

int pfs_ubifs_sbi_can_write(const pfs_ubifs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_UBIFS_MOUNT_RDWR == p_sbi->mount_state) ? 1 : 0;
}

// end
