// Doc Encode : UTF-8 BOM, Unix(LF)
// raw 后端: 落盘 base/; 符号 pfs_blkio_raw_*; facade 经 g_pfs_blkio_raw_ops 分派
#ifndef _WIN32
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
    #ifdef _FILE_OFFSET_BITS
        #undef _FILE_OFFSET_BITS
    #endif
    #define _FILE_OFFSET_BITS 64
    #ifdef _LARGEFILE_SOURCE
        #undef _LARGEFILE_SOURCE
    #endif
    #define _LARGEFILE_SOURCE
    #ifdef _LARGEFILE64_SOURCE
        #undef _LARGEFILE64_SOURCE
    #endif
    #define _LARGEFILE64_SOURCE
#endif

#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0601
    #endif
    #define WIN32_LEAN_AND_MEAN
#endif // _WIN32

#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define PFS_RAW_PRIV(p) ((pfs_blkio_raw_priv_t *)((p)->priv))

static pfs_blkio_raw_priv_t *pfs_blkio_raw_priv_or_null(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->priv)
    {
        return NULL;
    }
    return PFS_RAW_PRIV(p_blkio);
}


#define PFS_BLKIO_STATUS_MASK ( PFS_BLKIO_NOT_OPEN | PFS_BLKIO_CLOSED | PFS_BLKIO_ERR | \
                                PFS_BLKIO_ERR_READ | PFS_BLKIO_ERR_WRITE | PFS_BLKIO_ERR_FSYNC)

#define PFS_BLKIO_IO_ERR_MASK (PFS_BLKIO_ERR | PFS_BLKIO_ERR_READ | PFS_BLKIO_ERR_WRITE | PFS_BLKIO_ERR_FSYNC)


static void pfs_blkio_raw_read_fail(pfs_blkio_t *p_blkio, int err)
{
    // 仅传输失败路径: latch READ err; buf 可能已部分写入; 参数/前置检查失败不调用本函数
    p_blkio->status |= PFS_BLKIO_ERR_READ;
    p_blkio->last_error = err;
}

static void pfs_blkio_raw_write_fail(pfs_blkio_t *p_blkio, int err)
{
    // 仅传输失败路径: latch WRITE err
    p_blkio->status |= PFS_BLKIO_ERR_WRITE;
    p_blkio->last_error = err;
}

static bool pfs_blkio_raw_open_flags_valid(pfs_blkio_oflags_e flags)
{
    return (PFS_BLKIO_O_RDONLY == flags || PFS_BLKIO_O_WRONLY == flags || PFS_BLKIO_O_RDWR == flags);
}

static bool pfs_blkio_raw_io_size_valid(int size)
{
    return (size > 0 && size <= PFS_BLKIO_IO_SIZE_MAX);
}

// offset>=0 且 [offset, offset+size) 落在 int64 可寻址范围; 用 uint64 比较避免有符号溢出
static bool pfs_blkio_raw_io_range_valid(int64_t offset, int size)
{
    if (offset < 0 || !pfs_blkio_raw_io_size_valid(size))
    {
        return false;
    }
    if ((uint64_t)size > (uint64_t)INT64_MAX - (uint64_t)offset + 1ULL)
    {
        return false;
    }
    return true;
}

static bool pfs_blkio_raw_align_size_valid(int align_size)
{
    return (PFS_BLKIO_ALIGN_512 == align_size || PFS_BLKIO_ALIGN_1K == align_size ||
            PFS_BLKIO_ALIGN_2K == align_size || PFS_BLKIO_ALIGN_4K == align_size);
}

// 探测值映射为 open_direct 允许的 512/1024/2048/4096; 其余 -1
static int pfs_blkio_raw_sector_size_normalize(int sector_size)
{
    if (PFS_BLKIO_ALIGN_512 == sector_size || PFS_BLKIO_ALIGN_1K == sector_size ||
            PFS_BLKIO_ALIGN_2K == sector_size || PFS_BLKIO_ALIGN_4K == sector_size)
    {
        return sector_size;
    }
    return -1;
}

// open_direct: offset/size/buf 地址须 align_size 整数倍; buffered(align_size==0) 跳过
static bool pfs_blkio_raw_io_direct_valid(const pfs_blkio_t *p_blkio, const void *buf, int64_t offset, int size)
{
    if (NULL == p_blkio || p_blkio->align_size <= 0)
    {
        return true;
    }

    int align = p_blkio->align_size;
    if (0 != (offset % (int64_t)align))
    {
        return false;
    }
    if (0 != (size % align))
    {
        return false;
    }

    uintptr_t addr = (uintptr_t)buf;
    if (0 != (addr % (uintptr_t)align))
    {
        return false;
    }

    return true;
}

static bool pfs_blkio_raw_priv_ready(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->priv)
    {
        return false;
    }
    if (p_blkio->status & (PFS_BLKIO_NOT_OPEN | PFS_BLKIO_CLOSED | PFS_BLKIO_IO_ERR_MASK))
    {
        return false;
    }
    pfs_blkio_raw_priv_t *p_priv = PFS_RAW_PRIV(p_blkio);
#ifdef _WIN32
    return (INVALID_HANDLE_VALUE != p_priv->h_device);
#else
    return (p_priv->fd >= 0);
#endif
}

#ifdef _WIN32

#include <winioctl.h>

static DWORD pfs_blkio_raw_win_access(pfs_blkio_oflags_e flags)
{
    switch (flags)
    {
    case PFS_BLKIO_O_RDWR:
        return GENERIC_READ | GENERIC_WRITE;
    case PFS_BLKIO_O_WRONLY:
        // 缓冲未对齐 pwrite 需 RMW 预读, 须带 GENERIC_READ (API 仍为只写语义)
        return GENERIC_READ | GENERIC_WRITE;
    default:
        return GENERIC_READ;
    }
}

static bool pfs_blkio_raw_io_ready(const pfs_blkio_t *p_blkio)
{
    return pfs_blkio_raw_priv_ready(p_blkio);
}

// 释放 Win 缓冲 open 的 head/tail 扇区缓存
static void pfs_blkio_raw_win_sector_cache_free(pfs_blkio_t *p_blkio)
{
    pfs_blkio_raw_priv_t *p_priv = pfs_blkio_raw_priv_or_null(p_blkio);

    if (NULL == p_priv)
    {
        return;
    }
    if (NULL != p_priv->p_buf)
    {
        pfs_aligned_free(p_priv->p_buf);
    }
    p_priv->p_buf = NULL;
    p_priv->p_head_sector = NULL;
    p_priv->p_tail_sector = NULL;
    p_priv->buf_len = 0;
    p_priv->sector_size = 0;
}

// 分配 2 块扇区缓冲, 每块至少 4K, 整体 4K 对齐; 成功 0, 失败 -1
static int pfs_blkio_raw_win_sector_cache_alloc(pfs_blkio_t *p_blkio, int sector_size)
{
    int block_len = sector_size;

    // 每块最小 4K (逻辑扇区可能为 512/1K/2K)
    if (block_len < PFS_BLKIO_ALIGN_4K)
    {
        block_len = PFS_BLKIO_ALIGN_4K;
    }
    int buf_len = block_len * 2;

    uint8_t *p_buf = (uint8_t *)pfs_aligned_malloc((size_t)PFS_BLKIO_ALIGN_4K, (size_t)buf_len);
    if (NULL == p_buf)
    {
        return -1;
    }

    pfs_blkio_raw_priv_t *p_priv = PFS_RAW_PRIV(p_blkio);

    p_priv->p_buf = p_buf;
    p_priv->buf_len = buf_len;
    p_priv->p_head_sector = p_buf;
    p_priv->p_tail_sector = p_buf + block_len;
    p_priv->sector_size = sector_size;
    return 0;
}

// 底层 ReadFile: offset/size 须已按逻辑扇区整齐; 成功返回 size
// for_write_rmw: 非 0 表示 pwrite RMW 预读, 失败 latch WRITE err (非 READ)
static int pfs_blkio_raw_win_xfer_read(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset, int for_write_rmw)
{
    int done = 0;

    while (done < size)
    {
        DWORD req = (DWORD)(size - done);
        LARGE_INTEGER pos;
        pos.QuadPart = offset + (int64_t)done;
        DWORD chunk = 0;

        if (!SetFilePointerEx(PFS_RAW_PRIV(p_blkio)->h_device, pos, NULL, FILE_BEGIN))
        {
            if (for_write_rmw)
            {
                pfs_blkio_raw_write_fail(p_blkio, (int)GetLastError());
            }
            else
            {
                pfs_blkio_raw_read_fail(p_blkio, (int)GetLastError());
            }
            return PFS_BLKIO_RET_IO;
        }

        if (!ReadFile(PFS_RAW_PRIV(p_blkio)->h_device, (uint8_t *)buf + done, req, &chunk, NULL))
        {
            DWORD win_err = GetLastError();

            if (ERROR_HANDLE_EOF == win_err)
            {
                break;
            }
            if (for_write_rmw)
            {
                pfs_blkio_raw_write_fail(p_blkio, (int)win_err);
            }
            else
            {
                pfs_blkio_raw_read_fail(p_blkio, (int)win_err);
            }
            return PFS_BLKIO_RET_IO;
        }
        if (0 == chunk)
        {
            break;
        }

        done += (int)chunk;
    }

    if (done < size)
    {
        if (for_write_rmw)
        {
            pfs_blkio_raw_write_fail(p_blkio, (int)ERROR_HANDLE_EOF);
        }
        else
        {
            pfs_blkio_raw_read_fail(p_blkio, (int)ERROR_HANDLE_EOF);
        }
        return PFS_BLKIO_RET_IO;
    }

    return size;
}

// 底层 WriteFile: offset/size 须已按逻辑扇区整齐; 成功返回 size
static int pfs_blkio_raw_win_xfer_write(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    int done = 0;

    while (done < size)
    {
        DWORD req = (DWORD)(size - done);
        LARGE_INTEGER pos;
        pos.QuadPart = offset + (int64_t)done;
        DWORD chunk = 0;

        if (!SetFilePointerEx(PFS_RAW_PRIV(p_blkio)->h_device, pos, NULL, FILE_BEGIN))
        {
            pfs_blkio_raw_write_fail(p_blkio, (int)GetLastError());
            return PFS_BLKIO_RET_IO;
        }

        if (!WriteFile(PFS_RAW_PRIV(p_blkio)->h_device, (const uint8_t *)buf + done, req, &chunk, NULL))
        {
            pfs_blkio_raw_write_fail(p_blkio, (int)GetLastError());
            return PFS_BLKIO_RET_IO;
        }
        if (0 == chunk)
        {
            break;
        }

        done += (int)chunk;
    }

    if (done < size)
    {
        pfs_blkio_raw_write_fail(p_blkio, (int)ERROR_WRITE_FAULT);
        return PFS_BLKIO_RET_IO;
    }

    return size;
}

// Win 缓冲模式: 按逻辑扇区整齐读, 未对齐用 head/tail
static int pfs_blkio_raw_win_pread_buffered(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset)
{
    if (NULL == PFS_RAW_PRIV(p_blkio)->p_head_sector || NULL == PFS_RAW_PRIV(p_blkio)->p_tail_sector)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int ss = PFS_RAW_PRIV(p_blkio)->sector_size;
    if (ss <= 0)
    {
        return PFS_BLKIO_RET_STATE;
    }

    // 已整齐则直通
    if (0 == (offset % (int64_t)ss) && 0 == (size % ss))
    {
        return pfs_blkio_raw_win_xfer_read(p_blkio, buf, size, offset, 0);
    }

    uint8_t *out = (uint8_t *)buf;
    int done = 0;

    // 首扇区 partial
    if (0 != (offset % (int64_t)ss))
    {
        int64_t head_off = offset - (offset % (int64_t)ss);
        int skip = (int)(offset - head_off);
        int n = ss - skip;

        if (n > size)
        {
            n = size;
        }
        int rc = pfs_blkio_raw_win_xfer_read(p_blkio, PFS_RAW_PRIV(p_blkio)->p_head_sector, ss, head_off, 0);
        if (rc < 0)
        {
            return rc;
        }
        memcpy(out, PFS_RAW_PRIV(p_blkio)->p_head_sector + skip, (size_t)n);
        done = n;
    }

    // 中间整扇区
    if (done < size)
    {
        int64_t cur = offset + (int64_t)done;
        int64_t end = offset + (int64_t)size;
        int64_t mid_end = end - (end % (int64_t)ss);

        if (mid_end > cur)
        {
            int mid_n = (int)(mid_end - cur);

            int rc = pfs_blkio_raw_win_xfer_read(p_blkio, out + done, mid_n, cur, 0);
            if (rc < 0)
            {
                return rc;
            }
            done += mid_n;
        }
    }

    // 尾扇区 partial
    if (done < size)
    {
        int64_t end = offset + (int64_t)size;
        int64_t tail_off = end - (end % (int64_t)ss);
        int n = size - done;

        int rc = pfs_blkio_raw_win_xfer_read(p_blkio, PFS_RAW_PRIV(p_blkio)->p_tail_sector, ss, tail_off, 0);
        if (rc < 0)
        {
            return rc;
        }
        memcpy(out + done, PFS_RAW_PRIV(p_blkio)->p_tail_sector, (size_t)n);
    }

    return size;
}

// Win 缓冲模式: 按逻辑扇区整齐写, 未对齐 head/tail 先读改写
static int pfs_blkio_raw_win_pwrite_buffered(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    if (NULL == PFS_RAW_PRIV(p_blkio)->p_head_sector || NULL == PFS_RAW_PRIV(p_blkio)->p_tail_sector)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int ss = PFS_RAW_PRIV(p_blkio)->sector_size;
    if (ss <= 0)
    {
        return PFS_BLKIO_RET_STATE;
    }

    // 已整齐则直通
    if (0 == (offset % (int64_t)ss) && 0 == (size % ss))
    {
        return pfs_blkio_raw_win_xfer_write(p_blkio, buf, size, offset);
    }

    const uint8_t *in = (const uint8_t *)buf;
    int done = 0;

    // 首扇区 RMW
    if (0 != (offset % (int64_t)ss))
    {
        int64_t head_off = offset - (offset % (int64_t)ss);
        int skip = (int)(offset - head_off);
        int n = ss - skip;

        if (n > size)
        {
            n = size;
        }
        int rc = pfs_blkio_raw_win_xfer_read(p_blkio, PFS_RAW_PRIV(p_blkio)->p_head_sector, ss, head_off, 1);
        if (rc < 0)
        {
            return rc;
        }
        memcpy(PFS_RAW_PRIV(p_blkio)->p_head_sector + skip, in, (size_t)n);
        rc = pfs_blkio_raw_win_xfer_write(p_blkio, PFS_RAW_PRIV(p_blkio)->p_head_sector, ss, head_off);
        if (rc < 0)
        {
            return rc;
        }
        done = n;
    }

    // 中间整扇区
    if (done < size)
    {
        int64_t cur = offset + (int64_t)done;
        int64_t end = offset + (int64_t)size;
        int64_t mid_end = end - (end % (int64_t)ss);

        if (mid_end > cur)
        {
            int mid_n = (int)(mid_end - cur);

            int rc = pfs_blkio_raw_win_xfer_write(p_blkio, in + done, mid_n, cur);
            if (rc < 0)
            {
                return rc;
            }
            done += mid_n;
        }
    }

    // 尾扇区 RMW
    if (done < size)
    {
        int64_t end = offset + (int64_t)size;
        int64_t tail_off = end - (end % (int64_t)ss);
        int n = size - done;

        int rc = pfs_blkio_raw_win_xfer_read(p_blkio, PFS_RAW_PRIV(p_blkio)->p_tail_sector, ss, tail_off, 1);
        if (rc < 0)
        {
            return rc;
        }
        memcpy(PFS_RAW_PRIV(p_blkio)->p_tail_sector, in + done, (size_t)n);
        rc = pfs_blkio_raw_win_xfer_write(p_blkio, PFS_RAW_PRIV(p_blkio)->p_tail_sector, ss, tail_off);
        if (rc < 0)
        {
            return rc;
        }
    }

    return size;
}

int pfs_blkio_raw_open(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int sector_size)
{
    if (NULL == p_blkio || NULL == p_devpath || !pfs_blkio_raw_open_flags_valid(flags))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_blkio_raw_align_size_valid(sector_size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_blkio_raw_priv_t *p_priv = (pfs_blkio_raw_priv_t *)pfs_mallocz(sizeof(*p_priv));
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_NOT_OPEN | PFS_BLKIO_ERR;
        p_blkio->last_error = (int)ERROR_NOT_ENOUGH_MEMORY;
        return PFS_BLKIO_RET_IO;
    }
    p_priv->h_device = INVALID_HANDLE_VALUE;
    p_blkio->priv = p_priv;

    HANDLE h = CreateFileA(
            p_devpath,
            pfs_blkio_raw_win_access(flags),
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (INVALID_HANDLE_VALUE == h)
    {
        pfs_free(p_priv);
        p_blkio->priv = NULL;
        p_blkio->status = PFS_BLKIO_NOT_OPEN | PFS_BLKIO_ERR;
        p_blkio->last_error = (int)GetLastError();
        return PFS_BLKIO_RET_IO;
    }

    if (0 != pfs_blkio_raw_win_sector_cache_alloc(p_blkio, sector_size))
    {
        CloseHandle(h);
        pfs_free(p_priv);
        p_blkio->priv = NULL;
        p_blkio->status = PFS_BLKIO_NOT_OPEN | PFS_BLKIO_ERR;
        p_blkio->last_error = (int)ERROR_NOT_ENOUGH_MEMORY;
        return PFS_BLKIO_RET_IO;
    }

    p_priv->h_device = h;
    p_blkio->status = 0;
    p_blkio->last_error = 0;
    p_blkio->align_size = 0;
    return PFS_BLKIO_RET_OK;
}

int pfs_blkio_raw_open_direct(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int align_size)
{
    if (NULL == p_blkio || NULL == p_devpath || !pfs_blkio_raw_open_flags_valid(flags))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_blkio_raw_align_size_valid(align_size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_blkio_raw_priv_t *p_priv = (pfs_blkio_raw_priv_t *)pfs_mallocz(sizeof(*p_priv));
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_NOT_OPEN | PFS_BLKIO_ERR;
        p_blkio->last_error = (int)ERROR_NOT_ENOUGH_MEMORY;
        return PFS_BLKIO_RET_IO;
    }
    p_priv->h_device = INVALID_HANDLE_VALUE;
    p_blkio->priv = p_priv;

    HANDLE h = CreateFileA(
            p_devpath,
            pfs_blkio_raw_win_access(flags),
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_NO_BUFFERING,
            NULL);

    if (INVALID_HANDLE_VALUE == h)
    {
        pfs_free(p_priv);
        p_blkio->priv = NULL;
        p_blkio->status = PFS_BLKIO_NOT_OPEN | PFS_BLKIO_ERR;
        p_blkio->last_error = (int)GetLastError();
        return PFS_BLKIO_RET_IO;
    }

    p_priv->h_device = h;
    p_blkio->status = 0;
    p_blkio->last_error = 0;
    p_blkio->align_size = align_size;
    return PFS_BLKIO_RET_OK;
}

int pfs_blkio_raw_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }
    if (!pfs_blkio_raw_io_range_valid(offset, size) ||
            !pfs_blkio_raw_io_direct_valid(p_blkio, buf, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_blkio_raw_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    // open_direct: 调用方已对齐; 缓冲 open: 内部扇区整齐
    if (p_blkio->align_size > 0)
    {
        return pfs_blkio_raw_win_xfer_read(p_blkio, buf, size, offset, 0);
    }
    return pfs_blkio_raw_win_pread_buffered(p_blkio, buf, size, offset);
}

int pfs_blkio_raw_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }
    if (!pfs_blkio_raw_io_range_valid(offset, size) ||
            !pfs_blkio_raw_io_direct_valid(p_blkio, buf, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_blkio_raw_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (p_blkio->align_size > 0)
    {
        return pfs_blkio_raw_win_xfer_write(p_blkio, buf, size, offset);
    }
    return pfs_blkio_raw_win_pwrite_buffered(p_blkio, buf, size, offset);
}

int pfs_blkio_raw_fsync(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_blkio_raw_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (!FlushFileBuffers(PFS_RAW_PRIV(p_blkio)->h_device))
    {
        DWORD err = GetLastError();
        // 卷句柄在 FSCTL_DISMOUNT_VOLUME 后 FlushFileBuffers 常返回 ERROR_NOT_READY(21);
        // 此前写已成功时视为同步完成, 不 latch, 不向上报 IO 失败; 上层只判 sync_rc 即可
        if (ERROR_NOT_READY == err)
        {
            return PFS_BLKIO_RET_OK;
        }
        p_blkio->status |= PFS_BLKIO_ERR_FSYNC;
        p_blkio->last_error = (int)err;
        return PFS_BLKIO_RET_IO;
    }

    return PFS_BLKIO_RET_OK;
}

int pfs_blkio_raw_close(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    pfs_blkio_raw_priv_t *p_priv = pfs_blkio_raw_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_CLOSED;
        p_blkio->align_size = 0;
        return PFS_BLKIO_RET_OK;
    }

    if (INVALID_HANDLE_VALUE != p_priv->h_device)
    {
        if (!CloseHandle(p_priv->h_device))
        {
            // 句柄仍有效, 保留 h_device 供 retry close; 不置 CLOSED, 不释 priv
            p_blkio->status |= PFS_BLKIO_ERR;
            p_blkio->last_error = (int)GetLastError();
            return PFS_BLKIO_RET_IO;
        }
        p_priv->h_device = INVALID_HANDLE_VALUE;
    }

    pfs_blkio_raw_win_sector_cache_free(p_blkio);
    pfs_free(p_priv);
    p_blkio->priv = NULL;
    p_blkio->status = PFS_BLKIO_CLOSED;
    p_blkio->align_size = 0;
    return PFS_BLKIO_RET_OK;
}

static int pfs_blkio_raw_sector_size_probe(const char *p_devpath)
{
    HANDLE h = CreateFileA(
            p_devpath,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    DISK_GEOMETRY geo;
    DWORD geo_bytes = 0;

    if (0 != DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geo, sizeof(geo), &geo_bytes, NULL))
    {
        int sector_size = pfs_blkio_raw_sector_size_normalize((int)geo.BytesPerSector);
        CloseHandle(h);
        if (sector_size < 0)
        {
            return PFS_BLKIO_RET_PROBE;
        }
        return sector_size;
    }

    uint8_t geo_ex_buf[256];
    DISK_GEOMETRY_EX *p_geo_ex = (DISK_GEOMETRY_EX *)geo_ex_buf;
    if (0 != DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, p_geo_ex, (DWORD)sizeof(geo_ex_buf),
            &geo_bytes, NULL))
    {
        int sector_size = pfs_blkio_raw_sector_size_normalize((int)p_geo_ex->Geometry.BytesPerSector);
        CloseHandle(h);
        if (sector_size < 0)
        {
            return PFS_BLKIO_RET_PROBE;
        }
        return sector_size;
    }

    CloseHandle(h);
    return PFS_BLKIO_RET_PROBE;
}

static int pfs_blkio_raw_sector_size_physical_probe(const char *p_devpath)
{
    HANDLE h = CreateFileA(
            p_devpath,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    STORAGE_PROPERTY_QUERY query;
    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR align_desc;
    DWORD align_bytes = 0;

    memset(&query, 0, sizeof(query));
    query.PropertyId = StorageAccessAlignmentProperty;
    query.QueryType = PropertyStandardQuery;

    if (0 != DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &align_desc,
            sizeof(align_desc), &align_bytes, NULL) &&
            align_bytes >= (DWORD)sizeof(align_desc) && align_desc.BytesPerPhysicalSector > 0)
    {
        int sector_size = pfs_blkio_raw_sector_size_normalize((int)align_desc.BytesPerPhysicalSector);
        CloseHandle(h);
        if (sector_size >= 0)
        {
            return sector_size;
        }
    }
    else
    {
        CloseHandle(h);
    }

    // 物理 ioctl 不可用: fallback 到逻辑扇区
    return pfs_blkio_raw_sector_size_probe(p_devpath);
}

#else

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

// Linux syscall: EINTR 时重试, 其余 errno 立即返回
static int pfs_blkio_raw_linux_open(const char *path, int flags)
{
    int fd;

    do {
        fd = open(path, flags);
    } while (fd < 0 && EINTR == errno);
    return fd;
}

static ssize_t pfs_blkio_raw_linux_pread64(int fd, void *buf, size_t count, off64_t offset)
{
    ssize_t n;

    do {
        n = pread64(fd, buf, count, offset);
    } while (n < 0 && EINTR == errno);
    return n;
}

static ssize_t pfs_blkio_raw_linux_pwrite64(int fd, const void *buf, size_t count, off64_t offset)
{
    ssize_t n;

    do {
        n = pwrite64(fd, buf, count, offset);
    } while (n < 0 && EINTR == errno);
    return n;
}

static int pfs_blkio_raw_linux_fsync(int fd)
{
    int rc;

    do {
        rc = fsync(fd);
    } while (rc < 0 && EINTR == errno);
    return rc;
}

static int pfs_blkio_raw_linux_close(int fd)
{
    int rc;

    do {
        rc = close(fd);
    } while (rc < 0 && EINTR == errno);
    return rc;
}

static int pfs_blkio_raw_off64_from_i64(int64_t off, off64_t *p_pos)
{
    if (off < 0)
    {
        return -1;
    }

    off64_t pos = (off64_t)off;
    if ((int64_t)pos != off)
    {
        return -1;
    }

    if (NULL != p_pos)
    {
        *p_pos = pos;
    }
    return 0;
}

static bool pfs_blkio_raw_io_ready(const pfs_blkio_t *p_blkio)
{
    return pfs_blkio_raw_priv_ready(p_blkio);
}

int pfs_blkio_raw_open(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int sector_size)
{
    if (NULL == p_blkio || NULL == p_devpath || !pfs_blkio_raw_open_flags_valid(flags))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    // 与 Win 同参校验; Linux 缓冲 open 不强制扇区整齐, 忽略 sector_size
    if (!pfs_blkio_raw_align_size_valid(sector_size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_blkio_raw_priv_t *p_priv = (pfs_blkio_raw_priv_t *)pfs_mallocz(sizeof(*p_priv));
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_NOT_OPEN | PFS_BLKIO_ERR;
        p_blkio->last_error = ENOMEM;
        return PFS_BLKIO_RET_IO;
    }
    p_priv->fd = -1;
    p_blkio->priv = p_priv;

    int fd = pfs_blkio_raw_linux_open(p_devpath, (int)flags | O_LARGEFILE);
    if (fd < 0)
    {
        pfs_free(p_priv);
        p_blkio->priv = NULL;
        p_blkio->status = PFS_BLKIO_NOT_OPEN | PFS_BLKIO_ERR;
        p_blkio->last_error = errno;
        return PFS_BLKIO_RET_IO;
    }

    p_priv->fd = fd;
    p_blkio->status = 0;
    p_blkio->last_error = 0;
    p_blkio->align_size = 0;
    return PFS_BLKIO_RET_OK;
}

int pfs_blkio_raw_open_direct(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int align_size)
{
    if (NULL == p_blkio || NULL == p_devpath || !pfs_blkio_raw_open_flags_valid(flags))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_blkio_raw_align_size_valid(align_size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_blkio_raw_priv_t *p_priv = (pfs_blkio_raw_priv_t *)pfs_mallocz(sizeof(*p_priv));
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_NOT_OPEN | PFS_BLKIO_ERR;
        p_blkio->last_error = ENOMEM;
        return PFS_BLKIO_RET_IO;
    }
    p_priv->fd = -1;
    p_blkio->priv = p_priv;

    int fd = pfs_blkio_raw_linux_open(p_devpath, (int)flags | O_LARGEFILE | O_DIRECT);
    if (fd < 0)
    {
        pfs_free(p_priv);
        p_blkio->priv = NULL;
        p_blkio->status = PFS_BLKIO_NOT_OPEN | PFS_BLKIO_ERR;
        p_blkio->last_error = errno;
        return PFS_BLKIO_RET_IO;
    }

    p_priv->fd = fd;
    p_blkio->status = 0;
    p_blkio->last_error = 0;
    p_blkio->align_size = align_size;
    return PFS_BLKIO_RET_OK;
}

int pfs_blkio_raw_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }
    if (!pfs_blkio_raw_io_range_valid(offset, size) ||
            !pfs_blkio_raw_io_direct_valid(p_blkio, buf, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_blkio_raw_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    int done = 0;

    while (done < size)
    {
        int64_t pos64 = offset + (int64_t)done;
        off64_t pos = 0;

        if (0 != pfs_blkio_raw_off64_from_i64(pos64, &pos))
        {
            pfs_blkio_raw_read_fail(p_blkio, EINVAL);
            return PFS_BLKIO_RET_IO;
        }

        ssize_t nread = pfs_blkio_raw_linux_pread64(PFS_RAW_PRIV(p_blkio)->fd, (uint8_t *)buf + done, (size_t)(size - done), pos);

        if (nread < 0)
        {
            pfs_blkio_raw_read_fail(p_blkio, errno);
            return PFS_BLKIO_RET_IO;
        }
        if (0 == nread)
        {
            break;
        }

        done += (int)nread;
    }

    if (done < size)
    {
        pfs_blkio_raw_read_fail(p_blkio, EIO);
        return PFS_BLKIO_RET_IO;
    }

    return size;
}

int pfs_blkio_raw_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }
    if (!pfs_blkio_raw_io_range_valid(offset, size) ||
            !pfs_blkio_raw_io_direct_valid(p_blkio, buf, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_blkio_raw_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    int done = 0;

    while (done < size)
    {
        int64_t pos64 = offset + (int64_t)done;
        off64_t pos;
        ssize_t nwrite;

        if (0 != pfs_blkio_raw_off64_from_i64(pos64, &pos))
        {
            pfs_blkio_raw_write_fail(p_blkio, EINVAL);
            return PFS_BLKIO_RET_IO;
        }

        nwrite = pfs_blkio_raw_linux_pwrite64(PFS_RAW_PRIV(p_blkio)->fd, (const uint8_t *)buf + done, (size_t)(size - done), pos);

        if (nwrite < 0)
        {
            pfs_blkio_raw_write_fail(p_blkio, errno);
            return PFS_BLKIO_RET_IO;
        }
        if (0 == nwrite)
        {
            break;
        }

        done += (int)nwrite;
    }

    if (done < size)
    {
        pfs_blkio_raw_write_fail(p_blkio, EIO);
        return PFS_BLKIO_RET_IO;
    }

    return size;
}

int pfs_blkio_raw_fsync(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_blkio_raw_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (pfs_blkio_raw_linux_fsync(PFS_RAW_PRIV(p_blkio)->fd) < 0)
    {
        p_blkio->status |= PFS_BLKIO_ERR_FSYNC;
        p_blkio->last_error = errno;
        return PFS_BLKIO_RET_IO;
    }

    return PFS_BLKIO_RET_OK;
}

int pfs_blkio_raw_close(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    pfs_blkio_raw_priv_t *p_priv = pfs_blkio_raw_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_CLOSED;
        p_blkio->align_size = 0;
        return PFS_BLKIO_RET_OK;
    }

    if (p_priv->fd >= 0)
    {
        if (pfs_blkio_raw_linux_close(p_priv->fd) < 0)
        {
            // fd 仍有效, 保留 fd 供 retry close; 不置 CLOSED, 不释 priv
            p_blkio->status |= PFS_BLKIO_ERR;
            p_blkio->last_error = errno;
            return PFS_BLKIO_RET_IO;
        }
        p_priv->fd = -1;
    }

    pfs_free(p_priv);
    p_blkio->priv = NULL;
    p_blkio->status = PFS_BLKIO_CLOSED;
    p_blkio->align_size = 0;
    return PFS_BLKIO_RET_OK;
}

static int pfs_blkio_raw_sector_size_probe(const char *p_devpath)
{
    struct stat st;

    if (stat(p_devpath, &st) < 0)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    // 非块设备(镜像文件等): 无法 ioctl 探测 -> PROBE
    if (!S_ISBLK(st.st_mode))
    {
        return PFS_BLKIO_RET_PROBE;
    }

    int fd = pfs_blkio_raw_linux_open(p_devpath, O_RDONLY);
    if (fd < 0)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    int sector_size = 0;
    if (ioctl(fd, BLKSSZGET, &sector_size) < 0)
    {
        pfs_blkio_raw_linux_close(fd);
        return PFS_BLKIO_RET_PROBE;
    }

    pfs_blkio_raw_linux_close(fd);
    sector_size = pfs_blkio_raw_sector_size_normalize(sector_size);
    if (sector_size < 0)
    {
        return PFS_BLKIO_RET_PROBE;
    }
    return sector_size;
}

static int pfs_blkio_raw_sector_size_physical_probe(const char *p_devpath)
{
    struct stat st;

    if (stat(p_devpath, &st) < 0)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    // 非块设备(镜像文件等): 无法 ioctl 探测 -> PROBE
    if (!S_ISBLK(st.st_mode))
    {
        return PFS_BLKIO_RET_PROBE;
    }

    int fd = pfs_blkio_raw_linux_open(p_devpath, O_RDONLY);
    if (fd < 0)
    {
        return PFS_BLKIO_RET_PROBE;
    }

    int sector_size = 0;
    if (ioctl(fd, BLKBSZGET, &sector_size) < 0)
    {
        pfs_blkio_raw_linux_close(fd);
        // BLKBSZGET 失败: fallback 到逻辑扇区
        return pfs_blkio_raw_sector_size_probe(p_devpath);
    }

    pfs_blkio_raw_linux_close(fd);
    sector_size = pfs_blkio_raw_sector_size_normalize(sector_size);
    if (sector_size < 0)
    {
        return pfs_blkio_raw_sector_size_probe(p_devpath);
    }
    return sector_size;
}

#endif // _WIN32

int pfs_blkio_raw_sector_size(const char *p_devpath)
{
    if (NULL == p_devpath || '\0' == p_devpath[0])
    {
        return PFS_BLKIO_RET_PARAM;
    }
    return pfs_blkio_raw_sector_size_probe(p_devpath);
}

int pfs_blkio_raw_sector_size_physical(const char *p_devpath)
{
    if (NULL == p_devpath || '\0' == p_devpath[0])
    {
        return PFS_BLKIO_RET_PARAM;
    }
    return pfs_blkio_raw_sector_size_physical_probe(p_devpath);
}

int64_t pfs_blkio_raw_get_size(const pfs_blkio_t *p_blkio)
{
    pfs_blkio_raw_priv_t *p_priv = pfs_blkio_raw_priv_or_null(p_blkio);

    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_STATE;
    }
#ifdef _WIN32
    {
        LARGE_INTEGER sz;

        if (INVALID_HANDLE_VALUE == p_priv->h_device)
        {
            return PFS_BLKIO_RET_STATE;
        }
        if (!GetFileSizeEx(p_priv->h_device, &sz))
        {
            return PFS_BLKIO_RET_IO;
        }
        return (int64_t)sz.QuadPart;
    }
#else
    {
        off64_t end;

        if (p_priv->fd < 0)
        {
            return PFS_BLKIO_RET_STATE;
        }
        end = lseek64(p_priv->fd, 0, SEEK_END);
        if (end < 0)
        {
            return PFS_BLKIO_RET_IO;
        }
        return (int64_t)end;
    }
#endif
}

int pfs_blkio_raw_logical_sector_size(const pfs_blkio_t *p_blkio)
{
    pfs_blkio_raw_priv_t *p_priv = pfs_blkio_raw_priv_or_null(p_blkio);

    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }
#ifdef _WIN32
    if (NULL != p_priv && p_priv->sector_size > 0)
    {
        return p_priv->sector_size;
    }
#endif
    if (p_blkio->align_size > 0)
    {
        return p_blkio->align_size;
    }
    return PFS_BLKIO_ALIGN_512;
}

#ifdef _WIN32
HANDLE pfs_blkio_raw_handle(const pfs_blkio_t *p_blkio)
{
    pfs_blkio_raw_priv_t *p_priv = pfs_blkio_raw_priv_or_null(p_blkio);

    if (NULL == p_priv)
    {
        return INVALID_HANDLE_VALUE;
    }
    return p_priv->h_device;
}
#else
int pfs_blkio_raw_fd(const pfs_blkio_t *p_blkio)
{
    pfs_blkio_raw_priv_t *p_priv = pfs_blkio_raw_priv_or_null(p_blkio);

    if (NULL == p_priv)
    {
        return -1;
    }
    return p_priv->fd;
}
#endif

const pfs_blkio_ops_t g_pfs_blkio_raw_ops = {
    "raw",
    PFS_BLKIO_RAW,
    pfs_blkio_raw_open,
    pfs_blkio_raw_open_direct,
    pfs_blkio_raw_close,
    pfs_blkio_raw_pread,
    pfs_blkio_raw_pwrite,
    pfs_blkio_raw_fsync,
    pfs_blkio_raw_get_size,
    pfs_blkio_raw_logical_sector_size,
};

// end
