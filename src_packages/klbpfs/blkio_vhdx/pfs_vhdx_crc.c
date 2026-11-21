// Doc Encode : UTF-8 BOM, Unix(LF)
// VHDX CRC-32C (Castagnoli poly 0x1EDC6F41)
#include "blkio_vhdx/pfs_vhdx_internal.h"

#include <string.h>

static uint32_t g_pfs_vhdx_crc32c_tab[256];
static int g_pfs_vhdx_crc32c_ready;

static void pfs_vhdx_crc32c_init(void)
{
    if (g_pfs_vhdx_crc32c_ready)
    {
        return;
    }
    for (uint32_t i = 0; i < 256u; i++)
    {
        uint32_t c = i;
        for (int k = 0; k < 8; k++)
        {
            if (c & 1u)
            {
                c = 0x82F63B78u ^ (c >> 1);
            }
            else
            {
                c = c >> 1;
            }
        }
        g_pfs_vhdx_crc32c_tab[i] = c;
    }
    g_pfs_vhdx_crc32c_ready = 1;
}

uint32_t pfs_vhdx_crc32c(uint32_t crc, const uint8_t *buf, size_t size)
{
    pfs_vhdx_crc32c_init();
    for (size_t i = 0; i < size; i++)
    {
        crc = g_pfs_vhdx_crc32c_tab[(crc ^ buf[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc;
}

int pfs_vhdx_crc32c_valid(uint8_t *buf, size_t size, int crc_offset)
{
    uint32_t orig;
    uint32_t calc;
    uint8_t saved[4];

    if (NULL == buf || size < (size_t)crc_offset + 4u || crc_offset < 0)
    {
        return 0;
    }

    memcpy(saved, buf + crc_offset, 4);
    orig = (uint32_t)saved[0] | ((uint32_t)saved[1] << 8) |
           ((uint32_t)saved[2] << 16) | ((uint32_t)saved[3] << 24);
    memset(buf + crc_offset, 0, 4);
    calc = pfs_vhdx_crc32c(0xFFFFFFFFu, buf, size);
    memcpy(buf + crc_offset, saved, 4);
    return (calc == orig) ? 1 : 0;
}

void pfs_vhdx_crc_store(uint8_t *buf, size_t size, int crc_off)
{
    uint32_t c;

    memset(buf + crc_off, 0, 4);
    c = pfs_vhdx_crc32c(0xFFFFFFFFu, buf, size);
    buf[crc_off] = (uint8_t)(c & 0xFFu);
    buf[crc_off + 1] = (uint8_t)((c >> 8) & 0xFFu);
    buf[crc_off + 2] = (uint8_t)((c >> 16) & 0xFFu);
    buf[crc_off + 3] = (uint8_t)((c >> 24) & 0xFFu);
}

// end
