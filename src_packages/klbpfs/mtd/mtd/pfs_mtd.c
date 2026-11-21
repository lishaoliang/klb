// Doc Encode : UTF-8 BOM, Unix(LF)
// MTD/UBI blkio: PFUB 假 LEB 卷 (P1); ops 经 pfsmtd_get_ubi; 打开走标准 blkio facade
#include "pfsmtd.h"
#include "mtd/pfs_mtd_raw.h"
#include "base/pfsmtd_mem.h"

#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

typedef struct _pfs_mtd_ubi_priv_t {
    pfs_blkio_t             host;
    uint32_t                leb_size;
    uint32_t                leb_count;
    uint32_t                hdr_size;
} pfs_mtd_ubi_priv_t;

#define PFS_MTD_UBI_PRIV(p) ((pfs_mtd_ubi_priv_t *)((p)->priv))

static int pfs_mtd_flags_valid(pfs_blkio_oflags_e flags)
{
    return (PFS_BLKIO_O_RDONLY == flags || PFS_BLKIO_O_WRONLY == flags || PFS_BLKIO_O_RDWR == flags);
}

static int pfs_mtd_leb_geom_ok(uint32_t leb_size, uint32_t leb_count)
{
    if (leb_size < 512u || 0u != (leb_size % 512u))
    {
        return 0;
    }
    if (leb_count < 1u || leb_count > 1048576u)
    {
        return 0;
    }
    if ((uint64_t)leb_size * (uint64_t)leb_count > (uint64_t)0x7FFFFFFFFFFFFFFFull - (uint64_t)PFS_MTD_PFUB_HDR_SIZE)
    {
        return 0;
    }
    return 1;
}

static void pfs_mtd_put_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

static uint32_t pfs_mtd_get_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int pfs_mtd_ubi_io_ready(const pfs_blkio_t *p_blkio)
{
    pfs_mtd_ubi_priv_t *p_priv;

    if (NULL == p_blkio)
    {
        return 0;
    }
    if (0 != (p_blkio->status & (PFS_BLKIO_NOT_OPEN | PFS_BLKIO_CLOSED |
            PFS_BLKIO_ERR | PFS_BLKIO_ERR_READ | PFS_BLKIO_ERR_WRITE | PFS_BLKIO_ERR_FSYNC)))
    {
        return 0;
    }
    p_priv = PFS_MTD_UBI_PRIV(p_blkio);
    if (NULL == p_priv)
    {
        return 0;
    }
    return pfs_blkio_is_open(&p_priv->host);
}

static void pfs_mtd_ubi_fail_from_host(pfs_blkio_t *p_blkio, int err_bit)
{
    pfs_mtd_ubi_priv_t *p_priv = PFS_MTD_UBI_PRIV(p_blkio);

    p_blkio->status |= err_bit;
    if (NULL != p_priv)
    {
        p_blkio->last_error = p_priv->host.last_error;
    }
}

static void pfs_mtd_ubi_open_fail(pfs_mtd_ubi_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    if (pfs_blkio_is_open(&p_priv->host))
    {
        (void)pfs_blkio_close(&p_priv->host);
    }
    pfsmtd_free(p_priv);
}

static int pfs_mtd_ubi_open_ops(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size)
{
    (void)sector_size;
    if (NULL == p_blkio || NULL == path || !pfs_mtd_flags_valid(flags))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int ss = PFS_BLKIO_ALIGN_512;
    pfs_mtd_ubi_priv_t *p_priv = (pfs_mtd_ubi_priv_t *)pfsmtd_mallocz(sizeof(*p_priv));
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_IO;
    }

    (void)pfs_blkio_init(&p_priv->host);
    int rc = pfs_blkio_open(&p_priv->host, path, flags, ss);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfsmtd_free(p_priv);
        return rc;
    }

    uint8_t hdr[PFS_MTD_PFUB_HDR_SIZE];
    rc = pfs_blkio_pread(&p_priv->host, hdr, (int)sizeof(hdr), 0);
    if ((int)sizeof(hdr) != rc)
    {
        pfs_mtd_ubi_open_fail(p_priv);
        return (0 > rc) ? rc : PFS_BLKIO_RET_IO;
    }

    uint32_t magic = pfs_mtd_get_le32(hdr + 0);
    uint32_t ver = pfs_mtd_get_le32(hdr + 4);
    uint32_t leb_size = pfs_mtd_get_le32(hdr + 8);
    uint32_t leb_count = pfs_mtd_get_le32(hdr + 12);
    if (PFS_MTD_PFUB_MAGIC != magic || PFS_MTD_PFUB_VERSION != ver)
    {
        pfs_mtd_ubi_open_fail(p_priv);
        return PFS_BLKIO_RET_PROBE;
    }
    if (!pfs_mtd_leb_geom_ok(leb_size, leb_count))
    {
        pfs_mtd_ubi_open_fail(p_priv);
        return PFS_BLKIO_RET_PARAM;
    }

    int64_t host_size = pfs_blkio_get_size(&p_priv->host);
    int64_t need = (int64_t)PFS_MTD_PFUB_HDR_SIZE + (int64_t)leb_size * (int64_t)leb_count;
    if (host_size < need)
    {
        pfs_mtd_ubi_open_fail(p_priv);
        return PFS_BLKIO_RET_STATE;
    }

    p_priv->leb_size = leb_size;
    p_priv->leb_count = leb_count;
    p_priv->hdr_size = PFS_MTD_PFUB_HDR_SIZE;
    p_blkio->priv = p_priv;
    p_blkio->status = 0;
    p_blkio->last_error = 0;
    p_blkio->align_size = 0;
    return PFS_BLKIO_RET_OK;
}

static int pfs_mtd_ubi_close_ops(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    pfs_mtd_ubi_priv_t *p_priv = PFS_MTD_UBI_PRIV(p_blkio);
    if (NULL == p_priv)
    {
        p_blkio->ops = NULL;
        p_blkio->status = PFS_BLKIO_CLOSED;
        return PFS_BLKIO_RET_OK;
    }
    (void)pfs_blkio_close(&p_priv->host);
    pfsmtd_free(p_priv);
    p_blkio->priv = NULL;
    p_blkio->ops = NULL;
    p_blkio->status = PFS_BLKIO_CLOSED;
    p_blkio->last_error = 0;
    return PFS_BLKIO_RET_OK;
}

static int pfs_mtd_ubi_pread_ops(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (size < 0 || size > PFS_BLKIO_IO_SIZE_MAX || offset < 0)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return 0;
    }
    if (!pfs_mtd_ubi_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }
    pfs_mtd_ubi_priv_t *p_priv = PFS_MTD_UBI_PRIV(p_blkio);
    int64_t vol = (int64_t)p_priv->leb_size * (int64_t)p_priv->leb_count;
    if (offset > vol || (int64_t)size > vol - offset)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    int64_t host_off = (int64_t)p_priv->hdr_size + offset;
    int rc = pfs_blkio_pread(&p_priv->host, buf, size, host_off);
    if (0 > rc)
    {
        if (PFS_BLKIO_RET_IO == rc)
        {
            pfs_mtd_ubi_fail_from_host(p_blkio, PFS_BLKIO_ERR_READ);
        }
        return rc;
    }
    return rc;
}

static int pfs_mtd_ubi_pwrite_ops(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (size < 0 || size > PFS_BLKIO_IO_SIZE_MAX || offset < 0)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return 0;
    }
    if (!pfs_mtd_ubi_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }
    pfs_mtd_ubi_priv_t *p_priv = PFS_MTD_UBI_PRIV(p_blkio);
    int64_t vol = (int64_t)p_priv->leb_size * (int64_t)p_priv->leb_count;
    if (offset > vol || (int64_t)size > vol - offset)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    int64_t host_off = (int64_t)p_priv->hdr_size + offset;
    int rc = pfs_blkio_pwrite(&p_priv->host, buf, size, host_off);
    if (0 > rc)
    {
        if (PFS_BLKIO_RET_IO == rc)
        {
            pfs_mtd_ubi_fail_from_host(p_blkio, PFS_BLKIO_ERR_WRITE);
        }
        return rc;
    }
    return rc;
}

static int pfs_mtd_ubi_fsync_ops(pfs_blkio_t *p_blkio)
{
    if (!pfs_mtd_ubi_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }
    pfs_mtd_ubi_priv_t *p_priv = PFS_MTD_UBI_PRIV(p_blkio);
    int rc = pfs_blkio_fsync(&p_priv->host);
    if (PFS_BLKIO_RET_IO == rc)
    {
        pfs_mtd_ubi_fail_from_host(p_blkio, PFS_BLKIO_ERR_FSYNC);
    }
    return rc;
}

static int64_t pfs_mtd_ubi_get_size_ops(const pfs_blkio_t *p_blkio)
{
    if (!pfs_mtd_ubi_io_ready(p_blkio))
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    pfs_mtd_ubi_priv_t *p_priv = PFS_MTD_UBI_PRIV(p_blkio);
    return (int64_t)p_priv->leb_size * (int64_t)p_priv->leb_count;
}

static int pfs_mtd_ubi_logical_sector_size_ops(const pfs_blkio_t *p_blkio)
{
    if (!pfs_mtd_ubi_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }
    return PFS_BLKIO_ALIGN_512;
}

static const pfs_blkio_ops_t g_pfs_mtd_ubi_ops = {
    "ubi",
    (pfs_blkio_type_e)PFSMTD_BLKIO_UBI,
    pfs_mtd_ubi_open_ops,
    NULL,
    pfs_mtd_ubi_close_ops,
    pfs_mtd_ubi_pread_ops,
    pfs_mtd_ubi_pwrite_ops,
    pfs_mtd_ubi_fsync_ops,
    pfs_mtd_ubi_get_size_ops,
    pfs_mtd_ubi_logical_sector_size_ops,
};

const pfs_blkio_ops_t *pfsmtd_get_ubi(void)
{
    return &g_pfs_mtd_ubi_ops;
}

int pfsmtd_ubi_create(const char *path, uint32_t leb_size, uint32_t leb_count)
{
    if (NULL == path || !pfs_mtd_leb_geom_ok(leb_size, leb_count))
    {
        return PFS_EINVAL;
    }

    uint8_t hdr[PFS_MTD_PFUB_HDR_SIZE];
    memset(hdr, 0, sizeof(hdr));
    pfs_mtd_put_le32(hdr + 0, PFS_MTD_PFUB_MAGIC);
    pfs_mtd_put_le32(hdr + 4, PFS_MTD_PFUB_VERSION);
    pfs_mtd_put_le32(hdr + 8, leb_size);
    pfs_mtd_put_le32(hdr + 12, leb_count);
    uint64_t data_bytes = (uint64_t)leb_size * (uint64_t)leb_count;
#ifdef _WIN32
    HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_EIO;
    }
    DWORD wr;
    if (!WriteFile(h, hdr, (DWORD)sizeof(hdr), &wr, NULL) || wr != (DWORD)sizeof(hdr))
    {
        CloseHandle(h);
        return PFS_EIO;
    }
    LARGE_INTEGER li;
    li.QuadPart = (LONGLONG)((uint64_t)PFS_MTD_PFUB_HDR_SIZE + data_bytes);
    if (!SetFilePointerEx(h, li, NULL, FILE_BEGIN) || !SetEndOfFile(h))
    {
        CloseHandle(h);
        return PFS_EIO;
    }
    CloseHandle(h);
#else
    int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd < 0)
    {
        return PFS_EIO;
    }
    if ((ssize_t)sizeof(hdr) != write(fd, hdr, sizeof(hdr)))
    {
        close(fd);
        return PFS_EIO;
    }
    off_t total = (off_t)((uint64_t)PFS_MTD_PFUB_HDR_SIZE + data_bytes);
    if (0 != ftruncate(fd, total))
    {
        close(fd);
        return PFS_EIO;
    }
    close(fd);
#endif
    return PFS_OK;
}

int pfsmtd_ubi_leb_size(const pfs_blkio_t *p_blkio)
{
    if (!pfs_mtd_ubi_io_ready(p_blkio))
    {
        return PFS_EINVAL;
    }
    pfs_mtd_ubi_priv_t *p_priv = PFS_MTD_UBI_PRIV(p_blkio);
    return (int)p_priv->leb_size;
}

int pfsmtd_ubi_leb_count(const pfs_blkio_t *p_blkio)
{
    if (!pfs_mtd_ubi_io_ready(p_blkio))
    {
        return PFS_EINVAL;
    }
    pfs_mtd_ubi_priv_t *p_priv = PFS_MTD_UBI_PRIV(p_blkio);
    return (int)p_priv->leb_count;
}

int pfsmtd_ubi_leb_read(pfs_blkio_t *p_blkio, int lnum, int offs, void *buf, int len)
{
    if (NULL == buf || len < 0)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_mtd_ubi_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }
    pfs_mtd_ubi_priv_t *p_priv = PFS_MTD_UBI_PRIV(p_blkio);
    if (lnum < 0 || (uint32_t)lnum >= p_priv->leb_count || offs < 0)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if ((uint32_t)offs > p_priv->leb_size || (uint32_t)len > p_priv->leb_size - (uint32_t)offs)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    int64_t off = (int64_t)lnum * (int64_t)p_priv->leb_size + (int64_t)offs;
    return pfs_blkio_pread(p_blkio, buf, len, off);
}

int pfsmtd_ubi_leb_write(pfs_blkio_t *p_blkio, int lnum, int offs, const void *buf, int len)
{
    if (NULL == buf || len < 0)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_mtd_ubi_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }
    pfs_mtd_ubi_priv_t *p_priv = PFS_MTD_UBI_PRIV(p_blkio);
    if (lnum < 0 || (uint32_t)lnum >= p_priv->leb_count || offs < 0)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if ((uint32_t)offs > p_priv->leb_size || (uint32_t)len > p_priv->leb_size - (uint32_t)offs)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    int64_t off = (int64_t)lnum * (int64_t)p_priv->leb_size + (int64_t)offs;
    return pfs_blkio_pwrite(p_blkio, buf, len, off);
}

static int pfs_mtd_raw_size_ok(uint64_t nbytes)
{
    if (nbytes < 512u || 0u != (nbytes % 512u))
    {
        return 0;
    }
    if (nbytes > (uint64_t)0x7FFFFFFFFFFFFFFFu)
    {
        return 0;
    }
    return 1;
}

int pfsmtd_raw_create(const char *path, uint64_t nbytes)
{
    if (NULL == path || !pfs_mtd_raw_size_ok(nbytes))
    {
        return PFS_EINVAL;
    }

    uint32_t chunk = PFS_MTD_RAW_FILL_CHUNK;
    uint8_t *p_chunk = (uint8_t *)pfsmtd_malloc((size_t)chunk);
    if (NULL == p_chunk)
    {
        return PFS_ENOMEM;
    }
    memset(p_chunk, (int)PFS_MTD_RAW_ERASE_BYTE, (size_t)chunk);

#ifdef _WIN32
    HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == h)
    {
        pfsmtd_free(p_chunk);
        return PFS_EIO;
    }
    uint64_t written = 0;
    while (written < nbytes)
    {
        uint32_t n = chunk;
        if ((uint64_t)n > nbytes - written)
        {
            n = (uint32_t)(nbytes - written);
        }
        DWORD wr;
        if (!WriteFile(h, p_chunk, n, &wr, NULL) || wr != n)
        {
            CloseHandle(h);
            pfsmtd_free(p_chunk);
            return PFS_EIO;
        }
        written += (uint64_t)n;
    }
    LARGE_INTEGER li;
    li.QuadPart = (LONGLONG)nbytes;
    if (!SetFilePointerEx(h, li, NULL, FILE_BEGIN) || !SetEndOfFile(h))
    {
        CloseHandle(h);
        pfsmtd_free(p_chunk);
        return PFS_EIO;
    }
    CloseHandle(h);
#else
    int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd < 0)
    {
        pfsmtd_free(p_chunk);
        return PFS_EIO;
    }
    uint64_t written = 0;
    while (written < nbytes)
    {
        uint32_t n = chunk;
        if ((uint64_t)n > nbytes - written)
        {
            n = (uint32_t)(nbytes - written);
        }
        ssize_t wn = write(fd, p_chunk, (size_t)n);
        if (wn != (ssize_t)n)
        {
            close(fd);
            pfsmtd_free(p_chunk);
            return PFS_EIO;
        }
        written += (uint64_t)n;
    }
    if (0 != ftruncate(fd, (off_t)nbytes))
    {
        close(fd);
        pfsmtd_free(p_chunk);
        return PFS_EIO;
    }
    close(fd);
#endif

    pfsmtd_free(p_chunk);
    return PFS_OK;
}

// end
