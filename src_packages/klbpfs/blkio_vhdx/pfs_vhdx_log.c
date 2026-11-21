// Doc Encode : UTF-8 BOM, Unix(LF)
// VHDX 最小 log: 单条 8KiB entry 提交 4KiB 元数据; 打开时回放并清 log_guid
#include "blkio_vhdx/pfs_vhdx_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <string.h>

static int pfs_vhdx_host_write_all(pfs_blkio_t *p_host, const void *buf, int size, int64_t off)
{
    int rc = pfs_blkio_raw_pwrite(p_host, buf, size, off);
    if (0 > rc)
    {
        return rc;
    }
    else if (rc != size)
    {
        return PFS_BLKIO_RET_IO;
    }
    return PFS_BLKIO_RET_OK;
}

static void pfs_vhdx_fill_header_4k(uint8_t *blk, const pfs_vhdx_priv_t *p_priv, uint64_t seq)
{
    memset(blk, 0, PFS_VHDX_HEADER_CRC_SIZE);
    pfs_write_le32(blk, PFS_VHDX_HEAD_SIG);
    pfs_write_le64(blk + 8, seq);
    pfs_vhdx_guid_to_raw(blk + 16, &p_priv->file_write_guid);
    pfs_vhdx_guid_to_raw(blk + 32, &p_priv->data_write_guid);
    pfs_vhdx_guid_to_raw(blk + 48, &p_priv->log_guid);
    pfs_write_le16(blk + 64, PFS_VHDX_LOG_VERSION_0);
    pfs_write_le16(blk + 66, PFS_VHDX_VERSION_1);
    pfs_write_le32(blk + 68, p_priv->log_length);
    pfs_write_le64(blk + 72, p_priv->log_offset);
    pfs_vhdx_crc_store(blk, PFS_VHDX_HEADER_CRC_SIZE, 4);
}

int pfs_vhdx_headers_write(pfs_vhdx_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == p_priv->log_length || 0 == p_priv->log_offset)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t *p_hdr = (uint8_t *)pfs_malloc(PFS_VHDX_HEADER_CRC_SIZE);
    if (NULL == p_hdr)
    {
        return PFS_BLKIO_RET_IO;
    }

    uint64_t seq = p_priv->hdr_sequence + 1ull;
    pfs_vhdx_fill_header_4k(p_hdr, p_priv, seq);
    int rc = pfs_vhdx_host_write_all(&p_priv->host, p_hdr, (int)PFS_VHDX_HEADER_CRC_SIZE,
            (int64_t)PFS_VHDX_HEADER1_OFFSET);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_free(p_hdr);
        return rc;
    }

    seq++;
    pfs_vhdx_fill_header_4k(p_hdr, p_priv, seq);
    rc = pfs_vhdx_host_write_all(&p_priv->host, p_hdr, (int)PFS_VHDX_HEADER_CRC_SIZE,
            (int64_t)PFS_VHDX_HEADER2_OFFSET);
    pfs_free(p_hdr);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    p_priv->hdr_sequence = seq;
    rc = pfs_blkio_raw_fsync(&p_priv->host);
    if (0 > rc)
    {
        return rc;
    }
    return PFS_BLKIO_RET_OK;
}

static int pfs_vhdx_log_activate(pfs_vhdx_priv_t *p_priv)
{
    if (!pfs_vhdx_guid_is_zero(&p_priv->log_guid))
    {
        return PFS_BLKIO_RET_OK;
    }
    pfs_vhdx_guid_make(&p_priv->log_guid);
    if (0 == p_priv->log_sequence)
    {
        p_priv->log_sequence = 1;
    }
    return pfs_vhdx_headers_write(p_priv);
}

static int pfs_vhdx_log_deactivate(pfs_vhdx_priv_t *p_priv)
{
    memset(&p_priv->log_guid, 0, sizeof(p_priv->log_guid));
    return pfs_vhdx_headers_write(p_priv);
}

static int pfs_vhdx_log_build_entry(uint8_t *entry, size_t entry_cap,
        const pfs_vhdx_priv_t *p_priv, uint64_t file_offset, const uint8_t *data_4k,
        uint64_t seq, uint64_t flushed_size)
{
    if (entry_cap < (size_t)PFS_VHDX_LOG_SECTOR_SIZE * 2u)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 != (file_offset % (uint64_t)PFS_VHDX_LOG_SECTOR_SIZE))
    {
        return PFS_BLKIO_RET_PARAM;
    }

    memset(entry, 0, PFS_VHDX_LOG_SECTOR_SIZE * 2u);
    uint8_t *desc_sec = entry;
    uint8_t *data_sec = entry + PFS_VHDX_LOG_SECTOR_SIZE;

    // entry header (64B)
    pfs_write_le32(desc_sec, PFS_VHDX_LOGE_SIG);
    pfs_write_le32(desc_sec + 8, (uint32_t)(PFS_VHDX_LOG_SECTOR_SIZE * 2u)); // entry_length
    pfs_write_le32(desc_sec + 12, 0); // tail
    pfs_write_le64(desc_sec + 16, seq);
    pfs_write_le32(desc_sec + 24, 1); // descriptor_count
    pfs_vhdx_guid_to_raw(desc_sec + 32, &p_priv->log_guid);
    pfs_write_le64(desc_sec + 48, flushed_size);
    pfs_write_le64(desc_sec + 56, flushed_size);

    // data descriptor at +64
    uint8_t *desc = desc_sec + 64;
    pfs_write_le32(desc, PFS_VHDX_DESC_SIG);
    pfs_write_le32(desc + 4, pfs_read_le32(data_4k + 4092)); // trailing 4 bytes of sector
    // leading 8 bytes
    memcpy(desc + 8, data_4k, 8);
    // note: leading stored as LE uint64 in on-disk; bytes already little-endian memory order
    pfs_write_le64(desc + 16, file_offset);
    pfs_write_le64(desc + 24, seq);

    // data sector
    pfs_write_le32(data_sec, PFS_VHDX_DATA_SIG);
    pfs_write_le32(data_sec + 4, (uint32_t)(seq >> 32));
    memcpy(data_sec + 8, data_4k + 8, 4084);
    pfs_write_le32(data_sec + 4092, (uint32_t)(seq & 0xFFFFFFFFu));

    pfs_vhdx_crc_store(entry, PFS_VHDX_LOG_SECTOR_SIZE * 2u, 4);
    return PFS_BLKIO_RET_OK;
}

static int pfs_vhdx_log_apply_entry(pfs_vhdx_priv_t *p_priv, const uint8_t *entry)
{
    if (PFS_VHDX_LOGE_SIG != pfs_read_le32(entry))
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (1u != pfs_read_le32(entry + 24))
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (!pfs_vhdx_crc32c_valid((uint8_t *)entry, PFS_VHDX_LOG_SECTOR_SIZE * 2u, 4))
    {
        return PFS_BLKIO_RET_STATE;
    }

    const uint8_t *desc = entry + 64;
    const uint8_t *data_sec = entry + PFS_VHDX_LOG_SECTOR_SIZE;
    if (PFS_VHDX_DESC_SIG != pfs_read_le32(desc))
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (PFS_VHDX_DATA_SIG != pfs_read_le32(data_sec))
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint64_t seq = pfs_read_le64(entry + 16);
    uint64_t seq_data = ((uint64_t)pfs_read_le32(data_sec + 4) << 32) |
            (uint64_t)pfs_read_le32(data_sec + 4092);
    if (seq != seq_data || seq != pfs_read_le64(desc + 24))
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint64_t file_off = pfs_read_le64(desc + 16);
    uint8_t sector[PFS_VHDX_LOG_SECTOR_SIZE];
    memcpy(sector, desc + 8, 8);
    memcpy(sector + 8, data_sec + 8, 4084);
    pfs_write_le32(sector + 4092, pfs_read_le32(desc + 4));

    int rc = pfs_vhdx_host_write_all(&p_priv->host, sector, (int)PFS_VHDX_LOG_SECTOR_SIZE,
            (int64_t)file_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    return PFS_BLKIO_RET_OK;
}

int pfs_vhdx_log_commit_4k(pfs_vhdx_priv_t *p_priv, uint64_t file_offset, const uint8_t *data_4k)
{
    if (NULL == p_priv || NULL == data_4k)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (p_priv->log_length < (uint32_t)(PFS_VHDX_LOG_SECTOR_SIZE * 2u))
    {
        return PFS_BLKIO_RET_STATE;
    }

    int rc = pfs_vhdx_log_activate(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (host_size < 0)
    {
        return PFS_BLKIO_RET_IO;
    }
    uint64_t flushed = (uint64_t)host_size;
    flushed = (flushed + (uint64_t)PFS_VHDX_MIB - 1ull) & ~((uint64_t)PFS_VHDX_MIB - 1ull);

    uint64_t seq = p_priv->log_sequence;
    if (0 == seq)
    {
        seq = 1;
    }

    uint8_t *entry = (uint8_t *)pfs_malloc(PFS_VHDX_LOG_SECTOR_SIZE * 2u);
    if (NULL == entry)
    {
        return PFS_BLKIO_RET_IO;
    }

    rc = pfs_vhdx_log_build_entry(entry, PFS_VHDX_LOG_SECTOR_SIZE * 2u, p_priv,
            file_offset, data_4k, seq, flushed);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_free(entry);
        return rc;
    }

    // 1) 记 log
    rc = pfs_vhdx_host_write_all(&p_priv->host, entry, (int)(PFS_VHDX_LOG_SECTOR_SIZE * 2u),
            (int64_t)p_priv->log_offset);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_free(entry);
        return rc;
    }
    rc = pfs_blkio_raw_fsync(&p_priv->host);
    if (0 > rc)
    {
        pfs_free(entry);
        return rc;
    }

    // 2) 落盘目标 4KiB
    rc = pfs_vhdx_log_apply_entry(p_priv, entry);
    pfs_free(entry);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_blkio_raw_fsync(&p_priv->host);
    if (0 > rc)
    {
        return rc;
    }

    p_priv->log_sequence = seq + 1ull;

    // 3) 清 log_guid, 使后续 open 无需回放
    return pfs_vhdx_log_deactivate(p_priv);
}

int pfs_vhdx_log_replay_open(pfs_vhdx_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (pfs_vhdx_guid_is_zero(&p_priv->log_guid))
    {
        return PFS_BLKIO_RET_OK;
    }
    if (PFS_BLKIO_O_RDONLY == p_priv->open_flags)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (p_priv->log_length < (uint32_t)(PFS_VHDX_LOG_SECTOR_SIZE * 2u) ||
            0 == p_priv->log_offset)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t *entry = (uint8_t *)pfs_malloc(PFS_VHDX_LOG_SECTOR_SIZE * 2u);
    if (NULL == entry)
    {
        return PFS_BLKIO_RET_IO;
    }
    int rc = pfs_blkio_raw_pread(&p_priv->host, entry, (int)(PFS_VHDX_LOG_SECTOR_SIZE * 2u),
            (int64_t)p_priv->log_offset);
    if (0 > rc)
    {
        pfs_free(entry);
        return rc;
    }
    else if (rc != (int)(PFS_VHDX_LOG_SECTOR_SIZE * 2u))
    {
        pfs_free(entry);
        return PFS_BLKIO_RET_IO;
    }

    // 校验 log_guid 匹配
    {
        pfs_vhdx_guid_t g;
        pfs_vhdx_guid_from_raw(entry + 32, &g);
        if (!pfs_vhdx_guid_eq(&g, &p_priv->log_guid))
        {
            pfs_free(entry);
            return PFS_BLKIO_RET_STATE;
        }
    }

    rc = pfs_vhdx_log_apply_entry(p_priv, entry);
    pfs_free(entry);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_blkio_raw_fsync(&p_priv->host);
    if (0 > rc)
    {
        return rc;
    }

    return pfs_vhdx_log_deactivate(p_priv);
}

// end
