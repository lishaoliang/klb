// Doc Encode : UTF-8 BOM, Unix(LF)
// ext4 路径解析与 mkdir/rmdir/unlink/rename/access
#include "ext4/pfs_ext4_internal.h"
#include "pfs.h"

#include <string.h>

static const char *pfs_ext4_skip_slash(const char *p)
{
    while (NULL != p && '/' == *p)
    {
        p++;
    }
    return p;
}

static int pfs_ext4_next_component(const char *p, char *comp, size_t comp_cap, const char **p_rest)
{
    size_t n = 0;

    if (NULL == p || NULL == comp || 0u == comp_cap || NULL == p_rest)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    p = pfs_ext4_skip_slash(p);
    if ('\0' == *p)
    {
        comp[0] = '\0';
        *p_rest = p;
        return 0;
    }
    while ('\0' != *p && '/' != *p)
    {
        if (n + 1u >= comp_cap || n >= PFS_EXT4_NAME_LEN)
        {
            return PFS_EXT4_RET_ENAMETOOLONG;
        }
        comp[n] = *p;
        n++;
        p++;
    }
    comp[n] = '\0';
    *p_rest = p;
    return (int)n;
}

static int pfs_ext4_init_new_inode(pfs_ext4_sbi_t *p_sbi, pfs_ext4_inode_raw_t *p_raw,
        uint16_t mode)
{
    memset(p_raw, 0, sizeof(*p_raw));
    p_raw->i_mode = mode;
    p_raw->i_links_count = 1;
    if (0u != (p_sbi->feature_incompat & PFS_EXT4_FEATURE_INCOMPAT_EXTENTS))
    {
        pfs_ext4_extent_header_t *p_eh = (pfs_ext4_extent_header_t *)(void *)p_raw->i_block;
        p_raw->i_flags = PFS_EXT4_EXTENTS_FL;
        p_eh->eh_magic = pfs_cpu_to_le16(PFS_EXT4_EXT_MAGIC);
        p_eh->eh_entries = 0;
        p_eh->eh_max = pfs_cpu_to_le16((uint16_t)((PFS_EXT4_N_BLOCKS * 4u -
                (uint32_t)sizeof(pfs_ext4_extent_header_t)) /
                (uint32_t)sizeof(pfs_ext4_extent_t)));
        p_eh->eh_depth = 0;
    }
    return PFS_EXT4_RET_OK;
}

static int pfs_ext4_lookup_component(pfs_ext4_sbi_t *p_sbi, uint32_t dir_ino,
        pfs_ext4_inode_raw_t *p_dir_raw, const char *comp, uint32_t *p_ino,
        pfs_ext4_inode_raw_t *p_child_raw)
{
    uint32_t child_ino = 0;
    uint8_t ftype = 0;
    int rc = pfs_ext4_dir_lookup(p_sbi, dir_ino, p_dir_raw->i_block, p_dir_raw->i_flags,
            pfs_ext4_inode_size(p_dir_raw), comp, &child_ino, &ftype);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ext4_inode_read(p_sbi, child_ino, p_child_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    *p_ino = child_ino;
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_path_lookup(pfs_ext4_sbi_t *p_sbi, const char *path, uint32_t *p_ino,
        pfs_ext4_inode_raw_t *p_raw)
{
    if (NULL == p_sbi || NULL == path || NULL == p_ino || NULL == p_raw)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    pfs_ext4_inode_raw_t dir_raw;
    char comp[PFS_EXT4_NAME_LEN + 1];
    const char *p = pfs_ext4_skip_slash(path);
    uint32_t cur_ino = PFS_EXT4_ROOT_INO;
    int rc = pfs_ext4_inode_read(p_sbi, cur_ino, &dir_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if ('\0' == *p)
    {
        *p_ino = cur_ino;
        memcpy(p_raw, &dir_raw, sizeof(*p_raw));
        return PFS_EXT4_RET_OK;
    }

    for (;;)
    {
        int comp_len = pfs_ext4_next_component(p, comp, sizeof(comp), &p);
        if (0 > comp_len)
        {
            return comp_len;
        }
        if (0 == comp_len)
        {
            break;
        }
        if (PFS_EXT4_S_IFDIR != (dir_raw.i_mode & PFS_EXT4_S_IFMT))
        {
            return PFS_EXT4_RET_ENOENT;
        }
        rc = pfs_ext4_lookup_component(p_sbi, cur_ino, &dir_raw, comp, &cur_ino, p_raw);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        p = pfs_ext4_skip_slash(p);
        if ('\0' == *p)
        {
            *p_ino = cur_ino;
            return PFS_EXT4_RET_OK;
        }
        memcpy(&dir_raw, p_raw, sizeof(dir_raw));
    }

    *p_ino = cur_ino;
    memcpy(p_raw, &dir_raw, sizeof(*p_raw));
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_path_parent(pfs_ext4_sbi_t *p_sbi, const char *path, uint32_t *p_dir_ino,
        pfs_ext4_inode_raw_t *p_dir_raw, char *leaf, size_t leaf_cap)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dir_ino || NULL == p_dir_raw ||
            NULL == leaf || 0u == leaf_cap)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    const char *p = pfs_ext4_skip_slash(path);
    if ('\0' == *p)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    pfs_ext4_inode_raw_t cur_raw;
    char comp[PFS_EXT4_NAME_LEN + 1];
    const char *last_comp = p;
    uint32_t cur_ino = PFS_EXT4_ROOT_INO;
    int rc = pfs_ext4_inode_read(p_sbi, cur_ino, &cur_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    while ('\0' != *p)
    {
        while ('/' == *p)
        {
            p++;
        }
        if ('\0' == *p)
        {
            break;
        }
        last_comp = p;
        int comp_len = pfs_ext4_next_component(p, comp, sizeof(comp), &p);
        if (0 > comp_len)
        {
            return comp_len;
        }
        if (0 == comp_len)
        {
            break;
        }
        if ('\0' != *pfs_ext4_skip_slash(p))
        {
            if (PFS_EXT4_S_IFDIR != (cur_raw.i_mode & PFS_EXT4_S_IFMT))
            {
                return PFS_EXT4_RET_ENOENT;
            }
            rc = pfs_ext4_lookup_component(p_sbi, cur_ino, &cur_raw, comp, &cur_ino, &cur_raw);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
        }
    }

    int comp_len = pfs_ext4_next_component(last_comp, comp, leaf_cap, &p);
    if (0 > comp_len)
    {
        return comp_len;
    }
    if (0 == comp_len)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    memcpy(leaf, comp, (size_t)comp_len + 1u);

    if (last_comp == pfs_ext4_skip_slash(path))
    {
        *p_dir_ino = PFS_EXT4_ROOT_INO;
        return pfs_ext4_inode_read(p_sbi, PFS_EXT4_ROOT_INO, p_dir_raw);
    }

    *p_dir_ino = cur_ino;
    memcpy(p_dir_raw, &cur_raw, sizeof(*p_dir_raw));
    return PFS_EXT4_RET_OK;
}

static int pfs_ext4_dir_is_empty(pfs_ext4_sbi_t *p_sbi, uint32_t dir_ino,
        pfs_ext4_inode_raw_t *p_dir_raw)
{
    char name[PFS_EXT4_NAME_LEN + 1];
    pfs_ext4_dir_meta_t dir;

    memset(&dir, 0, sizeof(dir));
    dir.ino = dir_ino;
    dir.size = pfs_ext4_inode_size(p_dir_raw);
    dir.flags = p_dir_raw->i_flags;
    memcpy(dir.i_block, p_dir_raw->i_block, sizeof(dir.i_block));

    for (;;)
    {
        uint32_t child_ino;
        int rc = pfs_ext4_dir_readdir(p_sbi, &dir, name, (int)sizeof(name), NULL, &child_ino);
        if (PFS_EXT4_RET_ENOENT == rc)
        {
            return PFS_EXT4_RET_OK;
        }
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        if (1 == strlen(name) && '.' == name[0])
        {
            continue;
        }
        if (2 == strlen(name) && '.' == name[0] && '.' == name[1])
        {
            continue;
        }
        return PFS_EXT4_RET_ENOTEMPTY;
    }
}

static int pfs_ext4_inode_free_data(pfs_ext4_sbi_t *p_sbi, pfs_ext4_inode_raw_t *p_raw)
{
    uint64_t size = pfs_ext4_inode_size(p_raw);
    uint32_t num_blks = (uint32_t)((size + (uint64_t)p_sbi->block_size - 1ull) /
            (uint64_t)p_sbi->block_size);

    for (uint32_t lblk = 0; lblk < num_blks; lblk++)
    {
        uint64_t pblk;
        int rc = pfs_ext4_map_block(p_sbi, p_raw->i_block, p_raw->i_flags, lblk, &pblk);
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
        }
    }

    memset(p_raw->i_block, 0, sizeof(p_raw->i_block));
    p_raw->i_blocks_lo = 0;
    p_raw->i_blocks_high = 0;
    p_raw->i_size_lo = 0;
    p_raw->i_size_high = 0;
    if (0u != (p_sbi->feature_incompat & PFS_EXT4_FEATURE_INCOMPAT_EXTENTS))
    {
        (void)pfs_ext4_init_new_inode(p_sbi, p_raw, p_raw->i_mode);
    }
    return PFS_EXT4_RET_OK;
}

static int pfs_ext4_dir_init_dotdot(pfs_ext4_sbi_t *p_sbi, pfs_ext4_inode_raw_t *p_dir_raw,
        uint32_t dir_ino, uint32_t parent_ino)
{
    uint64_t pblk;
    int rc = pfs_ext4_map_block_alloc(p_sbi, p_dir_raw->i_block, &p_dir_raw->i_flags,
            &p_dir_raw->i_blocks_lo, 0u, &pblk);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    uint8_t *p_blk = (uint8_t *)pfs_mallocz(p_sbi->block_size);
    if (NULL == p_blk)
    {
        return PFS_EXT4_RET_ENOMEM;
    }

    pfs_ext4_dir_entry_2_t *p_dot = (pfs_ext4_dir_entry_2_t *)(void *)p_blk;
    p_dot->inode = dir_ino;
    p_dot->rec_len = 12;
    p_dot->name_len = 1;
    p_dot->file_type = PFS_EXT4_FT_DIR;
    p_dot->name[0] = '.';

    pfs_ext4_dir_entry_2_t *p_dotdot = (pfs_ext4_dir_entry_2_t *)(void *)(p_blk + 12);
    p_dotdot->inode = parent_ino;
    p_dotdot->rec_len = (uint16_t)(p_sbi->block_size - 12u);
    p_dotdot->name_len = 2;
    p_dotdot->file_type = PFS_EXT4_FT_DIR;
    p_dotdot->name[0] = '.';
    p_dotdot->name[1] = '.';

    rc = pfs_ext4_write_block(p_sbi, pblk, p_blk);
    pfs_free(p_blk);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    p_dir_raw->i_size_lo = p_sbi->block_size;
    p_dir_raw->i_size_high = 0;
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_mkdir(pfs_ext4_sbi_t *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    pfs_ext4_inode_raw_t parent_raw;
    pfs_ext4_inode_raw_t dir_raw;
    char leaf[PFS_EXT4_NAME_LEN + 1];
    uint32_t parent_ino;
    uint32_t dir_ino;

    int rc = pfs_ext4_path_parent(p_sbi, path, &parent_ino, &parent_raw, leaf, sizeof(leaf));
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_EXT4_S_IFDIR != (parent_raw.i_mode & PFS_EXT4_S_IFMT))
    {
        return PFS_EXT4_RET_ENOENT;
    }

    {
        uint32_t exist_ino;
        rc = pfs_ext4_dir_lookup(p_sbi, parent_ino, parent_raw.i_block, parent_raw.i_flags,
                pfs_ext4_inode_size(&parent_raw), leaf, &exist_ino, NULL);
        if (PFS_EXT4_RET_OK == rc)
        {
            return PFS_EXT4_RET_EEXIST;
        }
        if (PFS_EXT4_RET_ENOENT != rc)
        {
            return rc;
        }
    }

    rc = pfs_ext4_ialloc_new(p_sbi, &dir_ino);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_ext4_init_new_inode(p_sbi, &dir_raw, (uint16_t)(PFS_EXT4_S_IFDIR | 0755u));
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    dir_raw.i_links_count = 2;

    rc = pfs_ext4_dir_init_dotdot(p_sbi, &dir_raw, dir_ino, parent_ino);
    if (PFS_EXT4_RET_OK != rc)
    {
        (void)pfs_ext4_ialloc_free(p_sbi, dir_ino);
        return rc;
    }

    rc = pfs_ext4_inode_write(p_sbi, dir_ino, &dir_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        (void)pfs_ext4_inode_free_data(p_sbi, &dir_raw);
        (void)pfs_ext4_ialloc_free(p_sbi, dir_ino);
        return rc;
    }

    rc = pfs_ext4_dir_add(p_sbi, parent_ino, &parent_raw, leaf, dir_ino, PFS_EXT4_FT_DIR);
    if (PFS_EXT4_RET_OK != rc)
    {
        (void)pfs_ext4_inode_free_data(p_sbi, &dir_raw);
        (void)pfs_ext4_ialloc_free(p_sbi, dir_ino);
        return rc;
    }

    parent_raw.i_links_count++;
    rc = pfs_ext4_inode_write(p_sbi, parent_ino, &parent_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_rmdir(pfs_ext4_sbi_t *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    pfs_ext4_inode_raw_t parent_raw;
    pfs_ext4_inode_raw_t dir_raw;
    char leaf[PFS_EXT4_NAME_LEN + 1];
    uint32_t parent_ino;
    uint32_t dir_ino;

    int rc = pfs_ext4_path_parent(p_sbi, path, &parent_ino, &parent_raw, leaf, sizeof(leaf));
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ext4_dir_lookup(p_sbi, parent_ino, parent_raw.i_block, parent_raw.i_flags,
            pfs_ext4_inode_size(&parent_raw), leaf, &dir_ino, NULL);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ext4_inode_read(p_sbi, dir_ino, &dir_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_EXT4_S_IFDIR != (dir_raw.i_mode & PFS_EXT4_S_IFMT))
    {
        return PFS_EXT4_RET_ENOENT;
    }
    if (PFS_EXT4_ROOT_INO == dir_ino)
    {
        return PFS_EXT4_RET_EPERM;
    }

    rc = pfs_ext4_dir_is_empty(p_sbi, dir_ino, &dir_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_ext4_dir_remove(p_sbi, parent_ino, &parent_raw, leaf);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (parent_raw.i_links_count > 0u)
    {
        parent_raw.i_links_count--;
    }
    rc = pfs_ext4_inode_write(p_sbi, parent_ino, &parent_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    (void)pfs_ext4_inode_free_data(p_sbi, &dir_raw);
    rc = pfs_ext4_ialloc_free(p_sbi, dir_ino);
    return rc;
}

int pfs_ext4_unlink(pfs_ext4_sbi_t *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    pfs_ext4_inode_raw_t parent_raw;
    pfs_ext4_inode_raw_t file_raw;
    char leaf[PFS_EXT4_NAME_LEN + 1];
    uint32_t parent_ino;
    uint32_t file_ino;

    int rc = pfs_ext4_path_parent(p_sbi, path, &parent_ino, &parent_raw, leaf, sizeof(leaf));
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ext4_dir_lookup(p_sbi, parent_ino, parent_raw.i_block, parent_raw.i_flags,
            pfs_ext4_inode_size(&parent_raw), leaf, &file_ino, NULL);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ext4_inode_read(p_sbi, file_ino, &file_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_EXT4_S_IFREG != (file_raw.i_mode & PFS_EXT4_S_IFMT))
    {
        return PFS_EXT4_RET_EPERM;
    }

    rc = pfs_ext4_dir_remove(p_sbi, parent_ino, &parent_raw, leaf);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ext4_inode_write(p_sbi, parent_ino, &parent_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    (void)pfs_ext4_inode_free_data(p_sbi, &file_raw);
    rc = pfs_ext4_ialloc_free(p_sbi, file_ino);
    return rc;
}

int pfs_ext4_rename(pfs_ext4_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    if (NULL == p_sbi || NULL == oldpath || NULL == newpath)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    pfs_ext4_inode_raw_t old_parent;
    pfs_ext4_inode_raw_t new_parent;
    char old_leaf[PFS_EXT4_NAME_LEN + 1];
    char new_leaf[PFS_EXT4_NAME_LEN + 1];
    uint32_t old_parent_ino;
    uint32_t new_parent_ino;

    int rc = pfs_ext4_path_parent(p_sbi, oldpath, &old_parent_ino, &old_parent, old_leaf,
            sizeof(old_leaf));
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ext4_path_parent(p_sbi, newpath, &new_parent_ino, &new_parent, new_leaf,
            sizeof(new_leaf));
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (old_parent_ino != new_parent_ino || 0 != strcmp(old_leaf, new_leaf))
    {
        if (old_parent_ino != new_parent_ino)
        {
            return PFS_EXT4_RET_EOPNOTSUPP;
        }
    }
    else
    {
        return PFS_EXT4_RET_OK;
    }

    uint32_t ino = 0;
    uint8_t ftype = 0;
    rc = pfs_ext4_dir_lookup(p_sbi, old_parent_ino, old_parent.i_block, old_parent.i_flags,
            pfs_ext4_inode_size(&old_parent), old_leaf, &ino, &ftype);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    {
        uint32_t exist;
        rc = pfs_ext4_dir_lookup(p_sbi, new_parent_ino, new_parent.i_block, new_parent.i_flags,
                pfs_ext4_inode_size(&new_parent), new_leaf, &exist, NULL);
        if (PFS_EXT4_RET_OK == rc)
        {
            return PFS_EXT4_RET_EEXIST;
        }
        if (PFS_EXT4_RET_ENOENT != rc)
        {
            return rc;
        }
    }

    rc = pfs_ext4_dir_remove(p_sbi, old_parent_ino, &old_parent, old_leaf);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ext4_dir_add(p_sbi, new_parent_ino, &new_parent, new_leaf, ino, ftype);
    if (PFS_EXT4_RET_OK != rc)
    {
        (void)pfs_ext4_dir_add(p_sbi, old_parent_ino, &old_parent, old_leaf, ino, ftype);
        return rc;
    }

    rc = pfs_ext4_inode_write(p_sbi, old_parent_ino, &old_parent);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ext4_inode_write(p_sbi, new_parent_ino, &new_parent);
}

int pfs_ext4_access(pfs_ext4_sbi_t *p_sbi, const char *path, int mode)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    pfs_ext4_inode_raw_t raw;
    uint32_t ino;
    int rc = pfs_ext4_path_lookup(p_sbi, path, &ino, &raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (0u == mode || PFS_F_OK == mode)
    {
        return PFS_EXT4_RET_OK;
    }
    if (0u != (mode & PFS_R_OK))
    {
        /* MVP: mounted readable */
    }
    if (0u != (mode & PFS_W_OK))
    {
        if (!pfs_ext4_sbi_can_write(p_sbi))
        {
            return PFS_EXT4_RET_EPERM;
        }
        if (PFS_EXT4_S_IFDIR == (raw.i_mode & PFS_EXT4_S_IFMT))
        {
            return PFS_EXT4_RET_OK;
        }
        if (PFS_EXT4_S_IFREG != (raw.i_mode & PFS_EXT4_S_IFMT))
        {
            return PFS_EXT4_RET_EPERM;
        }
    }
    return PFS_EXT4_RET_OK;
}

// end
