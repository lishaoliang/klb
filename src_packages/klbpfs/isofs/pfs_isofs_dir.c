// Doc Encode : UTF-8 BOM, Unix(LF)
// ISO 9660 目录扫描 / 名解码 / 多 extent / 父目录 (只读)
#include "isofs/pfs_isofs_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

int pfs_isofs_normalize_name(const char *raw, int raw_len, char *name_out, int name_cap)
{
    if (NULL == raw || 0 > raw_len || NULL == name_out || 1 >= name_cap)
    {
        return PFS_ISOFS_RET_EINVAL;
    }

    int semi = -1;
    for (int i = 0; i < raw_len; i++)
    {
        if (';' == raw[i])
        {
            semi = i;
            break;
        }
    }
    if (0 <= semi)
    {
        raw_len = semi;
    }
    while (raw_len > 0 && (' ' == raw[raw_len - 1] || '\0' == raw[raw_len - 1]))
    {
        raw_len--;
    }
    if (raw_len > 0 && '.' == raw[raw_len - 1])
    {
        raw_len--;
    }

    int out_len = 0;
    for (int i = 0; i < raw_len && out_len < name_cap - 1; i++)
    {
        unsigned char c = (unsigned char)raw[i];
        if (c >= 'A' && c <= 'Z')
        {
            c = (unsigned char)(c - 'A' + 'a');
        }
        name_out[out_len] = (char)c;
        out_len++;
    }
    name_out[out_len] = '\0';
    return out_len;
}

bool pfs_isofs_name_equal(const pfs_isofs_sbi_t *p_sbi, const char *a, const char *b)
{
    if (NULL == a || NULL == b)
    {
        return false;
    }
    if (NULL != p_sbi && p_sbi->joliet_level > 0 && 0 == p_sbi->rock)
    {
        while ('\0' != *a && '\0' != *b)
        {
            unsigned char ca = (unsigned char)*a;
            unsigned char cb = (unsigned char)*b;
            if (ca >= 'A' && ca <= 'Z')
            {
                ca = (unsigned char)(ca - 'A' + 'a');
            }
            if (cb >= 'A' && cb <= 'Z')
            {
                cb = (unsigned char)(cb - 'A' + 'a');
            }
            if (ca != cb)
            {
                return false;
            }
            a++;
            b++;
        }
        return ('\0' == *a && '\0' == *b);
    }
    return (0 == strcmp(a, b));
}

int pfs_isofs_decode_name(pfs_isofs_sbi_t *p_sbi, const pfs_iso_directory_record_t *p_rec,
        int rec_len, char *name_out, int name_cap)
{
    if (NULL == p_sbi || NULL == p_rec || NULL == name_out || 1 >= name_cap)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    int name_len = (int)pfs_isonum_711(p_rec->name_len);
    if (1 == name_len && (0 == p_rec->name[0] || 1 == p_rec->name[0]))
    {
        return 1;
    }
    if (0 != (pfs_isonum_711(p_rec->flags) & PFS_ISO_FILE_ASSOCIATED))
    {
        return 1;
    }

    name_out[0] = '\0';
    int has_nm = 0;
    pfs_isofs_inode_t tmp;
    pfs_isofs_inode_from_rec(p_rec, &tmp);

    int rc;
    if (p_sbi->rock)
    {
        rc = pfs_isofs_rock_parse(p_sbi, p_rec, rec_len, &tmp, name_out, name_cap, &has_nm);
        if (0 > rc)
        {
            return rc;
        }
        if (1 == rc)
        {
            return 1;                                   // RE
        }
        if (has_nm && '\0' != name_out[0])
        {
            return 0;
        }
    }

    if (p_sbi->joliet_level > 0 && 0 == p_sbi->rock)
    {
        rc = pfs_isofs_joliet_name(p_rec->name, name_len, name_out, name_cap);
        if (0 > rc)
        {
            return rc;
        }
        return 0;
    }

    (void)pfs_isofs_normalize_name(p_rec->name, name_len, name_out, name_cap);
    return 0;
}

static int pfs_isofs_read_dir_block(pfs_isofs_sbi_t *p_sbi, uint32_t dir_extent,
        uint32_t block_off, uint8_t *blk, uint32_t lbs)
{
    int64_t off = pfs_isofs_lba_to_off(p_sbi, dir_extent + block_off);
    int nread = pfs_blkio_pread(p_sbi->p_blkio, blk, (int)lbs, off);
    if (nread != (int)lbs)
    {
        if (0 > nread)
        {
            return pfs_isofs_map_blkio_rc(nread);
        }
        return PFS_ISOFS_RET_EIO;
    }
    return PFS_ISOFS_RET_OK;
}

int pfs_isofs_expand_sections(pfs_isofs_sbi_t *p_sbi, const pfs_isofs_inode_t *p_dir,
        uint32_t byte_pos, pfs_isofs_inode_t *p_out, uint32_t *p_end_pos)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == p_out || NULL == p_end_pos)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    uint32_t lbs = p_sbi->logical_block_size;
    if (0 == lbs || PFS_ISOFS_TYPE_DIR != p_dir->type)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }
    uint8_t *blk = (uint8_t *)pfs_malloc((size_t)lbs);
    if (NULL == blk)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }

    memset(p_out, 0, sizeof(*p_out));
    uint32_t pos = byte_pos;
    uint32_t total = 0;
    int nsec = 0;
    int more = 1;

    while (more && pos < p_dir->size)
    {
        uint32_t block_off = pos / lbs;
        uint32_t in_blk = pos % lbs;
        int rc = pfs_isofs_read_dir_block(p_sbi, p_dir->extent, block_off, blk, lbs);
        if (0 > rc)
        {
            pfs_free(blk);
            return rc;
        }
        const pfs_iso_directory_record_t *p_rec =
                (const pfs_iso_directory_record_t *)(blk + in_blk);
        int rec_len = (int)pfs_isonum_711(p_rec->length);
        if (0 == rec_len)
        {
            pos = (block_off + 1u) * lbs;
            continue;
        }
        if (rec_len < 34 || in_blk + (uint32_t)rec_len > lbs)
        {
            pfs_free(blk);
            return PFS_ISOFS_RET_EBADMSG;
        }
        if (nsec >= PFS_ISOFS_MAX_SECTIONS)
        {
            pfs_free(blk);
            return PFS_ISOFS_RET_ERANGE;
        }
        if (0 == nsec)
        {
            pfs_isofs_inode_from_rec(p_rec, p_out);
            total = 0;
        }
        p_out->sec[nsec].extent = pfs_isofs_data_lba(p_rec);
        p_out->sec[nsec].size = pfs_isonum_733(p_rec->size);
        total += p_out->sec[nsec].size;
        nsec++;
        uint8_t flags = pfs_isonum_711(p_rec->flags);
        more = (0 != (flags & PFS_ISO_FILE_MULTI_EXTENT)) ? 1 : 0;
        pos += (uint32_t)rec_len;
    }

    pfs_free(blk);
    if (0 == nsec)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }
    p_out->n_sections = (uint8_t)nsec;
    p_out->size = total;
    p_out->extent = p_out->sec[0].extent;
    *p_end_pos = pos;
    return PFS_ISOFS_RET_OK;
}

static int pfs_isofs_apply_rock_to_inode(pfs_isofs_sbi_t *p_sbi,
        const pfs_iso_directory_record_t *p_rec, int rec_len, pfs_isofs_inode_t *p_inode)
{
    if (0 == p_sbi->rock)
    {
        return PFS_ISOFS_RET_OK;
    }
    int has_nm;
    int rc = pfs_isofs_rock_parse(p_sbi, p_rec, rec_len, p_inode, NULL, 0, &has_nm);
    (void)has_nm;
    if (0 > rc)
    {
        return rc;
    }
    if (1 == rc)
    {
        return 1;
    }
    return PFS_ISOFS_RET_OK;
}

int pfs_isofs_find_in_dir(pfs_isofs_sbi_t *p_sbi, const pfs_isofs_inode_t *p_dir,
        const char *name, pfs_isofs_inode_t *p_out)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == name || NULL == p_out)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (PFS_ISOFS_TYPE_DIR != p_dir->type || 0 == p_dir->extent)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    uint32_t lbs = p_sbi->logical_block_size;
    if (0 == lbs)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }

    char want[PFS_NAME_MAX + 1];
    if (p_sbi->rock)
    {
        strncpy(want, name, sizeof(want) - 1);
        want[sizeof(want) - 1] = '\0';
    }
    else if (p_sbi->joliet_level > 0)
    {
        strncpy(want, name, sizeof(want) - 1);
        want[sizeof(want) - 1] = '\0';
    }
    else
    {
        (void)pfs_isofs_normalize_name(name, (int)strlen(name), want, (int)sizeof(want));
    }

    uint8_t *blk = (uint8_t *)pfs_malloc((size_t)lbs);
    if (NULL == blk)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }

    uint32_t pos = 0;
    while (pos < p_dir->size)
    {
        uint32_t block_off = pos / lbs;
        uint32_t in_blk = pos % lbs;
        int rc = pfs_isofs_read_dir_block(p_sbi, p_dir->extent, block_off, blk, lbs);
        if (0 > rc)
        {
            pfs_free(blk);
            return rc;
        }
        const pfs_iso_directory_record_t *p_rec =
                (const pfs_iso_directory_record_t *)(blk + in_blk);
        int rec_len = (int)pfs_isonum_711(p_rec->length);
        if (0 == rec_len)
        {
            pos = (block_off + 1u) * lbs;
            continue;
        }
        if (rec_len < 34 || in_blk + (uint32_t)rec_len > lbs)
        {
            pfs_free(blk);
            return PFS_ISOFS_RET_EBADMSG;
        }

        char ent_name[PFS_NAME_MAX + 1];
        int prc = pfs_isofs_decode_name(p_sbi, p_rec, rec_len, ent_name, (int)sizeof(ent_name));
        if (0 > prc)
        {
            pfs_free(blk);
            return prc;
        }
        if (1 == prc)
        {
            pos += (uint32_t)rec_len;
            continue;
        }

        pfs_isofs_inode_t inode;
        uint32_t end_pos;
        rc = pfs_isofs_expand_sections(p_sbi, p_dir, pos, &inode, &end_pos);
        if (0 > rc)
        {
            pfs_free(blk);
            return rc;
        }
        // rock CL 等需基于首节 DE
        rc = pfs_isofs_apply_rock_to_inode(p_sbi, p_rec, rec_len, &inode);
        if (0 > rc)
        {
            pfs_free(blk);
            return rc;
        }
        if (1 == rc)
        {
            pos = end_pos;
            continue;
        }

        if (pfs_isofs_name_equal(p_sbi, ent_name, want))
        {
            *p_out = inode;
            pfs_free(blk);
            return PFS_ISOFS_RET_OK;
        }
        pos = end_pos;
    }

    pfs_free(blk);
    return PFS_ISOFS_RET_ENOENT;
}

int pfs_isofs_parent_dir(pfs_isofs_sbi_t *p_sbi, const pfs_isofs_inode_t *p_dir,
        pfs_isofs_inode_t *p_parent)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == p_parent)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (PFS_ISOFS_TYPE_DIR != p_dir->type || 0 == p_dir->extent)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    // 已是根: .. -> 根
    if (p_dir->extent == p_sbi->root.extent)
    {
        *p_parent = p_sbi->root;
        return PFS_ISOFS_RET_OK;
    }

    uint32_t lbs = p_sbi->logical_block_size;
    uint8_t *blk = (uint8_t *)pfs_malloc((size_t)lbs);
    if (NULL == blk)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }
    int64_t off = pfs_isofs_lba_to_off(p_sbi, p_dir->extent);
    int nread = pfs_blkio_pread(p_sbi->p_blkio, blk, (int)lbs, off);
    if (nread != (int)lbs)
    {
        pfs_free(blk);
        if (0 > nread)
        {
            return pfs_isofs_map_blkio_rc(nread);
        }
        return PFS_ISOFS_RET_EIO;
    }

    const pfs_iso_directory_record_t *p_dot = (const pfs_iso_directory_record_t *)blk;
    int len_dot = (int)pfs_isonum_711(p_dot->length);
    if (len_dot < 34 || len_dot > (int)lbs)
    {
        pfs_free(blk);
        return PFS_ISOFS_RET_EBADMSG;
    }
    uint32_t off_dd = (uint32_t)len_dot;
    if (off_dd >= lbs)
    {
        pfs_free(blk);
        return PFS_ISOFS_RET_EBADMSG;
    }
    const pfs_iso_directory_record_t *p_dotdot =
            (const pfs_iso_directory_record_t *)(blk + off_dd);
    int len_dd = (int)pfs_isonum_711(p_dotdot->length);
    if (len_dd < 34 || off_dd + (uint32_t)len_dd > lbs)
    {
        pfs_free(blk);
        return PFS_ISOFS_RET_EBADMSG;
    }
    if (1 != pfs_isonum_711(p_dotdot->name_len) || 1 != p_dotdot->name[0])
    {
        pfs_free(blk);
        return PFS_ISOFS_RET_EBADMSG;
    }
    pfs_isofs_inode_from_rec(p_dotdot, p_parent);
    pfs_free(blk);
    if (PFS_ISOFS_TYPE_DIR != p_parent->type || 0 == p_parent->extent)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }
    return PFS_ISOFS_RET_OK;
}

int pfs_isofs_dir_read_next(pfs_isofs_sbi_t *p_sbi, pfs_isofs_dir_meta_t *p_dm,
        pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dm || NULL == p_ent)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (!pfs_isofs_sbi_is_mounted(p_sbi))
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    uint32_t lbs = p_sbi->logical_block_size;
    if (0 == lbs)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }

    uint8_t *blk = (uint8_t *)pfs_malloc((size_t)lbs);
    if (NULL == blk)
    {
        return PFS_ISOFS_RET_ENOMEM;
    }

    uint32_t pos = p_dm->pos;
    while (pos < p_dm->dir.size)
    {
        uint32_t block_off = pos / lbs;
        uint32_t in_blk = pos % lbs;
        int rc = pfs_isofs_read_dir_block(p_sbi, p_dm->dir.extent, block_off, blk, lbs);
        if (0 > rc)
        {
            pfs_free(blk);
            return rc;
        }
        const pfs_iso_directory_record_t *p_rec =
                (const pfs_iso_directory_record_t *)(blk + in_blk);
        int rec_len = (int)pfs_isonum_711(p_rec->length);
        if (0 == rec_len)
        {
            pos = (block_off + 1u) * lbs;
            continue;
        }
        if (rec_len < 34 || in_blk + (uint32_t)rec_len > lbs)
        {
            pfs_free(blk);
            return PFS_ISOFS_RET_EBADMSG;
        }

        char name[PFS_NAME_MAX + 1];
        int prc = pfs_isofs_decode_name(p_sbi, p_rec, rec_len, name, (int)sizeof(name));
        if (0 > prc)
        {
            pfs_free(blk);
            return prc;
        }
        if (1 == prc)
        {
            pos += (uint32_t)rec_len;
            continue;
        }

        pfs_isofs_inode_t inode;
        uint32_t end_pos;
        rc = pfs_isofs_expand_sections(p_sbi, &p_dm->dir, pos, &inode, &end_pos);
        if (0 > rc)
        {
            pfs_free(blk);
            return rc;
        }
        rc = pfs_isofs_apply_rock_to_inode(p_sbi, p_rec, rec_len, &inode);
        if (0 > rc)
        {
            pfs_free(blk);
            return rc;
        }
        if (1 == rc)
        {
            pos = end_pos;
            continue;
        }

        p_dm->pos = end_pos;
        memset(p_ent, 0, sizeof(*p_ent));
        if (PFS_ISOFS_TYPE_DIR == inode.type)
        {
            p_ent->d_type = PFS_DT_DIR;
        }
        else
        {
            p_ent->d_type = PFS_DT_REG;
        }
        strncpy(p_ent->d_name, name, sizeof(p_ent->d_name) - 1);
        p_ent->d_name[sizeof(p_ent->d_name) - 1] = '\0';
        pfs_free(blk);
        return PFS_ISOFS_RET_OK;
    }

    p_dm->pos = pos;
    pfs_free(blk);
    p_ent->d_name[0] = '\0';
    return PFS_ISOFS_RET_OK;
}

// end
