// Doc Encode : UTF-8 BOM, Unix(LF)
// ext4 线性目录 (dir_entry_2; 不支持 htree INDEX_FL)
#include "ext4/pfs_ext4_internal.h"

#include <string.h>

#define PFS_EXT4_DIR_REC_LEN(namlen)    ((((namlen) + 8u + 3u) & ~3u))

static int pfs_ext4_dir_check_index(uint32_t flags)
{
    if (0u != (flags & PFS_EXT4_INDEX_FL))
    {
        return PFS_EXT4_RET_EOPNOTSUPP;
    }
    return PFS_EXT4_RET_OK;
}

static int pfs_ext4_dir_name_eq(const pfs_ext4_dir_entry_2_t *p_de, const char *name, size_t name_len)
{
    if (NULL == p_de || NULL == name)
    {
        return 0;
    }
    if (p_de->name_len != name_len)
    {
        return 0;
    }
    if (name_len > PFS_EXT4_NAME_LEN)
    {
        return 0;
    }
    return (0 == memcmp(p_de->name, name, name_len)) ? 1 : 0;
}

static int pfs_ext4_dir_skip_entry(const pfs_ext4_dir_entry_2_t *p_de)
{
    if (NULL == p_de)
    {
        return 1;
    }
    if (0u == p_de->inode)
    {
        return 1;
    }
    if (PFS_EXT4_FT_DIR_CSUM == p_de->file_type)
    {
        return 1;
    }
    if (p_de->rec_len < 12u)
    {
        return 1;
    }
    return 0;
}

static int pfs_ext4_dir_iter_block(pfs_ext4_sbi_t *p_sbi,
        const uint32_t i_block[PFS_EXT4_N_BLOCKS], uint32_t flags, uint64_t dir_size,
        uint32_t lblk, uint8_t *p_blk, uint64_t *p_pblk)
{

    int rc = pfs_ext4_map_block(p_sbi, i_block, flags, lblk, p_pblk);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (0ull == *p_pblk)
    {
        return PFS_EXT4_RET_EIO;
    }
    if ((uint64_t)lblk * (uint64_t)p_sbi->block_size >= dir_size)
    {
        return PFS_EXT4_RET_ENOENT;
    }
    return pfs_ext4_read_block(p_sbi, *p_pblk, p_blk);
}

int pfs_ext4_dir_lookup(pfs_ext4_sbi_t *p_sbi, uint32_t dir_ino,
        const uint32_t dir_i_block[PFS_EXT4_N_BLOCKS], uint32_t dir_flags, uint64_t dir_size,
        const char *name, uint32_t *p_ino, uint8_t *p_ftype)
{
    (void)dir_ino;
    if (NULL == p_sbi || NULL == dir_i_block || NULL == name || NULL == p_ino)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    int rc = pfs_ext4_dir_check_index(dir_flags);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    size_t name_len = strlen(name);
    if (0u == name_len || name_len > PFS_EXT4_NAME_LEN)
    {
        return PFS_EXT4_RET_ENAMETOOLONG;
    }

    uint8_t *p_blk = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_blk)
    {
        return PFS_EXT4_RET_ENOMEM;
    }

    uint32_t num_blks = (uint32_t)((dir_size + (uint64_t)p_sbi->block_size - 1ull) /
            (uint64_t)p_sbi->block_size);
    if (0u == num_blks && dir_size > 0ull)
    {
        num_blks = 1u;
    }

    for (uint32_t lblk = 0; lblk < num_blks; lblk++)
    {
        uint64_t pblk;
        uint32_t off;

        rc = pfs_ext4_dir_iter_block(p_sbi, dir_i_block, dir_flags, dir_size, lblk, p_blk, &pblk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_blk);
            return rc;
        }

        off = 0;
        while (off + 12u <= p_sbi->block_size)
        {
            pfs_ext4_dir_entry_2_t *p_de = (pfs_ext4_dir_entry_2_t *)(void *)(p_blk + off);
            uint16_t rec_len = p_de->rec_len;

            if (rec_len < 12u || off + rec_len > p_sbi->block_size)
            {
                break;
            }
            if (!pfs_ext4_dir_skip_entry(p_de) &&
                    pfs_ext4_dir_name_eq(p_de, name, name_len))
            {
                *p_ino = p_de->inode;
                if (NULL != p_ftype)
                {
                    *p_ftype = p_de->file_type;
                }
                pfs_free(p_blk);
                return PFS_EXT4_RET_OK;
            }
            off += rec_len;
        }
    }

    pfs_free(p_blk);
    return PFS_EXT4_RET_ENOENT;
}

int pfs_ext4_dir_readdir(pfs_ext4_sbi_t *p_sbi, pfs_ext4_dir_meta_t *p_dir,
        char *name, int name_cap, pfs_ext4_type_e *p_type, uint32_t *p_child_ino)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == name || name_cap <= 0)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    int rc = pfs_ext4_dir_check_index(p_dir->flags);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    if (p_dir->pos >= p_dir->size)
    {
        return PFS_EXT4_RET_ENOENT;
    }

    uint8_t *p_blk = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_blk)
    {
        return PFS_EXT4_RET_ENOMEM;
    }

    uint64_t pos = p_dir->pos;
    while (pos < p_dir->size)
    {
        uint32_t lblk = (uint32_t)(pos / (uint64_t)p_sbi->block_size);
        uint32_t off = (uint32_t)(pos % (uint64_t)p_sbi->block_size);
        uint64_t pblk;

        rc = pfs_ext4_dir_iter_block(p_sbi, p_dir->i_block, p_dir->flags, p_dir->size,
                lblk, p_blk, &pblk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_blk);
            return rc;
        }

        while (off + 12u <= p_sbi->block_size)
        {
            pfs_ext4_dir_entry_2_t *p_de = (pfs_ext4_dir_entry_2_t *)(void *)(p_blk + off);
            uint16_t rec_len = p_de->rec_len;
            uint8_t nlen;

            if (rec_len < 12u || off + rec_len > p_sbi->block_size)
            {
                pfs_free(p_blk);
                return PFS_EXT4_RET_EBADMSG;
            }
            pos = (uint64_t)lblk * (uint64_t)p_sbi->block_size + (uint64_t)off + (uint64_t)rec_len;
            if (!pfs_ext4_dir_skip_entry(p_de))
            {
                nlen = p_de->name_len;
                if ((int)nlen + 1 > name_cap)
                {
                    pfs_free(p_blk);
                    return PFS_EXT4_RET_ENAMETOOLONG;
                }
                memcpy(name, p_de->name, nlen);
                name[nlen] = '\0';
                if (NULL != p_child_ino)
                {
                    *p_child_ino = p_de->inode;
                }
                if (NULL != p_type)
                {
                    if (PFS_EXT4_FT_DIR == p_de->file_type)
                    {
                        *p_type = PFS_EXT4_TYPE_DIR;
                    }
                    else if (PFS_EXT4_FT_REG_FILE == p_de->file_type)
                    {
                        *p_type = PFS_EXT4_TYPE_FILE;
                    }
                    else
                    {
                        *p_type = PFS_EXT4_TYPE_OTHER;
                    }
                }
                p_dir->pos = pos;
                pfs_free(p_blk);
                return PFS_EXT4_RET_OK;
            }
            off += rec_len;
        }
        pos = (uint64_t)(lblk + 1u) * (uint64_t)p_sbi->block_size;
    }

    pfs_free(p_blk);
    return PFS_EXT4_RET_ENOENT;
}

static int pfs_ext4_dir_find_slot(uint8_t *p_blk, uint32_t block_size, uint16_t need,
        uint32_t *p_off, uint16_t *p_avail)
{
    uint32_t off = 0;

    while (off + 12u <= block_size)
    {
        pfs_ext4_dir_entry_2_t *p_de = (pfs_ext4_dir_entry_2_t *)(void *)(p_blk + off);
        uint16_t rec_len = p_de->rec_len;
        uint16_t actual;

        if (rec_len < 12u || off + rec_len > block_size)
        {
            return PFS_EXT4_RET_EBADMSG;
        }
        if (0u == p_de->inode)
        {
            if (rec_len >= need)
            {
                *p_off = off;
                *p_avail = rec_len;
                return PFS_EXT4_RET_OK;
            }
        }
        else
        {
            actual = (uint16_t)PFS_EXT4_DIR_REC_LEN(p_de->name_len);
            if (rec_len >= actual + need)
            {
                p_de->rec_len = actual;
                *p_off = off + actual;
                *p_avail = (uint16_t)(rec_len - actual);
                return PFS_EXT4_RET_OK;
            }
        }
        off += rec_len;
    }
    return PFS_EXT4_RET_ENOENT;
}

int pfs_ext4_dir_add(pfs_ext4_sbi_t *p_sbi, uint32_t dir_ino, pfs_ext4_inode_raw_t *p_dir_raw,
        const char *name, uint32_t child_ino, uint8_t ftype)
{
    (void)dir_ino;
    if (NULL == p_sbi || NULL == p_dir_raw || NULL == name || 0u == child_ino)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }
    int rc = pfs_ext4_dir_check_index(p_dir_raw->i_flags);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    size_t name_len = strlen(name);
    if (0u == name_len || name_len > PFS_EXT4_NAME_LEN)
    {
        return PFS_EXT4_RET_ENAMETOOLONG;
    }
    uint16_t need = (uint16_t)PFS_EXT4_DIR_REC_LEN(name_len);

    uint8_t *p_blk = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_blk)
    {
        return PFS_EXT4_RET_ENOMEM;
    }

    uint64_t new_size = pfs_ext4_inode_size(p_dir_raw);
    uint32_t num_blks = (uint32_t)((new_size + (uint64_t)p_sbi->block_size - 1ull) /
            (uint64_t)p_sbi->block_size);

    for (uint32_t lblk = 0; lblk < num_blks; lblk++)
    {
        uint64_t pblk;
        uint32_t off;
        uint16_t avail;

        rc = pfs_ext4_map_block(p_sbi, p_dir_raw->i_block, p_dir_raw->i_flags, lblk, &pblk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_blk);
            return rc;
        }
        if (0ull == pblk)
        {
            continue;
        }
        rc = pfs_ext4_read_block(p_sbi, pblk, p_blk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_blk);
            return rc;
        }
        rc = pfs_ext4_dir_find_slot(p_blk, p_sbi->block_size, need, &off, &avail);
        if (PFS_EXT4_RET_OK == rc)
        {
            pfs_ext4_dir_entry_2_t *p_new = (pfs_ext4_dir_entry_2_t *)(void *)(p_blk + off);
            uint16_t use_rec = need;
            uint16_t left;

            if (avail > need)
            {
                left = (uint16_t)(avail - need);
                p_new->rec_len = need;
                {
                    pfs_ext4_dir_entry_2_t *p_rest =
                            (pfs_ext4_dir_entry_2_t *)(void *)(p_blk + off + need);
                    p_rest->rec_len = left;
                    p_rest->inode = 0;
                    p_rest->name_len = 0;
                    p_rest->file_type = PFS_EXT4_FT_UNKNOWN;
                }
            }
            else
            {
                use_rec = avail;
            }
            p_new->inode = child_ino;
            p_new->rec_len = use_rec;
            p_new->name_len = (uint8_t)name_len;
            p_new->file_type = ftype;
            memcpy(p_new->name, name, name_len);

            rc = pfs_ext4_write_block(p_sbi, pblk, p_blk);
            pfs_free(p_blk);
            return rc;
        }
    }

    /* append new block */
    uint32_t lblk = num_blks;
    uint64_t pblk;
    rc = pfs_ext4_map_block_alloc(p_sbi, p_dir_raw->i_block, &p_dir_raw->i_flags,
            &p_dir_raw->i_blocks_lo, lblk, &pblk);
    if (PFS_EXT4_RET_OK != rc)
    {
        pfs_free(p_blk);
        return rc;
    }

    memset(p_blk, 0, p_sbi->block_size);
    {
        pfs_ext4_dir_entry_2_t *p_de = (pfs_ext4_dir_entry_2_t *)(void *)p_blk;
        p_de->inode = child_ino;
        p_de->rec_len = (uint16_t)p_sbi->block_size;
        p_de->name_len = (uint8_t)name_len;
        p_de->file_type = ftype;
        memcpy(p_de->name, name, name_len);
    }

    rc = pfs_ext4_write_block(p_sbi, pblk, p_blk);
    if (PFS_EXT4_RET_OK != rc)
    {
        pfs_free(p_blk);
        return rc;
    }

    new_size = (uint64_t)(lblk + 1u) * (uint64_t)p_sbi->block_size;
    p_dir_raw->i_size_lo = (uint32_t)new_size;
    p_dir_raw->i_size_high = (uint32_t)(new_size >> 32);

    pfs_free(p_blk);
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_dir_remove(pfs_ext4_sbi_t *p_sbi, uint32_t dir_ino, pfs_ext4_inode_raw_t *p_dir_raw,
        const char *name)
{
    (void)dir_ino;
    if (NULL == p_sbi || NULL == p_dir_raw || NULL == name)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }
    int rc = pfs_ext4_dir_check_index(p_dir_raw->i_flags);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    size_t name_len = strlen(name);
    if (0u == name_len)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    uint8_t *p_blk = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_blk)
    {
        return PFS_EXT4_RET_ENOMEM;
    }

    uint32_t num_blks = (uint32_t)((pfs_ext4_inode_size(p_dir_raw) + (uint64_t)p_sbi->block_size - 1ull) /
            (uint64_t)p_sbi->block_size);

    for (uint32_t lblk = 0; lblk < num_blks; lblk++)
    {
        uint64_t pblk;

        rc = pfs_ext4_map_block(p_sbi, p_dir_raw->i_block, p_dir_raw->i_flags, lblk, &pblk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_blk);
            return rc;
        }
        if (0ull == pblk)
        {
            continue;
        }
        rc = pfs_ext4_read_block(p_sbi, pblk, p_blk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_blk);
            return rc;
        }

        uint32_t off = 0;
        while (off + 12u <= p_sbi->block_size)
        {
            pfs_ext4_dir_entry_2_t *p_de = (pfs_ext4_dir_entry_2_t *)(void *)(p_blk + off);
            pfs_ext4_dir_entry_2_t *p_prev = NULL;
            uint16_t rec_len = p_de->rec_len;
            uint32_t scan;

            if (rec_len < 12u || off + rec_len > p_sbi->block_size)
            {
                break;
            }
            if (!pfs_ext4_dir_skip_entry(p_de) && pfs_ext4_dir_name_eq(p_de, name, name_len))
            {
                scan = 0;
                while (scan < off)
                {
                    pfs_ext4_dir_entry_2_t *p_tmp =
                            (pfs_ext4_dir_entry_2_t *)(void *)(p_blk + scan);
                    if (scan + p_tmp->rec_len == off)
                    {
                        p_prev = p_tmp;
                        break;
                    }
                    scan += p_tmp->rec_len;
                }
                if (NULL != p_prev)
                {
                    p_prev->rec_len = (uint16_t)(p_prev->rec_len + rec_len);
                }
                else
                {
                    p_de->inode = 0;
                    p_de->name_len = 0;
                    p_de->file_type = PFS_EXT4_FT_UNKNOWN;
                }
                rc = pfs_ext4_write_block(p_sbi, pblk, p_blk);
                pfs_free(p_blk);
                return rc;
            }
            off += rec_len;
        }
    }

    pfs_free(p_blk);
    return PFS_EXT4_RET_ENOENT;
}

// end
