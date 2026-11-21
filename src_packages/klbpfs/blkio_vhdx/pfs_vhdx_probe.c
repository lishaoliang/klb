// Doc Encode : UTF-8 BOM, Unix(LF)
// VHDX probe: file id / header / region / metadata; 填 priv 几何后加载 BAT
#include "blkio_vhdx/pfs_vhdx_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <stdbool.h>
#include <string.h>

static bool pfs_vhdx_is_pow2_u32(uint32_t v)
{
    return (0u != v && 0u == (v & (v - 1u)));
}

static bool pfs_vhdx_is_pow2_u64(uint64_t v)
{
    return (0ull != v && 0ull == (v & (v - 1ull)));
}

static void pfs_vhdx_header_from_raw(const uint8_t *p, pfs_vhdx_header_t *h)
{
    h->signature = pfs_read_le32(p);
    h->checksum = pfs_read_le32(p + 4);
    h->sequence_number = pfs_read_le64(p + 8);
    pfs_vhdx_guid_from_raw(p + 16, &h->file_write_guid);
    pfs_vhdx_guid_from_raw(p + 32, &h->data_write_guid);
    pfs_vhdx_guid_from_raw(p + 48, &h->log_guid);
    h->log_version = pfs_read_le16(p + 64);
    h->version = pfs_read_le16(p + 66);
    h->log_length = pfs_read_le32(p + 68);
    h->log_offset = pfs_read_le64(p + 72);
}

static int pfs_vhdx_read_header(pfs_blkio_t *p_host, uint64_t off, pfs_vhdx_header_t *h, bool *p_ok)
{
    *p_ok = false;
    uint8_t *p_buf = (uint8_t *)pfs_malloc(PFS_VHDX_HEADER_CRC_SIZE);
    if (NULL == p_buf)
    {
        return PFS_BLKIO_RET_IO;
    }
    int rc = pfs_blkio_raw_pread(p_host, p_buf, (int)PFS_VHDX_HEADER_CRC_SIZE, (int64_t)off);
    if (0 > rc)
    {
        pfs_free(p_buf);
        return rc;
    }
    else if (rc != (int)PFS_VHDX_HEADER_CRC_SIZE)
    {
        pfs_free(p_buf);
        return PFS_BLKIO_RET_IO;
    }
    if (PFS_VHDX_HEAD_SIG != pfs_read_le32(p_buf))
    {
        pfs_free(p_buf);
        return PFS_BLKIO_RET_OK;
    }
    if (!pfs_vhdx_crc32c_valid(p_buf, PFS_VHDX_HEADER_CRC_SIZE, 4))
    {
        pfs_free(p_buf);
        return PFS_BLKIO_RET_OK;
    }
    pfs_vhdx_header_from_raw(p_buf, h);
    pfs_free(p_buf);
    if (PFS_VHDX_VERSION_1 != h->version || PFS_VHDX_LOG_VERSION_0 != h->log_version)
    {
        return PFS_BLKIO_RET_OK;
    }
    *p_ok = true;
    return PFS_BLKIO_RET_OK;
}

static int pfs_vhdx_load_active_header(pfs_blkio_t *p_host, pfs_vhdx_header_t *h)
{
    pfs_vhdx_header_t h1;
    pfs_vhdx_header_t h2;

    memset(&h1, 0, sizeof(h1));
    memset(&h2, 0, sizeof(h2));
    bool ok1 = false;
    int rc = pfs_vhdx_read_header(p_host, PFS_VHDX_HEADER1_OFFSET, &h1, &ok1);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    bool ok2 = false;
    rc = pfs_vhdx_read_header(p_host, PFS_VHDX_HEADER2_OFFSET, &h2, &ok2);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    if (!ok1 && !ok2)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (ok1 && ok2)
    {
        *h = (h1.sequence_number >= h2.sequence_number) ? h1 : h2;
    }
    else if (ok1)
    {
        *h = h1;
    }
    else
    {
        *h = h2;
    }
    return PFS_BLKIO_RET_OK;
}

static int pfs_vhdx_read_region_table(pfs_blkio_t *p_host, uint64_t off,
        pfs_vhdx_region_ent_t *p_bat, pfs_vhdx_region_ent_t *p_meta, bool *p_ok)
{
    *p_ok = false;
    bool got_bat = false;
    bool got_meta = false;
    uint8_t *p_buf = (uint8_t *)pfs_malloc(PFS_VHDX_HEADER_BLOCK_SIZE);
    if (NULL == p_buf)
    {
        return PFS_BLKIO_RET_IO;
    }
    int rc = pfs_blkio_raw_pread(p_host, p_buf, (int)PFS_VHDX_HEADER_BLOCK_SIZE, (int64_t)off);
    if (0 > rc)
    {
        pfs_free(p_buf);
        return rc;
    }
    else if (rc != (int)PFS_VHDX_HEADER_BLOCK_SIZE)
    {
        pfs_free(p_buf);
        return PFS_BLKIO_RET_IO;
    }
    if (PFS_VHDX_REGI_SIG != pfs_read_le32(p_buf) ||
            !pfs_vhdx_crc32c_valid(p_buf, PFS_VHDX_HEADER_BLOCK_SIZE, 4))
    {
        pfs_free(p_buf);
        return PFS_BLKIO_RET_OK;
    }

    pfs_vhdx_region_hdr_t hdr;
    hdr.signature = pfs_read_le32(p_buf);
    hdr.checksum = pfs_read_le32(p_buf + 4);
    hdr.entry_count = pfs_read_le32(p_buf + 8);
    hdr.reserved = pfs_read_le32(p_buf + 12);
    if (0 == hdr.entry_count || hdr.entry_count > 2047u)
    {
        pfs_free(p_buf);
        return PFS_BLKIO_RET_OK;
    }

    for (uint32_t i = 0; i < hdr.entry_count; i++)
    {
        const uint8_t *pe = p_buf + 16 + i * 32u;
        pfs_vhdx_region_ent_t ent;

        pfs_vhdx_guid_from_raw(pe, &ent.guid);
        ent.file_offset = pfs_read_le64(pe + 16);
        ent.length = pfs_read_le32(pe + 24);
        ent.data_bits = pfs_read_le32(pe + 28);

        if (0 != (ent.data_bits & PFS_VHDX_REGION_REQUIRED) &&
                !pfs_vhdx_guid_eq(&ent.guid, pfs_vhdx_guid_bat()) &&
                !pfs_vhdx_guid_eq(&ent.guid, pfs_vhdx_guid_metadata()))
        {
            // 未知必选 region
            pfs_free(p_buf);
            return PFS_BLKIO_RET_STATE;
        }
        if (pfs_vhdx_guid_eq(&ent.guid, pfs_vhdx_guid_bat()))
        {
            if (ent.file_offset < PFS_VHDX_MIB || 0 != (ent.file_offset % PFS_VHDX_MIB) ||
                    0 == ent.length || 0 != (ent.length % PFS_VHDX_MIB))
            {
                pfs_free(p_buf);
                return PFS_BLKIO_RET_STATE;
            }
            *p_bat = ent;
            got_bat = true;
        }
        else if (pfs_vhdx_guid_eq(&ent.guid, pfs_vhdx_guid_metadata()))
        {
            if (ent.file_offset < PFS_VHDX_MIB || 0 != (ent.file_offset % PFS_VHDX_MIB) ||
                    0 == ent.length || 0 != (ent.length % PFS_VHDX_MIB))
            {
                pfs_free(p_buf);
                return PFS_BLKIO_RET_STATE;
            }
            *p_meta = ent;
            got_meta = true;
        }
    }
    pfs_free(p_buf);
    if (!got_bat || !got_meta)
    {
        return PFS_BLKIO_RET_STATE;
    }
    *p_ok = true;
    return PFS_BLKIO_RET_OK;
}

static int pfs_vhdx_parse_metadata(pfs_blkio_t *p_host, const pfs_vhdx_region_ent_t *p_meta,
        pfs_vhdx_priv_t *p_priv, bool *p_has_parent)
{
    *p_has_parent = false;
    bool got_params = false;
    bool got_vsize = false;
    bool got_lss = false;
    bool got_pss = false;
    uint32_t block_size = 0;
    uint32_t params_bits = 0;
    uint64_t vsize = 0;
    uint32_t lss = 0;
    uint32_t pss = 0;

    uint8_t *p_tbl = (uint8_t *)pfs_malloc(PFS_VHDX_HEADER_BLOCK_SIZE);
    if (NULL == p_tbl)
    {
        return PFS_BLKIO_RET_IO;
    }
    int rc = pfs_blkio_raw_pread(p_host, p_tbl, (int)PFS_VHDX_HEADER_BLOCK_SIZE,
            (int64_t)p_meta->file_offset);
    if (0 > rc)
    {
        pfs_free(p_tbl);
        return rc;
    }
    else if (rc != (int)PFS_VHDX_HEADER_BLOCK_SIZE)
    {
        pfs_free(p_tbl);
        return PFS_BLKIO_RET_IO;
    }
    if (PFS_VHDX_METADATA_SIG != pfs_read_le64(p_tbl))
    {
        pfs_free(p_tbl);
        return PFS_BLKIO_RET_STATE;
    }
    pfs_vhdx_meta_hdr_t mh;
    mh.reserved = pfs_read_le16(p_tbl + 8);
    mh.entry_count = pfs_read_le16(p_tbl + 10);
    (void)mh.signature;
    if (0 == mh.entry_count || mh.entry_count > 2047u)
    {
        pfs_free(p_tbl);
        return PFS_BLKIO_RET_STATE;
    }

    for (uint32_t i = 0; i < mh.entry_count; i++)
    {
        const uint8_t *pe = p_tbl + 32 + i * 32u;
        pfs_vhdx_meta_ent_t ent;

        pfs_vhdx_guid_from_raw(pe, &ent.item_id);
        ent.offset = pfs_read_le32(pe + 16);
        ent.length = pfs_read_le32(pe + 20);
        ent.data_bits = pfs_read_le32(pe + 24);
        ent.reserved2 = pfs_read_le32(pe + 28);

        if (0 == ent.length)
        {
            continue;
        }
        if (ent.offset < PFS_VHDX_HEADER_BLOCK_SIZE ||
                (uint64_t)ent.offset + (uint64_t)ent.length > (uint64_t)p_meta->length)
        {
            pfs_free(p_tbl);
            return PFS_BLKIO_RET_STATE;
        }

        uint8_t *p_item = (uint8_t *)pfs_malloc(ent.length);
        if (NULL == p_item)
        {
            pfs_free(p_tbl);
            return PFS_BLKIO_RET_IO;
        }
        rc = pfs_blkio_raw_pread(p_host, p_item, (int)ent.length,
                (int64_t)(p_meta->file_offset + (uint64_t)ent.offset));
        if (0 > rc)
        {
            pfs_free(p_item);
            pfs_free(p_tbl);
            return rc;
        }
        else if (rc != (int)ent.length)
        {
            pfs_free(p_item);
            pfs_free(p_tbl);
            return PFS_BLKIO_RET_IO;
        }

        if (pfs_vhdx_guid_eq(&ent.item_id, pfs_vhdx_guid_file_params()))
        {
            if (ent.length < 8u)
            {
                pfs_free(p_item);
                pfs_free(p_tbl);
                return PFS_BLKIO_RET_STATE;
            }
            block_size = pfs_read_le32(p_item);
            params_bits = pfs_read_le32(p_item + 4);
            got_params = true;
        }
        else if (pfs_vhdx_guid_eq(&ent.item_id, pfs_vhdx_guid_virtual_size()))
        {
            if (ent.length < 8u)
            {
                pfs_free(p_item);
                pfs_free(p_tbl);
                return PFS_BLKIO_RET_STATE;
            }
            vsize = pfs_read_le64(p_item);
            got_vsize = true;
        }
        else if (pfs_vhdx_guid_eq(&ent.item_id, pfs_vhdx_guid_logical_sector()))
        {
            if (ent.length < 4u)
            {
                pfs_free(p_item);
                pfs_free(p_tbl);
                return PFS_BLKIO_RET_STATE;
            }
            lss = pfs_read_le32(p_item);
            got_lss = true;
        }
        else if (pfs_vhdx_guid_eq(&ent.item_id, pfs_vhdx_guid_physical_sector()))
        {
            if (ent.length < 4u)
            {
                pfs_free(p_item);
                pfs_free(p_tbl);
                return PFS_BLKIO_RET_STATE;
            }
            pss = pfs_read_le32(p_item);
            got_pss = true;
        }
        else if (pfs_vhdx_guid_eq(&ent.item_id, pfs_vhdx_guid_parent_locator()))
        {
            *p_has_parent = true;
        }

        pfs_free(p_item);
    }
    pfs_free(p_tbl);

    if (!got_params || !got_vsize || !got_lss || !got_pss)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0 != (params_bits & PFS_VHDX_PARAMS_HAS_PARENT))
    {
        *p_has_parent = true;
    }
    if (!pfs_vhdx_is_pow2_u32(block_size) || block_size < PFS_VHDX_BLOCK_SIZE_MIN ||
            block_size > PFS_VHDX_BLOCK_SIZE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (512u != lss && 4096u != lss)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (512u != pss && 4096u != pss)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0 == vsize || 0 != (vsize % (uint64_t)lss))
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_priv->block_size = block_size;
    p_priv->virtual_disk_size = vsize;
    p_priv->logical_sector_size = lss;
    p_priv->physical_sector_size = pss;
    p_priv->sectors_per_block = block_size / lss;
    if (!pfs_vhdx_is_pow2_u32(p_priv->sectors_per_block))
    {
        return PFS_BLKIO_RET_STATE;
    }
    p_priv->chunk_ratio = ((uint64_t)PFS_VHDX_MAX_SECTORS_PER_BLOCK * (uint64_t)lss) /
            (uint64_t)block_size;
    if (!pfs_vhdx_is_pow2_u64(p_priv->chunk_ratio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (0 != (params_bits & PFS_VHDX_PARAMS_LEAVE_ALLOCATED))
    {
        p_priv->disk_type = PFS_VHDX_DISK_FIXED;
    }
    else
    {
        p_priv->disk_type = PFS_VHDX_DISK_DYNAMIC;
    }
    return PFS_BLKIO_RET_OK;
}

int pfs_vhdx_probe_open(pfs_vhdx_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (0 > host_size || host_size < (int64_t)PFS_VHDX_MIB)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    uint8_t sig[8];
    int rc = pfs_blkio_raw_pread(&p_priv->host, sig, 8, 0);
    if (0 > rc)
    {
        return rc;
    }
    else if (8 != rc || 0 != memcmp(sig, PFS_VHDX_FILE_SIG, PFS_VHDX_FILE_SIG_LEN))
    {
        return PFS_BLKIO_RET_PROBE;
    }

    pfs_vhdx_header_t hdr;
    rc = pfs_vhdx_load_active_header(&p_priv->host, &hdr);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    p_priv->hdr_sequence = hdr.sequence_number;
    p_priv->file_write_guid = hdr.file_write_guid;
    p_priv->data_write_guid = hdr.data_write_guid;
    p_priv->log_guid = hdr.log_guid;
    p_priv->log_offset = hdr.log_offset;
    p_priv->log_length = hdr.log_length;
    p_priv->log_sequence = 1;
    p_priv->first_visible_write = 1;

    if (0 == p_priv->log_offset || 0 == p_priv->log_length ||
            0 != (p_priv->log_offset % (uint64_t)PFS_VHDX_MIB) ||
            0 != (p_priv->log_length % PFS_VHDX_MIB))
    {
        // 允许无 log 区的只读镜像? 规范要求有 log 区; 写路径需要
        if (!pfs_vhdx_guid_is_zero(&hdr.log_guid))
        {
            return PFS_BLKIO_RET_STATE;
        }
    }

    // 脏 log: 可写打开时最小回放; 只读则拒绝
    if (!pfs_vhdx_guid_is_zero(&hdr.log_guid))
    {
        rc = pfs_vhdx_log_replay_open(p_priv);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
    }

    pfs_vhdx_region_ent_t bat_reg;
    pfs_vhdx_region_ent_t meta_reg;
    memset(&bat_reg, 0, sizeof(bat_reg));
    memset(&meta_reg, 0, sizeof(meta_reg));
    bool ok = false;
    rc = pfs_vhdx_read_region_table(&p_priv->host, PFS_VHDX_REGION1_OFFSET,
            &bat_reg, &meta_reg, &ok);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    if (!ok)
    {
        rc = pfs_vhdx_read_region_table(&p_priv->host, PFS_VHDX_REGION2_OFFSET,
                &bat_reg, &meta_reg, &ok);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
        if (!ok)
        {
            return PFS_BLKIO_RET_STATE;
        }
    }

    bool has_parent = false;
    rc = pfs_vhdx_parse_metadata(&p_priv->host, &meta_reg, p_priv, &has_parent);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    if (has_parent)
    {
        p_priv->disk_type = PFS_VHDX_DISK_DIFF;
        return PFS_BLKIO_RET_STATE;
    }

    // BAT 项数 (无 parent): data_blocks + floor((data_blocks-1)/chunk_ratio)
    uint32_t data_blocks = (uint32_t)((p_priv->virtual_disk_size + (uint64_t)p_priv->block_size - 1ull) /
            (uint64_t)p_priv->block_size);
    if (0 == data_blocks)
    {
        return PFS_BLKIO_RET_STATE;
    }
    uint32_t chunk_bits = 0;
    {
        uint64_t cr = p_priv->chunk_ratio;
        while (cr > 1ull)
        {
            cr >>= 1;
            chunk_bits++;
        }
    }
    uint32_t bat_need = data_blocks + ((data_blocks - 1u) >> chunk_bits);
    if ((uint64_t)bat_need * 8ull > (uint64_t)bat_reg.length)
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_priv->bat_offset = bat_reg.file_offset;
    p_priv->bat_length = bat_reg.length;
    p_priv->bat_entries = bat_need;
    rc = pfs_vhdx_bat_load(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    return PFS_BLKIO_RET_OK;
}

// end
