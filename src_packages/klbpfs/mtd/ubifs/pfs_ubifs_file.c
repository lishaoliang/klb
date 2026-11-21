// Doc Encode : UTF-8 BOM, Unix(LF)
// ubifs 文件读写 (creat + 按块 append; COMPR_NONE)
#include "ubifs/pfs_ubifs_internal.h"
#include "base/pfsmtd_mem.h"

#include <string.h>
#include <stdbool.h>

int pfs_ubifs_file_open(pfs_ubifs_sbi_t *p_sbi, const char *path, int mode, pfs_ubifs_file_meta_t *p_fm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_ubifs_sbi_is_mounted(p_sbi))
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
    if ((want_create || want_write) && !pfs_ubifs_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }

    uint64_t inum;
    bool is_dir;

    int rc = pfs_ubifs_lookup_path(p_sbi, path, &inum, &is_dir);
    if (PFS_ENOENT == rc && want_create)
    {
        rc = pfs_ubifs_creat(p_sbi, path, &inum);
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

    pfs_ubifs_ic_t *ic = pfs_ubifs_ic_find(p_sbi, inum);
    if (NULL == ic || PFS_UBIFS_S_IFREG != (ic->mode & PFS_UBIFS_S_IFMT))
    {
        return PFS_EINVAL;
    }
    p_fm->inum = inum;
    p_fm->size = ic->size;
    p_fm->writable = want_write;
    return PFS_OK;
}

void pfs_ubifs_file_close(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL != p_fm)
    {
        memset(p_fm, 0, sizeof(*p_fm));
    }
}

int pfs_ubifs_file_read(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got)
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
    if (0 == size || pos >= (int64_t)p_fm->size)
    {
        return PFS_OK;
    }

    int64_t remain = (int64_t)p_fm->size - pos;

    if (remain > (int64_t)size)
    {
        remain = (int64_t)size;
    }

    uint8_t *out = (uint8_t *)buf;
    int total = 0;
    uint8_t *tmp = (uint8_t *)pfsmtd_malloc(PFS_UBIFS_BLOCK_SIZE);

    if (NULL == tmp)
    {
        return PFS_ENOMEM;
    }

    while (remain > 0)
    {
        uint32_t block = (uint32_t)(pos / PFS_UBIFS_BLOCK_SIZE);
        uint32_t off_in_blk = (uint32_t)(pos % PFS_UBIFS_BLOCK_SIZE);
        pfs_ubifs_dref_t *dr = NULL;
        for (int i = 0; i < PFS_UBIFS_CACHE_DATA_MAX; i++)
        {
            if (p_sbi->p_drefs[i].used && p_sbi->p_drefs[i].inum == p_fm->inum && p_sbi->p_drefs[i].block == block)
            {
                dr = &p_sbi->p_drefs[i];
                break;
            }
        }
        memset(tmp, 0, PFS_UBIFS_BLOCK_SIZE);
        if (NULL != dr && dr->size > 0)
        {
            int rc = pfs_ubifs_leb_read(p_sbi, dr->lnum, dr->offs + dr->data_offs, tmp, (int)dr->size);
            if (PFS_OK != rc)
            {
                pfsmtd_free(tmp);
                return rc;
            }
        }

        int take = PFS_UBIFS_BLOCK_SIZE - (int)off_in_blk;
        if ((int64_t)take > remain)
        {
            take = (int)remain;
        }
        memcpy(out, tmp + off_in_blk, (size_t)take);
        out += take;
        pos += take;
        remain -= take;
        total += take;
    }

    pfsmtd_free(tmp);
    *p_got = total;
    return PFS_OK;
}

int pfs_ubifs_file_write(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put)
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
    if (!p_fm->writable || !pfs_ubifs_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }
    if (0 == size)
    {
        return PFS_OK;
    }
    if (pos != (int64_t)p_fm->size)
    {
        return PFS_EINVAL;
    }
    if (0 != (p_fm->size % PFS_UBIFS_BLOCK_SIZE))
    {
        return PFS_EINVAL;
    }

    const uint8_t *in = (const uint8_t *)buf;
    int left = size;

    while (left > 0)
    {
        uint32_t block = (uint32_t)(p_fm->size / PFS_UBIFS_BLOCK_SIZE);
        int chunk = left;
        if (chunk > PFS_UBIFS_BLOCK_SIZE)
        {
            chunk = PFS_UBIFS_BLOCK_SIZE;
        }

        int nlen = PFS_UBIFS_DATA_NODE_SZ + chunk;
        uint8_t *node = (uint8_t *)pfsmtd_mallocz((size_t)PFS_UBIFS_ALIGN8(nlen));
        if (NULL == node)
        {
            return PFS_ENOMEM;
        }

        pfs_ubifs_key_t key;

        pfs_ubifs_data_key_init(&key, p_fm->inum, block);
        pfs_ubifs_key_write_flash(node + PFS_UBIFS_OFF_KEY, &key);
        pfs_ubifs_put_le32(node + PFS_UBIFS_DATA_OFF_SIZE, (uint32_t)chunk);
        pfs_ubifs_put_le16(node + PFS_UBIFS_DATA_OFF_COMPR, PFS_UBIFS_COMPR_NONE);
        memcpy(node + PFS_UBIFS_DATA_OFF_DATA, in, (size_t)chunk);
        node[20] = PFS_UBIFS_DATA_NODE;

        int wrote_at_lnum = p_sbi->write_lnum;
        int wrote_at_offs = p_sbi->write_offs;
        // 预判跨 LEB
        if (wrote_at_offs + PFS_UBIFS_ALIGN8(nlen) > p_sbi->leb_size)
        {
            wrote_at_lnum = p_sbi->write_lnum + 1;
            wrote_at_offs = 0;
        }

        int rc = pfs_ubifs_append_node(p_sbi, node, nlen);
        if (PFS_OK == rc)
        {
            rc = pfs_ubifs_dref_upsert(p_sbi, p_fm->inum, block, (uint32_t)chunk,
                    wrote_at_lnum, wrote_at_offs, PFS_UBIFS_DATA_OFF_DATA, p_sbi->max_sqnum);
        }
        pfsmtd_free(node);
        if (PFS_OK != rc)
        {
            return rc;
        }
        in += chunk;
        left -= chunk;
        p_fm->size += (uint64_t)chunk;
    }

    pfs_ubifs_ic_t *ic = pfs_ubifs_ic_find(p_sbi, p_fm->inum);
    uint32_t mode = (NULL != ic) ? ic->mode : (PFS_UBIFS_S_IFREG | 0644);
    uint32_t nlink = (NULL != ic) ? ic->nlink : 1;
    uint8_t *ino = (uint8_t *)pfsmtd_mallocz(PFS_UBIFS_INO_NODE_SZ);
    if (NULL == ino)
    {
        return PFS_ENOMEM;
    }

    pfs_ubifs_key_t key;

    pfs_ubifs_ino_key_init(&key, p_fm->inum);
    pfs_ubifs_key_write_flash(ino + PFS_UBIFS_OFF_KEY, &key);
    pfs_ubifs_put_le64(ino + PFS_UBIFS_INO_OFF_SIZE, p_fm->size);
    pfs_ubifs_put_le32(ino + PFS_UBIFS_INO_OFF_NLINK, nlink);
    pfs_ubifs_put_le32(ino + PFS_UBIFS_INO_OFF_MODE, mode);
    pfs_ubifs_put_le16(ino + PFS_UBIFS_INO_OFF_COMPR, PFS_UBIFS_COMPR_NONE);
    ino[20] = PFS_UBIFS_INO_NODE;
    int rc = pfs_ubifs_append_node(p_sbi, ino, PFS_UBIFS_INO_NODE_SZ);
    if (PFS_OK == rc)
    {
        rc = pfs_ubifs_ic_upsert(p_sbi, p_fm->inum, mode, p_fm->size, nlink, p_sbi->max_sqnum);
    }
    pfsmtd_free(ino);
    if (PFS_OK != rc)
    {
        return rc;
    }

    *p_put = size;
    return PFS_OK;
}

int pfs_ubifs_file_truncate(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_file_meta_t *p_fm, int64_t size)
{
    (void)p_sbi;
    (void)p_fm;
    (void)size;
    return PFS_EPERM;
}

int pfs_ubifs_file_sync(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_file_meta_t *p_fm)
{
    (void)p_fm;
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (pfs_ubifs_sbi_can_write(p_sbi))
    {
        return pfs_ubifs_flush_mst(p_sbi);
    }
    return PFS_OK;
}

int64_t pfs_ubifs_file_get_size(pfs_ubifs_sbi_t *p_sbi, const pfs_ubifs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL == p_fm)
    {
        return -1;
    }
    return (int64_t)p_fm->size;
}

// end
