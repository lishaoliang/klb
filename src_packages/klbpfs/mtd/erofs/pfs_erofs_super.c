// Doc Encode : UTF-8 BOM, Unix(LF)
// erofs 卷生命周期 (只读: magic@1024 + FLAT 子集)
#include "erofs/pfs_erofs_internal.h"

#include <string.h>

int pfs_erofs_map_blkio_rc(int rc)
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

int pfs_erofs_init(pfs_erofs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_EROFS_UNMOUNTED;
    return PFS_OK;
}

void pfs_erofs_deinit(pfs_erofs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_EROFS_UNMOUNTED;
}

int pfs_erofs_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    pfs_erofs_super_block_t sb;

    int rc = pfs_erofs_read_sb(p_blkio, part_offset, &sb);
    if (PFS_OK != rc)
    {
        return (PFS_EINVAL == rc) ? PFS_ENOEXEC : rc;
    }
    if (sb.blkszbits < 9 || sb.blkszbits > 16)
    {
        return PFS_ENOEXEC;
    }
    if (0 != sb.dirblkbits)
    {
        return PFS_ENOEXEC;
    }
    return PFS_OK;
}

int pfs_erofs_mount(pfs_erofs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    if (NULL == p_sbi || NULL == p_blkio || part_offset < 0)
    {
        return PFS_EINVAL;
    }
    if (pfs_erofs_sbi_is_mounted(p_sbi))
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

    pfs_erofs_super_block_t sb;
    int rc = pfs_erofs_read_sb(p_blkio, part_offset, &sb);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (sb.blkszbits < 9 || sb.blkszbits > 16 || 0 != sb.dirblkbits)
    {
        return PFS_EINVAL;
    }
    if (0 != (sb.feature_incompat & (PFS_EROFS_INCOMPAT_DEVICE_TABLE | PFS_EROFS_INCOMPAT_METABOX)))
    {
        return PFS_EOPNOTSUPP;
    }

    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    p_sbi->blkszbits = (int)sb.blkszbits;
    p_sbi->blksz = 1u << sb.blkszbits;
    p_sbi->meta_blkaddr = sb.meta_blkaddr;
    p_sbi->feature_incompat = sb.feature_incompat;
    if (0 != (sb.feature_incompat & PFS_EROFS_INCOMPAT_48BIT) && 0 != sb.rootnid_8b)
    {
        p_sbi->root_nid = sb.rootnid_8b;
    }
    else
    {
        p_sbi->root_nid = (uint64_t)sb.rootnid_2b;
    }
    p_sbi->mount_state = PFS_EROFS_MOUNT_RDONLY;

    pfs_erofs_inode_t root;
    rc = pfs_erofs_read_inode(p_sbi, p_sbi->root_nid, &root);
    if (PFS_OK != rc)
    {
        p_sbi->mount_state = PFS_EROFS_UNMOUNTED;
        p_sbi->p_blkio = NULL;
        return rc;
    }
    if (PFS_EROFS_S_IFDIR != (root.mode & PFS_EROFS_S_IFMT))
    {
        p_sbi->mount_state = PFS_EROFS_UNMOUNTED;
        p_sbi->p_blkio = NULL;
        return PFS_EBADMSG;
    }
    return PFS_OK;
}

int pfs_erofs_remount(pfs_erofs_sbi_t *p_sbi, int mode)
{
    if (NULL == p_sbi || !pfs_erofs_sbi_is_mounted(p_sbi))
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
    p_sbi->mount_state = PFS_EROFS_MOUNT_RDONLY;
    return PFS_OK;
}

int pfs_erofs_umount(pfs_erofs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (PFS_EROFS_UNMOUNTED == p_sbi->mount_state)
    {
        return PFS_OK;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_EROFS_UNMOUNTED;
    return PFS_OK;
}

int pfs_erofs_sbi_is_mounted(const pfs_erofs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_EROFS_UNMOUNTED != p_sbi->mount_state) ? 1 : 0;
}

int pfs_erofs_sbi_can_write(const pfs_erofs_sbi_t *p_sbi)
{
    (void)p_sbi;
    return 0;
}

// end
