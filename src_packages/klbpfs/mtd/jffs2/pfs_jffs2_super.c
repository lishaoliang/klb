// Doc Encode : UTF-8 BOM, Unix(LF)
// jffs2 卷生命周期 (probe + 扫描挂载; 可 RDWR)
#include "jffs2/pfs_jffs2_internal.h"

#include <string.h>

int pfs_jffs2_map_blkio_rc(int rc)
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

int pfs_jffs2_init(pfs_jffs2_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_JFFS2_UNMOUNTED;
    return PFS_OK;
}

void pfs_jffs2_deinit(pfs_jffs2_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    pfs_jffs2_scan_free(p_sbi);
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_JFFS2_UNMOUNTED;
}

int pfs_jffs2_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_ENOEXEC;
    }
    int64_t sz = pfs_blkio_get_size(p_blkio);
    if (0 > sz)
    {
        return PFS_ENOEXEC;
    }
    if (part_offset >= sz)
    {
        return PFS_ENOEXEC;
    }
    int64_t flash_size = sz - part_offset;
    return pfs_jffs2_probe_magic(p_blkio, part_offset, flash_size);
}

int pfs_jffs2_mount(pfs_jffs2_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    if (NULL == p_sbi || NULL == p_blkio || part_offset < 0)
    {
        return PFS_EINVAL;
    }
    if (pfs_jffs2_sbi_is_mounted(p_sbi))
    {
        return PFS_EBUSY;
    }
    int want_rw = 0;
    if (PFS_MNT_RDWR == mode)
    {
        want_rw = 1;
    }
    else if (PFS_MNT_RDONLY != mode)
    {
        return PFS_EINVAL;
    }

    int64_t sz = pfs_blkio_get_size(p_blkio);
    if (0 > sz)
    {
        return pfs_jffs2_map_blkio_rc((int)sz);
    }
    if (part_offset >= sz)
    {
        return PFS_EINVAL;
    }

    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    p_sbi->flash_size = sz - part_offset;
    p_sbi->free_ofs = 0;
    p_sbi->next_ino = 2;
    p_sbi->ic_list = NULL;

    int rc = pfs_jffs2_scan_medium(p_sbi);
    if (PFS_OK != rc)
    {
        pfs_jffs2_scan_free(p_sbi);
        p_sbi->p_blkio = NULL;
        p_sbi->part_offset = 0;
        p_sbi->flash_size = 0;
        p_sbi->free_ofs = 0;
        p_sbi->next_ino = 0;
        return rc;
    }

    p_sbi->mount_state = want_rw ? PFS_JFFS2_MOUNT_RDWR : PFS_JFFS2_MOUNT_RDONLY;
    return PFS_OK;
}

int pfs_jffs2_remount(pfs_jffs2_sbi_t *p_sbi, int mode)
{
    if (NULL == p_sbi || !pfs_jffs2_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (PFS_MNT_RDWR == mode)
    {
        p_sbi->mount_state = PFS_JFFS2_MOUNT_RDWR;
        return PFS_OK;
    }
    if (PFS_MNT_RDONLY == mode)
    {
        p_sbi->mount_state = PFS_JFFS2_MOUNT_RDONLY;
        return PFS_OK;
    }
    return PFS_EINVAL;
}

int pfs_jffs2_umount(pfs_jffs2_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (PFS_JFFS2_UNMOUNTED == p_sbi->mount_state)
    {
        return PFS_OK;
    }
    if (PFS_JFFS2_MOUNT_RDWR == p_sbi->mount_state)
    {
        (void)pfs_jffs2_sync_media(p_sbi);
    }
    pfs_jffs2_scan_free(p_sbi);
    p_sbi->p_blkio = NULL;
    p_sbi->part_offset = 0;
    p_sbi->flash_size = 0;
    p_sbi->free_ofs = 0;
    p_sbi->next_ino = 0;
    p_sbi->mount_state = PFS_JFFS2_UNMOUNTED;
    return PFS_OK;
}

int pfs_jffs2_sbi_is_mounted(const pfs_jffs2_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_JFFS2_UNMOUNTED != p_sbi->mount_state) ? 1 : 0;
}

int pfs_jffs2_sbi_can_write(const pfs_jffs2_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_JFFS2_MOUNT_RDWR == p_sbi->mount_state) ? 1 : 0;
}

// end
