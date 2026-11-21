// Doc Encode : UTF-8 BOM, Unix(LF)
// romfs 卷生命周期 (只读)
#include "romfs/pfs_romfs_internal.h"

#include <string.h>

int pfs_romfs_map_blkio_rc(int rc)
{
    if (0 <= rc)
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

int pfs_romfs_init(pfs_romfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_ROMFS_UNMOUNTED;
    return PFS_OK;
}

void pfs_romfs_deinit(pfs_romfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_ROMFS_UNMOUNTED;
}

int pfs_romfs_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    if (NULL == p_blkio || 0 > part_offset)
    {
        return PFS_EINVAL;
    }

    uint32_t img_size;
    uint32_t root_pos;

    return pfs_romfs_read_super(p_blkio, part_offset, &img_size, &root_pos);
}

int pfs_romfs_mount(pfs_romfs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    if (NULL == p_sbi || NULL == p_blkio || 0 > part_offset)
    {
        return PFS_EINVAL;
    }
    if (pfs_romfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EBUSY;
    }
    if (PFS_MNT_RDWR == mode)
    {
        return PFS_EPERM;
    }
    if (PFS_MNT_RDONLY != mode)
    {
        return PFS_EINVAL;
    }

    uint32_t img_size;
    uint32_t root_pos;
    int rc = pfs_romfs_read_super(p_blkio, part_offset, &img_size, &root_pos);
    if (PFS_OK != rc)
    {
        return rc;
    }

    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    p_sbi->img_size = img_size;
    p_sbi->root_pos = root_pos;
    p_sbi->mount_state = PFS_ROMFS_MOUNT_RDONLY;

    pfs_romfs_fh_t root;
    rc = pfs_romfs_resolve_fh(p_sbi, root_pos, &root);
    if (PFS_OK != rc)
    {
        memset(p_sbi, 0, sizeof(*p_sbi));
        p_sbi->mount_state = PFS_ROMFS_UNMOUNTED;
        return rc;
    }
    if (PFS_ROMFH_DIR != root.type)
    {
        memset(p_sbi, 0, sizeof(*p_sbi));
        p_sbi->mount_state = PFS_ROMFS_UNMOUNTED;
        return PFS_ENOEXEC;
    }
    return PFS_OK;
}

int pfs_romfs_remount(pfs_romfs_sbi_t *p_sbi, int mode)
{
    if (NULL == p_sbi || !pfs_romfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 != (mode & PFS_MNT_RDWR))
    {
        return PFS_EPERM;
    }
    if (0 == (mode & PFS_MNT_RDONLY))
    {
        return PFS_EINVAL;
    }
    p_sbi->mount_state = PFS_ROMFS_MOUNT_RDONLY;
    return PFS_OK;
}

int pfs_romfs_umount(pfs_romfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (PFS_ROMFS_UNMOUNTED == p_sbi->mount_state)
    {
        return PFS_OK;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_ROMFS_UNMOUNTED;
    return PFS_OK;
}

int pfs_romfs_sbi_is_mounted(const pfs_romfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_ROMFS_UNMOUNTED != p_sbi->mount_state) ? 1 : 0;
}

int pfs_romfs_sbi_can_write(const pfs_romfs_sbi_t *p_sbi)
{
    (void)p_sbi;
    return 0;
}

// end
