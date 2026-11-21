// Doc Encode : UTF-8 BOM, Unix(LF)
// UDF tag/CRC helpers and blkio sector read
#include "udf/pfs_udf_internal.h"
#include "base/pfs_mem.h"
#include "pfs_endian.h"

#include <string.h>

int pfs_udf_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_UDF_RET_OK;
    }
    if (PFS_UDF_RET_EINVAL == rc || PFS_UDF_RET_ENOMEM == rc ||
            PFS_UDF_RET_EIO == rc || PFS_UDF_RET_EBUSY == rc)
    {
        return rc;
    }
    return PFS_UDF_RET_EIO;
}

uint8_t pfs_udf_tag_checksum(const pfs_udf_tag_t *p_tag)
{
    if (NULL == p_tag)
    {
        return 0;
    }
    const uint8_t *data = (const uint8_t *)p_tag;
    uint8_t sum = 0;
    for (int i = 0; i < (int)sizeof(pfs_udf_tag_t); i++)
    {
        if (4 != i)
        {
            sum = (uint8_t)(sum + data[i]);
        }
    }
    return sum;
}

uint16_t pfs_udf_crc16(const uint8_t *data, int len)
{
    uint16_t crc = 0;
    for (int i = 0; i < len; i++)
    {
        crc = (uint16_t)(crc ^ ((uint16_t)data[i] << 8));
        for (int b = 0; b < 8; b++)
        {
            if (0 != (crc & 0x8000u))
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            }
            else
            {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

int pfs_udf_verify_tag(const pfs_udf_tag_t *p_tag, uint32_t tag_location,
        const void *p_desc_body, int desc_body_max)
{
    if (NULL == p_tag)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (pfs_udf_le32(&p_tag->tagLocation) != tag_location)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    uint8_t sum = pfs_udf_tag_checksum(p_tag);
    if (sum != p_tag->tagChecksum)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    if (0x0002u != pfs_udf_le16(&p_tag->descVersion) &&
            0x0003u != pfs_udf_le16(&p_tag->descVersion))
    {
        return PFS_UDF_RET_EBADMSG;
    }

    uint16_t crc_len = pfs_udf_le16(&p_tag->descCRCLength);
    if (0 == crc_len)
    {
        return PFS_UDF_RET_OK;
    }
    if (NULL == p_desc_body || crc_len > (uint16_t)desc_body_max)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    uint16_t expect_crc = pfs_udf_le16(&p_tag->descCRC);
    uint16_t got_crc = pfs_udf_crc16((const uint8_t *)p_desc_body, (int)crc_len);
    if (expect_crc != got_crc)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    return PFS_UDF_RET_OK;
}

int64_t pfs_udf_vol_sector_to_off(int64_t part_offset, uint32_t sector_size, uint32_t vol_sector)
{
    return part_offset + ((int64_t)vol_sector * (int64_t)sector_size);
}

int pfs_udf_read_sector(pfs_blkio_t *p_blkio, int64_t part_offset, uint32_t sector_size,
        uint32_t vol_sector, void *buf)
{
    if (NULL == p_blkio || NULL == buf || 0 == sector_size)
    {
        return PFS_UDF_RET_EINVAL;
    }
    int64_t off = pfs_udf_vol_sector_to_off(part_offset, sector_size, vol_sector);
    int nread = pfs_blkio_pread(p_blkio, buf, (int)sector_size, off);
    if (nread != (int)sector_size)
    {
        if (nread < 0)
        {
            return pfs_udf_map_blkio_rc(nread);
        }
        return PFS_UDF_RET_EIO;
    }
    return PFS_UDF_RET_OK;
}

int pfs_udf_read_vol_block(pfs_blkio_t *p_blkio, int64_t part_offset, uint32_t sector_size,
        uint32_t block_size, uint32_t vol_block, void *buf)
{
    if (NULL == p_blkio || NULL == buf || 0 == sector_size || 0 == block_size)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (0 != (block_size % sector_size))
    {
        return PFS_UDF_RET_EBADMSG;
    }
    uint32_t bps = block_size / sector_size;
    uint32_t sec = vol_block * bps;
    if (1 == bps)
    {
        return pfs_udf_read_sector(p_blkio, part_offset, sector_size, sec, buf);
    }

    uint8_t *secbuf = (uint8_t *)pfs_malloc((size_t)sector_size);
    if (NULL == secbuf)
    {
        return PFS_UDF_RET_ENOMEM;
    }
    for (uint32_t in_sec = 0; in_sec < bps; in_sec++)
    {
        int rc = pfs_udf_read_sector(p_blkio, part_offset, sector_size, sec + in_sec,
                secbuf);
        if (PFS_UDF_RET_OK != rc)
        {
            pfs_free(secbuf);
            return rc;
        }
        memcpy((uint8_t *)buf + (size_t)in_sec * (size_t)sector_size, secbuf,
                (size_t)sector_size);
    }
    pfs_free(secbuf);
    return PFS_UDF_RET_OK;
}

int64_t pfs_udf_part_lba_to_off(const pfs_udf_sbi_t *p_sbi, uint32_t part_lba)
{
    if (NULL == p_sbi || 0 == p_sbi->n_maps)
    {
        return (int64_t)PFS_UDF_RET_EINVAL;
    }
    uint16_t part_ref = 0;
    for (; (int)part_ref < p_sbi->n_maps; part_ref++)
    {
        if (PFS_UDF_PMAP_TYPE1 == p_sbi->maps[part_ref].type)
        {
            break;
        }
    }
    if ((int)part_ref >= p_sbi->n_maps)
    {
        part_ref = 0;
    }
    return pfs_udf_map_to_off(p_sbi, part_ref, part_lba);
}

int pfs_udf_read_part_block(const pfs_udf_sbi_t *p_sbi, uint32_t part_lba, void *buf)
{
    if (NULL == p_sbi)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (0 == p_sbi->n_maps)
    {
        return PFS_UDF_RET_EINVAL;
    }
    uint16_t part_ref = p_sbi->root.part_ref;
    return pfs_udf_read_part_block_ref(p_sbi, part_ref, part_lba, buf);
}

int pfs_udf_read_tagged(pfs_blkio_t *p_blkio, int64_t part_offset, uint32_t sector_size,
        uint32_t block_size,         uint32_t vol_block, uint16_t expect_ident, void *buf)
{
    if (NULL == buf)
    {
        return PFS_UDF_RET_EINVAL;
    }
    int rc = pfs_udf_read_vol_block(p_blkio, part_offset, sector_size, block_size, vol_block, buf);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    pfs_udf_tag_t *p_tag = (pfs_udf_tag_t *)buf;
    if (0 != expect_ident && pfs_udf_le16(&p_tag->tagIdent) != expect_ident)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    rc = pfs_udf_verify_tag(p_tag, vol_block,
            (const uint8_t *)buf + sizeof(pfs_udf_tag_t),
            (int)block_size - (int)sizeof(pfs_udf_tag_t));
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    return PFS_UDF_RET_OK;
}

void pfs_udf_wle16(void *p, uint16_t v)
{
    pfs_write_le16((uint8_t *)p, v);
}

void pfs_udf_wle32(void *p, uint32_t v)
{
    pfs_write_le32((uint8_t *)p, v);
}

void pfs_udf_wle64(void *p, uint64_t v)
{
    pfs_write_le64((uint8_t *)p, v);
}

void pfs_udf_set_tag(pfs_udf_tag_t *p_tag, uint16_t ident, uint32_t tag_loc,
        void *p_body, int body_off, int body_crc_len, uint16_t desc_ver)
{
    if (NULL == p_tag)
    {
        return;
    }
    memset(p_tag, 0, sizeof(*p_tag));
    pfs_udf_wle16(&p_tag->tagIdent, ident);
    pfs_udf_wle16(&p_tag->descVersion, desc_ver);
    p_tag->reserved = 0;
    pfs_udf_wle16(&p_tag->tagSerialNum, 1);
    pfs_udf_wle32(&p_tag->tagLocation, tag_loc);
    if (body_crc_len > 0 && NULL != p_body)
    {
        uint8_t *p_crc_base = (uint8_t *)p_body + (size_t)body_off;
        pfs_udf_wle16(&p_tag->descCRCLength, (uint16_t)body_crc_len);
        pfs_udf_wle16(&p_tag->descCRC, pfs_udf_crc16(p_crc_base, body_crc_len));
    }
    else
    {
        pfs_udf_wle16(&p_tag->descCRCLength, 0);
        pfs_udf_wle16(&p_tag->descCRC, 0);
    }
    p_tag->tagChecksum = pfs_udf_tag_checksum(p_tag);
}

// end
