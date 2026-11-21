// Doc Encode : UTF-8 BOM, Unix(LF)
// ubifs 内存缓存 (替代完整 TNC; 按 sqnum 胜出)
#include "ubifs/pfs_ubifs_internal.h"
#include "base/pfsmtd_mem.h"

#include <string.h>

int pfs_ubifs_cache_alloc(pfs_ubifs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    p_sbi->p_ics = (pfs_ubifs_ic_t *)pfsmtd_mallocz(sizeof(pfs_ubifs_ic_t) * PFS_UBIFS_CACHE_INO_MAX);
    p_sbi->p_dcs = (pfs_ubifs_dc_t *)pfsmtd_mallocz(sizeof(pfs_ubifs_dc_t) * PFS_UBIFS_CACHE_DENT_MAX);
    p_sbi->p_drefs = (pfs_ubifs_dref_t *)pfsmtd_mallocz(sizeof(pfs_ubifs_dref_t) * PFS_UBIFS_CACHE_DATA_MAX);
    if (NULL == p_sbi->p_ics || NULL == p_sbi->p_dcs || NULL == p_sbi->p_drefs)
    {
        pfs_ubifs_cache_free(p_sbi);
        return PFS_ENOMEM;
    }
    p_sbi->ic_cnt = 0;
    p_sbi->dc_cnt = 0;
    p_sbi->dref_cnt = 0;
    return PFS_OK;
}

void pfs_ubifs_cache_free(pfs_ubifs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    pfsmtd_free(p_sbi->p_ics);
    pfsmtd_free(p_sbi->p_dcs);
    pfsmtd_free(p_sbi->p_drefs);
    p_sbi->p_ics = NULL;
    p_sbi->p_dcs = NULL;
    p_sbi->p_drefs = NULL;
    p_sbi->ic_cnt = 0;
    p_sbi->dc_cnt = 0;
    p_sbi->dref_cnt = 0;
}

pfs_ubifs_ic_t *pfs_ubifs_ic_find(pfs_ubifs_sbi_t *p_sbi, uint64_t inum)
{
    if (NULL == p_sbi || NULL == p_sbi->p_ics)
    {
        return NULL;
    }
    for (int i = 0; i < PFS_UBIFS_CACHE_INO_MAX; i++)
    {
        if (p_sbi->p_ics[i].used && p_sbi->p_ics[i].inum == inum)
        {
            return &p_sbi->p_ics[i];
        }
    }
    return NULL;
}

pfs_ubifs_dc_t *pfs_ubifs_dc_find(pfs_ubifs_sbi_t *p_sbi, uint64_t parent, const char *name, int nlen)
{
    if (NULL == p_sbi || NULL == p_sbi->p_dcs || NULL == name || nlen <= 0)
    {
        return NULL;
    }
    for (int i = 0; i < PFS_UBIFS_CACHE_DENT_MAX; i++)
    {
        if (p_sbi->p_dcs[i].used && p_sbi->p_dcs[i].parent == parent &&
            p_sbi->p_dcs[i].nlen == (uint16_t)nlen &&
            0 == memcmp(p_sbi->p_dcs[i].name, name, (size_t)nlen))
        {
            return &p_sbi->p_dcs[i];
        }
    }
    return NULL;
}

int pfs_ubifs_ic_upsert(pfs_ubifs_sbi_t *p_sbi, uint64_t inum, uint32_t mode, uint64_t size, uint32_t nlink, uint64_t sqnum)
{
    int free_i = -1;
    pfs_ubifs_ic_t *p = pfs_ubifs_ic_find(p_sbi, inum);
    if (NULL != p)
    {
        if (sqnum < p->sqnum)
        {
            return PFS_OK;
        }
        p->mode = mode;
        p->size = size;
        p->nlink = nlink;
        p->sqnum = sqnum;
        return PFS_OK;
    }
    for (int i = 0; i < PFS_UBIFS_CACHE_INO_MAX; i++)
    {
        if (!p_sbi->p_ics[i].used)
        {
            free_i = i;
            break;
        }
    }
    if (free_i < 0)
    {
        return PFS_ENOSPC;
    }
    p = &p_sbi->p_ics[free_i];
    p->inum = inum;
    p->mode = mode;
    p->size = size;
    p->nlink = nlink;
    p->sqnum = sqnum;
    p->used = 1;
    p_sbi->ic_cnt++;
    return PFS_OK;
}

int pfs_ubifs_dc_upsert(pfs_ubifs_sbi_t *p_sbi, uint64_t parent, uint64_t inum, uint8_t type, const char *name, int nlen, uint64_t sqnum)
{
    if (nlen <= 0 || nlen > PFS_UBIFS_MAX_NLEN || NULL == name)
    {
        return PFS_EINVAL;
    }

    int free_i = -1;
    pfs_ubifs_dc_t *p = pfs_ubifs_dc_find(p_sbi, parent, name, nlen);
    if (NULL != p)
    {
        if (sqnum < p->sqnum)
        {
            return PFS_OK;
        }
        p->inum = inum;
        p->type = type;
        p->sqnum = sqnum;
        return PFS_OK;
    }
    for (int i = 0; i < PFS_UBIFS_CACHE_DENT_MAX; i++)
    {
        if (!p_sbi->p_dcs[i].used)
        {
            free_i = i;
            break;
        }
    }
    if (free_i < 0)
    {
        return PFS_ENOSPC;
    }
    p = &p_sbi->p_dcs[free_i];
    p->parent = parent;
    p->inum = inum;
    p->type = type;
    p->nlen = (uint16_t)nlen;
    p->sqnum = sqnum;
    memcpy(p->name, name, (size_t)nlen);
    p->name[nlen] = '\0';
    p->used = 1;
    p_sbi->dc_cnt++;
    return PFS_OK;
}

int pfs_ubifs_dref_upsert(pfs_ubifs_sbi_t *p_sbi, uint64_t inum, uint32_t block, uint32_t size, int lnum, int offs, int data_offs, uint64_t sqnum)
{
    int free_i = -1;

    for (int i = 0; i < PFS_UBIFS_CACHE_DATA_MAX; i++)
    {
        if (p_sbi->p_drefs[i].used && p_sbi->p_drefs[i].inum == inum && p_sbi->p_drefs[i].block == block)
        {
            if (sqnum < p_sbi->p_drefs[i].sqnum)
            {
                return PFS_OK;
            }

            pfs_ubifs_dref_t *p = &p_sbi->p_drefs[i];
            p->size = size;
            p->lnum = lnum;
            p->offs = offs;
            p->data_offs = data_offs;
            p->sqnum = sqnum;
            return PFS_OK;
        }
        if (!p_sbi->p_drefs[i].used && free_i < 0)
        {
            free_i = i;
        }
    }
    if (free_i < 0)
    {
        return PFS_ENOSPC;
    }

    pfs_ubifs_dref_t *p = &p_sbi->p_drefs[free_i];

    p->inum = inum;
    p->block = block;
    p->size = size;
    p->lnum = lnum;
    p->offs = offs;
    p->data_offs = data_offs;
    p->sqnum = sqnum;
    p->used = 1;
    p_sbi->dref_cnt++;
    return PFS_OK;
}

int pfs_ubifs_tnc_lookup(pfs_ubifs_sbi_t *p_sbi, const pfs_ubifs_key_t *p_key, void *node, int node_len)
{
    (void)p_sbi;
    (void)p_key;
    (void)node;
    (void)node_len;
    return PFS_EOPNOTSUPP;
}

// end
