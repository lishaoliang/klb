// Doc Encode : UTF-8 BOM, Unix(LF)
// ext4 inode 读写与 meta 转换
#include "ext4/pfs_ext4_internal.h"

#include <string.h>

static int pfs_ext4_ino_to_group_index(const pfs_ext4_sbi_t *p_sbi, uint32_t ino,
        uint32_t *p_group, uint32_t *p_index)
{
    if (NULL == p_sbi || NULL == p_group || NULL == p_index || 0u == ino ||
            ino > p_sbi->inodes_count)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    ino--;
    *p_group = ino / p_sbi->inodes_per_group;
    *p_index = ino % p_sbi->inodes_per_group;
    if (*p_group >= p_sbi->groups_count)
    {
        return PFS_EXT4_RET_ERANGE;
    }
    return PFS_EXT4_RET_OK;
}

uint64_t pfs_ext4_inode_size(const pfs_ext4_inode_raw_t *p_raw)
{
    if (NULL == p_raw)
    {
        return 0ull;
    }
    uint64_t sz = p_raw->i_size_lo;
    sz |= ((uint64_t)p_raw->i_size_high << 32);
    return sz;
}

void pfs_ext4_inode_to_meta(const pfs_ext4_inode_raw_t *p_raw, pfs_ext4_file_meta_t *p_meta)
{
    if (NULL == p_raw || NULL == p_meta)
    {
        return;
    }

    memset(p_meta, 0, sizeof(*p_meta));

    uint32_t mode = p_raw->i_mode;
    if (PFS_EXT4_S_IFDIR == (mode & PFS_EXT4_S_IFMT))
    {
        p_meta->type = PFS_EXT4_TYPE_DIR;
    }
    else if (PFS_EXT4_S_IFREG == (mode & PFS_EXT4_S_IFMT))
    {
        p_meta->type = PFS_EXT4_TYPE_FILE;
    }
    else
    {
        p_meta->type = PFS_EXT4_TYPE_OTHER;
    }
    p_meta->size = pfs_ext4_inode_size(p_raw);
    p_meta->mode = mode;
    p_meta->flags = p_raw->i_flags;
    p_meta->links = p_raw->i_links_count;
    p_meta->blocks_512 = p_raw->i_blocks_lo;
    p_meta->blocks_512 |= ((uint32_t)p_raw->i_blocks_high << 16);
    memcpy(p_meta->i_block, p_raw->i_block, sizeof(p_meta->i_block));
}

int pfs_ext4_inode_read(pfs_ext4_sbi_t *p_sbi, uint32_t ino, pfs_ext4_inode_raw_t *p_raw)
{
    if (NULL == p_sbi || NULL == p_raw)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    uint32_t group;
    uint32_t index;

    int rc = pfs_ext4_ino_to_group_index(p_sbi, ino, &group, &index);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    pfs_ext4_group_desc_t gd;

    rc = pfs_ext4_gd_get(p_sbi, group, &gd);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    uint64_t table = pfs_ext4_gd_inode_table(p_sbi, &gd);
    uint64_t byte_off = (uint64_t)index * (uint64_t)p_sbi->inode_size;
    uint64_t block = table + byte_off / (uint64_t)p_sbi->block_size;
    uint32_t off = (uint32_t)(byte_off % (uint64_t)p_sbi->block_size);

    uint8_t *p_blk = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_blk)
    {
        return PFS_EXT4_RET_ENOMEM;
    }
    rc = pfs_ext4_read_block(p_sbi, block, p_blk);
    if (PFS_EXT4_RET_OK != rc)
    {
        pfs_free(p_blk);
        return rc;
    }
    memset(p_raw, 0, sizeof(*p_raw));

    uint32_t copy_len = p_sbi->inode_size;

    if (copy_len > (uint32_t)sizeof(*p_raw))
    {
        copy_len = (uint32_t)sizeof(*p_raw);
    }
    if (off + copy_len > p_sbi->block_size)
    {
        pfs_free(p_blk);
        return PFS_EXT4_RET_EBADMSG;
    }
    memcpy(p_raw, p_blk + off, copy_len);

    pfs_free(p_blk);
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_inode_write(pfs_ext4_sbi_t *p_sbi, uint32_t ino, const pfs_ext4_inode_raw_t *p_raw)
{
    if (NULL == p_sbi || NULL == p_raw)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    uint32_t group;
    uint32_t index;

    int rc = pfs_ext4_ino_to_group_index(p_sbi, ino, &group, &index);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    pfs_ext4_group_desc_t gd;

    rc = pfs_ext4_gd_get(p_sbi, group, &gd);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    uint64_t table = pfs_ext4_gd_inode_table(p_sbi, &gd);
    uint64_t byte_off = (uint64_t)index * (uint64_t)p_sbi->inode_size;
    uint64_t block = table + byte_off / (uint64_t)p_sbi->block_size;
    uint32_t off = (uint32_t)(byte_off % (uint64_t)p_sbi->block_size);

    uint8_t *p_blk = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_blk)
    {
        return PFS_EXT4_RET_ENOMEM;
    }
    rc = pfs_ext4_read_block(p_sbi, block, p_blk);
    if (PFS_EXT4_RET_OK != rc)
    {
        pfs_free(p_blk);
        return rc;
    }

    uint32_t copy_len = p_sbi->inode_size;

    if (copy_len > (uint32_t)sizeof(*p_raw))
    {
        copy_len = (uint32_t)sizeof(*p_raw);
    }
    if (off + copy_len > p_sbi->block_size)
    {
        pfs_free(p_blk);
        return PFS_EXT4_RET_EBADMSG;
    }
    // 仅覆盖已知布局; 超长 inode 尾部保留原盘内容
    memcpy(p_blk + off, p_raw, copy_len);

    rc = pfs_ext4_write_block(p_sbi, block, p_blk);
    pfs_free(p_blk);
    return rc;
}

// end
