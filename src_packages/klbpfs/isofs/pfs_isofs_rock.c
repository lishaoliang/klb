// Doc Encode : UTF-8 BOM, Unix(LF)
// Rock Ridge / SUSP (对照 linux fs/isofs/rock.c; 只读: SP/CE/ER/NM/CL/RE)
#include "isofs/pfs_isofs_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

#define PFS_ISOFS_SIG(a, b)     ((int)(((unsigned)(b) << 8) | (unsigned)(a)))

typedef struct _pfs_isofs_rock_state_t {
    const uint8_t  *chr;
    int             len;
    uint32_t        cont_extent;
    uint32_t        cont_offset;
    uint32_t        cont_size;
    uint8_t        *cont_buf;
} pfs_isofs_rock_state_t;

static void pfs_isofs_rock_setup(const pfs_iso_directory_record_t *p_rec, int rec_len,
        int rock_offset, pfs_isofs_rock_state_t *p_rs)
{
    memset(p_rs, 0, sizeof(*p_rs));
    int name_len = (int)pfs_isonum_711(p_rec->name_len);
    int base = (int)sizeof(pfs_iso_directory_record_t) - 1 + name_len;
    if (0 != (base & 1))
    {
        base++;
    }
    if (rock_offset > 0)
    {
        base += rock_offset;
    }
    if (base > rec_len)
    {
        p_rs->len = 0;
        return;
    }
    p_rs->chr = ((const uint8_t *)p_rec) + base;
    p_rs->len = rec_len - base;
}

static int pfs_isofs_rock_continue(pfs_isofs_sbi_t *p_sbi, pfs_isofs_rock_state_t *p_rs)
{
    if (0 == p_rs->cont_extent || 0 == p_rs->cont_size)
    {
        return 0;
    }
    if (p_rs->cont_size > 4096u)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }
    if (NULL != p_rs->cont_buf)
    {
        pfs_free(p_rs->cont_buf);
        p_rs->cont_buf = NULL;
    }
    p_rs->cont_buf = (uint8_t *)pfs_malloc((size_t)p_rs->cont_size);
    if (NULL == p_rs->cont_buf)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }
    int64_t off = pfs_isofs_lba_to_off(p_sbi, p_rs->cont_extent) + (int64_t)p_rs->cont_offset;
    int nread = pfs_blkio_pread(p_sbi->p_blkio, p_rs->cont_buf, (int)p_rs->cont_size, off);
    if (nread != (int)p_rs->cont_size)
    {
        pfs_free(p_rs->cont_buf);
        p_rs->cont_buf = NULL;
        if (nread < 0)
        {
            return pfs_isofs_map_blkio_rc(nread);
        }
        return PFS_ISOFS_RET_EIO;
    }
    p_rs->chr = p_rs->cont_buf;
    p_rs->len = (int)p_rs->cont_size;
    p_rs->cont_extent = 0;
    p_rs->cont_offset = 0;
    p_rs->cont_size = 0;
    return 1;
}

int pfs_isofs_rock_parse(pfs_isofs_sbi_t *p_sbi, const pfs_iso_directory_record_t *p_rec,
        int rec_len, pfs_isofs_inode_t *p_inode, char *name_out, int name_cap, int *p_has_nm)
{
    pfs_isofs_rock_state_t rs;
    int rock_off;
    int name_len;
    int skip_re;

    if (NULL != p_has_nm)
    {
        *p_has_nm = 0;
    }
    if (NULL == p_sbi || NULL == p_rec || rec_len < 34)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (0 == p_sbi->rock && p_sbi->rock_offset < 0)
    {
        // 探测阶段仍扫 SP/ER
    }

    rock_off = (p_sbi->rock_offset > 0) ? p_sbi->rock_offset : 0;
    pfs_isofs_rock_setup(p_rec, rec_len, rock_off, &rs);
    name_len = 0;
    skip_re = 0;

repeat:
    while (rs.len > 2)
    {
        const uint8_t *rr;
        int sig;
        int rr_len;

        rr = rs.chr;
        rr_len = (int)rr[2];
        if (rr_len < 3)
        {
            break;
        }
        if (rr_len > rs.len)
        {
            break;
        }
        sig = pfs_isonum_721(rr);
        rs.chr += rr_len;
        rs.len -= rr_len;

        if (PFS_ISOFS_SIG('S', 'P') == sig)
        {
            if (rr_len >= 7 && 0xBE == rr[4] && 0xEF == rr[5])
            {
                p_sbi->rock_offset = (int)rr[6];
            }
        }
        else if (PFS_ISOFS_SIG('E', 'R') == sig)
        {
            p_sbi->rock = 1;
        }
        else if (PFS_ISOFS_SIG('C', 'E') == sig)
        {
            if (rr_len >= 28)
            {
                rs.cont_extent = pfs_isonum_733(rr + 4);
                rs.cont_offset = pfs_isonum_733(rr + 12);
                rs.cont_size = pfs_isonum_733(rr + 20);
            }
        }
        else if (PFS_ISOFS_SIG('N', 'M') == sig)
        {
            uint8_t flags;
            int nlen;
            if (rr_len < 5 || NULL == name_out || name_cap <= 1)
            {
                continue;
            }
            flags = rr[4];
            if (0 != (flags & 6))
            {
                // . / ..
                continue;
            }
            nlen = rr_len - 5;
            if (name_len + nlen > name_cap - 1)
            {
                nlen = name_cap - 1 - name_len;
            }
            if (nlen > 0)
            {
                memcpy(name_out + name_len, rr + 5, (size_t)nlen);
                name_len += nlen;
                name_out[name_len] = '\0';
                if (NULL != p_has_nm)
                {
                    *p_has_nm = 1;
                }
            }
        }
        else if (PFS_ISOFS_SIG('C', 'L') == sig)
        {
            uint32_t loc;
            if (rr_len >= 12 && NULL != p_inode && PFS_ISOFS_TYPE_DIR == p_inode->type)
            {
                loc = pfs_isonum_733(rr + 4);
                p_inode->extent = loc;
                p_inode->sec[0].extent = loc;
                // size 仍用原 DE; 目录 listing 在 CL 指向处
            }
        }
        else if (PFS_ISOFS_SIG('R', 'E') == sig)
        {
            skip_re = 1;
        }
    }

    int rc = pfs_isofs_rock_continue(p_sbi, &rs);
    if (rc < 0)
    {
        if (NULL != rs.cont_buf)
        {
            pfs_free(rs.cont_buf);
        }
        return rc;
    }
    if (1 == rc)
    {
        goto repeat;
    }
    if (NULL != rs.cont_buf)
    {
        pfs_free(rs.cont_buf);
    }
    if (skip_re)
    {
        return 1;
    }
    return PFS_ISOFS_RET_OK;
}

int pfs_isofs_rock_detect(pfs_isofs_sbi_t *p_sbi, const pfs_isofs_inode_t *p_root)
{
    uint8_t *blk;
    uint32_t lbs;
    int64_t off;
    const pfs_iso_directory_record_t *p_rec;
    int rec_len;

    if (NULL == p_sbi || NULL == p_root)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    lbs = p_sbi->logical_block_size;
    if (0 == lbs || 0 == p_root->extent)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }
    blk = (uint8_t *)pfs_malloc((size_t)lbs);
    if (NULL == blk)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }
    off = pfs_isofs_lba_to_off(p_sbi, p_root->extent);
    int nread = pfs_blkio_pread(p_sbi->p_blkio, blk, (int)lbs, off);
    if (nread != (int)lbs)
    {
        pfs_free(blk);
        if (nread < 0)
        {
            return pfs_isofs_map_blkio_rc(nread);
        }
        return PFS_ISOFS_RET_EIO;
    }
    p_rec = (const pfs_iso_directory_record_t *)blk;
    rec_len = (int)pfs_isonum_711(p_rec->length);
    if (rec_len < 34 || rec_len > (int)lbs)
    {
        pfs_free(blk);
        return PFS_ISOFS_RET_OK;
    }
    p_sbi->rock = 0;
    p_sbi->rock_offset = -1;
    int rc = pfs_isofs_rock_parse(p_sbi, p_rec, rec_len, NULL, NULL, 0, NULL);
    pfs_free(blk);
    if (rc < 0)
    {
        return rc;
    }
    return PFS_ISOFS_RET_OK;
}

// end
