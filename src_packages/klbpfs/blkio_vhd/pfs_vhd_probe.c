// Doc Encode : UTF-8 BOM, Unix(LF)
// VHD probe: 读文件尾 footer / Dynamic 头, 校验 cookie / checksum, 转主机序
#include "blkio_vhd/pfs_vhd_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"

#include <string.h>

// footer.checksum 字段在结构内偏移 64
#define PFS_VHD_FOOTER_CHECKSUM_OFF  64
// dyn header.checksum 偏移 36
#define PFS_VHD_DYN_CHECKSUM_OFF     36

static uint32_t pfs_vhd_checksum_skip(const uint8_t *p_raw, int total, int skip_off, int skip_len)
{
    uint32_t sum = 0;

    for (int i = 0; i < total; i++)
    {
        if (i >= skip_off && i < skip_off + skip_len)
        {
            continue;
        }
        sum += (uint32_t)p_raw[i];
    }
    return ~sum;
}

static void pfs_vhd_footer_from_raw(const uint8_t *p_raw, pfs_vhd_footer_t *p_footer)
{
    memset(p_footer, 0, sizeof(*p_footer));
    memcpy(p_footer->cookie, p_raw, PFS_VHD_COOKIE_LEN);
    p_footer->features = pfs_read_be32(p_raw + 8);
    p_footer->file_format_version = pfs_read_be32(p_raw + 12);
    p_footer->data_offset = pfs_read_be64(p_raw + 16);
    p_footer->timestamp = pfs_read_be32(p_raw + 24);
    memcpy(p_footer->creator_app, p_raw + 28, 4);
    p_footer->creator_version = pfs_read_be32(p_raw + 32);
    memcpy(p_footer->creator_os, p_raw + 36, 4);
    p_footer->original_size = pfs_read_be64(p_raw + 40);
    p_footer->current_size = pfs_read_be64(p_raw + 48);
    p_footer->geometry.cylinder = pfs_read_be16(p_raw + 56);
    p_footer->geometry.heads = p_raw[58];
    p_footer->geometry.sectors_per_track = p_raw[59];
    p_footer->disk_type = pfs_read_be32(p_raw + 60);
    p_footer->checksum = pfs_read_be32(p_raw + 64);
    memcpy(p_footer->unique_id, p_raw + 68, 16);
    p_footer->saved_state = p_raw[84];
    memcpy(p_footer->reserved, p_raw + 85, 427);
}

static void pfs_vhd_dyn_header_from_raw(const uint8_t *p_raw, pfs_vhd_dyn_header_t *p_hdr)
{
    memset(p_hdr, 0, sizeof(*p_hdr));
    memcpy(p_hdr->cookie, p_raw, PFS_VHD_DYN_COOKIE_LEN);
    p_hdr->data_offset = pfs_read_be64(p_raw + 8);
    p_hdr->table_offset = pfs_read_be64(p_raw + 16);
    p_hdr->header_version = pfs_read_be32(p_raw + 24);
    p_hdr->max_table_entries = pfs_read_be32(p_raw + 28);
    p_hdr->block_size = pfs_read_be32(p_raw + 32);
    p_hdr->checksum = pfs_read_be32(p_raw + 36);
    memcpy(p_hdr->parent_unique_id, p_raw + 40, 16);
    p_hdr->parent_timestamp = pfs_read_be32(p_raw + 56);
    p_hdr->reserved = pfs_read_be32(p_raw + 60);
    for (int i = 0; i < 256; i++)
    {
        p_hdr->parent_unicode_name[i] = pfs_read_be16(p_raw + 64 + i * 2);
    }
    for (int i = 0; i < 8; i++)
    {
        const uint8_t *p = p_raw + 576 + i * 24;
        p_hdr->parent_locator[i].platform_code = pfs_read_be32(p);
        p_hdr->parent_locator[i].platform_data_space = pfs_read_be32(p + 4);
        p_hdr->parent_locator[i].platform_data_length = pfs_read_be32(p + 8);
        p_hdr->parent_locator[i].reserved = pfs_read_be32(p + 12);
        p_hdr->parent_locator[i].platform_data_offset = pfs_read_be64(p + 16);
    }
    memcpy(p_hdr->reserved2, p_raw + 768, 256);
}

int pfs_vhd_probe_footer(pfs_blkio_t *p_host, pfs_vhd_footer_t *p_footer)
{
    if (NULL == p_host || NULL == p_footer)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    int64_t host_size = pfs_blkio_raw_get_size(p_host);
    if (host_size < (int64_t)PFS_VHD_FOOTER_SIZE)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    uint8_t raw[PFS_VHD_FOOTER_SIZE];
    int rc = pfs_blkio_raw_pread(p_host, raw, PFS_VHD_FOOTER_SIZE, host_size - (int64_t)PFS_VHD_FOOTER_SIZE);
    if (0 > rc)
    {
        return rc;
    }
    else if (rc != PFS_VHD_FOOTER_SIZE)
    {
        return PFS_BLKIO_RET_IO;
    }

    if (0 != memcmp(raw, PFS_VHD_COOKIE, PFS_VHD_COOKIE_LEN))
    {
        return PFS_BLKIO_RET_PROBE;
    }

    uint32_t expect_sum = pfs_vhd_checksum_skip(raw, PFS_VHD_FOOTER_SIZE, PFS_VHD_FOOTER_CHECKSUM_OFF, 4);
    if (expect_sum != pfs_read_be32(raw + PFS_VHD_FOOTER_CHECKSUM_OFF))
    {
        return PFS_BLKIO_RET_PROBE;
    }

    pfs_vhd_footer_from_raw(raw, p_footer);

    if (PFS_VHD_VERSION_1_0 != p_footer->file_format_version)
    {
        return PFS_BLKIO_RET_PROBE;
    }
    if (PFS_VHD_DISK_TYPE_FIXED != p_footer->disk_type &&
            PFS_VHD_DISK_TYPE_DYNAMIC != p_footer->disk_type &&
            PFS_VHD_DISK_TYPE_DIFFERENCING != p_footer->disk_type)
    {
        return PFS_BLKIO_RET_PROBE;
    }
    if (0 == p_footer->current_size)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    return PFS_BLKIO_RET_OK;
}

int pfs_vhd_probe_dyn_header(pfs_blkio_t *p_host, uint64_t data_offset, pfs_vhd_dyn_header_t *p_hdr)
{
    if (NULL == p_host || NULL == p_hdr)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (data_offset > (uint64_t)INT64_MAX)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    int64_t host_size = pfs_blkio_raw_get_size(p_host);
    if (host_size < 0 ||
            (uint64_t)host_size < data_offset + (uint64_t)PFS_VHD_DYN_HEADER_SIZE)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    uint8_t raw[PFS_VHD_DYN_HEADER_SIZE];
    int rc = pfs_blkio_raw_pread(p_host, raw, PFS_VHD_DYN_HEADER_SIZE, (int64_t)data_offset);
    if (0 > rc)
    {
        return rc;
    }
    else if (rc != PFS_VHD_DYN_HEADER_SIZE)
    {
        return PFS_BLKIO_RET_IO;
    }

    if (0 != memcmp(raw, PFS_VHD_DYN_COOKIE, PFS_VHD_DYN_COOKIE_LEN))
    {
        return PFS_BLKIO_RET_PROBE;
    }

    uint32_t expect_sum = pfs_vhd_checksum_skip(raw, PFS_VHD_DYN_HEADER_SIZE, PFS_VHD_DYN_CHECKSUM_OFF, 4);
    if (expect_sum != pfs_read_be32(raw + PFS_VHD_DYN_CHECKSUM_OFF))
    {
        return PFS_BLKIO_RET_PROBE;
    }

    pfs_vhd_dyn_header_from_raw(raw, p_hdr);

    if (PFS_VHD_VERSION_1_0 != p_hdr->header_version)
    {
        return PFS_BLKIO_RET_PROBE;
    }
    if (0 == p_hdr->block_size || 0 != (p_hdr->block_size & (p_hdr->block_size - 1u)) ||
            p_hdr->block_size < (uint32_t)PFS_VHD_SECTOR_SIZE)
    {
        return PFS_BLKIO_RET_PROBE;
    }
    if (0 == p_hdr->max_table_entries || p_hdr->max_table_entries > (16u * 1024u * 1024u))
    {
        return PFS_BLKIO_RET_PROBE;
    }
    if (p_hdr->table_offset > (uint64_t)INT64_MAX)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    return PFS_BLKIO_RET_OK;
}

// end
