// Doc Encode : UTF-8 BOM, Unix(LF)
// VHDX Fixed/Dynamic 共用 payload 读/写
#include "blkio_vhdx/pfs_vhdx_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

static bool pfs_vhdx_range_ok(const pfs_vhdx_priv_t *p_priv, int64_t offset, int size)
{
    if (offset < 0 || size < 0 || size > PFS_BLKIO_IO_SIZE_MAX)
    {
        return false;
    }
    if (0 == size)
    {
        return true;
    }

    uint64_t off_u = (uint64_t)offset;
    if (off_u >= p_priv->virtual_disk_size)
    {
        return false;
    }
    uint64_t end_u = off_u + (uint64_t)size;
    if (end_u < off_u || end_u > p_priv->virtual_disk_size)
    {
        return false;
    }
    return true;
}

static uint32_t pfs_vhdx_ctz32(uint32_t v)
{
    uint32_t n = 0;
    while (0u == (v & 1u))
    {
        v >>= 1;
        n++;
    }
    return n;
}

static uint32_t pfs_vhdx_ctz64(uint64_t v)
{
    uint32_t n = 0;
    while (0ull == (v & 1ull))
    {
        v >>= 1;
        n++;
    }
    return n;
}

static uint64_t pfs_vhdx_round_up_mib(uint64_t v)
{
    return (v + (uint64_t)PFS_VHDX_MIB - 1ull) & ~((uint64_t)PFS_VHDX_MIB - 1ull);
}

static int pfs_vhdx_zero_range(pfs_vhdx_priv_t *p_priv, int64_t off, uint64_t bytes)
{
    uint32_t chunk = 64u * 1024u;
    if ((uint64_t)chunk > bytes)
    {
        chunk = (uint32_t)bytes;
    }
    uint8_t *p_zero = (uint8_t *)pfs_mallocz(chunk);
    if (NULL == p_zero)
    {
        return PFS_BLKIO_RET_IO;
    }
    uint64_t left = bytes;
    int64_t cur = off;
    while (left > 0)
    {
        uint32_t n = chunk;
        if ((uint64_t)n > left)
        {
            n = (uint32_t)left;
        }
        int rc = pfs_blkio_raw_pwrite(&p_priv->host, p_zero, (int)n, cur);
        if (0 > rc)
        {
            pfs_free(p_zero);
            return rc;
        }
        else if (rc != (int)n)
        {
            pfs_free(p_zero);
            return PFS_BLKIO_RET_IO;
        }
        cur += (int64_t)n;
        left -= (uint64_t)n;
    }
    pfs_free(p_zero);
    return PFS_BLKIO_RET_OK;
}

static int pfs_vhdx_touch_visible(pfs_vhdx_priv_t *p_priv)
{
    if (!p_priv->first_visible_write)
    {
        return PFS_BLKIO_RET_OK;
    }
    p_priv->first_visible_write = 0;
    pfs_vhdx_guid_make(&p_priv->file_write_guid);
    pfs_vhdx_guid_make(&p_priv->data_write_guid);
    return pfs_vhdx_headers_write(p_priv);
}

static int pfs_vhdx_alloc_payload(pfs_vhdx_priv_t *p_priv, uint32_t bat_idx, uint64_t *p_file_off)
{
    if (NULL == p_file_off || bat_idx >= p_priv->bat_entries)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (0 > host_size)
    {
        return PFS_BLKIO_RET_IO;
    }
    uint64_t new_off = pfs_vhdx_round_up_mib((uint64_t)host_size);

    // 先扩块并填 0 (崩溃时 BAT 仍未指向, 安全)
    int rc = pfs_vhdx_zero_range(p_priv, (int64_t)new_off, (uint64_t)p_priv->block_size);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_blkio_raw_fsync(&p_priv->host);
    if (0 > rc)
    {
        return rc;
    }

    uint64_t ent = (new_off & PFS_VHDX_BAT_FILE_OFF_MASK) | PFS_VHDX_PAYLOAD_FULLY_PRESENT;
    rc = pfs_vhdx_bat_set_entry(p_priv, bat_idx, ent);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    *p_file_off = new_off;
    return PFS_BLKIO_RET_OK;
}

int pfs_vhdx_payload_pread(pfs_vhdx_priv_t *p_priv, void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL == p_priv->p_bat || 0 == p_priv->block_size || 0 == p_priv->logical_sector_size)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (!pfs_vhdx_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }

    uint8_t *out = (uint8_t *)buf;
    int done = 0;
    uint32_t lss = p_priv->logical_sector_size;
    uint32_t spb_bits = pfs_vhdx_ctz32(p_priv->sectors_per_block);
    uint32_t chunk_bits = pfs_vhdx_ctz64(p_priv->chunk_ratio);

    while (done < size)
    {
        uint64_t byte_off = (uint64_t)offset + (uint64_t)done;
        uint64_t sector_num = byte_off / (uint64_t)lss;
        uint32_t block_idx = (uint32_t)(sector_num >> spb_bits);
        uint32_t bat_idx = block_idx + (block_idx >> chunk_bits);
        uint32_t off_in_block = (uint32_t)(byte_off % (uint64_t)p_priv->block_size);
        uint32_t left_in_block = p_priv->block_size - off_in_block;
        uint32_t n;
        uint64_t bat_ent;
        uint32_t state;
        uint64_t file_off;
        int rc;

        if (bat_idx >= p_priv->bat_entries)
        {
            return PFS_BLKIO_RET_STATE;
        }

        n = left_in_block;
        if (n > (uint32_t)(size - done))
        {
            n = (uint32_t)(size - done);
        }

        bat_ent = p_priv->p_bat[bat_idx];
        state = (uint32_t)(bat_ent & PFS_VHDX_BAT_STATE_MASK);
        file_off = bat_ent & PFS_VHDX_BAT_FILE_OFF_MASK;

        switch (state)
        {
        case PFS_VHDX_PAYLOAD_NOT_PRESENT:
        case PFS_VHDX_PAYLOAD_UNDEFINED:
        case PFS_VHDX_PAYLOAD_ZERO:
        case PFS_VHDX_PAYLOAD_UNMAPPED:
            memset(out + done, 0, (size_t)n);
            break;
        case PFS_VHDX_PAYLOAD_FULLY_PRESENT:
            if (0 == file_off)
            {
                return PFS_BLKIO_RET_STATE;
            }
            rc = pfs_blkio_raw_pread(&p_priv->host, out + done, (int)n,
                    (int64_t)(file_off + (uint64_t)off_in_block));
            if (0 > rc)
            {
                return rc;
            }
            else if (rc != (int)n)
            {
                return PFS_BLKIO_RET_IO;
            }
            break;
        case PFS_VHDX_PAYLOAD_PARTIAL:
        default:
            return PFS_BLKIO_RET_STATE;
        }

        done += (int)n;
    }

    return size;
}

int pfs_vhdx_payload_pwrite(pfs_vhdx_priv_t *p_priv, const void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (PFS_BLKIO_O_RDONLY == p_priv->open_flags)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (NULL == p_priv->p_bat || 0 == p_priv->block_size || 0 == p_priv->logical_sector_size)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (!pfs_vhdx_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }

    int rc = pfs_vhdx_touch_visible(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    const uint8_t *in = (const uint8_t *)buf;
    int done = 0;
    uint32_t spb_bits = pfs_vhdx_ctz32(p_priv->sectors_per_block);
    uint32_t chunk_bits = pfs_vhdx_ctz64(p_priv->chunk_ratio);

    while (done < size)
    {
        uint64_t byte_off = (uint64_t)offset + (uint64_t)done;
        uint64_t sector_num = byte_off / (uint64_t)p_priv->logical_sector_size;
        uint32_t block_idx = (uint32_t)(sector_num >> spb_bits);
        uint32_t bat_idx = block_idx + (block_idx >> chunk_bits);
        uint32_t off_in_block = (uint32_t)(byte_off % (uint64_t)p_priv->block_size);
        uint32_t left_in_block = p_priv->block_size - off_in_block;
        uint32_t n;
        uint64_t bat_ent;
        uint32_t state;
        uint64_t file_off;

        if (bat_idx >= p_priv->bat_entries)
        {
            return PFS_BLKIO_RET_STATE;
        }

        n = left_in_block;
        if (n > (uint32_t)(size - done))
        {
            n = (uint32_t)(size - done);
        }

        bat_ent = p_priv->p_bat[bat_idx];
        state = (uint32_t)(bat_ent & PFS_VHDX_BAT_STATE_MASK);
        file_off = bat_ent & PFS_VHDX_BAT_FILE_OFF_MASK;

        if (PFS_VHDX_PAYLOAD_FULLY_PRESENT != state)
        {
            if (PFS_VHDX_DISK_FIXED == p_priv->disk_type)
            {
                return PFS_BLKIO_RET_STATE;
            }
            rc = pfs_vhdx_alloc_payload(p_priv, bat_idx, &file_off);
            if (PFS_BLKIO_RET_OK != rc)
            {
                return rc;
            }
        }
        else if (0 == file_off)
        {
            return PFS_BLKIO_RET_STATE;
        }

        rc = pfs_blkio_raw_pwrite(&p_priv->host, in + done, (int)n,
                (int64_t)(file_off + (uint64_t)off_in_block));
        if (0 > rc)
        {
            return rc;
        }
        else if (rc != (int)n)
        {
            return PFS_BLKIO_RET_IO;
        }

        done += (int)n;
    }

    return size;
}

// end
