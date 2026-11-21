// Doc Encode : UTF-8 BOM, Unix(LF)
// squashfs 卷生命周期 (probe/mount; 只读; zlib)
#include "squashfs/pfs_squashfs_internal.h"
#include "base/pfsmtd_mem.h"
#include "base/pfsmtd_util.h"

#include <string.h>

int pfs_squashfs_map_blkio_rc(int rc)
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

static void pfs_squashfs_sbi_release_bufs(pfs_squashfs_sbi_t *p_sbi)
{
    if (NULL != p_sbi->fragment_index)
    {
        pfsmtd_free(p_sbi->fragment_index);
        p_sbi->fragment_index = NULL;
    }
    p_sbi->fragment_index_count = 0;
    if (NULL != p_sbi->meta_data)
    {
        pfsmtd_free(p_sbi->meta_data);
        p_sbi->meta_data = NULL;
    }
    if (NULL != p_sbi->comp_buf)
    {
        pfsmtd_free(p_sbi->comp_buf);
        p_sbi->comp_buf = NULL;
    }
    if (NULL != p_sbi->data_buf)
    {
        pfsmtd_free(p_sbi->data_buf);
        p_sbi->data_buf = NULL;
    }
    p_sbi->meta_block = (uint64_t)PFS_SQUASHFS_INVALID_BLK;
    p_sbi->meta_len = 0;
    p_sbi->data_buf_index = -1;
    p_sbi->data_buf_ino = 0;
}

int pfs_squashfs_init(pfs_squashfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_SQUASHFS_UNMOUNTED;
    p_sbi->meta_block = (uint64_t)PFS_SQUASHFS_INVALID_BLK;
    p_sbi->data_buf_index = -1;
    return PFS_OK;
}

void pfs_squashfs_deinit(pfs_squashfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    pfs_squashfs_sbi_release_bufs(p_sbi);
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_SQUASHFS_UNMOUNTED;
    p_sbi->meta_block = (uint64_t)PFS_SQUASHFS_INVALID_BLK;
    p_sbi->data_buf_index = -1;
}

static int pfs_squashfs_read_super_raw(pfs_blkio_t *p_blkio, int64_t part_offset,
        pfs_squashfs_super_block_t *p_sb)
{
    uint8_t buf[sizeof(pfs_squashfs_super_block_t)];

    if (NULL == p_blkio || part_offset < 0 || NULL == p_sb)
    {
        return PFS_EINVAL;
    }

    int nread = pfs_blkio_pread(p_blkio, buf, (int)sizeof(buf), part_offset);
    if (nread != (int)sizeof(buf))
    {
        if (nread < 0)
        {
            return pfs_squashfs_map_blkio_rc(nread);
        }
        return PFS_ENOEXEC;
    }

    p_sb->s_magic = pfs_squashfs_le32(buf + 0);
    p_sb->inodes = pfs_squashfs_le32(buf + 4);
    p_sb->mkfs_time = pfs_squashfs_le32(buf + 8);
    p_sb->block_size = pfs_squashfs_le32(buf + 12);
    p_sb->fragments = pfs_squashfs_le32(buf + 16);
    p_sb->compression = pfs_squashfs_le16(buf + 20);
    p_sb->block_log = pfs_squashfs_le16(buf + 22);
    p_sb->flags = pfs_squashfs_le16(buf + 24);
    p_sb->no_ids = pfs_squashfs_le16(buf + 26);
    p_sb->s_major = pfs_squashfs_le16(buf + 28);
    p_sb->s_minor = pfs_squashfs_le16(buf + 30);
    p_sb->root_inode = pfs_squashfs_le64(buf + 32);
    p_sb->bytes_used = pfs_squashfs_le64(buf + 40);
    p_sb->id_table_start = pfs_squashfs_le64(buf + 48);
    p_sb->xattr_id_table_start = pfs_squashfs_le64(buf + 56);
    p_sb->inode_table_start = pfs_squashfs_le64(buf + 64);
    p_sb->directory_table_start = pfs_squashfs_le64(buf + 72);
    p_sb->fragment_table_start = pfs_squashfs_le64(buf + 80);
    p_sb->lookup_table_start = pfs_squashfs_le64(buf + 88);
    return PFS_OK;
}

static int pfs_squashfs_super_check(const pfs_squashfs_super_block_t *p_sb)
{
    uint32_t expect;

    if (PFS_SQUASHFS_MAGIC != p_sb->s_magic)
    {
        return PFS_ENOEXEC;
    }
    if (PFS_SQUASHFS_MAJOR != p_sb->s_major)
    {
        return PFS_ENOEXEC;
    }
    if (0u == p_sb->block_size || p_sb->block_log > 20u)
    {
        return PFS_EINVAL;
    }
    expect = 1u << p_sb->block_log;
    if (expect != p_sb->block_size)
    {
        return PFS_EINVAL;
    }
    if (0u == p_sb->bytes_used || 0u == p_sb->inodes)
    {
        return PFS_EINVAL;
    }
    if (PFS_SQUASHFS_COMP_ZLIB != p_sb->compression)
    {
        // 首版仅 zlib; 探测仍可认 magic, mount 再拒
        return PFS_EOPNOTSUPP;
    }
    return PFS_OK;
}

int pfs_squashfs_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    pfs_squashfs_super_block_t sb;

    int rc = pfs_squashfs_read_super_raw(p_blkio, part_offset, &sb);
    if (PFS_OK != rc)
    {
        return rc;
    }
    rc = pfs_squashfs_super_check(&sb);
    if (PFS_EOPNOTSUPP == rc)
    {
        // 认出 squashfs 但压缩算法首版不支持: 当作不可选, 让探测序继续
        return PFS_ENOEXEC;
    }
    return rc;
}

int pfs_squashfs_mount(pfs_squashfs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    pfs_squashfs_super_block_t sb;
    unsigned int idx_bytes;
    unsigned int idx_count;
    int comp_cap;

    if (NULL == p_sbi || NULL == p_blkio || part_offset < 0)
    {
        return PFS_EINVAL;
    }
    if (pfs_squashfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EBUSY;
    }
    if (0 != (mode & PFS_MNT_RDWR))
    {
        return PFS_EPERM;
    }

    int rc = pfs_squashfs_read_super_raw(p_blkio, part_offset, &sb);
    if (PFS_OK != rc)
    {
        return rc;
    }
    rc = pfs_squashfs_super_check(&sb);
    if (PFS_OK != rc)
    {
        return rc;
    }

    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    p_sbi->block_size = sb.block_size;
    p_sbi->block_log = sb.block_log;
    p_sbi->compression = sb.compression;
    p_sbi->flags = sb.flags;
    p_sbi->s_major = sb.s_major;
    p_sbi->s_minor = sb.s_minor;
    p_sbi->inodes = sb.inodes;
    p_sbi->fragments = sb.fragments;
    p_sbi->bytes_used = sb.bytes_used;
    p_sbi->root_inode = sb.root_inode;
    p_sbi->inode_table = sb.inode_table_start;
    p_sbi->directory_table = sb.directory_table_start;
    p_sbi->fragment_table_start = sb.fragment_table_start;
    p_sbi->id_table_start = sb.id_table_start;

    p_sbi->meta_data = (uint8_t *)pfsmtd_mallocz(PFS_SQUASHFS_METADATA_SIZE);
    comp_cap = (int)PFSMTD_MAX(p_sbi->block_size, (uint32_t)PFS_SQUASHFS_METADATA_SIZE);
    p_sbi->comp_buf = (uint8_t *)pfsmtd_malloc((size_t)comp_cap);
    p_sbi->data_buf = (uint8_t *)pfsmtd_malloc(p_sbi->block_size);
    if (NULL == p_sbi->meta_data || NULL == p_sbi->comp_buf || NULL == p_sbi->data_buf)
    {
        pfs_squashfs_sbi_release_bufs(p_sbi);
        return PFS_ENOMEM;
    }
    p_sbi->meta_block = (uint64_t)PFS_SQUASHFS_INVALID_BLK;
    p_sbi->data_buf_index = -1;

    if (p_sbi->fragments > 0)
    {
        uint8_t *raw_idx;

        idx_count = PFS_SQUASHFS_FRAGMENT_INDEXES(p_sbi->fragments);
        idx_bytes = PFS_SQUASHFS_FRAGMENT_INDEX_BYTES(p_sbi->fragments);
        if (sb.fragment_table_start + idx_bytes > sb.bytes_used)
        {
            pfs_squashfs_sbi_release_bufs(p_sbi);
            return PFS_EINVAL;
        }
        raw_idx = (uint8_t *)pfsmtd_malloc(idx_bytes);
        p_sbi->fragment_index = (uint64_t *)pfsmtd_mallocz(sizeof(uint64_t) * idx_count);
        if (NULL == raw_idx || NULL == p_sbi->fragment_index)
        {
            pfsmtd_free(raw_idx);
            pfs_squashfs_sbi_release_bufs(p_sbi);
            return PFS_ENOMEM;
        }
        rc = pfs_squashfs_read_table(p_sbi, sb.fragment_table_start, raw_idx, (int)idx_bytes);
        if (PFS_OK != rc)
        {
            pfsmtd_free(raw_idx);
            pfs_squashfs_sbi_release_bufs(p_sbi);
            return rc;
        }
        for (unsigned int i = 0; i < idx_count; i++)
        {
            p_sbi->fragment_index[i] = pfs_squashfs_le64(raw_idx + (i * 8u));
        }
        pfsmtd_free(raw_idx);
        p_sbi->fragment_index_count = idx_count;
        if (p_sbi->fragment_index[0] >= sb.fragment_table_start)
        {
            pfs_squashfs_sbi_release_bufs(p_sbi);
            return PFS_EINVAL;
        }
    }

    p_sbi->mount_state = PFS_SQUASHFS_MOUNT_RDONLY;
    return PFS_OK;
}

int pfs_squashfs_remount(pfs_squashfs_sbi_t *p_sbi, int mode)
{
    if (NULL == p_sbi || !pfs_squashfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 != (mode & PFS_MNT_RDWR))
    {
        return PFS_EPERM;
    }
    p_sbi->mount_state = PFS_SQUASHFS_MOUNT_RDONLY;
    return PFS_OK;
}

int pfs_squashfs_umount(pfs_squashfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (PFS_SQUASHFS_UNMOUNTED == p_sbi->mount_state)
    {
        return PFS_OK;
    }
    pfs_squashfs_sbi_release_bufs(p_sbi);
    p_sbi->p_blkio = NULL;
    p_sbi->part_offset = 0;
    p_sbi->mount_state = PFS_SQUASHFS_UNMOUNTED;
    return PFS_OK;
}

int pfs_squashfs_sbi_is_mounted(const pfs_squashfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_SQUASHFS_UNMOUNTED != p_sbi->mount_state) ? 1 : 0;
}

int pfs_squashfs_sbi_can_write(const pfs_squashfs_sbi_t *p_sbi)
{
    (void)p_sbi;
    return 0;
}

// end
