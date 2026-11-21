// Doc Encode : UTF-8 BOM, Unix(LF)
// cramfs 卷生命周期 (只读)
#include "cramfs/pfs_cramfs_internal.h"

#include <string.h>

int pfs_cramfs_map_blkio_rc(int rc)
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

int pfs_cramfs_init(pfs_cramfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_CRAMFS_UNMOUNTED;
    return PFS_OK;
}

void pfs_cramfs_deinit(pfs_cramfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_CRAMFS_UNMOUNTED;
}

int pfs_cramfs_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    if (NULL == p_blkio || 0 > part_offset)
    {
        return PFS_EINVAL;
    }

    pfs_cramfs_super_t sb;
    pfs_cramfs_inode_t root;
    int64_t sb_rel;

    return pfs_cramfs_read_super(p_blkio, part_offset, &sb_rel, &sb, &root);
}

int pfs_cramfs_mount(pfs_cramfs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    if (NULL == p_sbi || NULL == p_blkio || 0 > part_offset)
    {
        return PFS_EINVAL;
    }
    if (pfs_cramfs_sbi_is_mounted(p_sbi))
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

    pfs_cramfs_super_t sb;
    pfs_cramfs_inode_t root;
    int64_t sb_rel;
    int rc = pfs_cramfs_read_super(p_blkio, part_offset, &sb_rel, &sb, &root);
    if (PFS_OK != rc)
    {
        return rc;
    }

    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    p_sbi->sb_rel = sb_rel;
    p_sbi->flags = sb.flags;
    p_sbi->root = root;
    if (0 != (sb.flags & PFS_CRAMFS_FLAG_FSID_VERSION_2) && 0 < sb.size)
    {
        p_sbi->size = sb.size;
    }
    else
    {
        int64_t disk_sz = pfs_blkio_get_size(p_blkio);
        if (part_offset < disk_sz)
        {
            p_sbi->size = (uint32_t)(disk_sz - part_offset);
        }
        else
        {
            p_sbi->size = (1u << 28);
        }
    }
    p_sbi->mount_state = PFS_CRAMFS_MOUNT_RDONLY;
    return PFS_OK;
}

int pfs_cramfs_remount(pfs_cramfs_sbi_t *p_sbi, int mode)
{
    if (NULL == p_sbi || !pfs_cramfs_sbi_is_mounted(p_sbi))
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
    p_sbi->mount_state = PFS_CRAMFS_MOUNT_RDONLY;
    return PFS_OK;
}

int pfs_cramfs_umount(pfs_cramfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (PFS_CRAMFS_UNMOUNTED == p_sbi->mount_state)
    {
        return PFS_OK;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_CRAMFS_UNMOUNTED;
    return PFS_OK;
}

int pfs_cramfs_sbi_is_mounted(const pfs_cramfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_CRAMFS_UNMOUNTED != p_sbi->mount_state) ? 1 : 0;
}

int pfs_cramfs_sbi_can_write(const pfs_cramfs_sbi_t *p_sbi)
{
    (void)p_sbi;
    return 0;
}

// end
