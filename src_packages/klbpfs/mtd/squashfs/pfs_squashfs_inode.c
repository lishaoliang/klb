// Doc Encode : UTF-8 BOM, Unix(LF)
// squashfs 读 inode (对照 inode.c; 仅 DIR/LDIR/REG/LREG)
#include "squashfs/pfs_squashfs_internal.h"

#include <string.h>
#include <stdbool.h>

bool pfs_squashfs_inode_is_dir(const pfs_squashfs_inode_info_t *p_info)
{
    if (NULL == p_info)
    {
        return false;
    }
    return (PFS_SQUASHFS_DIR_TYPE == p_info->type || PFS_SQUASHFS_LDIR_TYPE == p_info->type);
}

bool pfs_squashfs_inode_is_reg(const pfs_squashfs_inode_info_t *p_info)
{
    if (NULL == p_info)
    {
        return false;
    }
    return (PFS_SQUASHFS_REG_TYPE == p_info->type || PFS_SQUASHFS_LREG_TYPE == p_info->type);
}

static int pfs_squashfs_fill_frag(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_inode_info_t *p_info,
        uint32_t fragment, uint32_t frag_off, uint64_t file_size)
{
    p_info->fragment = fragment;
    if (PFS_SQUASHFS_INVALID_FRAG == fragment)
    {
        p_info->frag_block = (uint64_t)PFS_SQUASHFS_INVALID_BLK;
        p_info->frag_size = 0;
        p_info->offset = 0;
        return PFS_OK;
    }

    if (0u == (file_size & ((uint64_t)p_sbi->block_size - 1u)))
    {
        return PFS_EINVAL;
    }

    uint64_t frag_blk;
    int frag_size = pfs_squashfs_frag_lookup(p_sbi, fragment, &frag_blk);
    if (frag_size < 0)
    {
        return frag_size;
    }
    p_info->frag_block = frag_blk;
    p_info->frag_size = frag_size;
    p_info->offset = frag_off;
    return PFS_OK;
}

int pfs_squashfs_read_inode(pfs_squashfs_sbi_t *p_sbi, uint64_t ino, pfs_squashfs_inode_info_t *p_info)
{
    if (NULL == p_sbi || NULL == p_info)
    {
        return PFS_EINVAL;
    }
    if (!pfs_squashfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_info, 0, sizeof(*p_info));
    p_info->ino = ino;
    p_info->fragment = PFS_SQUASHFS_INVALID_FRAG;
    p_info->frag_block = (uint64_t)PFS_SQUASHFS_INVALID_BLK;

    uint64_t block = (uint64_t)PFS_SQUASHFS_INODE_BLK(ino) + p_sbi->inode_table;
    int offset = (int)PFS_SQUASHFS_INODE_OFFSET(ino);
    uint8_t raw_base[sizeof(pfs_squashfs_base_inode_t)];
    pfs_squashfs_base_inode_t base;

    int rc = pfs_squashfs_read_metadata(p_sbi, raw_base, &block, &offset, (int)sizeof(raw_base));
    if (rc < 0)
    {
        return rc;
    }

    base.inode_type = pfs_squashfs_le16(raw_base + 0);
    base.mode = pfs_squashfs_le16(raw_base + 2);
    base.uid = pfs_squashfs_le16(raw_base + 4);
    base.guid = pfs_squashfs_le16(raw_base + 6);
    base.mtime = pfs_squashfs_le32(raw_base + 8);
    base.inode_number = pfs_squashfs_le32(raw_base + 12);

    if (0u == base.inode_number)
    {
        return PFS_EINVAL;
    }

    p_info->inode_number = base.inode_number;
    p_info->mode = base.mode;
    uint16_t type = base.inode_type;
    p_info->type = type;

    // 重定位到 inode 起点再按类型读完整固定头 (与内核一致)
    block = (uint64_t)PFS_SQUASHFS_INODE_BLK(ino) + p_sbi->inode_table;
    offset = (int)PFS_SQUASHFS_INODE_OFFSET(ino);

    switch (type) {
    case PFS_SQUASHFS_REG_TYPE: {
        uint8_t raw[sizeof(pfs_squashfs_reg_inode_t)];
        pfs_squashfs_reg_inode_t reg;

        rc = pfs_squashfs_read_metadata(p_sbi, raw, &block, &offset, (int)sizeof(raw));
        if (rc < 0)
        {
            return rc;
        }
        reg.start_block = pfs_squashfs_le32(raw + 16);
        reg.fragment = pfs_squashfs_le32(raw + 20);
        reg.offset = pfs_squashfs_le32(raw + 24);
        reg.file_size = pfs_squashfs_le32(raw + 28);

        p_info->file_size = reg.file_size;
        p_info->start = reg.start_block;
        p_info->block_list_block = block;
        p_info->block_list_offset = offset;
        return pfs_squashfs_fill_frag(p_sbi, p_info, reg.fragment, reg.offset, p_info->file_size);
    }
    case PFS_SQUASHFS_LREG_TYPE: {
        uint8_t raw[sizeof(pfs_squashfs_lreg_inode_t)];
        pfs_squashfs_lreg_inode_t lreg;

        rc = pfs_squashfs_read_metadata(p_sbi, raw, &block, &offset, (int)sizeof(raw));
        if (rc < 0)
        {
            return rc;
        }
        lreg.start_block = pfs_squashfs_le64(raw + 16);
        lreg.file_size = pfs_squashfs_le64(raw + 24);
        lreg.fragment = pfs_squashfs_le32(raw + 44);
        lreg.offset = pfs_squashfs_le32(raw + 48);

        p_info->file_size = lreg.file_size;
        p_info->start = lreg.start_block;
        p_info->block_list_block = block;
        p_info->block_list_offset = offset;
        return pfs_squashfs_fill_frag(p_sbi, p_info, lreg.fragment, lreg.offset, p_info->file_size);
    }
    case PFS_SQUASHFS_DIR_TYPE: {
        uint8_t raw[sizeof(pfs_squashfs_dir_inode_t)];

        rc = pfs_squashfs_read_metadata(p_sbi, raw, &block, &offset, (int)sizeof(raw));
        if (rc < 0)
        {
            return rc;
        }
        p_info->start = pfs_squashfs_le32(raw + 16);
        p_info->file_size = pfs_squashfs_le16(raw + 24);
        p_info->offset = pfs_squashfs_le16(raw + 26);
        p_info->parent_inode = pfs_squashfs_le32(raw + 28);
        p_info->dir_idx_cnt = 0;
        return PFS_OK;
    }
    case PFS_SQUASHFS_LDIR_TYPE: {
        uint8_t raw[sizeof(pfs_squashfs_ldir_inode_t)];

        rc = pfs_squashfs_read_metadata(p_sbi, raw, &block, &offset, (int)sizeof(raw));
        if (rc < 0)
        {
            return rc;
        }
        p_info->file_size = pfs_squashfs_le32(raw + 20);
        p_info->start = pfs_squashfs_le32(raw + 24);
        p_info->parent_inode = pfs_squashfs_le32(raw + 28);
        p_info->dir_idx_cnt = pfs_squashfs_le16(raw + 32);
        p_info->offset = pfs_squashfs_le16(raw + 34);
        p_info->dir_idx_block = block;
        p_info->dir_idx_offset = offset;
        return PFS_OK;
    }
    default:
        // symlink/设备等: 保留 type/size 占位; 打开文件时拒绝
        return PFS_OK;
    }
}

// end
