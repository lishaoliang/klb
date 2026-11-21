// Doc Encode : UTF-8 BOM, Unix(LF)
// erofs 目录流 (可选合成 '.'; 盘上 dirent)
#include "erofs/pfs_erofs_internal.h"
#include "base/pfsmtd_mem.h"

#include <string.h>
#include <stdbool.h>

static uint8_t pfs_erofs_map_ftype(uint8_t ft, uint16_t mode)
{
    if (PFS_EROFS_FT_DIR == ft || PFS_EROFS_S_IFDIR == (mode & PFS_EROFS_S_IFMT))
    {
        return PFS_DT_DIR;
    }
    if (PFS_EROFS_FT_REG_FILE == ft || PFS_EROFS_S_IFREG == (mode & PFS_EROFS_S_IFMT))
    {
        return PFS_DT_REG;
    }
    return PFS_DT_UNKNOWN;
}

int pfs_erofs_dir_open(pfs_erofs_sbi_t *p_sbi, const char *path, pfs_erofs_dir_meta_t *p_dm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_erofs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_dm, 0, sizeof(*p_dm));
    uint64_t nid = 0;
    bool is_dir = false;
    int rc = pfs_erofs_lookup_path(p_sbi, path, &nid, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }
    rc = pfs_erofs_read_inode(p_sbi, nid, &p_dm->vi);
    if (PFS_OK != rc)
    {
        return rc;
    }
    p_dm->pos = 0;
    p_dm->invent_dot = p_dm->vi.dot_omitted ? 1 : 0;
    return PFS_OK;
}

void pfs_erofs_dir_close(pfs_erofs_sbi_t *p_sbi, pfs_erofs_dir_meta_t *p_dm)
{
    (void)p_sbi;
    if (NULL != p_dm)
    {
        memset(p_dm, 0, sizeof(*p_dm));
    }
}

int pfs_erofs_dir_readdir(pfs_erofs_sbi_t *p_sbi, pfs_erofs_dir_meta_t *p_dm, pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dm || NULL == p_ent)
    {
        return PFS_EINVAL;
    }
    if (!pfs_erofs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_ent, 0, sizeof(*p_ent));
    if (p_dm->invent_dot)
    {
        p_ent->d_type = PFS_DT_DIR;
        p_ent->d_name[0] = '.';
        p_ent->d_name[1] = '\0';
        p_dm->invent_dot = 0;
        return PFS_OK;
    }

    if ((int64_t)p_dm->vi.i_size <= p_dm->pos)
    {
        return PFS_ENOENT;
    }

    uint8_t *blk = (uint8_t *)pfsmtd_malloc((size_t)p_sbi->blksz);
    if (NULL == blk)
    {
        return PFS_ENOMEM;
    }

    while (p_dm->pos < (int64_t)p_dm->vi.i_size)
    {
        int64_t blk_pos = p_dm->pos - (p_dm->pos % (int64_t)p_sbi->blksz);
        unsigned int maxsize = p_sbi->blksz;
        if ((uint64_t)blk_pos + maxsize > p_dm->vi.i_size)
        {
            maxsize = (unsigned int)(p_dm->vi.i_size - (uint64_t)blk_pos);
        }
        int got;
        int rc = pfs_erofs_map_read(p_sbi, &p_dm->vi, blk_pos, blk, (int)maxsize, &got);
        if (PFS_OK != rc)
        {
            pfsmtd_free(blk);
            return rc;
        }
        if ((int)sizeof(pfs_erofs_dirent_t) > got)
        {
            p_dm->pos = blk_pos + (int64_t)p_sbi->blksz;
            continue;
        }

        pfs_erofs_dirent_t *de = (pfs_erofs_dirent_t *)blk;
        unsigned int nameoff0 = de[0].nameoff;
        if (0 == nameoff0 || nameoff0 >= (unsigned int)got || 0 != (nameoff0 % sizeof(pfs_erofs_dirent_t)))
        {
            pfsmtd_free(blk);
            return PFS_EBADMSG;
        }
        unsigned int ndirents = nameoff0 / (unsigned int)sizeof(pfs_erofs_dirent_t);
        unsigned int de_index = (unsigned int)((p_dm->pos - blk_pos) / (int64_t)sizeof(pfs_erofs_dirent_t));
        if (de_index >= ndirents)
        {
            p_dm->pos = blk_pos + (int64_t)p_sbi->blksz;
            continue;
        }

        unsigned int idx = de_index;
        unsigned int nameoff = de[idx].nameoff;
        if (nameoff >= (unsigned int)got)
        {
            p_dm->pos += (int64_t)sizeof(pfs_erofs_dirent_t);
            continue;
        }
        unsigned int namelen;
        if (idx + 1 < ndirents)
        {
            namelen = de[idx + 1].nameoff - nameoff;
        }
        else
        {
            namelen = (unsigned int)strnlen((const char *)(blk + nameoff), (size_t)got - nameoff);
        }

        p_ent->d_type = pfs_erofs_map_ftype(de[idx].file_type, 0);
        size_t ncopy = namelen;
        if (ncopy > sizeof(p_ent->d_name) - 1)
        {
            ncopy = sizeof(p_ent->d_name) - 1;
        }
        memcpy(p_ent->d_name, blk + nameoff, ncopy);
        p_ent->d_name[ncopy] = '\0';
        p_dm->pos += (int64_t)sizeof(pfs_erofs_dirent_t);
        pfsmtd_free(blk);
        return PFS_OK;
    }

    pfsmtd_free(blk);
    return PFS_ENOENT;
}

// end
