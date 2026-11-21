// Doc Encode : UTF-8 BOM, Unix(LF)
// 辅助与时间戳 (linux-7.1 fs/exfat/misc.c)
#include "exfat/pfs_exfat_internal.h"

void pfs_exfat_chain_set(pfs_exfat_chain_t *p_ec, uint32_t dir, uint32_t size, uint8_t flags)
{
    if (NULL == p_ec)
    {
        return;
    }
    p_ec->dir = dir;
    p_ec->size = size;
    p_ec->flags = flags;
}

void pfs_exfat_chain_dup(pfs_exfat_chain_t *p_dup, const pfs_exfat_chain_t *p_ec)
{
    if (NULL == p_dup || NULL == p_ec)
    {
        return;
    }
    *p_dup = *p_ec;
}

int pfs_exfat_cluster_walk(pfs_exfat_sbi_t *p_sbi, uint32_t *p_clu, uint32_t step, uint8_t flags)
{
    if (NULL == p_sbi || NULL == p_clu)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    uint32_t clu = *p_clu;
    if (EXFAT_ALLOC_NO_FAT_CHAIN == flags)
    {
        clu += step;
        *p_clu = clu;
        return PFS_EXFAT_RET_OK;
    }

    while (step > 0)
    {
        uint32_t next;
        int rc = pfs_exfat_ent_get(p_sbi, clu, &next);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        clu = next;
        step--;
    }

    *p_clu = clu;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_chain_advance(pfs_exfat_sbi_t *p_sbi, pfs_exfat_chain_t *p_chain, uint32_t step)
{
    if (NULL == p_sbi || NULL == p_chain)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    if (p_chain->size < step)
    {
        return PFS_EXFAT_RET_EIO;
    }

    uint32_t clu = p_chain->dir;
    int rc = pfs_exfat_cluster_walk(p_sbi, &clu, step, p_chain->flags);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    p_chain->size -= step;
    if (0 == p_chain->size && EXFAT_ALLOC_NO_FAT_CHAIN == p_chain->flags)
    {
        p_chain->dir = EXFAT_EOF_CLUSTER;
    }
    else
    {
        p_chain->dir = clu;
    }

    return PFS_EXFAT_RET_OK;
}

// end
