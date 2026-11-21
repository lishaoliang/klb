// Doc Encode : UTF-8 BOM, Unix(LF)
// 创建空 VHD (Fixed 全尺寸宿主文件 / Dynamic 瘦文件; 均不使用 NTFS sparse)
#include "blkio_vhd/pfs_vhd.h"
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

#define PFS_VHD_CREATE_BLOCK_SIZE   (2u * 1024u * 1024u)
#define PFS_VHD_CREATE_APP          "pfs "
#define PFS_VHD_CREATE_OS           "Wi2k"

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

// Microsoft VHD 附录 CHS 算法
static void pfs_vhd_calc_geometry(uint64_t size_bytes, uint16_t *p_cyl, uint8_t *p_heads, uint8_t *p_spt)
{
    uint64_t total_sectors = size_bytes / (uint64_t)PFS_VHD_SECTOR_SIZE;
    uint32_t spt = 0;
    uint32_t heads = 0;
    uint32_t cyl_heads = 0;
    uint32_t cylinders;

    if (total_sectors > 65535ull * 16ull * 255ull)
    {
        total_sectors = 65535ull * 16ull * 255ull;
    }

    if (total_sectors >= 65535ull * 16ull * 63ull)
    {
        spt = 255;
        heads = 16;
        cyl_heads = (uint32_t)(total_sectors / spt);
    }
    else
    {
        spt = 17;
        cyl_heads = (uint32_t)(total_sectors / spt);
        heads = (cyl_heads + 1023u) / 1024u;
        if (heads < 4u)
        {
            heads = 4u;
        }
        if (cyl_heads >= (heads * 1024u) || heads > 16u)
        {
            spt = 31;
            heads = 16;
            cyl_heads = (uint32_t)(total_sectors / spt);
        }
        if (cyl_heads >= (heads * 1024u))
        {
            spt = 63;
            heads = 16;
            cyl_heads = (uint32_t)(total_sectors / spt);
        }
    }
    cylinders = cyl_heads / heads;
    *p_cyl = (uint16_t)cylinders;
    *p_heads = (uint8_t)heads;
    *p_spt = (uint8_t)spt;
}

static uint32_t pfs_vhd_timestamp_now(void)
{
    // 自 2000-01-01 00:00:00 UTC 起的秒数
    const time_t epoch2000 = 946684800;
    time_t now = time(NULL);

    if (now < epoch2000)
    {
        return 0;
    }
    return (uint32_t)(now - epoch2000);
}

static void pfs_vhd_fill_footer_raw(uint8_t *raw, uint64_t size_bytes, uint32_t disk_type, uint64_t data_offset)
{
    uint16_t cyl;
    uint8_t heads;
    uint8_t spt;

    memset(raw, 0, PFS_VHD_FOOTER_SIZE);
    memcpy(raw, PFS_VHD_COOKIE, PFS_VHD_COOKIE_LEN);
    pfs_write_be32(raw + 8, PFS_VHD_FEATURE_RESERVED);
    pfs_write_be32(raw + 12, PFS_VHD_VERSION_1_0);
    pfs_write_be64(raw + 16, data_offset);
    pfs_write_be32(raw + 24, pfs_vhd_timestamp_now());
    memcpy(raw + 28, PFS_VHD_CREATE_APP, 4);
    pfs_write_be32(raw + 32, 0x00010000u);
    memcpy(raw + 36, PFS_VHD_CREATE_OS, 4);
    pfs_write_be64(raw + 40, size_bytes);
    pfs_write_be64(raw + 48, size_bytes);
    pfs_vhd_calc_geometry(size_bytes, &cyl, &heads, &spt);
    pfs_write_be16(raw + 56, cyl);
    raw[58] = heads;
    raw[59] = spt;
    pfs_write_be32(raw + 60, disk_type);
    // unique_id: 简单用时间戳填充前 4 字节, 其余 0
    pfs_write_be32(raw + 68, pfs_vhd_timestamp_now());
    raw[84] = 0;
    uint32_t sum = pfs_vhd_checksum_skip(raw, PFS_VHD_FOOTER_SIZE, 64, 4);
    pfs_write_be32(raw + 64, sum);
}

static void pfs_vhd_fill_dyn_header_raw(uint8_t *raw, uint64_t table_offset,
        uint32_t max_entries, uint32_t block_size)
{
    memset(raw, 0, PFS_VHD_DYN_HEADER_SIZE);
    memcpy(raw, PFS_VHD_DYN_COOKIE, PFS_VHD_DYN_COOKIE_LEN);
    pfs_write_be64(raw + 8, 0xFFFFFFFFFFFFFFFFull);
    pfs_write_be64(raw + 16, table_offset);
    pfs_write_be32(raw + 24, PFS_VHD_VERSION_1_0);
    pfs_write_be32(raw + 28, max_entries);
    pfs_write_be32(raw + 32, block_size);
    uint32_t sum = pfs_vhd_checksum_skip(raw, PFS_VHD_DYN_HEADER_SIZE, 36, 4);
    pfs_write_be32(raw + 36, sum);
}

static int pfs_vhd_ensure_parent_dir(const char *path)
{
#ifdef _WIN32
    if (NULL == path)
    {
        return -1;
    }

    char dir[MAX_PATH];
    size_t n = strlen(path);
    char *slash;

    if (0 == n || n >= sizeof(dir))
    {
        return -1;
    }
    memcpy(dir, path, n + 1);
    slash = strrchr(dir, '\\');
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
    if (0 != CreateDirectoryA(dir, NULL))
    {
        return 0;
    }
    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        return 0;
    }
    // 尝试逐级创建 (仅一层失败时常见已存在父路径)
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
                // 跳过盘符后首斜杠 "D:"
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

// 创建/截断到 file_bytes; 不设 NTFS sparse (Win 附加 VHD 要求)
static int pfs_vhd_host_prepare(const char *path, uint64_t file_bytes)
{
#ifdef _WIN32
    if (0 != pfs_vhd_ensure_parent_dir(path))
    {
        return PFS_BLKIO_RET_IO;
    }

    HANDLE h;
    LARGE_INTEGER li;
    DWORD err;

    h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_BLKIO_RET_IO;
    }
    li.QuadPart = (LONGLONG)file_bytes;
    if (!SetFilePointerEx(h, li, NULL, FILE_BEGIN) || !SetEndOfFile(h))
    {
        err = GetLastError();
        CloseHandle(h);
        (void)err;
        return PFS_BLKIO_RET_IO;
    }
    CloseHandle(h);
    return PFS_BLKIO_RET_OK;
#else
    if (0 != pfs_vhd_ensure_parent_dir(path))
    {
        return PFS_BLKIO_RET_IO;
    }

    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0)
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

static int pfs_vhd_host_write(pfs_blkio_t *p_host, const void *buf, int size, int64_t off)
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

int pfs_vhd_create_fixed(const char *path, uint64_t size_bytes)
{
    if (NULL == path || 0 == size_bytes || 0 != (size_bytes % (uint64_t)PFS_VHD_SECTOR_SIZE))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (size_bytes > (uint64_t)INT64_MAX - (uint64_t)PFS_VHD_FOOTER_SIZE)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint64_t file_bytes = size_bytes + (uint64_t)PFS_VHD_FOOTER_SIZE;
    int rc = pfs_vhd_host_prepare(path, file_bytes);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    pfs_blkio_t host;
    uint8_t footer[PFS_VHD_FOOTER_SIZE];

    (void)pfs_blkio_init(&host);
    host.ops = &g_pfs_blkio_raw_ops;
    rc = pfs_blkio_raw_open(&host, path, PFS_BLKIO_O_RDWR, PFS_VHD_SECTOR_SIZE);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    pfs_vhd_fill_footer_raw(footer, size_bytes, PFS_VHD_DISK_TYPE_FIXED, 0xFFFFFFFFFFFFFFFFull);
    rc = pfs_vhd_host_write(&host, footer, PFS_VHD_FOOTER_SIZE, (int64_t)size_bytes);
    (void)pfs_blkio_raw_close(&host);
    return rc;
}

int pfs_vhd_create_dynamic(const char *path, uint64_t size_bytes)
{
    if (NULL == path || 0 == size_bytes || 0 != (size_bytes % (uint64_t)PFS_VHD_SECTOR_SIZE))
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint32_t block_size = PFS_VHD_CREATE_BLOCK_SIZE;
    uint64_t need_blocks = (size_bytes + (uint64_t)block_size - 1ull) / (uint64_t)block_size;
    if (0 == need_blocks || need_blocks > (uint64_t)UINT32_MAX)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint32_t max_entries = (uint32_t)need_blocks;
    uint32_t bat_bytes = max_entries * 4u;
    uint32_t bat_bytes_aligned = (bat_bytes + (uint32_t)PFS_VHD_SECTOR_SIZE - 1u) &
            ~((uint32_t)PFS_VHD_SECTOR_SIZE - 1u);
    uint64_t table_offset = (uint64_t)PFS_VHD_FOOTER_SIZE + (uint64_t)PFS_VHD_DYN_HEADER_SIZE;
    uint64_t file_bytes = table_offset + (uint64_t)bat_bytes_aligned + (uint64_t)PFS_VHD_FOOTER_SIZE;

    int rc = pfs_vhd_host_prepare(path, file_bytes);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    pfs_blkio_t host;
    uint8_t footer[PFS_VHD_FOOTER_SIZE];
    uint8_t dyn[PFS_VHD_DYN_HEADER_SIZE];
    uint8_t *p_bat = NULL;

    (void)pfs_blkio_init(&host);
    host.ops = &g_pfs_blkio_raw_ops;
    rc = pfs_blkio_raw_open(&host, path, PFS_BLKIO_O_RDWR, PFS_VHD_SECTOR_SIZE);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    pfs_vhd_fill_footer_raw(footer, size_bytes, PFS_VHD_DISK_TYPE_DYNAMIC,
            (uint64_t)PFS_VHD_FOOTER_SIZE);
    pfs_vhd_fill_dyn_header_raw(dyn, table_offset, max_entries, block_size);

    p_bat = (uint8_t *)pfs_mallocz(bat_bytes_aligned);
    if (NULL == p_bat)
    {
        (void)pfs_blkio_raw_close(&host);
        return PFS_BLKIO_RET_IO;
    }
    for (uint32_t i = 0; i < max_entries; i++)
    {
        pfs_write_be32(p_bat + i * 4u, PFS_VHD_BAT_ENTRY_UNUSED);
    }

    rc = pfs_vhd_host_write(&host, footer, PFS_VHD_FOOTER_SIZE, 0);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }
    rc = pfs_vhd_host_write(&host, dyn, PFS_VHD_DYN_HEADER_SIZE, (int64_t)PFS_VHD_FOOTER_SIZE);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }
    rc = pfs_vhd_host_write(&host, p_bat, (int)bat_bytes_aligned, (int64_t)table_offset);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }
    rc = pfs_vhd_host_write(&host, footer, PFS_VHD_FOOTER_SIZE,
            (int64_t)(file_bytes - (uint64_t)PFS_VHD_FOOTER_SIZE));

out:
    pfs_free(p_bat);
    (void)pfs_blkio_raw_close(&host);
    return rc;
}

// end
