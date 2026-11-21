// Doc Encode : UTF-8 BOM, Unix(LF)
// 文件读写与簇映射 (linux-7.1 fs/fat/inode.c + cache.c fat_bmap / fat_get_cluster)
#include "fat/pfs_fat_internal.h"

#include <string.h>

int pfs_fat_map_cluster(pfs_fat_sbi_t *p_sbi, pfs_fat_file_meta_t *p_meta,
        uint32_t clu_off, uint32_t *p_clu)
{
    if (NULL == p_sbi || NULL == p_meta || NULL == p_clu)
    {
        return PFS_FAT_RET_EINVAL;
    }

    if (0 == p_meta->start_clu || FAT_ENT_FREE == p_meta->start_clu ||
            FAT_ENT_EOF == p_meta->start_clu)
    {
        *p_clu = FAT_ENT_EOF;
        return PFS_FAT_RET_OK;
    }
    if (!pfs_fat_is_valid_cluster(p_sbi, p_meta->start_clu))
    {
        return PFS_FAT_RET_EIO;
    }

    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);
    if (p_meta->size <= 0 || 0 == p_bs->cluster_size)
    {
        *p_clu = FAT_ENT_EOF;
        return PFS_FAT_RET_OK;
    }
    uint32_t num_clusters = (uint32_t)(((uint64_t)p_meta->size + (uint64_t)p_bs->cluster_size - 1ULL) >>
            p_bs->cluster_size_bits);
    if (clu_off >= num_clusters)
    {
        *p_clu = FAT_ENT_EOF;
        return PFS_FAT_RET_OK;
    }

    // 顺序读提示: 目标偏移 >= hint 时从 hint 前进, 否则从文件首簇重走
    uint32_t clu;
    uint32_t steps;
    if (pfs_fat_is_valid_cluster(p_sbi, p_meta->hint_clu) &&
            clu_off >= p_meta->hint_clu_off)
    {
        clu = p_meta->hint_clu;
        steps = clu_off - p_meta->hint_clu_off;
    }
    else
    {
        clu = p_meta->start_clu;
        steps = clu_off;
    }

    int rc = pfs_fat_cluster_walk(p_sbi, &clu, steps);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    if (FAT_ENT_EOF == clu || FAT_ENT_FREE == clu || !pfs_fat_is_valid_cluster(p_sbi, clu))
    {
        return PFS_FAT_RET_EIO;
    }

    *p_clu = clu;
    p_meta->hint_clu_off = clu_off;
    p_meta->hint_clu = clu;
    return PFS_FAT_RET_OK;
}

int pfs_fat_file_read(pfs_fat_sbi_t *p_sbi, pfs_fat_file_meta_t *p_meta,
        int64_t pos, void *buf, int len, int *p_got)
{
    if (NULL == p_sbi || NULL == p_meta || NULL == buf || NULL == p_got || len <= 0)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (pos < 0)
    {
        return PFS_FAT_RET_EINVAL;
    }

    *p_got = 0;
    if (pos >= p_meta->size)
    {
        return PFS_FAT_RET_OK;
    }

    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);
    uint32_t cluster_size = p_bs->cluster_size;
    uint32_t sector_size = p_bs->sector_size;

    if (0 == cluster_size || 0 == sector_size)
    {
        return PFS_FAT_RET_EBADMSG;
    }

    int64_t end = pos + (int64_t)len;
    if (end > p_meta->size)
    {
        end = p_meta->size;
    }

    uint8_t *dst = (uint8_t *)buf;
    int64_t cur = pos;
    int remain = (int)(end - pos);
    int got = 0;

    while (remain > 0)
    {
        uint32_t clu_off = (uint32_t)((uint64_t)cur >> p_bs->cluster_size_bits);
        uint32_t in_clu = (uint32_t)((uint64_t)cur & (uint64_t)(cluster_size - 1U));
        uint32_t clu;

        int rc = pfs_fat_map_cluster(p_sbi, p_meta, clu_off, &clu);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (FAT_ENT_EOF == clu)
        {
            return PFS_FAT_RET_EIO;
        }

        int chunk = (int)(cluster_size - in_clu);

        if (chunk > remain)
        {
            chunk = remain;
        }

        uint32_t byte_in_sec = (uint32_t)(((uint64_t)cur) & (uint64_t)(sector_size - 1U));
        int max_in_sec = (int)(sector_size - byte_in_sec);

        if (chunk > max_in_sec)
        {
            chunk = max_in_sec;
        }

        int64_t byte_off = pfs_fat_cluster_to_byte(p_sbi, clu) + (int64_t)in_clu;
        rc = pfs_fat_pread_at(p_sbi, dst + got, chunk, byte_off);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }

        got += chunk;
        cur += chunk;
        remain -= chunk;
    }

    *p_got = got;
    return PFS_FAT_RET_OK;
}

static int pfs_fat_file_extend_to(pfs_fat_sbi_t *p_sbi, pfs_fat_file_meta_t *p_meta, uint32_t need_clu)
{
    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);

    if (0 == need_clu)
    {
        return PFS_FAT_RET_OK;
    }

    uint32_t have;
    uint32_t last;
    int rc;

    if (0 == p_meta->start_clu || FAT_ENT_EOF == p_meta->start_clu || p_meta->size <= 0)
    {
        uint32_t new_clu;

        rc = pfs_fat_alloc_cluster(p_sbi, 0, &new_clu);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        p_meta->start_clu = new_clu;
        p_meta->hint_clu = new_clu;
        p_meta->hint_clu_off = 0;
        have = 1;
        last = new_clu;
    }
    else
    {
        have = (uint32_t)(((uint64_t)p_meta->size + (uint64_t)p_bs->cluster_size - 1ULL) >>
                p_bs->cluster_size_bits);
        if (0 == have)
        {
            have = 1;
        }
        last = p_meta->start_clu;
        rc = pfs_fat_cluster_walk(p_sbi, &last, have - 1U);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
    }

    while (have < need_clu)
    {
        uint32_t new_clu;

        rc = pfs_fat_alloc_cluster(p_sbi, last, &new_clu);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_fat_ent_set(p_sbi, last, new_clu);
        if (PFS_FAT_RET_OK != rc)
        {
            (void)pfs_fat_free_cluster_chain(p_sbi, new_clu);
            return rc;
        }
        last = new_clu;
        have++;
    }
    return PFS_FAT_RET_OK;
}

int pfs_fat_file_write(pfs_fat_sbi_t *p_sbi, pfs_fat_file_meta_t *p_meta,
        int64_t pos, const void *buf, int len, int *p_put)
{
    if (NULL == p_sbi || NULL == p_meta || NULL == buf || NULL == p_put || len <= 0)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (pos < 0 || !pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EINVAL;
    }

    *p_put = 0;

    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);
    uint32_t cluster_size = p_bs->cluster_size;
    uint32_t sector_size = p_bs->sector_size;

    if (0 == cluster_size || 0 == sector_size)
    {
        return PFS_FAT_RET_EBADMSG;
    }

    int64_t end = pos + (int64_t)len;
    uint32_t need_clu = (uint32_t)(((uint64_t)end + (uint64_t)cluster_size - 1ULL) >> p_bs->cluster_size_bits);

    int rc = pfs_fat_file_extend_to(p_sbi, p_meta, need_clu);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    if (end > p_meta->size)
    {
        p_meta->size = end;
    }

    const uint8_t *src = (const uint8_t *)buf;
    int64_t cur = pos;
    int remain = len;
    int put = 0;

    while (remain > 0)
    {
        uint32_t clu_off = (uint32_t)((uint64_t)cur >> p_bs->cluster_size_bits);
        uint32_t in_clu = (uint32_t)((uint64_t)cur & (uint64_t)(cluster_size - 1U));
        uint32_t clu;

        rc = pfs_fat_map_cluster(p_sbi, p_meta, clu_off, &clu);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (FAT_ENT_EOF == clu)
        {
            return PFS_FAT_RET_EIO;
        }
        int chunk = (int)(cluster_size - in_clu);

        if (chunk > remain)
        {
            chunk = remain;
        }

        uint32_t byte_in_sec = (uint32_t)(((uint64_t)cur) & (uint64_t)(sector_size - 1U));
        int max_in_sec = (int)(sector_size - byte_in_sec);

        if (chunk > max_in_sec)
        {
            chunk = max_in_sec;
        }

        int64_t byte_off = pfs_fat_cluster_to_byte(p_sbi, clu) + (int64_t)in_clu;
        rc = pfs_fat_pwrite_at(p_sbi, src + put, chunk, byte_off);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        put += chunk;
        cur += chunk;
        remain -= chunk;
    }
    *p_put = put;
    return PFS_FAT_RET_OK;
}

int pfs_fat_file_truncate(pfs_fat_sbi_t *p_sbi, pfs_fat_file_meta_t *p_meta, int64_t new_size)
{
    if (NULL == p_sbi || NULL == p_meta || new_size < 0)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }

    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);

    if (new_size == p_meta->size)
    {
        return PFS_FAT_RET_OK;
    }

    if (new_size > p_meta->size)
    {
        uint32_t keep = (uint32_t)(((uint64_t)new_size + (uint64_t)p_bs->cluster_size - 1ULL) >>
                p_bs->cluster_size_bits);
        int rc = pfs_fat_file_extend_to(p_sbi, p_meta, keep);

        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        p_meta->size = new_size;
        return PFS_FAT_RET_OK;
    }

    if (0 == new_size)
    {
        int rc;

        if (0 != p_meta->start_clu)
        {
            rc = pfs_fat_free_cluster_chain(p_sbi, p_meta->start_clu);
            if (PFS_FAT_RET_OK != rc)
            {
                return rc;
            }
        }
        p_meta->start_clu = 0;
        p_meta->size = 0;
        p_meta->hint_clu = 0;
        p_meta->hint_clu_off = 0;
        return PFS_FAT_RET_OK;
    }

    uint32_t keep = (uint32_t)(((uint64_t)new_size + (uint64_t)p_bs->cluster_size - 1ULL) >>
            p_bs->cluster_size_bits);
    uint32_t old_n = (uint32_t)(((uint64_t)p_meta->size + (uint64_t)p_bs->cluster_size - 1ULL) >>
            p_bs->cluster_size_bits);

    if (keep >= old_n)
    {
        p_meta->size = new_size;
        return PFS_FAT_RET_OK;
    }

    uint32_t last = p_meta->start_clu;
    int rc = pfs_fat_cluster_walk(p_sbi, &last, keep - 1U);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }

    uint32_t next;

    rc = pfs_fat_ent_get(p_sbi, last, &next);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_fat_ent_set(p_sbi, last, FAT_ENT_EOF);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    if (FAT_ENT_EOF != next && FAT_ENT_FREE != next)
    {
        rc = pfs_fat_free_cluster_chain(p_sbi, next);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
    }
    p_meta->size = new_size;
    p_meta->hint_clu_off = 0;
    p_meta->hint_clu = p_meta->start_clu;
    return PFS_FAT_RET_OK;
}

// end
