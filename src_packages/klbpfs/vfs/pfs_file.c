// Doc Encode : UTF-8 BOM, Unix(LF)
// pfs 对外文件 API (stdio 风格; 数据面经 pfs_fs_ops)
#include "vfs/pfs_file.h"
#include "pfs_compiler.h"
#include "vfs/pfs_ctx.h"
#include "base/pfs_mem.h"
#include "pfs_blkio.h"

#include <stdbool.h>

static bool pfs_file_mode_ok(int mode)
{
    // 仅允许 RD/RDWR/CREAT; 访问位须恰好为 RD 或 RDWR; CREAT 须带 RDWR
    if (0 != (mode & ~(PFS_O_RD | PFS_O_RDWR | PFS_O_CREAT)))
    {
        return false;
    }

    int access = mode & (PFS_O_RD | PFS_O_RDWR);
    if ((PFS_O_RD != access) && (PFS_O_RDWR != access))
    {
        return false;
    }
    if ((0 != (mode & PFS_O_CREAT)) && (PFS_O_RDWR != access))
    {
        return false;
    }
    return true;
}

static bool pfs_file_ctx_mounted(const pfs_ctx_t *p_ctx)
{
    if (pfs_unlikely(NULL == p_ctx))
    {
        return false;
    }
    return (PFS_NONE != p_ctx->fs_type && NULL != p_ctx->p_fs_ops && NULL != p_ctx->p_sbi);
}

static bool pfs_file_can_write(const pfs_file_t *p_file)
{
    if (pfs_unlikely(NULL == p_file))
    {
        return false;
    }
    return (0 != (p_file->mode & PFS_O_RDWR));
}

static void pfs_file_refresh_size(pfs_file_t *p_file)
{
    if (NULL == p_file || NULL == p_file->p_ctx || NULL == p_file->p_fpriv)
    {
        return;
    }

    int64_t sz = pfs_fs_file_get_size(p_file->p_ctx->p_fs_ops, p_file->p_ctx->p_sbi, p_file->p_fpriv);
    if (sz >= 0)
    {
        p_file->size = sz;
    }
}

pfs_file_t *pfs_fopen(pfs_ctx_t *p_ctx, const char *path, int mode)
{
    if (pfs_unlikely((NULL == p_ctx) || (NULL == path) || ('\0' == path[0]) ||
            !pfs_file_mode_ok(mode) || !pfs_file_ctx_mounted(p_ctx)))
    {
        return NULL;
    }

    bool want_write = (0 != (mode & PFS_O_RDWR));
    bool want_creat = (0 != (mode & PFS_O_CREAT));

    if (want_creat && !want_write)
    {
        return NULL;
    }
    if (want_creat && PFS_OK != pfs_fs_path_check_path(p_ctx->p_fs_ops, p_ctx->p_sbi, path, PFS_PATH_CHECK_CREAT))
    {
        return NULL;
    }
    if (want_write && !pfs_fs_super_can_write(p_ctx->p_fs_ops, p_ctx->p_sbi))
    {
        return NULL;
    }

    int priv_size = p_ctx->p_fs_ops->file.priv_size;

    if (priv_size <= 0)
    {
        return NULL;
    }

    pfs_file_t *p_file = (pfs_file_t *)pfs_mallocz(sizeof(*p_file));
    if (pfs_unlikely(NULL == p_file))
    {
        return NULL;
    }

    p_file->p_fpriv = pfs_mallocz((size_t)priv_size);
    if (pfs_unlikely(NULL == p_file->p_fpriv))
    {
        pfs_free(p_file);
        return NULL;
    }

    p_file->p_ctx = p_ctx;
    p_file->mode = mode;
    p_file->pos = 0;
    p_file->err = PFS_OK;
    p_file->eof = 0;

    int rc = pfs_fs_file_open(p_ctx->p_fs_ops, p_ctx->p_sbi, path, mode, p_file->p_fpriv);

    if (PFS_OK != rc)
    {
        pfs_free(p_file->p_fpriv);
        pfs_free(p_file);
        return NULL;
    }

    pfs_file_refresh_size(p_file);
    return p_file;
}

int pfs_fclose(pfs_file_t *p_file)
{
    if (pfs_unlikely(NULL == p_file))
    {
        return PFS_EINVAL;
    }

    int rc = PFS_OK;
    if (NULL != p_file->p_ctx && NULL != p_file->p_fpriv)
    {
        if (pfs_file_can_write(p_file))
        {
            rc = pfs_fs_file_fdatasync(p_file->p_ctx->p_fs_ops, p_file->p_ctx->p_sbi, p_file->p_fpriv);
        }
        pfs_fs_file_close(p_file->p_ctx->p_fs_ops, p_file->p_ctx->p_sbi, p_file->p_fpriv);
    }
    if (NULL != p_file->p_fpriv)
    {
        pfs_free(p_file->p_fpriv);
        p_file->p_fpriv = NULL;
    }
    p_file->p_ctx = NULL;
    pfs_free(p_file);
    return rc;
}

int pfs_fread(void *buf, int size, int nmemb, pfs_file_t *p_file)
{
    if (pfs_unlikely((NULL == p_file) || (NULL == buf) || (size <= 0) || (nmemb <= 0)))
    {
        if (NULL != p_file)
        {
            p_file->err = PFS_EINVAL;
        }
        return PFS_EINVAL;
    }
    if (pfs_unlikely(0 != p_file->err))
    {
        return p_file->err;
    }
    if (pfs_unlikely(NULL == p_file->p_ctx || NULL == p_file->p_fpriv))
    {
        p_file->err = PFS_EINVAL;
        return PFS_EINVAL;
    }

    int64_t want64 = (int64_t)size * (int64_t)nmemb;
    if (want64 <= 0 || want64 > (int64_t)0x7FFFFFFF)
    {
        p_file->err = PFS_EINVAL;
        return PFS_EINVAL;
    }
    int want = (int)want64;
    int got = 0;
    pfs_ctx_t *p_ctx = p_file->p_ctx;

    int rc = pfs_fs_file_read(p_ctx->p_fs_ops, p_ctx->p_sbi, p_file->p_fpriv,
            p_file->pos, buf, want, &got);
    if (PFS_OK != rc)
    {
        p_file->err = rc;
        return rc;
    }

    p_file->pos += got;
    if (got < want)
    {
        p_file->eof = 1;
    }
    int n = got / size;
    return n;
}

int pfs_fwrite(const void *buf, int size, int nmemb, pfs_file_t *p_file)
{
    if (pfs_unlikely((NULL == p_file) || (NULL == buf) || (size <= 0) || (nmemb <= 0)))
    {
        if (NULL != p_file)
        {
            p_file->err = PFS_EINVAL;
        }
        return PFS_EINVAL;
    }
    if (pfs_unlikely(!pfs_file_can_write(p_file)))
    {
        p_file->err = PFS_EINVAL;
        return PFS_EINVAL;
    }
    if (pfs_unlikely(0 != p_file->err))
    {
        return p_file->err;
    }
    if (pfs_unlikely(NULL == p_file->p_ctx || NULL == p_file->p_fpriv))
    {
        p_file->err = PFS_EINVAL;
        return PFS_EINVAL;
    }

    int64_t want64 = (int64_t)size * (int64_t)nmemb;
    if (want64 <= 0 || want64 > (int64_t)0x7FFFFFFF)
    {
        p_file->err = PFS_EINVAL;
        return PFS_EINVAL;
    }
    int want = (int)want64;
    int put = 0;
    pfs_ctx_t *p_ctx = p_file->p_ctx;

    int rc = pfs_fs_file_write(p_ctx->p_fs_ops, p_ctx->p_sbi, p_file->p_fpriv,
            p_file->pos, buf, want, &put);
    if (PFS_OK != rc)
    {
        p_file->err = rc;
        return rc;
    }

    p_file->pos += put;
    pfs_file_refresh_size(p_file);
    p_file->eof = 0;
    int n = put / size;
    return n;
}

int pfs_fgetc(pfs_file_t *p_file)
{
    if (pfs_unlikely(NULL == p_file))
    {
        return PFS_EINVAL;
    }

    unsigned char c;
    int n = pfs_fread(&c, 1, 1, p_file);
    if (pfs_unlikely(n < 0))
    {
        return n;
    }
    if (pfs_unlikely(1 != n))
    {
        return -1;                                      // EOF (与 PFS_EPERM==-1 同值; 用 feof 区分)
    }
    return (int)c;
}

int pfs_fputc(int c, pfs_file_t *p_file)
{
    if (pfs_unlikely(NULL == p_file))
    {
        return PFS_EINVAL;
    }

    unsigned char b = (unsigned char)(c & 0xFF);
    int n = pfs_fwrite(&b, 1, 1, p_file);
    if (pfs_unlikely(n < 0))
    {
        return n;
    }
    if (pfs_unlikely(1 != n))
    {
        return PFS_EIO;
    }
    return (int)b;
}

int pfs_fseek(pfs_file_t *p_file, int64_t offset, int whence)
{
    if (pfs_unlikely(NULL == p_file))
    {
        return PFS_EINVAL;
    }

    int64_t new_pos;

    if (PFS_SEEK_SET == whence)
    {
        new_pos = offset;
    }
    else if (PFS_SEEK_CUR == whence)
    {
        new_pos = p_file->pos + offset;
    }
    else if (PFS_SEEK_END == whence)
    {
        pfs_file_refresh_size(p_file);
        new_pos = p_file->size + offset;
    }
    else
    {
        p_file->err = PFS_EINVAL;
        return PFS_EINVAL;
    }

    if (pfs_unlikely(new_pos < 0))
    {
        p_file->err = PFS_EINVAL;
        return PFS_EINVAL;
    }

    p_file->pos = new_pos;
    p_file->eof = 0;
    p_file->err = PFS_OK;
    return PFS_OK;
}

int64_t pfs_ftell(pfs_file_t *p_file)
{
    if (pfs_unlikely(NULL == p_file))
    {
        return (int64_t)-1;
    }
    return p_file->pos;
}

int pfs_ferror(pfs_file_t *p_file)
{
    if (pfs_unlikely(NULL == p_file))
    {
        return PFS_EINVAL;
    }
    return p_file->err;
}

int pfs_feof(pfs_file_t *p_file)
{
    if (pfs_unlikely(NULL == p_file))
    {
        return 0;
    }
    return p_file->eof;
}

int pfs_fflush(pfs_file_t *p_file)
{
    if (pfs_unlikely(NULL == p_file))
    {
        return PFS_EINVAL;
    }
    if (!pfs_file_can_write(p_file))
    {
        return PFS_OK;
    }
    if (NULL == p_file->p_ctx || NULL == p_file->p_fpriv)
    {
        return PFS_EINVAL;
    }
    int rc = pfs_fs_file_fdatasync(p_file->p_ctx->p_fs_ops, p_file->p_ctx->p_sbi, p_file->p_fpriv);
    if (PFS_OK != rc)
    {
        p_file->err = rc;
    }
    return rc;
}

int pfs_fsync(pfs_file_t *p_file)
{
    if (pfs_unlikely(NULL == p_file))
    {
        return PFS_EINVAL;
    }
    if (!pfs_file_can_write(p_file))
    {
        return PFS_OK;
    }
    int rc = pfs_fflush(p_file);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (NULL == p_file->p_ctx || NULL == p_file->p_ctx->p_sbi)
    {
        return PFS_EINVAL;
    }
    rc = pfs_fs_ctrl_fsync(p_file->p_ctx->p_fs_ops, p_file->p_ctx->p_sbi);
    if (PFS_OK != rc && PFS_EOPNOTSUPP != rc)
    {
        p_file->err = rc;
        return rc;
    }
    if (NULL == p_file->p_ctx->p_blkio)
    {
        return PFS_EINVAL;
    }
    if (pfs_blkio_fsync(p_file->p_ctx->p_blkio) < 0)
    {
        p_file->err = PFS_EIO;
        return PFS_EIO;
    }
    return PFS_OK;
}

int pfs_ftruncate(pfs_file_t *p_file, int64_t size)
{
    if (pfs_unlikely((NULL == p_file) || (size < 0)))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(!pfs_file_can_write(p_file)))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(NULL == p_file->p_ctx || NULL == p_file->p_fpriv))
    {
        p_file->err = PFS_EINVAL;
        return PFS_EINVAL;
    }

    pfs_ctx_t *p_ctx = p_file->p_ctx;
    int rc = pfs_fs_file_truncate(p_ctx->p_fs_ops, p_ctx->p_sbi, p_file->p_fpriv, size);
    if (PFS_OK != rc)
    {
        p_file->err = rc;
        return rc;
    }

    pfs_file_refresh_size(p_file);
    if (p_file->pos > p_file->size)
    {
        p_file->pos = p_file->size;
    }
    p_file->eof = 0;
    return PFS_OK;
}

// end
