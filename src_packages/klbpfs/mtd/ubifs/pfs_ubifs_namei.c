// Doc Encode : UTF-8 BOM, Unix(LF)
// ubifs 路径查找 / mkdir / creat (扫描缓存写子集)
#include "ubifs/pfs_ubifs_internal.h"
#include "base/pfsmtd_mem.h"

#include <string.h>
#include <stdbool.h>

static int pfs_ubifs_component(const char **pp, char *out, int out_cap)
{
    const char *p = *pp;
    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        *pp = p;
        return 0;
    }
    int n = 0;
    while ('\0' != *p && '/' != *p)
    {
        if (out_cap <= n + 1)
        {
            return -1;
        }
        out[n++] = *p++;
    }
    out[n] = '\0';
    *pp = p;
    return n;
}

static int pfs_ubifs_write_ino_node(pfs_ubifs_sbi_t *p_sbi, uint64_t inum, uint32_t mode, uint32_t nlink, uint64_t size)
{
    uint8_t *ino = (uint8_t *)pfsmtd_mallocz(PFS_UBIFS_INO_NODE_SZ);
    if (NULL == ino)
    {
        return PFS_ENOMEM;
    }
    pfs_ubifs_key_t key;
    pfs_ubifs_ino_key_init(&key, inum);
    pfs_ubifs_key_write_flash(ino + PFS_UBIFS_OFF_KEY, &key);
    pfs_ubifs_put_le64(ino + PFS_UBIFS_INO_OFF_CREAT_SQ, p_sbi->max_sqnum + 1);
    pfs_ubifs_put_le64(ino + PFS_UBIFS_INO_OFF_SIZE, size);
    pfs_ubifs_put_le32(ino + PFS_UBIFS_INO_OFF_NLINK, nlink);
    pfs_ubifs_put_le32(ino + PFS_UBIFS_INO_OFF_MODE, mode);
    pfs_ubifs_put_le16(ino + PFS_UBIFS_INO_OFF_COMPR, PFS_UBIFS_COMPR_NONE);
    ino[20] = PFS_UBIFS_INO_NODE;


    int rc = pfs_ubifs_append_node(p_sbi, ino, PFS_UBIFS_INO_NODE_SZ);
    if (PFS_OK == rc)
    {
        rc = pfs_ubifs_ic_upsert(p_sbi, inum, mode, size, nlink, p_sbi->max_sqnum);
    }
    pfsmtd_free(ino);
    return rc;
}

static int pfs_ubifs_write_dent_node(pfs_ubifs_sbi_t *p_sbi, uint64_t parent, uint64_t inum, uint8_t type, const char *name, int nlen)
{
    int total = PFS_UBIFS_ALIGN8(PFS_UBIFS_DENT_NODE_SZ + nlen + 1);
    uint8_t *dent = (uint8_t *)pfsmtd_mallocz((size_t)total);
    if (NULL == dent)
    {
        return PFS_ENOMEM;
    }
    pfs_ubifs_key_t key;
    pfs_ubifs_dent_key_init(&key, parent, name, nlen);
    pfs_ubifs_key_write_flash(dent + PFS_UBIFS_OFF_KEY, &key);
    pfs_ubifs_put_le64(dent + PFS_UBIFS_DENT_OFF_INUM, inum);
    dent[PFS_UBIFS_DENT_OFF_TYPE] = type;
    pfs_ubifs_put_le16(dent + PFS_UBIFS_DENT_OFF_NLEN, (uint16_t)nlen);
    memcpy(dent + PFS_UBIFS_DENT_OFF_NAME, name, (size_t)nlen);
    dent[PFS_UBIFS_DENT_OFF_NAME + nlen] = '\0';
    dent[20] = PFS_UBIFS_DENT_NODE;
    // ch.len 须为实际节点长 (含名+NUL), 非对齐
    int nlen_node = PFS_UBIFS_DENT_NODE_SZ + nlen + 1;
    int rc = pfs_ubifs_append_node(p_sbi, dent, nlen_node);
    if (PFS_OK == rc)
    {
        rc = pfs_ubifs_dc_upsert(p_sbi, parent, inum, type, name, nlen, p_sbi->max_sqnum);
    }
    pfsmtd_free(dent);
    return rc;
}

int pfs_ubifs_lookup_path(pfs_ubifs_sbi_t *p_sbi, const char *path, uint64_t *p_inum, bool *p_is_dir)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_ubifs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    uint64_t cur = PFS_UBIFS_ROOT_INO;
    const char *p = path;
    char name[PFS_UBIFS_MAX_NLEN + 1];
    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        if (NULL != p_inum)
        {
            *p_inum = cur;
        }
        if (NULL != p_is_dir)
        {
            *p_is_dir = true;
        }
        return PFS_OK;
    }

    while ('\0' != *p)
    {
        int nlen = pfs_ubifs_component(&p, name, (int)sizeof(name));
        if (0 > nlen)
        {
            return PFS_ENAMETOOLONG;
        }
        if (0 == nlen)
        {
            break;
        }
        if (1 == nlen && '.' == name[0])
        {
            continue;
        }
        if (2 == nlen && '.' == name[0] && '.' == name[1])
        {
            if (PFS_UBIFS_ROOT_INO != cur)
            {
                return PFS_ENOENT;
            }
            continue;
        }
        pfs_ubifs_dc_t *dc = pfs_ubifs_dc_find(p_sbi, cur, name, nlen);
        if (NULL == dc)
        {
            return PFS_ENOENT;
        }
        cur = dc->inum;
    }

    pfs_ubifs_ic_t *ic = pfs_ubifs_ic_find(p_sbi, cur);
    if (NULL == ic)
    {
        return PFS_ENOENT;
    }
    if (NULL != p_inum)
    {
        *p_inum = cur;
    }
    if (NULL != p_is_dir)
    {
        *p_is_dir = (PFS_UBIFS_S_IFDIR == (ic->mode & PFS_UBIFS_S_IFMT));
    }
    return PFS_OK;
}

int pfs_ubifs_access(pfs_ubifs_sbi_t *p_sbi, const char *path, int mode)
{
    uint64_t inum;
    bool is_dir;

    (void)mode;
    int rc = pfs_ubifs_lookup_path(p_sbi, path, &inum, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    (void)inum;
    (void)is_dir;
    return PFS_OK;
}

static int pfs_ubifs_split_parent(const char *path, char *parent, int parent_cap, char *leaf, int leaf_cap)
{
    const char *p = path;
    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        return PFS_EINVAL;
    }

    const char *slash = strrchr(p, '/');
    if (NULL == slash)
    {
        if (parent_cap < 2 || leaf_cap <= (int)strlen(p))
        {
            return PFS_ENAMETOOLONG;
        }
        parent[0] = '/';
        parent[1] = '\0';
        memcpy(leaf, p, (size_t)strlen(p) + 1);
        return PFS_OK;
    }
    int plen = (int)(slash - path);
    if (0 == plen)
    {
        plen = 1;
    }
    int llen = (int)strlen(slash + 1);
    if (llen <= 0 || plen >= parent_cap || llen >= leaf_cap)
    {
        return PFS_ENAMETOOLONG;
    }
    memcpy(parent, path, (size_t)plen);
    parent[plen] = '\0';
    memcpy(leaf, slash + 1, (size_t)llen + 1);
    return PFS_OK;
}

int pfs_ubifs_mkdir(pfs_ubifs_sbi_t *p_sbi, const char *path, int mode)
{
    (void)mode;
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_ubifs_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }

    char parent[512];
    char leaf[PFS_UBIFS_MAX_NLEN + 1];
    int rc = pfs_ubifs_split_parent(path, parent, (int)sizeof(parent), leaf, (int)sizeof(leaf));
    if (PFS_OK != rc)
    {
        return rc;
    }
    uint64_t pinum;
    bool is_dir;
    rc = pfs_ubifs_lookup_path(p_sbi, parent, &pinum, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }
    if (NULL != pfs_ubifs_dc_find(p_sbi, pinum, leaf, (int)strlen(leaf)))
    {
        return PFS_EEXIST;
    }
    p_sbi->highest_inum++;
    uint64_t ninum = p_sbi->highest_inum;
    rc = pfs_ubifs_write_ino_node(p_sbi, ninum, PFS_UBIFS_S_IFDIR | 0755, 2, 0);
    if (PFS_OK != rc)
    {
        return rc;
    }
    rc = pfs_ubifs_write_dent_node(p_sbi, pinum, ninum, PFS_UBIFS_ITYPE_DIR, leaf, (int)strlen(leaf));
    if (PFS_OK != rc)
    {
        return rc;
    }
    pfs_ubifs_ic_t *pic = pfs_ubifs_ic_find(p_sbi, pinum);
    if (NULL != pic)
    {
        (void)pfs_ubifs_write_ino_node(p_sbi, pinum, pic->mode, pic->nlink + 1, pic->size);
    }
    return pfs_ubifs_flush_mst(p_sbi);
}

int pfs_ubifs_creat(pfs_ubifs_sbi_t *p_sbi, const char *path, uint64_t *p_inum)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_ubifs_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }

    char parent[512];
    char leaf[PFS_UBIFS_MAX_NLEN + 1];
    int rc = pfs_ubifs_split_parent(path, parent, (int)sizeof(parent), leaf, (int)sizeof(leaf));
    if (PFS_OK != rc)
    {
        return rc;
    }
    uint64_t pinum;
    bool is_dir;
    rc = pfs_ubifs_lookup_path(p_sbi, parent, &pinum, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }
    if (NULL != pfs_ubifs_dc_find(p_sbi, pinum, leaf, (int)strlen(leaf)))
    {
        return PFS_EEXIST;
    }
    p_sbi->highest_inum++;
    uint64_t ninum = p_sbi->highest_inum;
    rc = pfs_ubifs_write_ino_node(p_sbi, ninum, PFS_UBIFS_S_IFREG | 0644, 1, 0);
    if (PFS_OK != rc)
    {
        return rc;
    }
    rc = pfs_ubifs_write_dent_node(p_sbi, pinum, ninum, PFS_UBIFS_ITYPE_REG, leaf, (int)strlen(leaf));
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (NULL != p_inum)
    {
        *p_inum = ninum;
    }
    return pfs_ubifs_flush_mst(p_sbi);
}

int pfs_ubifs_rmdir(pfs_ubifs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

int pfs_ubifs_rename(pfs_ubifs_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    (void)p_sbi;
    (void)oldpath;
    (void)newpath;
    return PFS_EPERM;
}

int pfs_ubifs_unlink(pfs_ubifs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

// end
