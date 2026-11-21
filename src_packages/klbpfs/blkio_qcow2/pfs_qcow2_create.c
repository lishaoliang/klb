// Doc Encode : UTF-8 BOM, Unix(LF)
// 创建空 qcow2 v3: header + reftable + 首个 refcount block + L1; 无数据簇
#include "blkio_qcow2/pfs_qcow2.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <stdint.h>
#include <string.h>

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

#define PFS_QCOW2_CREATE_CLUSTER_BITS   16u
#define PFS_QCOW2_CREATE_REFCOUNT_ORDER 4u

static int pfs_qcow2_ensure_parent_dir(const char *path)
{
#ifdef _WIN32
    if (NULL == path)
    {
        return -1;
    }
    size_t n = strlen(path);
    if (0 == n || n >= MAX_PATH)
    {
        return -1;
    }
    char dir[MAX_PATH];
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
    size_t n = strlen(path);
    if (0 == n || n >= 512)
    {
        return -1;
    }
    char dir[512];
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

static int pfs_qcow2_host_prepare(const char *path, uint64_t file_bytes)
{
#ifdef _WIN32
    if (0 != pfs_qcow2_ensure_parent_dir(path))
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
    if (0 != pfs_qcow2_ensure_parent_dir(path))
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

static int pfs_qcow2_host_write(pfs_blkio_t *p_host, const void *buf, int size, int64_t off)
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

static void pfs_qcow2_fill_header_v3(uint8_t *raw, uint64_t size_bytes, uint32_t l1_size,
        uint64_t l1_off, uint64_t reftable_off, uint32_t reftable_clusters)
{
    memset(raw, 0, PFS_QCOW2_HEADER_SIZE_V3 + 8);
    pfs_write_be32(raw + 0, PFS_QCOW2_MAGIC);
    pfs_write_be32(raw + 4, PFS_QCOW2_VERSION_3);
    pfs_write_be64(raw + 8, 0);                 // backing_file_offset
    pfs_write_be32(raw + 16, 0);                // backing_file_size
    pfs_write_be32(raw + 20, PFS_QCOW2_CREATE_CLUSTER_BITS);
    pfs_write_be64(raw + 24, size_bytes);
    pfs_write_be32(raw + 32, PFS_QCOW2_CRYPT_NONE);
    pfs_write_be32(raw + 36, l1_size);
    pfs_write_be64(raw + 40, l1_off);
    pfs_write_be64(raw + 48, reftable_off);
    pfs_write_be32(raw + 56, reftable_clusters);
    pfs_write_be32(raw + 60, 0);                // nb_snapshots
    pfs_write_be64(raw + 64, 0);                // snapshots_offset
    pfs_write_be64(raw + 72, 0);                // incompatible_features
    pfs_write_be64(raw + 80, 0);                // compatible_features
    pfs_write_be64(raw + 88, 0);                // autoclear_features
    pfs_write_be32(raw + 96, PFS_QCOW2_CREATE_REFCOUNT_ORDER);
    pfs_write_be32(raw + 100, PFS_QCOW2_HEADER_SIZE_V3);
    // header extension end
    pfs_write_be32(raw + 104, 0);
    pfs_write_be32(raw + 108, 0);
}

int pfs_qcow2_create(const char *path, uint64_t size_bytes)
{
    if (NULL == path || 0 == size_bytes ||
            0 != (size_bytes % (uint64_t)PFS_QCOW2_SECTOR_SIZE))
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint32_t cluster_bits = PFS_QCOW2_CREATE_CLUSTER_BITS;
    uint32_t cluster_size = 1u << cluster_bits;
    uint32_t l2_entries = cluster_size / 8u;
    uint32_t refcount_bits = 1u << PFS_QCOW2_CREATE_REFCOUNT_ORDER;
    uint32_t refcount_block_entries = (uint32_t)(((uint64_t)cluster_size * 8ull) /
            (uint64_t)refcount_bits);
    if (0 == l2_entries || 0 == refcount_block_entries)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint64_t guest_clusters = (size_bytes + (uint64_t)cluster_size - 1ull) /
            (uint64_t)cluster_size;
    if (0 == guest_clusters)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    uint64_t l1_size_u = (guest_clusters + (uint64_t)l2_entries - 1ull) / (uint64_t)l2_entries;
    if (0 == l1_size_u || l1_size_u > (uint64_t)UINT32_MAX)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    uint32_t l1_size = (uint32_t)l1_size_u;
    uint64_t l1_bytes = (uint64_t)l1_size * 8ull;
    if (l1_bytes > (uint64_t)PFS_QCOW2_L1_SIZE_MAX_BYTES)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    uint32_t l1_clusters = (uint32_t)((l1_bytes + (uint64_t)cluster_size - 1ull) /
            (uint64_t)cluster_size);
    if (0 == l1_clusters)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    // 布局: [0]=header [1]=reftable [2]=refcount block [3..]=L1
    uint32_t reftable_clusters = 1u;
    uint64_t meta_clusters = 3ull + (uint64_t)l1_clusters;
    if (meta_clusters > (uint64_t)refcount_block_entries)
    {
        // 首个 rb 盖不住元数据 (极罕见)
        return PFS_BLKIO_RET_STATE;
    }
    if (meta_clusters > (uint64_t)(UINT64_MAX / cluster_size))
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint64_t reftable_off = (uint64_t)cluster_size * 1ull;
    uint64_t rb_off = (uint64_t)cluster_size * 2ull;
    uint64_t l1_off = (uint64_t)cluster_size * 3ull;
    uint64_t file_bytes = meta_clusters * (uint64_t)cluster_size;

    int rc = pfs_qcow2_host_prepare(path, file_bytes);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    pfs_blkio_t host;
    (void)pfs_blkio_init(&host);
    host.ops = &g_pfs_blkio_raw_ops;
    rc = pfs_blkio_raw_open(&host, path, PFS_BLKIO_O_RDWR, PFS_QCOW2_SECTOR_SIZE);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    uint8_t hdr[PFS_QCOW2_HEADER_SIZE_V3 + 8];
    pfs_qcow2_fill_header_v3(hdr, size_bytes, l1_size, l1_off, reftable_off, reftable_clusters);

    uint8_t *p_reftable = (uint8_t *)pfs_mallocz(cluster_size);
    uint8_t *p_rb = (uint8_t *)pfs_mallocz(cluster_size);
    if (NULL == p_reftable || NULL == p_rb)
    {
        rc = PFS_BLKIO_RET_IO;
        goto out;
    }

    pfs_write_be64(p_reftable, rb_off);
    for (uint32_t i = 0; i < (uint32_t)meta_clusters; i++)
    {
        pfs_write_be16(p_rb + i * 2u, 1u);
    }

    rc = pfs_qcow2_host_write(&host, hdr, (int)sizeof(hdr), 0);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }
    rc = pfs_qcow2_host_write(&host, p_reftable, (int)cluster_size, (int64_t)reftable_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }
    rc = pfs_qcow2_host_write(&host, p_rb, (int)cluster_size, (int64_t)rb_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        goto out;
    }
    // L1 区已由 host_prepare 置 0, 无需再写

out:
    pfs_free(p_reftable);
    pfs_free(p_rb);
    (void)pfs_blkio_raw_close(&host);
    return rc;
}

// end
