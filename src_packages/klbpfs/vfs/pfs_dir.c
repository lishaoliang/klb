// Doc Encode : UTF-8 BOM, Unix(LF)
// pfs 目录流 API (opendir/readdir; 磁盘项经 pfs_fs_ops; . / .. 由 vfs 合成)
#include "vfs/pfs_dir.h"
#include "pfs_compiler.h"
#include "vfs/pfs_ctx.h"
#include "base/pfs_mem.h"

#include <string.h>
#include <stdbool.h>

static bool pfs_dir_ctx_mounted(const pfs_ctx_t *p_ctx)
{
    if (pfs_unlikely(NULL == p_ctx))
    {
        return false;
    }
    return (PFS_NONE != p_ctx->fs_type && NULL != p_ctx->p_fs_ops && NULL != p_ctx->p_sbi);
}

pfs_dir_t *pfs_opendir(pfs_ctx_t *p_ctx, const char *path)
{
    if (pfs_unlikely((NULL == p_ctx) || (NULL == path) || ('\0' == path[0]) ||
            !pfs_dir_ctx_mounted(p_ctx)))
    {
        return NULL;
    }

    int priv_size = p_ctx->p_fs_ops->dir.priv_size;
    if (priv_size <= 0)
    {
        return NULL;
    }

    pfs_dir_t *p_dir = (pfs_dir_t *)pfs_mallocz(sizeof(*p_dir));
    if (pfs_unlikely(NULL == p_dir))
    {
        return NULL;
    }

    p_dir->p_dpriv = pfs_mallocz((size_t)priv_size);
    if (pfs_unlikely(NULL == p_dir->p_dpriv))
    {
        pfs_free(p_dir);
        return NULL;
    }

    p_dir->p_ctx = p_ctx;
    p_dir->dots_done = 0;
    p_dir->eof = 0;
    p_dir->err = PFS_OK;

    int rc = pfs_fs_dir_open(p_ctx->p_fs_ops, p_ctx->p_sbi, path, p_dir->p_dpriv);
    if (PFS_OK != rc)
    {
        pfs_free(p_dir->p_dpriv);
        pfs_free(p_dir);
        return NULL;
    }
    return p_dir;
}

void pfs_closedir(pfs_dir_t *p_dir)
{
    if (pfs_unlikely(NULL == p_dir))
    {
        return;
    }

    if (NULL != p_dir->p_ctx && NULL != p_dir->p_dpriv)
    {
        pfs_fs_dir_close(p_dir->p_ctx->p_fs_ops, p_dir->p_ctx->p_sbi, p_dir->p_dpriv);
    }
    if (NULL != p_dir->p_dpriv)
    {
        pfs_free(p_dir->p_dpriv);
        p_dir->p_dpriv = NULL;
    }
    p_dir->p_ctx = NULL;
    pfs_free(p_dir);
}

pfs_dirent_t *pfs_readdir(pfs_dir_t *p_dir)
{
    if (pfs_unlikely(NULL == p_dir))
    {
        return NULL;
    }
    if (pfs_unlikely(0 != p_dir->err))
    {
        return NULL;
    }
    if (pfs_unlikely(0 != p_dir->eof))
    {
        return NULL;
    }
    if (pfs_unlikely(NULL == p_dir->p_ctx || NULL == p_dir->p_dpriv))
    {
        p_dir->err = PFS_EINVAL;
        return NULL;
    }

    if (0 == p_dir->dots_done)
    {
        p_dir->entry.d_type = PFS_DT_DIR;
        (void)strncpy(p_dir->entry.d_name, ".", sizeof(p_dir->entry.d_name) - 1U);
        p_dir->entry.d_name[sizeof(p_dir->entry.d_name) - 1U] = '\0';
        p_dir->dots_done = 1;
        return &p_dir->entry;
    }

    if (1 == p_dir->dots_done)
    {
        p_dir->entry.d_type = PFS_DT_DIR;
        (void)strncpy(p_dir->entry.d_name, "..", sizeof(p_dir->entry.d_name) - 1U);
        p_dir->entry.d_name[sizeof(p_dir->entry.d_name) - 1U] = '\0';
        p_dir->dots_done = 2;
        return &p_dir->entry;
    }

    int rc = pfs_fs_dir_readdir(p_dir->p_ctx->p_fs_ops, p_dir->p_ctx->p_sbi,
            p_dir->p_dpriv, &p_dir->entry);
    if (rc < 0)
    {
        p_dir->err = rc;
        return NULL;
    }
    if (0 == rc)
    {
        p_dir->eof = 1;
        return NULL;
    }
    return &p_dir->entry;
}

int pfs_direrror(pfs_dir_t *p_dir)
{
    if (pfs_unlikely(NULL == p_dir))
    {
        return PFS_EINVAL;
    }
    return p_dir->err;
}

// end
