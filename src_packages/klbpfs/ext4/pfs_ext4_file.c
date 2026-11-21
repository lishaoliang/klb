// Doc Encode : UTF-8 BOM, Unix(LF)
// ext4 文件 open/read/write/truncate/sync
#include "ext4/pfs_ext4_internal.h"
#include "pfs.h"

#include <string.h>

static int pfs_ext4_file_mode_check(int mode)
{
    int acc = mode & (PFS_O_RD | PFS_O_RDWR);
    if (PFS_O_RD == acc || PFS_O_RDWR == acc)
    {
        return PFS_EXT4_RET_OK;
    }
    return PFS_EXT4_RET_EINVAL;
}

static int pfs_ext4_file_create(pfs_ext4_sbi_t *p_sbi, const char *path,
        pfs_ext4_file_meta_t *p_meta)
{
    uint32_t parent_ino;
    pfs_ext4_inode_raw_t parent_raw;
    char leaf[PFS_EXT4_NAME_LEN + 1];

    int rc = pfs_ext4_path_parent(p_sbi, path, &parent_ino, &parent_raw, leaf, sizeof(leaf));
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_EXT4_S_IFDIR != (parent_raw.i_mode & PFS_EXT4_S_IFMT))
    {
        return PFS_EXT4_RET_ENOENT;
    }

    uint32_t exist;

    rc = pfs_ext4_dir_lookup(p_sbi, parent_ino, parent_raw.i_block, parent_raw.i_flags,
            pfs_ext4_inode_size(&parent_raw), leaf, &exist, NULL);
    if (PFS_EXT4_RET_OK == rc)
    {
        return PFS_EXT4_RET_EEXIST;
    }
    if (PFS_EXT4_RET_ENOENT != rc)
    {
        return rc;
    }

    uint32_t file_ino;

    rc = pfs_ext4_ialloc_new(p_sbi, &file_ino);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    pfs_ext4_inode_raw_t file_raw;

    memset(&file_raw, 0, sizeof(file_raw));
    file_raw.i_mode = (uint16_t)(PFS_EXT4_S_IFREG | 0644u);
    file_raw.i_links_count = 1;
    if (0u != (p_sbi->feature_incompat & PFS_EXT4_FEATURE_INCOMPAT_EXTENTS))
    {
        pfs_ext4_extent_header_t *p_eh = (pfs_ext4_extent_header_t *)(void *)file_raw.i_block;
        file_raw.i_flags = PFS_EXT4_EXTENTS_FL;
        p_eh->eh_magic = pfs_cpu_to_le16(PFS_EXT4_EXT_MAGIC);
        p_eh->eh_entries = 0;
        p_eh->eh_max = pfs_cpu_to_le16((uint16_t)((PFS_EXT4_N_BLOCKS * 4u -
                (uint32_t)sizeof(pfs_ext4_extent_header_t)) /
                (uint32_t)sizeof(pfs_ext4_extent_t)));
        p_eh->eh_depth = 0;
    }

    rc = pfs_ext4_inode_write(p_sbi, file_ino, &file_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        (void)pfs_ext4_ialloc_free(p_sbi, file_ino);
        return rc;
    }

    rc = pfs_ext4_dir_add(p_sbi, parent_ino, &parent_raw, leaf, file_ino, PFS_EXT4_FT_REG_FILE);
    if (PFS_EXT4_RET_OK != rc)
    {
        (void)pfs_ext4_ialloc_free(p_sbi, file_ino);
        return rc;
    }
    rc = pfs_ext4_inode_write(p_sbi, parent_ino, &parent_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    p_meta->ino = file_ino;
    pfs_ext4_inode_to_meta(&file_raw, p_meta);
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_file_open(pfs_ext4_sbi_t *p_sbi, const char *path, int mode,
        pfs_ext4_file_meta_t *p_meta)
{
    if (NULL == p_sbi || NULL == path || NULL == p_meta)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    int rc = pfs_ext4_file_mode_check(mode);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (0u != (mode & PFS_O_CREAT))
    {
        if (0u == (mode & PFS_O_RDWR))
        {
            return PFS_EXT4_RET_EINVAL;
        }
        if (!pfs_ext4_sbi_can_write(p_sbi))
        {
            return PFS_EXT4_RET_EPERM;
        }
    }
    else if (0u != (mode & PFS_O_RDWR) && !pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    uint32_t ino;
    pfs_ext4_inode_raw_t raw;

    rc = pfs_ext4_path_lookup(p_sbi, path, &ino, &raw);
    if (PFS_EXT4_RET_ENOENT == rc && (mode & PFS_O_CREAT))
    {
        return pfs_ext4_file_create(p_sbi, path, p_meta);
    }
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    if (PFS_EXT4_S_IFDIR == (raw.i_mode & PFS_EXT4_S_IFMT))
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (PFS_EXT4_S_IFREG != (raw.i_mode & PFS_EXT4_S_IFMT))
    {
        return PFS_EXT4_RET_EPERM;
    }
    if (0u != (raw.i_flags & PFS_EXT4_INDEX_FL))
    {
        return PFS_EXT4_RET_EOPNOTSUPP;
    }

    p_meta->ino = ino;
    pfs_ext4_inode_to_meta(&raw, p_meta);
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_file_read(pfs_ext4_sbi_t *p_sbi, pfs_ext4_file_meta_t *p_meta,
        int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_sbi || NULL == p_meta || NULL == buf || NULL == p_got || 0 > size)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (0 > pos || pos >= (int64_t)p_meta->size)
    {
        *p_got = 0;
        return PFS_EXT4_RET_OK;
    }

    int64_t end = (int64_t)p_meta->size;
    if (pos + (int64_t)size > end)
    {
        size = (int)(end - pos);
    }
    if (0 >= size)
    {
        *p_got = 0;
        return PFS_EXT4_RET_OK;
    }

    uint8_t *p_blk = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_blk)
    {
        return PFS_EXT4_RET_ENOMEM;
    }

    int total = 0;
    while (total < size)
    {
        uint32_t lblk = (uint32_t)((uint64_t)pos / (uint64_t)p_sbi->block_size);
        uint32_t off = (uint32_t)((uint64_t)pos % (uint64_t)p_sbi->block_size);
        uint64_t pblk;
        int rc = pfs_ext4_map_block(p_sbi, p_meta->i_block, p_meta->flags, lblk, &pblk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_blk);
            return rc;
        }

        int chunk;
        if (0ull == pblk)
        {
            chunk = (int)p_sbi->block_size - (int)off;
            if (chunk > size - total)
            {
                chunk = size - total;
            }
            memset((uint8_t *)buf + total, 0, (size_t)chunk);
        }
        else
        {
            rc = pfs_ext4_read_block(p_sbi, pblk, p_blk);
            if (PFS_EXT4_RET_OK != rc)
            {
                pfs_free(p_blk);
                return rc;
            }
            chunk = (int)p_sbi->block_size - (int)off;
            if (chunk > size - total)
            {
                chunk = size - total;
            }
            memcpy((uint8_t *)buf + total, p_blk + off, (size_t)chunk);
        }
        total += chunk;
        pos += chunk;
    }

    pfs_free(p_blk);
    *p_got = total;
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_file_write(pfs_ext4_sbi_t *p_sbi, pfs_ext4_file_meta_t *p_meta,
        int64_t pos, const void *buf, int size, int *p_put)
{
    if (NULL == p_sbi || NULL == p_meta || NULL == buf || NULL == p_put || 0 > size)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }
    if (0 > pos)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (0 == size)
    {
        *p_put = 0;
        return PFS_EXT4_RET_OK;
    }

    uint8_t *p_blk = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_blk)
    {
        return PFS_EXT4_RET_ENOMEM;
    }

    int total = 0;
    while (total < size)
    {
        uint32_t lblk = (uint32_t)((uint64_t)pos / (uint64_t)p_sbi->block_size);
        uint32_t off = (uint32_t)((uint64_t)pos % (uint64_t)p_sbi->block_size);
        uint64_t pblk;
        int rc = pfs_ext4_map_block_alloc(p_sbi, p_meta->i_block, &p_meta->flags,
                &p_meta->blocks_512, lblk, &pblk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_blk);
            return rc;
        }

        rc = pfs_ext4_read_block(p_sbi, pblk, p_blk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_blk);
            return rc;
        }

        int chunk = (int)p_sbi->block_size - (int)off;
        if (chunk > size - total)
        {
            chunk = size - total;
        }
        memcpy(p_blk + off, (const uint8_t *)buf + total, (size_t)chunk);
        rc = pfs_ext4_write_block(p_sbi, pblk, p_blk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_blk);
            return rc;
        }

        total += chunk;
        pos += chunk;
    }

    pfs_free(p_blk);
    if ((uint64_t)pos > p_meta->size)
    {
        p_meta->size = (uint64_t)pos;
    }
    *p_put = total;
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_file_truncate(pfs_ext4_sbi_t *p_sbi, pfs_ext4_file_meta_t *p_meta, int64_t size)
{
    if (NULL == p_sbi || NULL == p_meta || 0 > size)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    if ((uint64_t)size >= p_meta->size)
    {
        p_meta->size = (uint64_t)size;
        return PFS_EXT4_RET_OK;
    }

    uint32_t old_blks = (uint32_t)((p_meta->size + (uint64_t)p_sbi->block_size - 1ull) /
            (uint64_t)p_sbi->block_size);
    uint32_t new_blks = (uint32_t)(((uint64_t)size + (uint64_t)p_sbi->block_size - 1ull) /
            (uint64_t)p_sbi->block_size);

    pfs_ext4_inode_raw_t raw;
    int rc = pfs_ext4_inode_read(p_sbi, p_meta->ino, &raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    for (uint32_t lblk = new_blks; lblk < old_blks; lblk++)
    {
        uint64_t pblk;
        rc = pfs_ext4_map_block(p_sbi, raw.i_block, raw.i_flags, lblk, &pblk);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        if (0ull != pblk)
        {
            rc = pfs_ext4_balloc_free(p_sbi, pblk);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
            if (raw.i_blocks_lo >= (p_sbi->block_size >> 9))
            {
                raw.i_blocks_lo -= (p_sbi->block_size >> 9);
            }
            else
            {
                raw.i_blocks_lo = 0;
            }
        }
    }

    p_meta->size = (uint64_t)size;
    raw.i_size_lo = (uint32_t)p_meta->size;
    raw.i_size_high = (uint32_t)(p_meta->size >> 32);
    raw.i_blocks_lo = p_meta->blocks_512;
    memcpy(raw.i_block, p_meta->i_block, sizeof(raw.i_block));
    raw.i_flags = p_meta->flags;

    rc = pfs_ext4_inode_write(p_sbi, p_meta->ino, &raw);
    return rc;
}

int pfs_ext4_file_sync(pfs_ext4_sbi_t *p_sbi, pfs_ext4_file_meta_t *p_meta)
{
    if (NULL == p_sbi || NULL == p_meta)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    pfs_ext4_inode_raw_t raw;
    int rc = pfs_ext4_inode_read(p_sbi, p_meta->ino, &raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    raw.i_size_lo = (uint32_t)p_meta->size;
    raw.i_size_high = (uint32_t)(p_meta->size >> 32);
    raw.i_blocks_lo = p_meta->blocks_512;
    raw.i_blocks_high = (uint16_t)(p_meta->blocks_512 >> 16);
    raw.i_flags = p_meta->flags;
    memcpy(raw.i_block, p_meta->i_block, sizeof(raw.i_block));

    return pfs_ext4_inode_write(p_sbi, p_meta->ino, &raw);
}

// end
