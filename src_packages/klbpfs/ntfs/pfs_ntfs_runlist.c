// Doc Encode : UTF-8 BOM, Unix(LF)
// 运行列表 / 簇映射 (linux-7.1 fs/ntfs/runlist.c)
#include "ntfs/pfs_ntfs_internal.h"
#include "base/pfs_mem.h"

void pfs_ntfs_runlist_reset(pfs_ntfs_runlist_t *p_rl)
{
    if (NULL == p_rl)
    {
        return;
    }
    if (NULL != p_rl->p_rl)
    {
        pfs_free(p_rl->p_rl);
        p_rl->p_rl = NULL;
    }
    p_rl->count = 0;
    p_rl->hint = -1;
}

static int64_t pfs_ntfs_read_signed_le(const uint8_t *p, int n)
{
    int64_t v = 0;

    if (NULL == p || 0 >= n || n > 8)
    {
        return 0;
    }
    for (int i = 0; i < n; i++)
    {
        v |= ((int64_t)p[i]) << (8 * i);
    }
    if (n < 8 && 0 != (p[n - 1] & 0x80))
    {
        for (int i = n; i < 8; i++)
        {
            v |= (int64_t)0xff << (8 * i);
        }
    }
    return v;
}

int pfs_ntfs_runlist_decompress(pfs_ntfs_runlist_t *p_out, const ntfs_attr_record_t *p_attr)
{
    if (NULL == p_out || NULL == p_attr)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (0U == p_attr->non_resident)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_runlist_reset(p_out);

    uint32_t attr_len = pfs_le32_to_cpu(p_attr->length);
    uint16_t mp_ofs = pfs_le16_to_cpu(p_attr->data.non_resident.mapping_pairs_offset);
    if (mp_ofs < 64U || mp_ofs >= attr_len)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    int64_t lowest = (int64_t)pfs_le64_to_cpu(p_attr->data.non_resident.lowest_vcn);
    int64_t highest = (int64_t)pfs_le64_to_cpu(p_attr->data.non_resident.highest_vcn);
    const uint8_t *p_buf = (const uint8_t *)p_attr + mp_ofs;
    const uint8_t *p_end = (const uint8_t *)p_attr + attr_len;
    int64_t vcn = lowest;
    int64_t lcn = 0;
    uint32_t count = 0;
    uint32_t cap = 16;

    pfs_ntfs_run_t *p_rl = (pfs_ntfs_run_t *)pfs_malloc((size_t)(cap + 1U) * sizeof(pfs_ntfs_run_t));
    if (NULL == p_rl)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    while (p_buf < p_end && 0U != *p_buf)
    {
        int len_sz = (int)(*p_buf & 0x0f);
        int lcn_sz = (int)((*p_buf >> 4) & 0x0f);
        int64_t length;
        int64_t delta;
        pfs_ntfs_run_t *p_grow;

        if (0 == len_sz)
        {
            pfs_free(p_rl);
            return PFS_NTFS_RET_EBADMSG;
        }
        if (p_buf + 1 + len_sz + lcn_sz > p_end)
        {
            pfs_free(p_rl);
            return PFS_NTFS_RET_EBADMSG;
        }

        length = pfs_ntfs_read_signed_le(p_buf + 1, len_sz);
        if (0 > length)
        {
            pfs_free(p_rl);
            return PFS_NTFS_RET_EBADMSG;
        }

        if (0 == lcn_sz)
        {
            // 零长 hole 跳过
            if (0 == length)
            {
                p_buf += 1 + len_sz;
                continue;
            }
            if (count >= cap)
            {
                cap *= 2U;
                p_grow = (pfs_ntfs_run_t *)pfs_realloc(p_rl, (size_t)(cap + 1U) * sizeof(pfs_ntfs_run_t));
                if (NULL == p_grow)
                {
                    pfs_free(p_rl);
                    return PFS_NTFS_RET_ENOMEM;
                }
                p_rl = p_grow;
            }
            p_rl[count].vcn = vcn;
            p_rl[count].lcn = PFS_NTFS_LCN_HOLE;
            p_rl[count].length = length;
            count++;
        }
        else
        {
            if (0 == length)
            {
                pfs_free(p_rl);
                return PFS_NTFS_RET_EBADMSG;
            }
            delta = pfs_ntfs_read_signed_le(p_buf + 1 + len_sz, lcn_sz);
            lcn += delta;
            if (-1 > lcn)
            {
                pfs_free(p_rl);
                return PFS_NTFS_RET_EBADMSG;
            }
            if (count >= cap)
            {
                cap *= 2U;
                p_grow = (pfs_ntfs_run_t *)pfs_realloc(p_rl, (size_t)(cap + 1U) * sizeof(pfs_ntfs_run_t));
                if (NULL == p_grow)
                {
                    pfs_free(p_rl);
                    return PFS_NTFS_RET_ENOMEM;
                }
                p_rl = p_grow;
            }
            p_rl[count].vcn = vcn;
            p_rl[count].lcn = lcn;
            p_rl[count].length = length;
            count++;
        }

        vcn += length;
        p_buf += 1 + len_sz + lcn_sz;
    }

    if (0 != highest && (vcn - 1) != highest)
    {
        pfs_free(p_rl);
        return PFS_NTFS_RET_EBADMSG;
    }

    p_rl[count].vcn = vcn;
    p_rl[count].lcn = PFS_NTFS_LCN_ENOENT;
    p_rl[count].length = 0;

    p_out->p_rl = p_rl;
    p_out->count = count;
    p_out->hint = -1;
    return PFS_NTFS_RET_OK;
}

int64_t pfs_ntfs_runlist_vcn_to_lcn(const pfs_ntfs_runlist_t *p_rl, int64_t vcn)
{
    if (NULL == p_rl || NULL == p_rl->p_rl)
    {
        return PFS_NTFS_LCN_RL_NOT_MAPPED;
    }
    const pfs_ntfs_run_t *p_runs = p_rl->p_rl;
    if (vcn < p_runs[0].vcn)
    {
        return PFS_NTFS_LCN_ENOENT;
    }

    int start = p_rl->hint;
    if (start < 0 || (uint32_t)start >= p_rl->count)
    {
        start = 0;
    }
    int i;
    for (i = start; p_runs[i].length != 0; i++)
    {
        if (vcn < p_runs[i + 1].vcn)
        {
            if (p_runs[i].lcn >= 0)
            {
                return p_runs[i].lcn + (vcn - p_runs[i].vcn);
            }
            return p_runs[i].lcn;
        }
    }
    if (p_runs[i].lcn < 0)
    {
        return p_runs[i].lcn;
    }
    return PFS_NTFS_LCN_ENOENT;
}

static int pfs_ntfs_write_significant_bytes(uint8_t *p_dst, const uint8_t *p_dst_max, int64_t n)
{
    int64_t l = n;
    int i = 0;
    int8_t j;

    do {
        if (p_dst > p_dst_max)
        {
            return PFS_NTFS_RET_ENOSPC;
        }
        *p_dst++ = (uint8_t)(l & 0xff);
        l >>= 8;
        i++;
    } while (0 != l && -1 != l);

    j = (int8_t)((n >> (8 * (i - 1))) & 0xff);
    if (n < 0 && j >= 0)
    {
        if (p_dst > p_dst_max)
        {
            return PFS_NTFS_RET_ENOSPC;
        }
        i++;
        *p_dst = (uint8_t)0xff;
    }
    else if (n > 0 && j < 0)
    {
        if (p_dst > p_dst_max)
        {
            return PFS_NTFS_RET_ENOSPC;
        }
        i++;
        *p_dst = 0;
    }
    return i;
}

int pfs_ntfs_runlist_merge(pfs_ntfs_runlist_t *p_dst, pfs_ntfs_runlist_t *p_src)
{
    if (NULL == p_dst || NULL == p_src)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (NULL == p_src->p_rl || 0U == p_src->count)
    {
        pfs_ntfs_runlist_reset(p_src);
        return PFS_NTFS_RET_OK;
    }

    uint32_t dst_count = p_dst->count;
    uint32_t src_count = p_src->count;
    if (0U == dst_count || NULL == p_dst->p_rl)
    {
        p_dst->p_rl = p_src->p_rl;
        p_dst->count = src_count;
        p_dst->hint = -1;
        p_src->p_rl = NULL;
        p_src->count = 0;
        p_src->hint = -1;
        return PFS_NTFS_RET_OK;
    }

    int64_t dst_end_vcn = p_dst->p_rl[dst_count].vcn;
    int64_t vcn_delta = dst_end_vcn - p_src->p_rl[0].vcn;
    uint32_t total = dst_count + src_count;
    pfs_ntfs_run_t *p_new = (pfs_ntfs_run_t *)pfs_malloc((size_t)(total + 1U) * sizeof(pfs_ntfs_run_t));
    if (NULL == p_new)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    for (uint32_t i = 0; i < dst_count; i++)
    {
        p_new[i] = p_dst->p_rl[i];
    }
    for (uint32_t j = 0; j < src_count; j++)
    {
        p_new[dst_count + j] = p_src->p_rl[j];
        p_new[dst_count + j].vcn += vcn_delta;
    }
    p_new[total] = p_src->p_rl[src_count];
    p_new[total].vcn += vcn_delta;

    // 合并相邻同 LCN 连续段
    for (int i = 0; i < total; i++)
    {
        if (i + 1U < total &&
                p_new[i].lcn >= 0 &&
                p_new[i + 1U].lcn >= 0 &&
                p_new[i].lcn + p_new[i].length == p_new[i + 1U].lcn &&
                p_new[i].vcn + p_new[i].length == p_new[i + 1U].vcn)
        {
            p_new[i].length += p_new[i + 1U].length;
            memmove(&p_new[i + 1U], &p_new[i + 2U],
                    (size_t)(total - i) * sizeof(pfs_ntfs_run_t));
            total--;
            i--;
        }
    }

    pfs_free(p_dst->p_rl);
    p_dst->p_rl = p_new;
    p_dst->count = total;
    p_dst->hint = -1;

    pfs_free(p_src->p_rl);
    p_src->p_rl = NULL;
    p_src->count = 0;
    p_src->hint = -1;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_runlist_truncate(pfs_ntfs_runlist_t *p_rl, int64_t new_vcn_count)
{
    if (NULL == p_rl)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (0 > new_vcn_count)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (NULL == p_rl->p_rl || 0U == p_rl->count)
    {
        return PFS_NTFS_RET_OK;
    }

    pfs_ntfs_run_t *p_runs = p_rl->p_rl;
    uint32_t count = p_rl->count;
    uint32_t keep = 0;
    for (int i = 0; i < (int)count && p_runs[i].length != 0; i++)
    {
        if (p_runs[i].vcn >= new_vcn_count)
        {
            break;
        }
        if (p_runs[i].vcn + p_runs[i].length > new_vcn_count)
        {
            p_runs[i].length = new_vcn_count - p_runs[i].vcn;
        }
        keep = (uint32_t)(i + 1);
    }

    if (0U == keep)
    {
        pfs_ntfs_runlist_reset(p_rl);
        return PFS_NTFS_RET_OK;
    }

    p_runs[keep].vcn = new_vcn_count;
    p_runs[keep].lcn = PFS_NTFS_LCN_ENOENT;
    p_runs[keep].length = 0;
    p_rl->count = keep;
    p_rl->hint = -1;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_mapping_pairs_build(const pfs_ntfs_runlist_t *p_rl, uint8_t *p_dst, uint32_t dst_cap,
        uint32_t *p_built)
{
    if (NULL == p_dst || NULL == p_built || dst_cap < 1U)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (NULL == p_rl || NULL == p_rl->p_rl || 0U == p_rl->count)
    {
        p_dst[0] = 0;
        *p_built = 1;
        return PFS_NTFS_RET_OK;
    }

    const pfs_ntfs_run_t *p_runs = p_rl->p_rl;
    uint32_t count = p_rl->count;
    uint8_t *p_cur = p_dst;
    uint8_t *p_max = p_dst + dst_cap - 1U;
    int64_t prev_lcn = 0;

    for (int i = 0; i < (int)count && p_runs[i].length != 0; i++)
    {
        int64_t length = p_runs[i].length;
        uint8_t *p_next;

        if (length <= 0 || p_runs[i].lcn < PFS_NTFS_LCN_HOLE)
        {
            return PFS_NTFS_RET_EBADMSG;
        }
        if (PFS_NTFS_LCN_HOLE == p_runs[i].lcn)
        {
            continue;
        }

        int len_len = pfs_ntfs_write_significant_bytes(p_cur + 1, p_max, length);
        if (0 > len_len)
        {
            return len_len;
        }
        int lcn_len = pfs_ntfs_write_significant_bytes(p_cur + 1 + (uint32_t)len_len, p_max,
                p_runs[i].lcn - prev_lcn);
        if (0 > lcn_len)
        {
            return lcn_len;
        }
        p_next = p_cur + 1U + (uint32_t)len_len + (uint32_t)lcn_len;
        if (p_next > p_max)
        {
            return PFS_NTFS_RET_ENOSPC;
        }
        *p_cur = (uint8_t)((lcn_len << 4) | len_len);
        p_cur = p_next;
        prev_lcn = p_runs[i].lcn;
    }

    if (p_cur > p_max)
    {
        return PFS_NTFS_RET_ENOSPC;
    }
    *p_cur = 0;
    *p_built = (uint32_t)(p_cur - p_dst + 1U);
    return PFS_NTFS_RET_OK;
}

// end
