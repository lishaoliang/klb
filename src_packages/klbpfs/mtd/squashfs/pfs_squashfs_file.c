// Doc Encode : UTF-8 BOM, Unix(LF)
// squashfs 文件只读 (对照 file.c 简化: 顺序读 block_list + fragment)
#include "squashfs/pfs_squashfs_internal.h"

#include <string.h>

static uint32_t pfs_squashfs_file_block_count(const pfs_squashfs_sbi_t *p_sbi,
        const pfs_squashfs_inode_info_t *p_info)
{
    uint64_t size = p_info->file_size;

    if (PFS_SQUASHFS_INVALID_FRAG != p_info->fragment)
    {
        return (uint32_t)(size >> p_sbi->block_log);
    }
    if (0 == size)
    {
        return 0;
    }
    return (uint32_t)((size + p_sbi->block_size - 1u) >> p_sbi->block_log);
}

static int pfs_squashfs_read_blocklist_size(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm,
        uint32_t index, uint64_t *p_disk_block, int *p_size_code)
{
    uint64_t block = p_fm->info.block_list_block;
    int offset = p_fm->info.block_list_offset;
    uint64_t disk = p_fm->info.start;

    for (uint32_t i = 0; i <= index; i++)
    {
        uint8_t raw[4];
        int rc = pfs_squashfs_read_metadata(p_sbi, raw, &block, &offset, 4);
        if (rc < 0)
        {
            return rc;
        }

        int size_code = pfs_squashfs_block_size_u32(pfs_squashfs_le32(raw));
        if (size_code < 0)
        {
            return PFS_EIO;
        }
        if (i == index)
        {
            *p_disk_block = disk;
            *p_size_code = size_code;
            return PFS_OK;
        }
        if (0 != size_code)
        {
            disk += (uint64_t)PFS_SQUASHFS_COMPRESSED_SIZE_BLOCK((unsigned int)size_code);
        }
    }
    return PFS_EIO;
}

static int pfs_squashfs_ensure_data_block(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm,
        uint32_t index, int *p_data_len)
{
    if (p_sbi->data_buf_ino == p_fm->info.ino && p_sbi->data_buf_index == (int)index
            && p_sbi->data_buf_index >= 0)
    {
        *p_data_len = (int)p_sbi->block_size;
        return PFS_OK;
    }

    uint64_t disk;
    int size_code;
    int out_len;

    int rc = pfs_squashfs_read_blocklist_size(p_sbi, p_fm, index, &disk, &size_code);
    if (PFS_OK != rc)
    {
        return rc;
    }

    rc = pfs_squashfs_read_data_block(p_sbi, disk, size_code, p_sbi->data_buf, (int)p_sbi->block_size,
            &out_len);
    if (PFS_OK != rc)
    {
        p_sbi->data_buf_index = -1;
        return rc;
    }

    p_sbi->data_buf_ino = p_fm->info.ino;
    p_sbi->data_buf_index = (int)index;
    *p_data_len = out_len;
    return PFS_OK;
}

int pfs_squashfs_file_open(pfs_squashfs_sbi_t *p_sbi, const char *path, int mode, pfs_squashfs_file_meta_t *p_fm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_squashfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 != (mode & (PFS_O_RDWR | PFS_O_CREAT)))
    {
        return PFS_EPERM;
    }


    memset(p_fm, 0, sizeof(*p_fm));
    int rc = pfs_squashfs_lookup_path(p_sbi, path, &p_fm->info);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!pfs_squashfs_inode_is_reg(&p_fm->info))
    {
        return PFS_EINVAL;
    }
    return PFS_OK;
}

void pfs_squashfs_file_close(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm)
{
    if (NULL != p_sbi && NULL != p_fm && p_sbi->data_buf_ino == p_fm->info.ino)
    {
        p_sbi->data_buf_index = -1;
        p_sbi->data_buf_ino = 0;
    }
    if (NULL != p_fm)
    {
        memset(p_fm, 0, sizeof(*p_fm));
    }
}

int pfs_squashfs_file_read(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm, int64_t pos, void *buf,
        int size, int *p_got)
{
    if (NULL == p_sbi || NULL == p_fm || NULL == buf || NULL == p_got || size < 0)
    {
        return PFS_EINVAL;
    }
    *p_got = 0;
    if (0 > pos)
    {
        return PFS_EINVAL;
    }

    uint64_t fsize = p_fm->info.file_size;

    if ((uint64_t)pos >= fsize || 0 == size)
    {
        return PFS_OK;
    }
    if ((uint64_t)pos + (uint64_t)size > fsize)
    {
        size = (int)(fsize - (uint64_t)pos);
    }

    uint8_t *dst = (uint8_t *)buf;
    int remain = size;
    int got = 0;
    uint32_t nblocks = pfs_squashfs_file_block_count(p_sbi, &p_fm->info);

    while (remain > 0)
    {
        uint64_t cur = (uint64_t)pos + (uint64_t)got;
        uint32_t blk_index = (uint32_t)(cur >> p_sbi->block_log);
        uint32_t off_in = (uint32_t)(cur & ((uint64_t)p_sbi->block_size - 1u));
        int chunk;
        int data_len;

        if (blk_index < nblocks)
        {
            int rc = pfs_squashfs_ensure_data_block(p_sbi, p_fm, blk_index, &data_len);
            if (PFS_OK != rc)
            {
                return rc;
            }
            if (off_in >= (uint32_t)data_len)
            {
                return PFS_EIO;
            }
            chunk = data_len - (int)off_in;
            if (chunk > remain)
            {
                chunk = remain;
            }
            // 文件尾块可能短于 block_size
            if (cur + (uint64_t)chunk > fsize)
            {
                chunk = (int)(fsize - cur);
            }
            memcpy(dst + got, p_sbi->data_buf + off_in, (size_t)chunk);
            got += chunk;
            remain -= chunk;
            continue;
        }

        // fragment 尾
        if (PFS_SQUASHFS_INVALID_FRAG == p_fm->info.fragment
                || (uint64_t)PFS_SQUASHFS_INVALID_BLK == p_fm->info.frag_block)
        {
            break;
        }

        int frag_out;
        uint32_t frag_off = p_fm->info.offset;
        uint32_t in_frag;

        int rc = pfs_squashfs_read_data_block(p_sbi, p_fm->info.frag_block, p_fm->info.frag_size,
                p_sbi->data_buf, (int)p_sbi->block_size, &frag_out);
        if (PFS_OK != rc)
        {
            return rc;
        }
        in_frag = (uint32_t)(cur - ((uint64_t)nblocks << p_sbi->block_log));
        if (frag_off + in_frag >= (uint32_t)frag_out)
        {
            break;
        }
        chunk = frag_out - (int)(frag_off + in_frag);
        if (chunk > remain)
        {
            chunk = remain;
        }
        if (cur + (uint64_t)chunk > fsize)
        {
            chunk = (int)(fsize - cur);
        }
        memcpy(dst + got, p_sbi->data_buf + frag_off + in_frag, (size_t)chunk);
        p_sbi->data_buf_index = -1;
        got += chunk;
        remain -= chunk;
    }

    *p_got = got;
    return PFS_OK;
}

int pfs_squashfs_file_write(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm, int64_t pos,
        const void *buf, int size, int *p_put)
{
    (void)p_sbi;
    (void)p_fm;
    (void)pos;
    (void)buf;
    (void)size;
    (void)p_put;
    return PFS_EPERM;
}

int pfs_squashfs_file_truncate(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm, int64_t size)
{
    (void)p_sbi;
    (void)p_fm;
    (void)size;
    return PFS_EPERM;
}

int pfs_squashfs_file_sync(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm)
{
    (void)p_sbi;
    (void)p_fm;
    return PFS_OK;
}

int64_t pfs_squashfs_file_get_size(pfs_squashfs_sbi_t *p_sbi, const pfs_squashfs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL == p_fm)
    {
        return -1;
    }
    return (int64_t)p_fm->info.file_size;
}

// end
