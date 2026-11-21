// Doc Encode : UTF-8 BOM, Unix(LF)
// yaffs2 卷生命周期 (探测几何 + 扫描挂载; 可 RDWR)
#include "yaffs/pfs_yaffs_internal.h"

#include <string.h>

int pfs_yaffs_map_blkio_rc(int rc)
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

int pfs_yaffs_init(pfs_yaffs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_YAFFS_UNMOUNTED;
    return PFS_OK;
}

void pfs_yaffs_deinit(pfs_yaffs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    pfs_yaffs_scan_free(p_sbi);
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_YAFFS_UNMOUNTED;
}

int pfs_yaffs_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_ENOEXEC;
    }
    int64_t sz = pfs_blkio_get_size(p_blkio);
    if (0 > sz || part_offset >= sz)
    {
        return PFS_ENOEXEC;
    }
    int64_t flash_size = sz - part_offset;
    int chunk_size;
    int spare_size;
    return pfs_yaffs_detect_geom(p_blkio, part_offset, flash_size, &chunk_size, &spare_size);
}

int pfs_yaffs_mount(pfs_yaffs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    if (NULL == p_sbi || NULL == p_blkio || part_offset < 0)
    {
        return PFS_EINVAL;
    }
    if (pfs_yaffs_sbi_is_mounted(p_sbi))
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
        return pfs_yaffs_map_blkio_rc((int)sz);
    }
    if (part_offset >= sz)
    {
        return PFS_EINVAL;
    }

    int chunk_size;
    int spare_size;
    int rc = pfs_yaffs_detect_geom(p_blkio, part_offset, sz - part_offset, &chunk_size, &spare_size);
    if (PFS_OK != rc)
    {
        return rc;
    }

    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    p_sbi->flash_size = sz - part_offset;
    p_sbi->chunk_size = chunk_size;
    p_sbi->spare_size = spare_size;
    p_sbi->page_size = chunk_size + spare_size;
    p_sbi->page_count = p_sbi->flash_size / (int64_t)p_sbi->page_size;
    p_sbi->free_page = 0;
    p_sbi->next_obj_id = PFS_YAFFS_OBJECTID_DELETED + 1u;
    p_sbi->next_seq = PFS_YAFFS_LOWEST_SEQUENCE_NUMBER;
    p_sbi->obj_list = NULL;

    rc = pfs_yaffs_scan_medium(p_sbi);
    if (PFS_OK != rc)
    {
        pfs_yaffs_scan_free(p_sbi);
        p_sbi->p_blkio = NULL;
        p_sbi->part_offset = 0;
        p_sbi->flash_size = 0;
        p_sbi->chunk_size = 0;
        p_sbi->spare_size = 0;
        p_sbi->page_size = 0;
        p_sbi->page_count = 0;
        p_sbi->free_page = 0;
        p_sbi->next_obj_id = 0;
        p_sbi->next_seq = 0;
        return rc;
    }

    p_sbi->mount_state = want_rw ? PFS_YAFFS_MOUNT_RDWR : PFS_YAFFS_MOUNT_RDONLY;
    return PFS_OK;
}

int pfs_yaffs_remount(pfs_yaffs_sbi_t *p_sbi, int mode)
{
    if (NULL == p_sbi || !pfs_yaffs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (PFS_MNT_RDWR == mode)
    {
        p_sbi->mount_state = PFS_YAFFS_MOUNT_RDWR;
        return PFS_OK;
    }
    if (PFS_MNT_RDONLY == mode)
    {
        p_sbi->mount_state = PFS_YAFFS_MOUNT_RDONLY;
        return PFS_OK;
    }
    return PFS_EINVAL;
}

int pfs_yaffs_umount(pfs_yaffs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (PFS_YAFFS_UNMOUNTED == p_sbi->mount_state)
    {
        return PFS_OK;
    }
    if (PFS_YAFFS_MOUNT_RDWR == p_sbi->mount_state)
    {
        (void)pfs_yaffs_sync_media(p_sbi);
    }
    pfs_yaffs_scan_free(p_sbi);
    p_sbi->p_blkio = NULL;
    p_sbi->part_offset = 0;
    p_sbi->flash_size = 0;
    p_sbi->chunk_size = 0;
    p_sbi->spare_size = 0;
    p_sbi->page_size = 0;
    p_sbi->page_count = 0;
    p_sbi->free_page = 0;
    p_sbi->next_obj_id = 0;
    p_sbi->next_seq = 0;
    p_sbi->mount_state = PFS_YAFFS_UNMOUNTED;
    return PFS_OK;
}

int pfs_yaffs_sbi_is_mounted(const pfs_yaffs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_YAFFS_UNMOUNTED != p_sbi->mount_state) ? 1 : 0;
}

int pfs_yaffs_sbi_can_write(const pfs_yaffs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_YAFFS_MOUNT_RDWR == p_sbi->mount_state) ? 1 : 0;
}

// end
