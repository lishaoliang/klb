// Doc Encode : UTF-8 BOM, Unix(LF)
// VMDK probe: sparse magic (VMDK) 或文本 "# Disk DescriptorFile"; 完整头校验
#include "blkio_vmdk/pfs_vmdk_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"

#include <string.h>
#include <stdbool.h>

static bool pfs_vmdk_has_desc_mark(const uint8_t *p, int n)
{
    if (n < PFS_VMDK_DESC_MARK_LEN)
    {
        return false;
    }
    if (0 == memcmp(p, PFS_VMDK_DESC_MARK, PFS_VMDK_DESC_MARK_LEN))
    {
        return true;
    }
    // 允许 BOM / 前导空白后出现标记
    int lim = n - PFS_VMDK_DESC_MARK_LEN;
    for (int i = 0; i <= lim && i < 64; i++)
    {
        if (0 == memcmp(p + i, PFS_VMDK_DESC_MARK, PFS_VMDK_DESC_MARK_LEN))
        {
            return true;
        }
    }
    return false;
}

static bool pfs_vmdk_is_pow2_u64(uint64_t v)
{
    return (0 != v) && (0 == (v & (v - 1ull)));
}

static void pfs_vmdk_hdr_from_raw(const uint8_t *raw, pfs_vmdk_sparse_header_t *p_hdr)
{
    memset(p_hdr, 0, sizeof(*p_hdr));
    p_hdr->magic = pfs_read_le32(raw + 0);
    p_hdr->version = pfs_read_le32(raw + 4);
    p_hdr->flags = pfs_read_le32(raw + 8);
    p_hdr->capacity = pfs_read_le64(raw + 12);
    p_hdr->grain_size = pfs_read_le64(raw + 20);
    p_hdr->descriptor_offset = pfs_read_le64(raw + 28);
    p_hdr->descriptor_size = pfs_read_le64(raw + 36);
    p_hdr->num_gtes_per_gt = pfs_read_le32(raw + 44);
    p_hdr->rgd_offset = pfs_read_le64(raw + 48);
    p_hdr->gd_offset = pfs_read_le64(raw + 56);
    p_hdr->overhead = pfs_read_le64(raw + 64);
    p_hdr->unclean_shutdown = raw[72];
    p_hdr->single_end_line_char = raw[73];
    p_hdr->non_end_line_char = raw[74];
    p_hdr->double_end_line_char1 = raw[75];
    p_hdr->double_end_line_char2 = raw[76];
    p_hdr->compress_algorithm = pfs_read_le16(raw + 77);
}

int pfs_vmdk_probe_sparse_header(pfs_blkio_t *p_host, pfs_vmdk_sparse_header_t *p_hdr)
{
    uint8_t raw[PFS_VMDK_SPARSE_HEADER_SIZE];

    if (NULL == p_host || NULL == p_hdr)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    memset(raw, 0, sizeof(raw));
    int rc = pfs_blkio_raw_pread(p_host, raw, (int)sizeof(raw), 0);
    if (0 > rc)
    {
        if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
        {
            return rc;
        }
        return PFS_BLKIO_RET_PROBE;
    }
    if ((int)sizeof(raw) > rc)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    pfs_vmdk_hdr_from_raw(raw, p_hdr);

    if (PFS_VMDK_MAGIC_KDMV != p_hdr->magic)
    {
        // COWD 等旧格式本阶段不支持
        return PFS_BLKIO_RET_STATE;
    }
    if (p_hdr->version < 1u || p_hdr->version > 3u)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0 != (p_hdr->flags & (PFS_VMDK_FLAG_COMPRESSED | PFS_VMDK_FLAG_MARKERS)))
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0 != p_hdr->compress_algorithm)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0 == p_hdr->capacity || 0 == p_hdr->grain_size)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (!pfs_vmdk_is_pow2_u64(p_hdr->grain_size))
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0 == p_hdr->num_gtes_per_gt || !pfs_vmdk_is_pow2_u64((uint64_t)p_hdr->num_gtes_per_gt))
    {
        return PFS_BLKIO_RET_STATE;
    }
    // streamOptimized: gdOffset == (uint64_t)-1
    if ((uint64_t)0xFFFFFFFFFFFFFFFFull == p_hdr->gd_offset)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0 == p_hdr->gd_offset)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0 != (p_hdr->capacity % p_hdr->grain_size))
    {
        return PFS_BLKIO_RET_STATE;
    }

    return PFS_BLKIO_RET_OK;
}

int pfs_vmdk_probe(pfs_blkio_t *p_host, int *p_kind, pfs_vmdk_sparse_header_t *p_hdr)
{
    uint8_t raw[PFS_VMDK_PROBE_SIZE];

    if (NULL == p_host || NULL == p_kind)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    *p_kind = PFS_VMDK_KIND_NONE;
    memset(raw, 0, sizeof(raw));
    int rc = pfs_blkio_raw_pread(p_host, raw, (int)sizeof(raw), 0);
    if (0 > rc)
    {
        if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
        {
            return rc;
        }
        return PFS_BLKIO_RET_PROBE;
    }
    if (4 > rc)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    uint32_t magic = pfs_read_le32(raw);
    if (PFS_VMDK_MAGIC_KDMV == magic || PFS_VMDK_MAGIC_COWD == magic)
    {
        *p_kind = PFS_VMDK_KIND_SPARSE;
        if (NULL != p_hdr)
        {
            if (rc >= (int)sizeof(raw))
            {
                pfs_vmdk_hdr_from_raw(raw, p_hdr);
            }
            else
            {
                memset(p_hdr, 0, sizeof(*p_hdr));
                p_hdr->magic = magic;
            }
        }
        return PFS_BLKIO_RET_OK;
    }

    if (pfs_vmdk_has_desc_mark(raw, rc))
    {
        *p_kind = PFS_VMDK_KIND_DESCRIPTOR;
        if (NULL != p_hdr)
        {
            memset(p_hdr, 0, sizeof(*p_hdr));
        }
        return PFS_BLKIO_RET_OK;
    }

    return PFS_BLKIO_RET_PROBE;
}

// end
