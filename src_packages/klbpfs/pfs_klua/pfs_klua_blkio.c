// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua_blkio.h"
#include "pfs_klua_util.h"

#include <string.h>

void pfs_klua_blkio_sess_init(pfs_klua_blkio_sess_t *p_sess)
{
    if (NULL == p_sess)
    {
        return;
    }

    memset(p_sess, 0, sizeof(*p_sess));
}

void pfs_klua_blkio_sess_done(pfs_klua_blkio_sess_t *p_sess)
{
    if (NULL == p_sess)
    {
        return;
    }

    if (0 != p_sess->open)
    {
        pfs_blkio_close(&p_sess->blkio);
        pfs_blkio_deinit(&p_sess->blkio);
        p_sess->open = 0;
    }
}

static int pfs_klua_blkio_sess_open(pfs_klua_blkio_sess_t *p_sess, const char *path, pfs_blkio_oflags_e flags)
{
    if (NULL == p_sess || NULL == path || '\0' == path[0])
    {
        return PFS_EINVAL;
    }

    pfs_klua_blkio_sess_done(p_sess);

    if (PFS_BLKIO_RET_OK != pfs_blkio_init(&p_sess->blkio))
    {
        return PFS_EIO;
    }

    int sector_size = pfs_klua_resolve_open_sector_size(path);
    int rc = pfs_blkio_open(&p_sess->blkio, path, flags, sector_size);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_blkio_deinit(&p_sess->blkio);
        return PFS_EIO;
    }

    p_sess->open = 1;
    return PFS_OK;
}

int pfs_klua_blkio_sess_open_rdonly(pfs_klua_blkio_sess_t *p_sess, const char *path)
{
    return pfs_klua_blkio_sess_open(p_sess, path, PFS_BLKIO_O_RDONLY);
}

int pfs_klua_blkio_sess_open_rdwr(pfs_klua_blkio_sess_t *p_sess, const char *path)
{
    return pfs_klua_blkio_sess_open(p_sess, path, PFS_BLKIO_O_RDWR);
}

// end
