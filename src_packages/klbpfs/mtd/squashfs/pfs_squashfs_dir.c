// Doc Encode : UTF-8 BOM, Unix(LF)
// squashfs 目录流 (对照 dir.c; . / .. 由 portfs vfs 合成, 此处只吐磁盘项)
// readdir: 1=有项; 0=EOF; <0=错误
#include "squashfs/pfs_squashfs_internal.h"

#include <string.h>

static uint8_t pfs_squashfs_map_dtype(uint16_t type)
{
    switch (type) {
    case PFS_SQUASHFS_DIR_TYPE:
    case PFS_SQUASHFS_LDIR_TYPE:
        return PFS_DT_DIR;
    case PFS_SQUASHFS_REG_TYPE:
    case PFS_SQUASHFS_LREG_TYPE:
        return PFS_DT_REG;
    default:
        return PFS_DT_UNKNOWN;
    }
}

int pfs_squashfs_dir_open(pfs_squashfs_sbi_t *p_sbi, const char *path, pfs_squashfs_dir_meta_t *p_dm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_squashfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }


    memset(p_dm, 0, sizeof(*p_dm));
    int rc = pfs_squashfs_lookup_path(p_sbi, path, &p_dm->info);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!pfs_squashfs_inode_is_dir(&p_dm->info))
    {
        return PFS_EINVAL;
    }

    p_dm->block = p_dm->info.start + p_sbi->directory_table;
    p_dm->offset = (int)p_dm->info.offset;
    p_dm->length = 0;
    p_dm->invent = 0;
    p_dm->remaining = 0;
    p_dm->hdr_valid = 0;
    return PFS_OK;
}

void pfs_squashfs_dir_close(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_dir_meta_t *p_dm)
{
    (void)p_sbi;
    if (NULL != p_dm)
    {
        memset(p_dm, 0, sizeof(*p_dm));
    }
}

int pfs_squashfs_dir_readdir(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_dir_meta_t *p_dm, pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dm || NULL == p_ent)
    {
        return PFS_EINVAL;
    }
    if (!pfs_squashfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    uint8_t raw_hdr[sizeof(pfs_squashfs_dir_header_t)];
    uint8_t raw_ent[sizeof(pfs_squashfs_dir_entry_t)];
    char ename[PFS_SQUASHFS_NAME_LEN + 1];

    for (;;)
    {
        if (p_dm->length >= (int)p_dm->info.file_size)
        {
            return 0;
        }

        if (!p_dm->hdr_valid || 0u == p_dm->remaining)
        {
            int rc = pfs_squashfs_read_metadata(p_sbi, raw_hdr, &p_dm->block, &p_dm->offset,
                    (int)sizeof(raw_hdr));
            if (rc < 0)
            {
                return rc;
            }
            p_dm->length += (int)sizeof(raw_hdr);
            p_dm->hdr.count = pfs_squashfs_le32(raw_hdr + 0);
            p_dm->hdr.start_block = pfs_squashfs_le32(raw_hdr + 4);
            p_dm->hdr.inode_number = pfs_squashfs_le32(raw_hdr + 8);
            p_dm->remaining = p_dm->hdr.count + 1u;
            p_dm->hdr_valid = 1;
            if (p_dm->remaining > PFS_SQUASHFS_DIR_COUNT)
            {
                return PFS_EIO;
            }
        }

        p_dm->remaining--;
        int rc = pfs_squashfs_read_metadata(p_sbi, raw_ent, &p_dm->block, &p_dm->offset,
                (int)sizeof(raw_ent));
        if (rc < 0)
        {
            return rc;
        }

        uint16_t ent_type = pfs_squashfs_le16(raw_ent + 4);
        uint16_t ent_size = pfs_squashfs_le16(raw_ent + 6);
        unsigned int size = (unsigned int)ent_size + 1u;

        if (size > PFS_SQUASHFS_NAME_LEN)
        {
            return PFS_EIO;
        }
        rc = pfs_squashfs_read_metadata(p_sbi, ename, &p_dm->block, &p_dm->offset, (int)size);
        if (rc < 0)
        {
            return rc;
        }
        ename[size] = '\0';
        p_dm->length += (int)sizeof(raw_ent) + (int)size;

        p_ent->d_type = pfs_squashfs_map_dtype(ent_type);
        if (size >= sizeof(p_ent->d_name))
        {
            size = (unsigned int)sizeof(p_ent->d_name) - 1u;
        }
        memcpy(p_ent->d_name, ename, size);
        p_ent->d_name[size] = '\0';
        return 1;
    }
}

// end
