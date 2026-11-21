// Doc Encode : UTF-8 BOM, Unix(LF)
// UDF directory FID walk (只读)
#include "udf/pfs_udf_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

static int pfs_udf_fid_round_up(int len)
{
    return (len + 3) & ~3;
}

static int pfs_udf_decode_fid_name(const uint8_t *p_ident, int ident_len, char *name_out,
        int name_cap)
{

    if (NULL == p_ident || ident_len <= 0)
    {
        name_out[0] = '\0';
        return 0;
    }
    int rc = pfs_udf_unicode_to_utf8(p_ident, ident_len, name_out, name_cap);
    if (rc < 0)
    {
        return rc;
    }
    return 0;
}

static int pfs_udf_read_dir_bytes(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_dir,
        uint64_t pos, void *buf, int len)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == buf || len < 0)
    {
        return PFS_UDF_RET_EINVAL;
    }
    pfs_udf_file_meta_t fm;

    memset(&fm, 0, sizeof(fm));
    fm.inode = *p_dir;

    int got = 0;
    int rc = pfs_udf_file_read(p_sbi, &fm, (int64_t)pos, buf, len, &got);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    if (got != len)
    {
        return PFS_UDF_RET_EIO;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_parse_fid_at(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_dir,
        uint64_t pos, char *name_out, int name_cap, pfs_udf_inode_t *p_child,
        int *p_fid_len)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == name_out || NULL == p_child ||
            NULL == p_fid_len)
    {
        return PFS_UDF_RET_EINVAL;
    }

    uint8_t hdr_buf[sizeof(pfs_udf_file_ident_desc_t) + 256];

    int rc = pfs_udf_read_dir_bytes(p_sbi, p_dir, pos, hdr_buf,
            (int)sizeof(pfs_udf_file_ident_desc_t));
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    const pfs_udf_file_ident_desc_t *p_fid = (const pfs_udf_file_ident_desc_t *)hdr_buf;

    if (PFS_UDF_TAG_IDENT_FID != pfs_udf_le16(&p_fid->descTag.tagIdent))
    {
        return PFS_UDF_RET_EBADMSG;
    }

    int imp_len = (int)pfs_udf_le16(&p_fid->lengthOfImpUse);
    int ident_len = (int)p_fid->lengthFileIdent;
    int total = (int)sizeof(pfs_udf_file_ident_desc_t) + imp_len + ident_len;
    total = pfs_udf_fid_round_up(total);
    if (0 != (p_fid->fileCharacteristics & PFS_UDF_FID_CHAR_DELETED))
    {
        *p_fid_len = total;
        return 1;
    }
    if (0 != (p_fid->fileCharacteristics & PFS_UDF_FID_CHAR_METADATA))
    {
        *p_fid_len = total;
        return 1;
    }

    if (total > (int)sizeof(hdr_buf))
    {
        return PFS_UDF_RET_EBADMSG;
    }
    rc = pfs_udf_read_dir_bytes(p_sbi, p_dir, pos, hdr_buf, total);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    p_fid = (const pfs_udf_file_ident_desc_t *)hdr_buf;

    name_out[0] = '\0';
    if (ident_len > 0)
    {
        const uint8_t *p_ident = hdr_buf + sizeof(pfs_udf_file_ident_desc_t) + (size_t)imp_len;

        rc = pfs_udf_decode_fid_name(p_ident, ident_len, name_out, name_cap);
        if (rc < 0)
        {
            return rc;
        }
    }

    rc = pfs_udf_icb_read(p_sbi, &p_fid->icb.extLocation, p_child);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    *p_fid_len = total;
    return 0;
}

int pfs_udf_find_in_dir(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_dir,
        const char *name, pfs_udf_inode_t *p_out)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == name || NULL == p_out)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_TYPE_DIR != p_dir->type)
    {
        return PFS_UDF_RET_EINVAL;
    }

    uint64_t pos = 0;
    char ent_name[PFS_NAME_MAX + 1];

    while (pos < p_dir->size)
    {
        pfs_udf_inode_t child;
        int fid_len;
        int prc = pfs_udf_parse_fid_at(p_sbi, p_dir, pos, ent_name, (int)sizeof(ent_name),
                &child, &fid_len);

        if (prc < 0)
        {
            return prc;
        }
        if (1 == prc || 0 == fid_len)
        {
            if (0 == fid_len)
            {
                break;
            }
            pos += (uint64_t)fid_len;
            continue;
        }
        if (pfs_udf_name_equal(ent_name, name))
        {
            *p_out = child;
            return PFS_UDF_RET_OK;
        }
        pos += (uint64_t)fid_len;
    }
    return PFS_UDF_RET_ENOENT;
}

int pfs_udf_find_fid_in_dir(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_dir,
        const char *name, pfs_udf_inode_t *p_child, uint64_t *p_fid_pos, int *p_fid_len)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == name || NULL == p_child ||
            NULL == p_fid_pos || NULL == p_fid_len)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_TYPE_DIR != p_dir->type)
    {
        return PFS_UDF_RET_EINVAL;
    }

    uint64_t pos = 0;
    char ent_name[PFS_NAME_MAX + 1];

    while (pos < p_dir->size)
    {
        pfs_udf_inode_t child;
        int fid_len;
        int prc = pfs_udf_parse_fid_at(p_sbi, p_dir, pos, ent_name, (int)sizeof(ent_name),
                &child, &fid_len);

        if (prc < 0)
        {
            return prc;
        }
        if (1 == prc || 0 == fid_len)
        {
            if (0 == fid_len)
            {
                break;
            }
            pos += (uint64_t)fid_len;
            continue;
        }
        if (pfs_udf_name_equal(ent_name, name))
        {
            *p_child = child;
            *p_fid_pos = pos;
            *p_fid_len = fid_len;
            return PFS_UDF_RET_OK;
        }
        pos += (uint64_t)fid_len;
    }
    return PFS_UDF_RET_ENOENT;
}

int pfs_udf_parent_dir(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_dir,
        pfs_udf_inode_t *p_parent)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == p_parent)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_TYPE_DIR != p_dir->type)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (p_dir->iblock == p_sbi->root.iblock &&
            p_dir->part_ref == p_sbi->root.part_ref)
    {
        *p_parent = p_sbi->root;
        return PFS_UDF_RET_OK;
    }

    uint64_t pos = 0;

    while (pos < p_dir->size)
    {
        pfs_udf_inode_t child;
        char ent_name[PFS_NAME_MAX + 1];
        int fid_len;
        int prc;
        const pfs_udf_file_ident_desc_t *p_fid_hdr;
        uint8_t hdr[sizeof(pfs_udf_file_ident_desc_t)];

        int rc = pfs_udf_read_dir_bytes(p_sbi, p_dir, pos, hdr, (int)sizeof(hdr));
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }

        p_fid_hdr = (const pfs_udf_file_ident_desc_t *)hdr;
        if (0 != (p_fid_hdr->fileCharacteristics & PFS_UDF_FID_CHAR_PARENT))
        {
            prc = pfs_udf_parse_fid_at(p_sbi, p_dir, pos, ent_name, (int)sizeof(ent_name),
                    &child, &fid_len);
            if (prc < 0)
            {
                return prc;
            }
            if (0 == prc)
            {
                *p_parent = child;
                return PFS_UDF_RET_OK;
            }
        }
        prc = pfs_udf_parse_fid_at(p_sbi, p_dir, pos, ent_name, (int)sizeof(ent_name),
                &child, &fid_len);
        if (prc < 0)
        {
            return prc;
        }
        if (0 == fid_len)
        {
            break;
        }
        pos += (uint64_t)fid_len;
    }
    return PFS_UDF_RET_EBADMSG;
}

int pfs_udf_dir_read_next(pfs_udf_sbi_t *p_sbi, pfs_udf_dir_meta_t *p_dm,
        pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dm || NULL == p_ent)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_is_mounted(p_sbi))
    {
        return PFS_UDF_RET_EINVAL;
    }

    pfs_udf_inode_t child;
    char name[PFS_NAME_MAX + 1];
    int fid_len;

    while (p_dm->pos < p_dm->dir.size)
    {
        int prc = pfs_udf_parse_fid_at(p_sbi, &p_dm->dir, p_dm->pos, name, (int)sizeof(name),
                &child, &fid_len);

        if (prc < 0)
        {
            return prc;
        }
        if (0 == fid_len)
        {
            break;
        }
        p_dm->pos += (uint64_t)fid_len;
        if (1 == prc)
        {
            continue;
        }
        if ('\0' == name[0])
        {
            continue;
        }
        if (1 == strlen(name) && '.' == name[0])
        {
            continue;
        }
        if (2 == strlen(name) && '.' == name[0] && '.' == name[1])
        {
            continue;
        }

        memset(p_ent, 0, sizeof(*p_ent));
        if (PFS_UDF_TYPE_DIR == child.type)
        {
            p_ent->d_type = PFS_DT_DIR;
        }
        else
        {
            p_ent->d_type = PFS_DT_REG;
        }
        strncpy(p_ent->d_name, name, sizeof(p_ent->d_name) - 1);
        p_ent->d_name[sizeof(p_ent->d_name) - 1] = '\0';
        return PFS_UDF_RET_OK;
    }

    p_ent->d_name[0] = '\0';
    return PFS_UDF_RET_OK;
}

// end
