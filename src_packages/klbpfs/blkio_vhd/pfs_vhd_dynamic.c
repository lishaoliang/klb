// Doc Encode : UTF-8 BOM, Unix(LF)
// VHD Dynamic: BAT + 块内 sector bitmap; 未分配块读 0; 写时按需扩块
#include "blkio_vhd/pfs_vhd_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

static bool pfs_vhd_dyn_range_ok(const pfs_vhd_priv_t *p_priv, int64_t offset, int size)
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
    if (off_u >= p_priv->current_size)
    {
        return false;
    }
    uint64_t end_u = off_u + (uint64_t)size;
    if (end_u < off_u || end_u > p_priv->current_size)
    {
        return false;
    }
    return true;
}

// 规范: 块内第 0 扇区对应 bitmap 首字节最高位
static bool pfs_vhd_dyn_bitmap_test(const uint8_t *p_bm, uint32_t sector_in_block)
{
    uint32_t bi = sector_in_block / 8u;
    uint8_t mask = (uint8_t)(0x80u >> (sector_in_block % 8u));
    return (0 != (p_bm[bi] & mask));
}

static void pfs_vhd_dyn_bitmap_set(uint8_t *p_bm, uint32_t sector_in_block)
{
    uint32_t bi = sector_in_block / 8u;
    uint8_t mask = (uint8_t)(0x80u >> (sector_in_block % 8u));
    p_bm[bi] |= mask;
}

static int64_t pfs_vhd_dyn_sector_host_off(const pfs_vhd_priv_t *p_priv,
        uint32_t bat_sector, uint32_t sector_in_block)
{
    int64_t block_off = (int64_t)bat_sector * (int64_t)PFS_VHD_SECTOR_SIZE;
    int64_t data_off = block_off + (int64_t)p_priv->bitmap_sectors * (int64_t)PFS_VHD_SECTOR_SIZE;
    return data_off + (int64_t)sector_in_block * (int64_t)PFS_VHD_SECTOR_SIZE;
}

// 在文件尾 footer 前插入新数据块; 返回 BAT 扇区号
static int pfs_vhd_dyn_alloc_block(pfs_vhd_priv_t *p_priv, uint32_t block_idx, uint32_t *p_bat_sector)
{
    if (NULL == p_priv || NULL == p_bat_sector || block_idx >= p_priv->max_table_entries)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == p_priv->bitmap_sectors || 0 == p_priv->block_size)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (0 > host_size || host_size < (int64_t)PFS_VHD_FOOTER_SIZE)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t footer[PFS_VHD_FOOTER_SIZE];
    int64_t footer_off = host_size - (int64_t)PFS_VHD_FOOTER_SIZE;
    int rc = pfs_blkio_raw_pread(&p_priv->host, footer, PFS_VHD_FOOTER_SIZE, footer_off);
    if (0 > rc)
    {
        return rc;
    }
    else if (rc != PFS_VHD_FOOTER_SIZE)
    {
        return PFS_BLKIO_RET_IO;
    }

    uint64_t block_bytes = (uint64_t)p_priv->bitmap_sectors * (uint64_t)PFS_VHD_SECTOR_SIZE +
            (uint64_t)p_priv->block_size;
    int64_t block_off = footer_off;
    if (block_off < 0 || (uint64_t)block_off % (uint64_t)PFS_VHD_SECTOR_SIZE != 0)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if ((uint64_t)block_off / (uint64_t)PFS_VHD_SECTOR_SIZE > (uint64_t)UINT32_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }
    uint32_t bat_sector = (uint32_t)((uint64_t)block_off / (uint64_t)PFS_VHD_SECTOR_SIZE);
    int64_t new_footer_off = block_off + (int64_t)block_bytes;

    // 新块先填 0 (bitmap 全 0 => 未写扇区读为零)
    uint32_t zero_chunk = 64 * 1024;
    if (zero_chunk > (uint32_t)block_bytes)
    {
        zero_chunk = (uint32_t)block_bytes;
    }
    uint8_t *p_zero = (uint8_t *)pfs_mallocz(zero_chunk);
    if (NULL == p_zero)
    {
        return PFS_BLKIO_RET_IO;
    }
    uint64_t left = block_bytes;
    int64_t cur = block_off;
    while (left > 0)
    {
        uint32_t n = zero_chunk;
        if ((uint64_t)n > left)
        {
            n = (uint32_t)left;
        }
        rc = pfs_blkio_raw_pwrite(&p_priv->host, p_zero, (int)n, cur);
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

    rc = pfs_blkio_raw_pwrite(&p_priv->host, footer, PFS_VHD_FOOTER_SIZE, new_footer_off);
    if (0 > rc)
    {
        return rc;
    }
    else if (rc != PFS_VHD_FOOTER_SIZE)
    {
        return PFS_BLKIO_RET_IO;
    }

    rc = pfs_vhd_bat_set(p_priv, block_idx, bat_sector);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    *p_bat_sector = bat_sector;
    return PFS_BLKIO_RET_OK;
}

static int pfs_vhd_dyn_load_bitmap(pfs_vhd_priv_t *p_priv, uint32_t bat_sector, uint8_t *p_bm)
{
    int bm_bytes = (int)p_priv->bitmap_sectors * PFS_VHD_SECTOR_SIZE;
    int64_t off = (int64_t)bat_sector * (int64_t)PFS_VHD_SECTOR_SIZE;
    int rc = pfs_blkio_raw_pread(&p_priv->host, p_bm, bm_bytes, off);
    if (0 > rc)
    {
        return rc;
    }
    else if (rc != bm_bytes)
    {
        return PFS_BLKIO_RET_IO;
    }
    return PFS_BLKIO_RET_OK;
}

static int pfs_vhd_dyn_store_bitmap(pfs_vhd_priv_t *p_priv, uint32_t bat_sector, const uint8_t *p_bm)
{
    int bm_bytes = (int)p_priv->bitmap_sectors * PFS_VHD_SECTOR_SIZE;
    int64_t off = (int64_t)bat_sector * (int64_t)PFS_VHD_SECTOR_SIZE;
    int rc = pfs_blkio_raw_pwrite(&p_priv->host, p_bm, bm_bytes, off);
    if (0 > rc)
    {
        return rc;
    }
    else if (rc != bm_bytes)
    {
        return PFS_BLKIO_RET_IO;
    }
    return PFS_BLKIO_RET_OK;
}

static int pfs_vhd_dyn_read_chunk(pfs_vhd_priv_t *p_priv, uint8_t *buf, int size, int64_t offset)
{
    uint32_t block_idx = (uint32_t)((uint64_t)offset / (uint64_t)p_priv->block_size);
    uint32_t off_in_block = (uint32_t)((uint64_t)offset % (uint64_t)p_priv->block_size);
    int chunk = (int)(p_priv->block_size - off_in_block);
    if (chunk > size)
    {
        chunk = size;
    }

    uint32_t bat_sector = 0;
    int rc = pfs_vhd_bat_lookup(p_priv, block_idx, &bat_sector);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    if (PFS_VHD_BAT_ENTRY_UNUSED == bat_sector)
    {
        memset(buf, 0, (size_t)chunk);
        return chunk;
    }

    int bm_bytes = (int)p_priv->bitmap_sectors * PFS_VHD_SECTOR_SIZE;
    uint8_t *p_bm = (uint8_t *)pfs_malloc(bm_bytes);
    if (NULL == p_bm)
    {
        return PFS_BLKIO_RET_IO;
    }
    rc = pfs_vhd_dyn_load_bitmap(p_priv, bat_sector, p_bm);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_free(p_bm);
        return rc;
    }

    int done = 0;
    uint8_t sec_buf[PFS_VHD_SECTOR_SIZE];
    while (done < chunk)
    {
        uint32_t sector_in_block = (off_in_block + (uint32_t)done) / (uint32_t)PFS_VHD_SECTOR_SIZE;
        uint32_t sec_off = (off_in_block + (uint32_t)done) % (uint32_t)PFS_VHD_SECTOR_SIZE;
        int n = PFS_VHD_SECTOR_SIZE - (int)sec_off;
        if (n > chunk - done)
        {
            n = chunk - done;
        }

        if (!pfs_vhd_dyn_bitmap_test(p_bm, sector_in_block))
        {
            memset(buf + done, 0, (size_t)n);
        }
        else
        {
            int64_t host_off = pfs_vhd_dyn_sector_host_off(p_priv, bat_sector, sector_in_block);

            if (0 == sec_off && PFS_VHD_SECTOR_SIZE == n)
            {
                rc = pfs_blkio_raw_pread(&p_priv->host, buf + done, n, host_off);
                if (0 > rc)
                {
                    pfs_free(p_bm);
                    return rc;
                }
                else if (rc != n)
                {
                    pfs_free(p_bm);
                    return PFS_BLKIO_RET_IO;
                }
            }
            else
            {
                rc = pfs_blkio_raw_pread(&p_priv->host, sec_buf, PFS_VHD_SECTOR_SIZE, host_off);
                if (0 > rc)
                {
                    pfs_free(p_bm);
                    return rc;
                }
                else if (rc != PFS_VHD_SECTOR_SIZE)
                {
                    pfs_free(p_bm);
                    return PFS_BLKIO_RET_IO;
                }
                memcpy(buf + done, sec_buf + sec_off, (size_t)n);
            }
        }
        done += n;
    }

    pfs_free(p_bm);
    return chunk;
}

static int pfs_vhd_dyn_write_chunk(pfs_vhd_priv_t *p_priv, const uint8_t *buf, int size, int64_t offset)
{
    uint32_t block_idx = (uint32_t)((uint64_t)offset / (uint64_t)p_priv->block_size);
    uint32_t off_in_block = (uint32_t)((uint64_t)offset % (uint64_t)p_priv->block_size);
    int chunk = (int)(p_priv->block_size - off_in_block);
    if (chunk > size)
    {
        chunk = size;
    }

    uint32_t bat_sector = 0;
    int rc = pfs_vhd_bat_lookup(p_priv, block_idx, &bat_sector);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    if (PFS_VHD_BAT_ENTRY_UNUSED == bat_sector)
    {
        rc = pfs_vhd_dyn_alloc_block(p_priv, block_idx, &bat_sector);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
    }

    int bm_bytes = (int)p_priv->bitmap_sectors * PFS_VHD_SECTOR_SIZE;
    uint8_t *p_bm = (uint8_t *)pfs_malloc(bm_bytes);
    if (NULL == p_bm)
    {
        return PFS_BLKIO_RET_IO;
    }
    rc = pfs_vhd_dyn_load_bitmap(p_priv, bat_sector, p_bm);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_free(p_bm);
        return rc;
    }

    bool bm_dirty = false;
    int done = 0;
    uint8_t sec_buf[PFS_VHD_SECTOR_SIZE];
    while (done < chunk)
    {
        uint32_t sector_in_block = (off_in_block + (uint32_t)done) / (uint32_t)PFS_VHD_SECTOR_SIZE;
        uint32_t sec_off = (off_in_block + (uint32_t)done) % (uint32_t)PFS_VHD_SECTOR_SIZE;
        int n = PFS_VHD_SECTOR_SIZE - (int)sec_off;
        if (n > chunk - done)
        {
            n = chunk - done;
        }

        {
            int64_t host_off = pfs_vhd_dyn_sector_host_off(p_priv, bat_sector, sector_in_block);

            if (0 == sec_off && PFS_VHD_SECTOR_SIZE == n)
            {
                rc = pfs_blkio_raw_pwrite(&p_priv->host, buf + done, n, host_off);
                if (0 > rc)
                {
                    pfs_free(p_bm);
                    return rc;
                }
                else if (rc != n)
                {
                    pfs_free(p_bm);
                    return PFS_BLKIO_RET_IO;
                }
            }
            else
            {
                if (pfs_vhd_dyn_bitmap_test(p_bm, sector_in_block))
                {
                    rc = pfs_blkio_raw_pread(&p_priv->host, sec_buf, PFS_VHD_SECTOR_SIZE, host_off);
                    if (0 > rc)
                    {
                        pfs_free(p_bm);
                        return rc;
                    }
                    else if (rc != PFS_VHD_SECTOR_SIZE)
                    {
                        pfs_free(p_bm);
                        return PFS_BLKIO_RET_IO;
                    }
                }
                else
                {
                    memset(sec_buf, 0, sizeof(sec_buf));
                }
                memcpy(sec_buf + sec_off, buf + done, (size_t)n);
                rc = pfs_blkio_raw_pwrite(&p_priv->host, sec_buf, PFS_VHD_SECTOR_SIZE, host_off);
                if (0 > rc)
                {
                    pfs_free(p_bm);
                    return rc;
                }
                else if (rc != PFS_VHD_SECTOR_SIZE)
                {
                    pfs_free(p_bm);
                    return PFS_BLKIO_RET_IO;
                }
            }
        }

        if (!pfs_vhd_dyn_bitmap_test(p_bm, sector_in_block))
        {
            pfs_vhd_dyn_bitmap_set(p_bm, sector_in_block);
            bm_dirty = true;
        }
        done += n;
    }

    if (bm_dirty)
    {
        rc = pfs_vhd_dyn_store_bitmap(p_priv, bat_sector, p_bm);
        if (PFS_BLKIO_RET_OK != rc)
        {
            pfs_free(p_bm);
            return rc;
        }
    }

    pfs_free(p_bm);
    return chunk;
}

int pfs_vhd_dynamic_pread(pfs_vhd_priv_t *p_priv, void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == p_priv->block_size || NULL == p_priv->p_bat)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (!pfs_vhd_dyn_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }

    uint8_t *out = (uint8_t *)buf;
    int done = 0;
    while (done < size)
    {
        int rc = pfs_vhd_dyn_read_chunk(p_priv, out + done, size - done, offset + done);
        if (0 > rc)
        {
            return rc;
        }
        else if (0 == rc)
        {
            return PFS_BLKIO_RET_IO;
        }
        done += rc;
    }
    return size;
}

int pfs_vhd_dynamic_pwrite(pfs_vhd_priv_t *p_priv, const void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == p_priv->block_size || NULL == p_priv->p_bat)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (!pfs_vhd_dyn_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }

    const uint8_t *in = (const uint8_t *)buf;
    int done = 0;
    while (done < size)
    {
        int rc = pfs_vhd_dyn_write_chunk(p_priv, in + done, size - done, offset + done);
        if (0 > rc)
        {
            return rc;
        }
        else if (0 == rc)
        {
            return PFS_BLKIO_RET_IO;
        }
        done += rc;
    }
    return size;
}

// end
