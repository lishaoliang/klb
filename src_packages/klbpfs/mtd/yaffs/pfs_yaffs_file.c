// Doc Encode : UTF-8 BOM, Unix(LF)
// yaffs2 文件读写 (按 chunk_id; creat + EOF append)
#include "yaffs/pfs_yaffs_internal.h"
#include "base/pfsmtd_mem.h"

#include <string.h>
#include <stdbool.h>

static pfs_yaffs_dchunk_t *pfs_yaffs_find_chunk(pfs_yaffs_obj_t *p_obj, uint32_t chunk_id)
{
    pfs_yaffs_dchunk_t *p_ch;

    if (NULL == p_obj || chunk_id < 1)
    {
        return NULL;
    }
    for (p_ch = p_obj->chunks; NULL != p_ch; p_ch = p_ch->next)
    {
        if (p_ch->chunk_id == chunk_id)
        {
            return p_ch;
        }
    }
    return NULL;
}

int pfs_yaffs_file_open(pfs_yaffs_sbi_t *p_sbi, const char *path, int mode, pfs_yaffs_file_meta_t *p_fm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_yaffs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_fm, 0, sizeof(*p_fm));

    int want_create = (0 != (mode & PFS_O_CREAT)) ? 1 : 0;
    int want_write = (0 != (mode & PFS_O_RDWR)) ? 1 : 0;

    if (want_create && !want_write)
    {
        return PFS_EINVAL;
    }
    if ((want_create || want_write) && !pfs_yaffs_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }

    uint32_t obj_id;
    bool is_dir;

    int rc = pfs_yaffs_lookup_path(p_sbi, path, &obj_id, &is_dir);
    if (PFS_ENOENT == rc && want_create)
    {
        rc = pfs_yaffs_creat(p_sbi, path, &obj_id);
        if (PFS_OK != rc)
        {
            return rc;
        }
        is_dir = false;
    }
    else if (PFS_OK != rc)
    {
        return rc;
    }
    if (is_dir)
    {
        return PFS_EINVAL;
    }

    pfs_yaffs_obj_t *p_obj = pfs_yaffs_obj_find(p_sbi, obj_id);
    if (NULL == p_obj || PFS_YAFFS_OBJECT_TYPE_FILE != p_obj->type)
    {
        return PFS_EINVAL;
    }

    p_fm->obj_id = obj_id;
    p_fm->file_size = p_obj->file_size;
    p_fm->writable = want_write;
    return PFS_OK;
}

void pfs_yaffs_file_close(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL != p_fm)
    {
        memset(p_fm, 0, sizeof(*p_fm));
    }
}

int pfs_yaffs_file_read(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_got)
    {
        return PFS_EINVAL;
    }
    *p_got = 0;
    if (NULL == p_sbi || NULL == p_fm || NULL == buf || size < 0 || 0 > pos)
    {
        return PFS_EINVAL;
    }
    if (!pfs_yaffs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 == size)
    {
        return PFS_OK;
    }
    if (pos >= (int64_t)p_fm->file_size)
    {
        return PFS_OK;
    }

    int64_t remain = (int64_t)p_fm->file_size - pos;

    if (remain > (int64_t)size)
    {
        remain = (int64_t)size;
    }

    pfs_yaffs_obj_t *p_obj = pfs_yaffs_obj_find(p_sbi, p_fm->obj_id);
    if (NULL == p_obj)
    {
        return PFS_ENOENT;
    }

    uint8_t *pagebuf = (uint8_t *)pfsmtd_malloc((size_t)p_sbi->chunk_size);

    if (NULL == pagebuf)
    {
        return PFS_ENOMEM;
    }

    uint8_t *out = (uint8_t *)buf;
    int total = 0;

    while (remain > 0)
    {
        uint32_t chunk_id = (uint32_t)(pos / (int64_t)p_sbi->chunk_size) + 1u;
        uint32_t off_in_chunk = (uint32_t)(pos % (int64_t)p_sbi->chunk_size);
        pfs_yaffs_dchunk_t *p_ch = pfs_yaffs_find_chunk(p_obj, chunk_id);
        if (NULL == p_ch || 0 == p_ch->n_bytes)
        {
            // 稀疏空洞
            out[0] = 0;
            out++;
            pos++;
            remain--;
            total++;
            continue;
        }

        int64_t abs_off = p_sbi->part_offset + (int64_t)p_ch->page * (int64_t)p_sbi->page_size;
        int n = pfs_blkio_pread(p_sbi->p_blkio, pagebuf, (int)p_sbi->chunk_size, abs_off);
        if (n != p_sbi->chunk_size)
        {
            pfsmtd_free(pagebuf);
            if (n < 0)
            {
                return pfs_yaffs_map_blkio_rc(n);
            }
            return PFS_EIO;
        }

        uint32_t avail = p_ch->n_bytes;
        if (off_in_chunk >= avail)
        {
            out[0] = 0;
            out++;
            pos++;
            remain--;
            total++;
            continue;
        }

        uint32_t take = avail - off_in_chunk;
        if ((int64_t)take > remain)
        {
            take = (uint32_t)remain;
        }
        memcpy(out, pagebuf + off_in_chunk, (size_t)take);
        out += take;
        pos += (int64_t)take;
        remain -= (int64_t)take;
        total += (int)take;
    }

    pfsmtd_free(pagebuf);
    *p_got = total;
    return PFS_OK;
}

int pfs_yaffs_file_write(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put)
{
    if (NULL == p_put)
    {
        return PFS_EINVAL;
    }
    *p_put = 0;
    if (NULL == p_sbi || NULL == p_fm || NULL == buf || size < 0 || 0 > pos)
    {
        return PFS_EINVAL;
    }
    if (!p_fm->writable || !pfs_yaffs_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }
    if (0 == size)
    {
        return PFS_OK;
    }
    if (pos != (int64_t)p_fm->file_size)
    {
        return PFS_EINVAL;
    }

    pfs_yaffs_obj_t *p_obj = pfs_yaffs_obj_find(p_sbi, p_fm->obj_id);
    if (NULL == p_obj || PFS_YAFFS_OBJECT_TYPE_FILE != p_obj->type)
    {
        return PFS_ENOENT;
    }

    uint32_t mode = p_obj->mode;

    if (0 == (mode & PFS_YAFFS_S_IFMT))
    {
        mode = PFS_YAFFS_S_IFREG | 0644;
    }

    uint8_t *pagebuf = (uint8_t *)pfsmtd_malloc((size_t)p_sbi->chunk_size);
    if (NULL == pagebuf)
    {
        return PFS_ENOMEM;
    }

    const uint8_t *in = (const uint8_t *)buf;
    int left = size;

    while (left > 0)
    {
        uint32_t chunk_id = (p_fm->file_size / (uint32_t)p_sbi->chunk_size) + 1u;
        uint32_t off = p_fm->file_size % (uint32_t)p_sbi->chunk_size;
        uint32_t take = (uint32_t)p_sbi->chunk_size - off;
        if (take > (uint32_t)left)
        {
            take = (uint32_t)left;
        }

        memset(pagebuf, 0xff, (size_t)p_sbi->chunk_size);
        if (off > 0)
        {
            pfs_yaffs_dchunk_t *p_ch = pfs_yaffs_find_chunk(p_obj, chunk_id);
            if (NULL != p_ch && p_ch->n_bytes > 0)
            {
                int64_t abs_off = p_sbi->part_offset + (int64_t)p_ch->page * (int64_t)p_sbi->page_size;
                int n = pfs_blkio_pread(p_sbi->p_blkio, pagebuf, (int)p_sbi->chunk_size, abs_off);
                if (n != p_sbi->chunk_size)
                {
                    pfsmtd_free(pagebuf);
                    if (n < 0)
                    {
                        return pfs_yaffs_map_blkio_rc(n);
                    }
                    return PFS_EIO;
                }
            }
            else
            {
                memset(pagebuf, 0, (size_t)off);
            }
        }
        memcpy(pagebuf + off, in, (size_t)take);

        uint32_t n_bytes = off + take;
        int rc = pfs_yaffs_emit_page(p_sbi, p_fm->obj_id, chunk_id, n_bytes, pagebuf);
        if (PFS_OK != rc)
        {
            pfsmtd_free(pagebuf);
            return rc;
        }
        in += take;
        left -= (int)take;
        p_fm->file_size += take;
        p_obj->file_size = p_fm->file_size;
    }

    pfsmtd_free(pagebuf);

    int rc = pfs_yaffs_emit_obj_hdr(p_sbi, p_fm->obj_id, PFS_YAFFS_OBJECT_TYPE_FILE, p_obj->parent_id, mode, p_fm->file_size, p_obj->name);
    if (PFS_OK != rc)
    {
        return rc;
    }
    *p_put = size;
    return PFS_OK;
}

int pfs_yaffs_file_truncate(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_file_meta_t *p_fm, int64_t size)
{
    (void)p_sbi;
    (void)p_fm;
    (void)size;
    return PFS_EPERM;
}

int pfs_yaffs_file_sync(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_file_meta_t *p_fm)
{
    (void)p_fm;
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (pfs_yaffs_sbi_can_write(p_sbi))
    {
        return pfs_yaffs_sync_media(p_sbi);
    }
    return PFS_OK;
}

int64_t pfs_yaffs_file_get_size(pfs_yaffs_sbi_t *p_sbi, const pfs_yaffs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL == p_fm)
    {
        return -1;
    }
    return (int64_t)p_fm->file_size;
}

// end
