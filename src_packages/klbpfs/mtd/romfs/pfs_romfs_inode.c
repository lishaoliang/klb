// Doc Encode : UTF-8 BOM, Unix(LF)
// romfs super/filehdr 读 (对照 fs/romfs/super.c; BE + 16 对齐)
#include "romfs/pfs_romfs_internal.h"

#include <string.h>

#define PFS_ROMFS_HRD_MAX_DEPTH         32

int pfs_romfs_read_super(pfs_blkio_t *p_blkio, int64_t part_offset, uint32_t *p_img_size, uint32_t *p_root_pos)
{
    if (NULL == p_blkio || NULL == p_img_size || NULL == p_root_pos || 0 > part_offset)
    {
        return PFS_EINVAL;
    }

    uint8_t buf[PFS_ROMFS_CHECKSUM_BYTES];
    int n = pfs_blkio_pread(p_blkio, buf, (int)sizeof(buf), part_offset);
    if (n < PFS_ROMFS_FH_SIZE)
    {
        if (0 > n)
        {
            return pfs_romfs_map_blkio_rc(n);
        }
        return PFS_ENOEXEC;
    }
    if (0 != memcmp(buf, PFS_ROMFS_MAGIC_WORD, 8))
    {
        return PFS_ENOEXEC;
    }

    uint32_t img_size = pfs_romfs_be32(buf + 8);
    if (PFS_ROMFS_FH_SIZE > img_size)
    {
        return PFS_ENOEXEC;
    }

    uint32_t words = (uint32_t)n / 4u;
    if (img_size < (uint32_t)n)
    {
        words = img_size / 4u;
    }
    uint32_t csum = 0;
    for (uint32_t i = 0; i < words; i++)
    {
        csum += pfs_romfs_be32(buf + i * 4u);
    }
    if (0 != csum)
    {
        return PFS_ENOEXEC;
    }

    int vlen = 0;
    {
        int maxv = n - 16;
        if (maxv > PFS_ROMFS_MAXFN)
        {
            maxv = PFS_ROMFS_MAXFN;
        }
        while (vlen < maxv && '\0' != buf[16 + vlen])
        {
            vlen++;
        }
    }
    uint32_t root_pos = pfs_romfs_align16(16u + (uint32_t)vlen + 1u);
    if (root_pos + PFS_ROMFS_FH_SIZE > img_size)
    {
        return PFS_ENOEXEC;
    }

    *p_img_size = img_size;
    *p_root_pos = root_pos;
    return PFS_OK;
}

int pfs_romfs_read_fh(pfs_romfs_sbi_t *p_sbi, uint32_t pos, pfs_romfs_fh_t *p_fh, char *name, int name_cap, int *p_nlen)
{
    if (NULL == p_sbi || NULL == p_fh)
    {
        return PFS_EINVAL;
    }
    if (0 != (pos & 15u) || pos + PFS_ROMFS_FH_SIZE > p_sbi->img_size)
    {
        return PFS_EBADMSG;
    }

    uint8_t hdr[PFS_ROMFS_FH_SIZE];
    uint8_t nbuf[PFS_ROMFS_MAXFN + 1];
    int n = pfs_blkio_pread(p_sbi->p_blkio, hdr, PFS_ROMFS_FH_SIZE, p_sbi->part_offset + (int64_t)pos);
    if (n != PFS_ROMFS_FH_SIZE)
    {
        if (0 > n)
        {
            return pfs_romfs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }

    uint32_t next_raw = pfs_romfs_be32(hdr + 0);
    p_fh->pos = pos;
    p_fh->next = next_raw & PFS_ROMFS_FH_MASK;
    p_fh->type = (uint8_t)(next_raw & PFS_ROMFH_TYPE);
    p_fh->spec = pfs_romfs_be32(hdr + 4);
    p_fh->size = pfs_romfs_be32(hdr + 8);

    int max_name = (int)(p_sbi->img_size - pos - PFS_ROMFS_FH_SIZE);
    if (max_name > PFS_ROMFS_MAXFN)
    {
        max_name = PFS_ROMFS_MAXFN;
    }
    if (0 > max_name)
    {
        return PFS_EBADMSG;
    }

    int nlen = 0;
    if (0 < max_name)
    {
        int to_read = max_name + 1;
        if (pos + PFS_ROMFS_FH_SIZE + (uint32_t)to_read > p_sbi->img_size)
        {
            to_read = (int)(p_sbi->img_size - pos - PFS_ROMFS_FH_SIZE);
        }
        if (1 > to_read)
        {
            return PFS_EBADMSG;
        }
        n = pfs_blkio_pread(p_sbi->p_blkio, nbuf, to_read, p_sbi->part_offset + (int64_t)pos + PFS_ROMFS_FH_SIZE);
        if (1 > n)
        {
            if (0 > n)
            {
                return pfs_romfs_map_blkio_rc(n);
            }
            return PFS_EIO;
        }
        while (nlen < n && nlen < PFS_ROMFS_MAXFN && '\0' != nbuf[nlen])
        {
            nlen++;
        }
        if (PFS_ROMFS_MAXFN <= nlen && (nlen >= n || '\0' != nbuf[nlen]))
        {
            return PFS_ENAMETOOLONG;
        }
        // 须有结尾 NUL (对照 romfs_dev_strcmp)
        if (nlen >= n || '\0' != nbuf[nlen])
        {
            return PFS_EBADMSG;
        }
    }

    p_fh->meta_sz = pfs_romfs_align16(PFS_ROMFS_FH_SIZE + (uint32_t)nlen + 1u);
    p_fh->data_off = pos + p_fh->meta_sz;
    if (p_fh->data_off > p_sbi->img_size)
    {
        return PFS_EBADMSG;
    }
    if ((PFS_ROMFH_REG == p_fh->type || PFS_ROMFH_SYM == p_fh->type) && p_fh->data_off + p_fh->size > p_sbi->img_size)
    {
        return PFS_EBADMSG;
    }

    if (NULL != p_nlen)
    {
        *p_nlen = nlen;
    }
    if (NULL != name && name_cap > 0)
    {
        int ncopy = nlen;
        if (ncopy >= name_cap)
        {
            ncopy = name_cap - 1;
        }
        if (ncopy > 0)
        {
            memcpy(name, nbuf, (size_t)ncopy);
        }
        name[ncopy] = '\0';
    }
    return PFS_OK;
}

int pfs_romfs_resolve_fh(pfs_romfs_sbi_t *p_sbi, uint32_t pos, pfs_romfs_fh_t *p_fh)
{
    if (NULL == p_sbi || NULL == p_fh)
    {
        return PFS_EINVAL;
    }

    for (int depth = 0; depth < PFS_ROMFS_HRD_MAX_DEPTH; depth++)
    {
        int rc = pfs_romfs_read_fh(p_sbi, pos, p_fh, NULL, 0, NULL);
        if (PFS_OK != rc)
        {
            return rc;
        }
        if (PFS_ROMFH_HRD != p_fh->type)
        {
            return PFS_OK;
        }
        pos = p_fh->spec & PFS_ROMFS_FH_MASK;
        if (0 == pos)
        {
            return PFS_EBADMSG;
        }
    }
    return PFS_EBADMSG;
}

// end
