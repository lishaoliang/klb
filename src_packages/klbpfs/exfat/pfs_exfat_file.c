// Doc Encode : UTF-8 BOM, Unix(LF)
// 文件读写与簇映射 (linux-7.1 fs/exfat/inode.c get_block / map_cluster)
#include "exfat/pfs_exfat_internal.h"

#include <stdbool.h>
#include <string.h>

int pfs_exfat_map_cluster(pfs_exfat_sbi_t *p_sbi, pfs_exfat_file_meta_t *p_meta,
        uint32_t clu_off, uint32_t *p_clu)
{
    if (NULL == p_sbi || NULL == p_meta || NULL == p_clu)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    if (EXFAT_EOF_CLUSTER == p_meta->start_clu || EXFAT_FREE_CLUSTER == p_meta->start_clu)
    {
        *p_clu = EXFAT_EOF_CLUSTER;
        return PFS_EXFAT_RET_OK;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    if (p_meta->size <= 0)
    {
        *p_clu = EXFAT_EOF_CLUSTER;
        return PFS_EXFAT_RET_OK;
    }
    uint32_t num_clusters = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)p_meta->size, p_bs);
    if (clu_off >= num_clusters)
    {
        *p_clu = EXFAT_EOF_CLUSTER;
        return PFS_EXFAT_RET_OK;
    }

    if (EXFAT_ALLOC_NO_FAT_CHAIN == p_meta->flags)
    {
        uint32_t clu = p_meta->start_clu + clu_off;
        if (!pfs_exfat_is_valid_cluster(p_sbi, clu))
        {
            return PFS_EXFAT_RET_EIO;
        }
        *p_clu = clu;
        p_meta->hint_clu_off = clu_off;
        p_meta->hint_clu = clu;
        return PFS_EXFAT_RET_OK;
    }

    // 顺序读提示: 目标偏移 >= hint 时从 hint 前进, 否则从文件首簇重走
    bool use_hint = (p_meta->hint_clu != EXFAT_EOF_CLUSTER &&
            p_meta->hint_clu != EXFAT_FREE_CLUSTER &&
            clu_off >= p_meta->hint_clu_off);
    uint32_t clu = use_hint ? p_meta->hint_clu : p_meta->start_clu;
    uint32_t steps = use_hint ? (clu_off - p_meta->hint_clu_off) : clu_off;

    int rc = pfs_exfat_cluster_walk(p_sbi, &clu, steps, p_meta->flags);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (EXFAT_EOF_CLUSTER == clu || EXFAT_FREE_CLUSTER == clu)
    {
        return PFS_EXFAT_RET_EIO;
    }
    if (!pfs_exfat_is_valid_cluster(p_sbi, clu))
    {
        return PFS_EXFAT_RET_EIO;
    }

    *p_clu = clu;
    p_meta->hint_clu_off = clu_off;
    p_meta->hint_clu = clu;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_file_read(pfs_exfat_sbi_t *p_sbi, pfs_exfat_file_meta_t *p_meta,
        int64_t pos, void *buf, int len, int *p_got)
{
    const pfs_exfat_bs_t *p_bs;
    uint8_t *dst;
    int64_t end;
    int64_t valid_end;
    int64_t cur;
    int remain;
    int got;
    uint32_t cluster_size;
    uint32_t sector_size;
    uint32_t clu_off;
    uint32_t in_clu;
    uint32_t clu;
    int chunk;
    int64_t byte_off;

    if (NULL == p_sbi || NULL == p_meta || NULL == buf || NULL == p_got || len <= 0)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (pos < 0)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    *p_got = 0;
    if (pos >= p_meta->size)
    {
        return PFS_EXFAT_RET_OK;
    }

    p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    cluster_size = p_bs->cluster_size;
    sector_size = 1u << p_bs->boot.sect_size_bits;
    if (0 == cluster_size || 0 == sector_size)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    end = pos + (int64_t)len;
    if (end > p_meta->size)
    {
        end = p_meta->size;
    }
    valid_end = p_meta->valid_size;
    if (valid_end > p_meta->size)
    {
        valid_end = p_meta->size;
    }
    if (valid_end < 0)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    dst = (uint8_t *)buf;
    cur = pos;
    remain = (int)(end - pos);
    got = 0;

    while (remain > 0)
    {
        if (cur >= valid_end)
        {
            // VDL 之后至 DataLength: 填 0, 不读盘 (对齐 exfat_get_block 未写区)
            chunk = remain;
            memset(dst + got, 0, (size_t)chunk);
            got += chunk;
            cur += chunk;
            remain = 0;
            break;
        }

        clu_off = (uint32_t)((uint64_t)cur >> p_bs->cluster_size_bits);
        in_clu = (uint32_t)((uint64_t)cur & (uint64_t)(cluster_size - 1U));
        int rc = pfs_exfat_map_cluster(p_sbi, p_meta, clu_off, &clu);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        if (EXFAT_EOF_CLUSTER == clu)
        {
            return PFS_EXFAT_RET_EIO;
        }

        chunk = (int)(cluster_size - in_clu);
        if (chunk > remain)
        {
            chunk = remain;
        }
        // 不超过 valid_size
        if (cur + (int64_t)chunk > valid_end)
        {
            chunk = (int)(valid_end - cur);
        }
        // 单次 pread_at 不跨扇区
        {
            uint32_t byte_in_sec = (uint32_t)(((uint64_t)cur) & (uint64_t)(sector_size - 1U));
            int max_in_sec = (int)(sector_size - byte_in_sec);

            if (chunk > max_in_sec)
            {
                chunk = max_in_sec;
            }
        }

        byte_off = pfs_exfat_cluster_to_byte(p_sbi, clu) + (int64_t)in_clu;
        rc = pfs_exfat_pread_at(p_sbi, dst + got, chunk, byte_off);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }

        got += chunk;
        cur += chunk;
        remain -= chunk;
    }

    *p_got = got;
    return PFS_EXFAT_RET_OK;
}

static int pfs_exfat_file_extend_to(pfs_exfat_sbi_t *p_sbi, pfs_exfat_file_meta_t *p_meta,
        uint32_t need_clu)
{
    const pfs_exfat_bs_t *p_bs;
    uint32_t have;
    uint32_t last;
    uint32_t new_clu;

    p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    if (0 == need_clu)
    {
        return PFS_EXFAT_RET_OK;
    }

    int rc;
    if (EXFAT_EOF_CLUSTER == p_meta->start_clu || EXFAT_FREE_CLUSTER == p_meta->start_clu ||
            p_meta->size <= 0)
    {
        rc = pfs_exfat_alloc_cluster(p_sbi, &new_clu);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        p_meta->start_clu = new_clu;
        p_meta->flags = EXFAT_ALLOC_FAT_CHAIN;
        p_meta->hint_clu = new_clu;
        p_meta->hint_clu_off = 0;
        have = 1;
        last = new_clu;
    }
    else
    {
        have = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)p_meta->size, p_bs);
        if (0 == have)
        {
            have = 1;
        }
        last = p_meta->start_clu;
        if (EXFAT_ALLOC_NO_FAT_CHAIN == p_meta->flags)
        {
            // 简化: 连续链改为 FAT 链后再扩展
            for (uint32_t i = 0; i + 1 < have; i++)
            {
                rc = pfs_exfat_ent_set(p_sbi, p_meta->start_clu + i, p_meta->start_clu + i + 1U);
                if (PFS_EXFAT_RET_OK != rc)
                {
                    return rc;
                }
            }
            rc = pfs_exfat_ent_set(p_sbi, p_meta->start_clu + have - 1U, EXFAT_EOF_CLUSTER);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }
            p_meta->flags = EXFAT_ALLOC_FAT_CHAIN;
            last = p_meta->start_clu + have - 1U;
        }
        else
        {
            last = p_meta->start_clu;
            rc = pfs_exfat_cluster_walk(p_sbi, &last, have - 1U, p_meta->flags);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }
        }
    }

    while (have < need_clu)
    {
        rc = pfs_exfat_alloc_cluster(p_sbi, &new_clu);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_exfat_ent_set(p_sbi, last, new_clu);
        if (PFS_EXFAT_RET_OK != rc)
        {
            (void)pfs_exfat_free_cluster_chain(p_sbi, new_clu, 1, EXFAT_ALLOC_FAT_CHAIN);
            return rc;
        }
        last = new_clu;
        have++;
    }
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_file_write(pfs_exfat_sbi_t *p_sbi, pfs_exfat_file_meta_t *p_meta,
        int64_t pos, const void *buf, int len, int *p_put)
{
    const pfs_exfat_bs_t *p_bs;
    const uint8_t *src;
    int64_t end;
    int64_t cur;
    int remain;
    int put;
    uint32_t cluster_size;
    uint32_t sector_size;
    uint32_t need_clu;
    uint32_t clu_off;
    uint32_t in_clu;
    uint32_t clu;
    int chunk;
    int64_t byte_off;

    if (NULL == p_sbi || NULL == p_meta || NULL == buf || NULL == p_put || len <= 0)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (pos < 0 || !pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    *p_put = 0;
    p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    cluster_size = p_bs->cluster_size;
    sector_size = 1u << p_bs->boot.sect_size_bits;
    if (0 == cluster_size || 0 == sector_size)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    end = pos + (int64_t)len;
    need_clu = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)end, p_bs);
    if (0 == need_clu && end > 0)
    {
        need_clu = 1;
    }
    int rc = pfs_exfat_file_extend_to(p_sbi, p_meta, need_clu);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    // 逻辑大小至少覆盖写入末尾; VDL 跟到已写末尾
    if (end > p_meta->size)
    {
        p_meta->size = end;
    }
    if (end > p_meta->valid_size)
    {
        p_meta->valid_size = end;
    }

    src = (const uint8_t *)buf;
    cur = pos;
    remain = len;
    put = 0;

    while (remain > 0)
    {
        clu_off = (uint32_t)((uint64_t)cur >> p_bs->cluster_size_bits);
        in_clu = (uint32_t)((uint64_t)cur & (uint64_t)(cluster_size - 1U));
        rc = pfs_exfat_map_cluster(p_sbi, p_meta, clu_off, &clu);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        if (EXFAT_EOF_CLUSTER == clu)
        {
            return PFS_EXFAT_RET_EIO;
        }

        chunk = (int)(cluster_size - in_clu);
        if (chunk > remain)
        {
            chunk = remain;
        }
        {
            uint32_t byte_in_sec = (uint32_t)(((uint64_t)cur) & (uint64_t)(sector_size - 1U));
            int max_in_sec = (int)(sector_size - byte_in_sec);

            if (chunk > max_in_sec)
            {
                chunk = max_in_sec;
            }
        }

        byte_off = pfs_exfat_cluster_to_byte(p_sbi, clu) + (int64_t)in_clu;
        rc = pfs_exfat_pwrite_at(p_sbi, src + put, chunk, byte_off);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }

        put += chunk;
        cur += chunk;
        remain -= chunk;
    }

    *p_put = put;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_file_truncate(pfs_exfat_sbi_t *p_sbi, pfs_exfat_file_meta_t *p_meta, int64_t new_size)
{
    const pfs_exfat_bs_t *p_bs;
    uint32_t old_clu;
    uint32_t new_clu;
    uint32_t keep;
    uint32_t last;
    uint32_t next;

    if (NULL == p_sbi || NULL == p_meta || new_size < 0)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    if (new_size == p_meta->size)
    {
        return PFS_EXFAT_RET_OK;
    }

    int rc;
    if (new_size > p_meta->size)
    {
        new_clu = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)new_size, p_bs);
        rc = pfs_exfat_file_extend_to(p_sbi, p_meta, new_clu);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        p_meta->size = new_size;
        // 扩展区视为未写, VDL 不变 (若原 VDL 更大则钳制)
        if (p_meta->valid_size > new_size)
        {
            p_meta->valid_size = new_size;
        }
        return PFS_EXFAT_RET_OK;
    }

    // shrink
    if (0 == new_size)
    {
        if (EXFAT_EOF_CLUSTER != p_meta->start_clu && EXFAT_FREE_CLUSTER != p_meta->start_clu)
        {
            old_clu = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)p_meta->size, p_bs);
            if (0 == old_clu)
            {
                old_clu = 1;
            }
            rc = pfs_exfat_free_cluster_chain(p_sbi, p_meta->start_clu, old_clu, p_meta->flags);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }
        }
        p_meta->start_clu = EXFAT_EOF_CLUSTER;
        p_meta->flags = EXFAT_ALLOC_FAT_CHAIN;
        p_meta->size = 0;
        p_meta->valid_size = 0;
        p_meta->hint_clu = EXFAT_EOF_CLUSTER;
        p_meta->hint_clu_off = 0;
        return PFS_EXFAT_RET_OK;
    }

    keep = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)new_size, p_bs);
    old_clu = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)p_meta->size, p_bs);
    if (keep >= old_clu)
    {
        p_meta->size = new_size;
        if (p_meta->valid_size > new_size)
        {
            p_meta->valid_size = new_size;
        }
        return PFS_EXFAT_RET_OK;
    }

    if (EXFAT_ALLOC_NO_FAT_CHAIN == p_meta->flags)
    {
        rc = pfs_exfat_free_cluster_chain(p_sbi, p_meta->start_clu + keep, old_clu - keep,
                EXFAT_ALLOC_NO_FAT_CHAIN);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }
    else
    {
        last = p_meta->start_clu;
        rc = pfs_exfat_cluster_walk(p_sbi, &last, keep - 1U, p_meta->flags);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_exfat_ent_get(p_sbi, last, &next);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_exfat_ent_set(p_sbi, last, EXFAT_EOF_CLUSTER);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        if (EXFAT_EOF_CLUSTER != next && EXFAT_FREE_CLUSTER != next)
        {
            rc = pfs_exfat_free_cluster_chain(p_sbi, next, old_clu - keep, EXFAT_ALLOC_FAT_CHAIN);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }
        }
    }

    p_meta->size = new_size;
    if (p_meta->valid_size > new_size)
    {
        p_meta->valid_size = new_size;
    }
    p_meta->hint_clu_off = 0;
    p_meta->hint_clu = p_meta->start_clu;
    return PFS_EXFAT_RET_OK;
}

// end
