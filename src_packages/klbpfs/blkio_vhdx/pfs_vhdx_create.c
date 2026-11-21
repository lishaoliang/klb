// Doc Encode : UTF-8 BOM, Unix(LF)
// 创建空 VHDX (Fixed 预分配 / Dynamic 瘦文件); log_guid=0; 逻辑扇区 512; 块 2MiB
#include "blkio_vhdx/pfs_vhdx.h"
#include "blkio_vhdx/pfs_vhdx_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#define PFS_VHDX_CREATE_BLOCK_SIZE  (2u * 1024u * 1024u)
#define PFS_VHDX_CREATE_LOG_SIZE    PFS_VHDX_MIB
#define PFS_VHDX_CREATE_LSS         512u
#define PFS_VHDX_CREATE_PSS         512u

// VirtualDiskID / Page83: beca12ab-b2e6-4523-93ef-c309e000c746
static const pfs_vhdx_guid_t g_vhdx_page83 = {
    0xBECA12ABu, 0xB2E6u, 0x4523u,
    {0x93, 0xEF, 0xC3, 0x09, 0xE0, 0x00, 0xC7, 0x46}
};

static uint64_t pfs_vhdx_round_up_mib(uint64_t v)
{
    return (v + (uint64_t)PFS_VHDX_MIB - 1ull) & ~((uint64_t)PFS_VHDX_MIB - 1ull);
}

static void pfs_vhdx_put_guid(uint8_t *p, const pfs_vhdx_guid_t *g)
{
    pfs_write_le32(p, g->data1);
    pfs_write_le16(p + 4, g->data2);
    pfs_write_le16(p + 6, g->data3);
    memcpy(p + 8, g->data4, 8);
}

static void pfs_vhdx_make_guid(pfs_vhdx_guid_t *g)
{
    uint32_t t = (uint32_t)time(NULL);

    memset(g, 0, sizeof(*g));
    g->data1 = t;
    g->data2 = 0x4F50u;
    g->data3 = 0x41F1u;
    g->data4[0] = 0x8Au;
    g->data4[1] = 0xB1u;
    g->data4[2] = (uint8_t)(t >> 24);
    g->data4[3] = (uint8_t)(t >> 16);
    g->data4[4] = (uint8_t)(t >> 8);
    g->data4[5] = (uint8_t)t;
    g->data4[6] = 0x70u;
    g->data4[7] = 0x73u; // "ps"
}

static int pfs_vhdx_ensure_parent_dir(const char *path)
{
#ifdef _WIN32
    if (NULL == path)
    {
        return -1;
    }

    char dir[MAX_PATH];
    size_t n = strlen(path);
    if (0 == n || n >= sizeof(dir))
    {
        return -1;
    }
    memcpy(dir, path, n + 1);
    char *slash = strrchr(dir, '\\');
    if (NULL == slash)
    {
        slash = strrchr(dir, '/');
    }
    if (NULL == slash)
    {
        return 0;
    }
    *slash = '\0';
    if ('\0' == dir[0])
    {
        return 0;
    }
    {
        char tmp[MAX_PATH];
        size_t len = strlen(dir);

        if (len >= sizeof(tmp))
        {
            return -1;
        }
        memcpy(tmp, dir, len + 1);
        for (size_t i = 0; i < len; i++)
        {
            if ('\\' == tmp[i] || '/' == tmp[i])
            {
                char save = tmp[i];
                if (0 == i)
                {
                    continue;
                }
                if (1 == i || (i >= 2 && ':' == tmp[i - 1]))
                {
                    continue;
                }
                tmp[i] = '\0';
                if (0 == CreateDirectoryA(tmp, NULL) &&
                        ERROR_ALREADY_EXISTS != GetLastError())
                {
                    return -1;
                }
                tmp[i] = save;
            }
        }
        if (0 == CreateDirectoryA(tmp, NULL) &&
                ERROR_ALREADY_EXISTS != GetLastError())
        {
            return -1;
        }
    }
    return 0;
#else
    if (NULL == path)
    {
        return -1;
    }

    char dir[512];
    size_t n = strlen(path);
    if (0 == n || n >= sizeof(dir))
    {
        return -1;
    }
    memcpy(dir, path, n + 1);
    for (size_t i = 1; i < n; i++)
    {
        if ('/' == dir[i])
        {
            dir[i] = '\0';
            if (0 != mkdir(dir, 0755) && EEXIST != errno)
            {
                return -1;
            }
            dir[i] = '/';
        }
    }
    return 0;
#endif
}

static int pfs_vhdx_host_prepare(const char *path, uint64_t file_bytes)
{
#ifdef _WIN32
    if (0 != pfs_vhdx_ensure_parent_dir(path))
    {
        return PFS_BLKIO_RET_IO;
    }

    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_BLKIO_RET_IO;
    }
    LARGE_INTEGER li;
    li.QuadPart = (LONGLONG)file_bytes;
    if (!SetFilePointerEx(h, li, NULL, FILE_BEGIN) || !SetEndOfFile(h))
    {
        CloseHandle(h);
        return PFS_BLKIO_RET_IO;
    }
    CloseHandle(h);
    return PFS_BLKIO_RET_OK;
#else
    if (0 != pfs_vhdx_ensure_parent_dir(path))
    {
        return PFS_BLKIO_RET_IO;
    }

    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (0 > fd)
    {
        return PFS_BLKIO_RET_IO;
    }
    if (0 != ftruncate(fd, (off_t)file_bytes))
    {
        close(fd);
        return PFS_BLKIO_RET_IO;
    }
    close(fd);
    return PFS_BLKIO_RET_OK;
#endif
}

static int pfs_vhdx_host_write(pfs_blkio_t *p_host, const void *buf, int size, int64_t off)
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

static void pfs_vhdx_fill_file_id(uint8_t *blk)
{
    // creator UTF-16LE "pfs"
    static const uint16_t creator[] = {'p', 'f', 's', 0};

    memset(blk, 0, PFS_VHDX_HEADER_BLOCK_SIZE);
    memcpy(blk, PFS_VHDX_FILE_SIG, PFS_VHDX_FILE_SIG_LEN);
    memcpy(blk + 8, creator, sizeof(creator));
}

static void pfs_vhdx_fill_header_blk(uint8_t *blk4k, uint64_t seq,
        const pfs_vhdx_guid_t *fw, const pfs_vhdx_guid_t *dw)
{
    memset(blk4k, 0, PFS_VHDX_HEADER_CRC_SIZE);
    pfs_write_le32(blk4k, PFS_VHDX_HEAD_SIG);
    pfs_write_le64(blk4k + 8, seq);
    pfs_vhdx_put_guid(blk4k + 16, fw);
    pfs_vhdx_put_guid(blk4k + 32, dw);
    // log_guid = 0
    pfs_write_le16(blk4k + 64, PFS_VHDX_LOG_VERSION_0);
    pfs_write_le16(blk4k + 66, PFS_VHDX_VERSION_1);
    pfs_write_le32(blk4k + 68, PFS_VHDX_CREATE_LOG_SIZE);
    pfs_write_le64(blk4k + 72, (uint64_t)PFS_VHDX_MIB); // log at 1MiB
    pfs_vhdx_crc_store(blk4k, PFS_VHDX_HEADER_CRC_SIZE, 4);
}

static uint32_t pfs_vhdx_calc_bat_entries(uint64_t size_bytes, uint32_t block_size, uint32_t lss)
{
    uint32_t data_blocks = (uint32_t)((size_bytes + (uint64_t)block_size - 1ull) / (uint64_t)block_size);
    uint64_t chunk_ratio = ((uint64_t)PFS_VHDX_MAX_SECTORS_PER_BLOCK * (uint64_t)lss) / (uint64_t)block_size;
    uint32_t chunk_bits = 0;
    uint64_t cr = chunk_ratio;
    while (cr > 1ull)
    {
        cr >>= 1;
        chunk_bits++;
    }
    return data_blocks + ((data_blocks - 1u) >> chunk_bits);
}

static int pfs_vhdx_create_common(const char *path, uint64_t size_bytes, int is_fixed)
{
    if (NULL == path || 0 == size_bytes || 0 != (size_bytes % (uint64_t)PFS_VHDX_CREATE_LSS))
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint32_t block_size = PFS_VHDX_CREATE_BLOCK_SIZE;
    uint32_t lss = PFS_VHDX_CREATE_LSS;
    uint32_t data_blocks = (uint32_t)((size_bytes + (uint64_t)block_size - 1ull) / (uint64_t)block_size);
    if (0 == data_blocks)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint64_t chunk_ratio = ((uint64_t)PFS_VHDX_MAX_SECTORS_PER_BLOCK * (uint64_t)lss) / (uint64_t)block_size;
    uint32_t chunk_bits = 0;
    {
        uint64_t cr = chunk_ratio;
        while (cr > 1ull)
        {
            cr >>= 1;
            chunk_bits++;
        }
    }
    uint32_t bat_entries = pfs_vhdx_calc_bat_entries(size_bytes, block_size, lss);

    // BAT @ 2MiB (header section 1MiB + log 1MiB)
    uint64_t bat_offset = (uint64_t)PFS_VHDX_MIB + (uint64_t)PFS_VHDX_CREATE_LOG_SIZE;
    uint64_t bat_length = pfs_vhdx_round_up_mib((uint64_t)bat_entries * 8ull);
    uint64_t meta_offset = pfs_vhdx_round_up_mib(bat_offset + bat_length);
    uint64_t data_offset = pfs_vhdx_round_up_mib(meta_offset + (uint64_t)PFS_VHDX_MIB);
    uint64_t file_bytes;
    if (is_fixed)
    {
        file_bytes = data_offset + size_bytes;
    }
    else
    {
        file_bytes = meta_offset + (uint64_t)PFS_VHDX_MIB;
    }
    if (file_bytes > (uint64_t)INT64_MAX)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    int rc = pfs_vhdx_host_prepare(path, file_bytes);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    pfs_blkio_t host;
    uint8_t *p_id = NULL;
    uint8_t *p_hdr = NULL;
    uint8_t *p_reg = NULL;
    uint8_t *p_meta_tbl = NULL;
    uint8_t *p_meta_items = NULL;
    uint8_t *p_bat = NULL;
    pfs_vhdx_guid_t fw;
    pfs_vhdx_guid_t dw;
    pfs_vhdx_guid_t page83;
    uint32_t params_bits;
    (void)pfs_blkio_init(&host);
    host.ops = &g_pfs_blkio_raw_ops;
    rc = pfs_blkio_raw_open(&host, path, PFS_BLKIO_O_RDWR, (int)lss);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    pfs_vhdx_make_guid(&fw);
    pfs_vhdx_make_guid(&dw);
    page83 = g_vhdx_page83;
    page83.data1 ^= (uint32_t)time(NULL);

    p_id = (uint8_t *)pfs_mallocz(PFS_VHDX_HEADER_BLOCK_SIZE);
    p_hdr = (uint8_t *)pfs_mallocz(PFS_VHDX_HEADER_CRC_SIZE);
    p_reg = (uint8_t *)pfs_mallocz(PFS_VHDX_HEADER_BLOCK_SIZE);
    p_meta_tbl = (uint8_t *)pfs_mallocz(PFS_VHDX_HEADER_BLOCK_SIZE);
    p_meta_items = (uint8_t *)pfs_mallocz(256);
    if (NULL == p_id || NULL == p_hdr || NULL == p_reg || NULL == p_meta_tbl || NULL == p_meta_items)
    {
        rc = PFS_BLKIO_RET_IO;
        goto out;
    }

    // (A) file id
    pfs_vhdx_fill_file_id(p_id);
    rc = pfs_vhdx_host_write(&host, p_id, (int)PFS_VHDX_HEADER_BLOCK_SIZE, 0);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }

    // (B)(C) headers
    pfs_vhdx_fill_header_blk(p_hdr, 1, &fw, &dw);
    rc = pfs_vhdx_host_write(&host, p_hdr, (int)PFS_VHDX_HEADER_CRC_SIZE,
            (int64_t)PFS_VHDX_HEADER1_OFFSET);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }
    pfs_vhdx_fill_header_blk(p_hdr, 2, &fw, &dw);
    rc = pfs_vhdx_host_write(&host, p_hdr, (int)PFS_VHDX_HEADER_CRC_SIZE,
            (int64_t)PFS_VHDX_HEADER2_OFFSET);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }

    // (D)(E) region tables: BAT + Metadata
    memset(p_reg, 0, PFS_VHDX_HEADER_BLOCK_SIZE);
    pfs_write_le32(p_reg, PFS_VHDX_REGI_SIG);
    pfs_write_le32(p_reg + 8, 2); // entry_count
    pfs_vhdx_put_guid(p_reg + 16, pfs_vhdx_guid_bat());
    pfs_write_le64(p_reg + 32, bat_offset);
    pfs_write_le32(p_reg + 40, (uint32_t)bat_length);
    pfs_write_le32(p_reg + 44, PFS_VHDX_REGION_REQUIRED);
    pfs_vhdx_put_guid(p_reg + 48, pfs_vhdx_guid_metadata());
    pfs_write_le64(p_reg + 64, meta_offset);
    pfs_write_le32(p_reg + 72, PFS_VHDX_MIB);
    pfs_write_le32(p_reg + 76, PFS_VHDX_REGION_REQUIRED);
    pfs_vhdx_crc_store(p_reg, PFS_VHDX_HEADER_BLOCK_SIZE, 4);
    rc = pfs_vhdx_host_write(&host, p_reg, (int)PFS_VHDX_HEADER_BLOCK_SIZE,
            (int64_t)PFS_VHDX_REGION1_OFFSET);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }
    rc = pfs_vhdx_host_write(&host, p_reg, (int)PFS_VHDX_HEADER_BLOCK_SIZE,
            (int64_t)PFS_VHDX_REGION2_OFFSET);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }

    // Metadata table + items
    params_bits = is_fixed ? PFS_VHDX_PARAMS_LEAVE_ALLOCATED : 0u;
    {
        uint32_t off = 0;
        uint8_t *it = p_meta_items;

        pfs_write_le32(it + off, block_size);
        pfs_write_le32(it + off + 4, params_bits);
        off += 8;
        pfs_write_le64(it + off, size_bytes);
        off += 8;
        pfs_vhdx_put_guid(it + off, &page83);
        off += 16;
        pfs_write_le32(it + off, lss);
        off += 4;
        pfs_write_le32(it + off, PFS_VHDX_CREATE_PSS);
        off += 4;

        memset(p_meta_tbl, 0, PFS_VHDX_HEADER_BLOCK_SIZE);
        pfs_write_le64(p_meta_tbl, PFS_VHDX_METADATA_SIG);
        pfs_write_le16(p_meta_tbl + 10, 5);

        // entries start at +32; item data at +64KiB in region
        {
            uint32_t eoff = 32;
            uint32_t ioff = PFS_VHDX_HEADER_BLOCK_SIZE;
            const pfs_vhdx_guid_t *ids[5];
            uint32_t lens[5];
            uint32_t flags[5];

            ids[0] = pfs_vhdx_guid_file_params();
            lens[0] = 8;
            flags[0] = PFS_VHDX_META_IS_REQUIRED;
            ids[1] = pfs_vhdx_guid_virtual_size();
            lens[1] = 8;
            flags[1] = PFS_VHDX_META_IS_REQUIRED | PFS_VHDX_META_IS_VIRTUAL_DISK;
            ids[2] = &g_vhdx_page83;
            lens[2] = 16;
            flags[2] = PFS_VHDX_META_IS_REQUIRED | PFS_VHDX_META_IS_VIRTUAL_DISK;
            ids[3] = pfs_vhdx_guid_logical_sector();
            lens[3] = 4;
            flags[3] = PFS_VHDX_META_IS_REQUIRED | PFS_VHDX_META_IS_VIRTUAL_DISK;
            ids[4] = pfs_vhdx_guid_physical_sector();
            lens[4] = 4;
            flags[4] = PFS_VHDX_META_IS_REQUIRED | PFS_VHDX_META_IS_VIRTUAL_DISK;

            for (uint32_t k = 0; k < 5u; k++)
            {
                pfs_vhdx_put_guid(p_meta_tbl + eoff, ids[k]);
                pfs_write_le32(p_meta_tbl + eoff + 16, ioff);
                pfs_write_le32(p_meta_tbl + eoff + 20, lens[k]);
                pfs_write_le32(p_meta_tbl + eoff + 24, flags[k]);
                eoff += 32;
                ioff += lens[k];
            }
            // 表项 2 用 page83 可变 GUID
            pfs_vhdx_put_guid(p_meta_tbl + 32 + 2 * 32, &page83);
        }

        rc = pfs_vhdx_host_write(&host, p_meta_tbl, (int)PFS_VHDX_HEADER_BLOCK_SIZE,
                (int64_t)meta_offset);
        if (PFS_BLKIO_RET_OK != rc)
        {
            goto out;
        }
        rc = pfs_vhdx_host_write(&host, p_meta_items, (int)off,
                (int64_t)(meta_offset + (uint64_t)PFS_VHDX_HEADER_BLOCK_SIZE));
        if (PFS_BLKIO_RET_OK != rc)
        {
            goto out;
        }
    }

    // BAT
    if (bat_length > (uint64_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        rc = PFS_BLKIO_RET_STATE;
        goto out;
    }
    p_bat = (uint8_t *)pfs_mallocz((size_t)bat_length);
    if (NULL == p_bat)
    {
        rc = PFS_BLKIO_RET_IO;
        goto out;
    }
    if (is_fixed)
    {
        for (uint32_t i = 0; i < data_blocks; i++)
        {
            uint32_t bat_idx = i + (i >> chunk_bits);
            uint64_t file_off = pfs_vhdx_round_up_mib(data_offset + (uint64_t)i * (uint64_t)block_size);
            uint64_t ent = (file_off & PFS_VHDX_BAT_FILE_OFF_MASK) | PFS_VHDX_PAYLOAD_FULLY_PRESENT;

            if (bat_idx >= bat_entries)
            {
                rc = PFS_BLKIO_RET_STATE;
                goto out;
            }
            pfs_write_le64(p_bat + bat_idx * 8u, ent);
        }
    }
    // Dynamic: BAT 全 0 = NOT_PRESENT, 读为零
    rc = pfs_vhdx_host_write(&host, p_bat, (int)bat_length, (int64_t)bat_offset);

out:
    pfs_free(p_id);
    pfs_free(p_hdr);
    pfs_free(p_reg);
    pfs_free(p_meta_tbl);
    pfs_free(p_meta_items);
    pfs_free(p_bat);
    (void)pfs_blkio_raw_close(&host);
    return rc;
}

int pfs_vhdx_create_fixed(const char *path, uint64_t size_bytes)
{
    return pfs_vhdx_create_common(path, size_bytes, 1);
}

int pfs_vhdx_create_dynamic(const char *path, uint64_t size_bytes)
{
    return pfs_vhdx_create_common(path, size_bytes, 0);
}

// end
