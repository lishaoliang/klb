// Doc Encode : UTF-8 BOM, Unix(LF)
// squashfs 路径查找 (对照 namei.c; 首版线性扫目录, 跳过 index 加速)
#include "squashfs/pfs_squashfs_internal.h"

#include <string.h>

static int pfs_squashfs_lookup_name(pfs_squashfs_sbi_t *p_sbi, const pfs_squashfs_inode_info_t *p_dir,
        const char *name, int name_len, pfs_squashfs_inode_info_t *p_out)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == name || NULL == p_out || name_len <= 0)
    {
        return PFS_EINVAL;
    }
    if (!pfs_squashfs_inode_is_dir(p_dir))
    {
        return PFS_EINVAL;
    }
    if (name_len > PFS_SQUASHFS_NAME_LEN)
    {
        return PFS_EINVAL;
    }

    uint64_t block = p_dir->start + p_sbi->directory_table;
    int offset = (int)p_dir->offset;
    int length = 0;
    pfs_squashfs_dir_header_t dirh;
    uint8_t raw_hdr[sizeof(pfs_squashfs_dir_header_t)];
    uint8_t raw_ent[sizeof(pfs_squashfs_dir_entry_t)];
    char ename[PFS_SQUASHFS_NAME_LEN + 1];

    while (length < (int)p_dir->file_size)
    {
        int rc = pfs_squashfs_read_metadata(p_sbi, raw_hdr, &block, &offset, (int)sizeof(raw_hdr));
        if (rc < 0)
        {
            return rc;
        }
        length += (int)sizeof(raw_hdr);

        dirh.count = pfs_squashfs_le32(raw_hdr + 0);
        dirh.start_block = pfs_squashfs_le32(raw_hdr + 4);
        dirh.inode_number = pfs_squashfs_le32(raw_hdr + 8);
        unsigned int dir_count = dirh.count + 1u;
        if (dir_count > PFS_SQUASHFS_DIR_COUNT)
        {
            return PFS_EIO;
        }

        while (dir_count > 0u)
        {
            uint16_t ent_off;
            uint16_t ent_ino_delta;
            uint16_t ent_type;
            uint16_t ent_size;
            uint64_t child_ino;
            uint32_t child_num;

            dir_count--;
            rc = pfs_squashfs_read_metadata(p_sbi, raw_ent, &block, &offset, (int)sizeof(raw_ent));
            if (rc < 0)
            {
                return rc;
            }
            ent_off = pfs_squashfs_le16(raw_ent + 0);
            ent_ino_delta = pfs_squashfs_le16(raw_ent + 2);
            ent_type = pfs_squashfs_le16(raw_ent + 4);
            ent_size = pfs_squashfs_le16(raw_ent + 6);
            unsigned int size = (unsigned int)ent_size + 1u;
            if (size > PFS_SQUASHFS_NAME_LEN)
            {
                return PFS_EIO;
            }

            rc = pfs_squashfs_read_metadata(p_sbi, ename, &block, &offset, (int)size);
            if (rc < 0)
            {
                return rc;
            }
            ename[size] = '\0';
            length += (int)sizeof(raw_ent) + (int)size;

            if ((unsigned char)name[0] < (unsigned char)ename[0])
            {
                return PFS_ENOENT;
            }
            if (name_len == (int)size && 0 == memcmp(name, ename, (size_t)size))
            {
                child_num = dirh.inode_number + (uint32_t)(int16_t)ent_ino_delta;
                child_ino = (uint64_t)PFS_SQUASHFS_MKINODE(dirh.start_block, ent_off);
                (void)ent_type;
                (void)child_num;
                return pfs_squashfs_read_inode(p_sbi, child_ino, p_out);
            }
        }
    }

    return PFS_ENOENT;
}

int pfs_squashfs_lookup_path(pfs_squashfs_sbi_t *p_sbi, const char *path, pfs_squashfs_inode_info_t *p_info)
{
    if (NULL == p_sbi || NULL == path || NULL == p_info)
    {
        return PFS_EINVAL;
    }
    if (!pfs_squashfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    pfs_squashfs_inode_info_t cur;
    pfs_squashfs_inode_info_t next;
    const char *p;
    char name[PFS_SQUASHFS_NAME_LEN + 1];
    int nlen;

    int rc = pfs_squashfs_read_inode(p_sbi, p_sbi->root_inode, &cur);
    if (PFS_OK != rc)
    {
        return rc;
    }

    p = path;
    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        *p_info = cur;
        return PFS_OK;
    }

    while ('\0' != *p)
    {
        nlen = 0;
        while ('\0' != *p && '/' != *p)
        {
            if (nlen >= PFS_SQUASHFS_NAME_LEN)
            {
                return PFS_EINVAL;
            }
            name[nlen] = *p;
            nlen++;
            p++;
        }
        name[nlen] = '\0';
        while ('/' == *p)
        {
            p++;
        }

        if (0 == nlen)
        {
            continue;
        }
        if (1 == nlen && '.' == name[0])
        {
            continue;
        }
        if (2 == nlen && '.' == name[0] && '.' == name[1])
        {
            // 无 parent 定位表; 根上 .. 停在根
            if (cur.ino == p_sbi->root_inode)
            {
                continue;
            }
            // 简化: 不支持 .. 跳出 (首版)
            return PFS_EOPNOTSUPP;
        }

        rc = pfs_squashfs_lookup_name(p_sbi, &cur, name, nlen, &next);
        if (PFS_OK != rc)
        {
            return rc;
        }
        cur = next;
    }

    *p_info = cur;
    return PFS_OK;
}

int pfs_squashfs_access(pfs_squashfs_sbi_t *p_sbi, const char *path, int mode)
{
    pfs_squashfs_inode_info_t info;

    (void)mode;
    int rc = pfs_squashfs_lookup_path(p_sbi, path, &info);
    if (PFS_OK != rc)
    {
        return rc;
    }
    return PFS_OK;
}

int pfs_squashfs_mkdir(pfs_squashfs_sbi_t *p_sbi, const char *path, int mode)
{
    (void)p_sbi;
    (void)path;
    (void)mode;
    return PFS_EPERM;
}

int pfs_squashfs_rmdir(pfs_squashfs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

int pfs_squashfs_rename(pfs_squashfs_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    (void)p_sbi;
    (void)oldpath;
    (void)newpath;
    return PFS_EPERM;
}

int pfs_squashfs_unlink(pfs_squashfs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

// end
