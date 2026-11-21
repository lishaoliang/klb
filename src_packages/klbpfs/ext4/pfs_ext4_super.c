// Doc Encode : UTF-8 BOM, Unix(LF)
// ext4 super: probe/mount/umount; 加载 GDT (兼挂 ext2/3)
#include "ext4/pfs_ext4_internal.h"

#include <string.h>

// sb 关键字段偏移 (linux ext4_super_block)
#define OFF_INODES_COUNT        0x000u
#define OFF_BLOCKS_COUNT_LO     0x004u
#define OFF_FREE_BLOCKS_LO      0x00Cu
#define OFF_FREE_INODES         0x010u
#define OFF_FIRST_DATA_BLOCK    0x014u
#define OFF_LOG_BLOCK_SIZE      0x018u
#define OFF_BLOCKS_PER_GROUP    0x020u
#define OFF_INODES_PER_GROUP    0x028u
#define OFF_MAGIC               0x038u
#define OFF_STATE               0x03Au
#define OFF_REV_LEVEL           0x04Cu
#define OFF_FIRST_INO           0x054u
#define OFF_INODE_SIZE          0x058u
#define OFF_FEATURE_COMPAT      0x05Cu
#define OFF_FEATURE_INCOMPAT    0x060u
#define OFF_FEATURE_RO_COMPAT   0x064u
#define OFF_DESC_SIZE           0x0FEu
#define OFF_BLOCKS_COUNT_HI     0x150u
#define OFF_FREE_BLOCKS_HI      0x158u

static int pfs_ext4_parse_sb(pfs_ext4_sbi_t *p_sbi)
{
    const uint8_t *sb = p_sbi->sb_raw;

    if (PFS_EXT4_SUPER_MAGIC != pfs_ext4_sb_u16(sb, OFF_MAGIC))
    {
        return PFS_EXT4_RET_ENOEXEC;
    }

    uint32_t log_bs = pfs_ext4_sb_u32(sb, OFF_LOG_BLOCK_SIZE);
    if (log_bs > 6u)
    {
        return PFS_EXT4_RET_EBADMSG;
    }
    p_sbi->block_size = PFS_EXT4_MIN_BLOCK_SIZE << log_bs;
    p_sbi->block_bits = 10u + log_bs;
    if (p_sbi->block_size < PFS_EXT4_MIN_BLOCK_SIZE ||
            p_sbi->block_size > PFS_EXT4_MAX_BLOCK_SIZE)
    {
        return PFS_EXT4_RET_EBADMSG;
    }

    uint32_t incompat = pfs_ext4_sb_u32(sb, OFF_FEATURE_INCOMPAT);
    if (0u != (incompat & ~PFS_EXT4_INCOMPAT_SUPP))
    {
        return PFS_EXT4_RET_EOPNOTSUPP;
    }
    if (0u != (incompat & PFS_EXT4_FEATURE_INCOMPAT_META_BG))
    {
        return PFS_EXT4_RET_EOPNOTSUPP;
    }

    uint32_t rev = pfs_ext4_sb_u32(sb, OFF_REV_LEVEL);
    uint32_t inode_size = (0u == rev) ? PFS_EXT4_GOOD_OLD_INODE_SIZE :
            (uint32_t)pfs_ext4_sb_u16(sb, OFF_INODE_SIZE);
    if (inode_size < PFS_EXT4_GOOD_OLD_INODE_SIZE ||
            inode_size > p_sbi->block_size ||
            0u != (inode_size & (inode_size - 1u)))
    {
        return PFS_EXT4_RET_EBADMSG;
    }

    uint32_t desc_size = PFS_EXT4_MIN_DESC_SIZE;
    if (0u != (incompat & PFS_EXT4_FEATURE_INCOMPAT_64BIT))
    {
        desc_size = (uint32_t)pfs_ext4_sb_u16(sb, OFF_DESC_SIZE);
        if (desc_size < PFS_EXT4_MIN_DESC_SIZE_64BIT)
        {
            desc_size = PFS_EXT4_MIN_DESC_SIZE_64BIT;
        }
    }
    if (0u == desc_size || desc_size > p_sbi->block_size ||
            0u != (desc_size & (desc_size - 1u)))
    {
        return PFS_EXT4_RET_EBADMSG;
    }

    uint64_t blocks = (uint64_t)pfs_ext4_sb_u32(sb, OFF_BLOCKS_COUNT_LO);
    if (0u != (incompat & PFS_EXT4_FEATURE_INCOMPAT_64BIT))
    {
        blocks |= ((uint64_t)pfs_ext4_sb_u32(sb, OFF_BLOCKS_COUNT_HI) << 32);
    }
    if (0ull == blocks)
    {
        return PFS_EXT4_RET_EBADMSG;
    }

    p_sbi->blocks_per_group = pfs_ext4_sb_u32(sb, OFF_BLOCKS_PER_GROUP);
    p_sbi->inodes_per_group = pfs_ext4_sb_u32(sb, OFF_INODES_PER_GROUP);
    p_sbi->first_data_block = pfs_ext4_sb_u32(sb, OFF_FIRST_DATA_BLOCK);
    p_sbi->inodes_count = pfs_ext4_sb_u32(sb, OFF_INODES_COUNT);
    p_sbi->first_ino = (0u == rev) ? PFS_EXT4_GOOD_OLD_FIRST_INO :
            pfs_ext4_sb_u32(sb, OFF_FIRST_INO);
    if (0u == p_sbi->first_ino)
    {
        p_sbi->first_ino = PFS_EXT4_GOOD_OLD_FIRST_INO;
    }

    if (0u == p_sbi->blocks_per_group || 0u == p_sbi->inodes_per_group ||
            0u == p_sbi->inodes_count)
    {
        return PFS_EXT4_RET_EBADMSG;
    }

    uint32_t groups = (uint32_t)((blocks - (uint64_t)p_sbi->first_data_block +
            (uint64_t)p_sbi->blocks_per_group - 1ull) /
            (uint64_t)p_sbi->blocks_per_group);
    if (0u == groups)
    {
        return PFS_EXT4_RET_EBADMSG;
    }

    p_sbi->blocks_count = blocks;
    p_sbi->groups_count = groups;
    p_sbi->inode_size = inode_size;
    p_sbi->desc_size = desc_size;
    p_sbi->feature_compat = pfs_ext4_sb_u32(sb, OFF_FEATURE_COMPAT);
    p_sbi->feature_incompat = incompat;
    p_sbi->feature_ro_compat = pfs_ext4_sb_u32(sb, OFF_FEATURE_RO_COMPAT);
    p_sbi->free_inodes_count = pfs_ext4_sb_u32(sb, OFF_FREE_INODES);
    p_sbi->free_blocks_count = pfs_ext4_sb_u32(sb, OFF_FREE_BLOCKS_LO);
    if (0u != (incompat & PFS_EXT4_FEATURE_INCOMPAT_64BIT))
    {
        p_sbi->free_blocks_count = (uint32_t)(((uint64_t)pfs_ext4_sb_u32(sb, OFF_FREE_BLOCKS_HI) << 32) |
                (uint64_t)p_sbi->free_blocks_count);
    }
    return PFS_EXT4_RET_OK;
}

static int pfs_ext4_read_sb(pfs_blkio_t *p_blkio, int64_t part_offset, pfs_ext4_sb_raw_t sb)
{

    memset(sb, 0, PFS_EXT4_SUPER_SIZE);
    int rc = pfs_blkio_pread(p_blkio, sb, (int)PFS_EXT4_SUPER_SIZE,
            part_offset + (int64_t)PFS_EXT4_SUPER_OFFSET);
    if (rc < 0)
    {
        return pfs_ext4_map_blkio_rc(rc);
    }
    if (rc != (int)PFS_EXT4_SUPER_SIZE)
    {
        return PFS_EXT4_RET_EIO;
    }
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_gd_load(pfs_ext4_sbi_t *p_sbi)
{
    uint32_t desc_per_block;
    uint32_t gdb_count;
    uint64_t gdb0;
    uint64_t bytes;
    uint8_t *p_buf;

    pfs_ext4_gd_free(p_sbi);
    if (0u == p_sbi->desc_size || 0u == p_sbi->groups_count)
    {
        return PFS_EXT4_RET_EBUSY;
    }

    desc_per_block = p_sbi->block_size / p_sbi->desc_size;
    if (0u == desc_per_block)
    {
        return PFS_EXT4_RET_EBADMSG;
    }
    gdb_count = (p_sbi->groups_count + desc_per_block - 1u) / desc_per_block;

    // 非 META_BG: GDT 紧随超块所在块
    // 1KiB: 超块在块1, GDT 从块2; 其它: 超块在块0偏移1024, GDT 从块1
    if (1024u == p_sbi->block_size)
    {
        gdb0 = 2ull;
    }
    else
    {
        gdb0 = 1ull;
    }

    bytes = (uint64_t)p_sbi->groups_count * (uint64_t)p_sbi->desc_size;
    if (bytes > (uint64_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_EXT4_RET_EOPNOTSUPP;
    }
    p_buf = (uint8_t *)pfs_mallocz((size_t)bytes);
    if (NULL == p_buf)
    {
        return PFS_EXT4_RET_ENOMEM;
    }

    for (uint32_t i = 0; i < gdb_count; i++)
    {
        uint32_t copy;
        uint8_t *blk;

        blk = (uint8_t *)pfs_malloc(p_sbi->block_size);
        if (NULL == blk)
        {
            pfs_free(p_buf);
            return PFS_EXT4_RET_ENOMEM;
        }
        int rc = pfs_ext4_read_block(p_sbi, gdb0 + i, blk);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(blk);
            pfs_free(p_buf);
            return rc;
        }
        copy = desc_per_block * p_sbi->desc_size;
        if ((uint64_t)i * copy + copy > bytes)
        {
            copy = (uint32_t)(bytes - (uint64_t)i * copy);
        }
        memcpy(p_buf + (size_t)i * desc_per_block * p_sbi->desc_size, blk, copy);
        pfs_free(blk);
    }

    p_sbi->p_gdt = p_buf;
    return PFS_EXT4_RET_OK;
}

void pfs_ext4_gd_free(pfs_ext4_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    if (NULL != p_sbi->p_gdt)
    {
        pfs_free(p_sbi->p_gdt);
        p_sbi->p_gdt = NULL;
    }
}

int pfs_ext4_gd_get(const pfs_ext4_sbi_t *p_sbi, uint32_t group, pfs_ext4_group_desc_t *p_gd)
{
    const uint8_t *p;
    uint32_t n;

    if (NULL == p_sbi || NULL == p_gd || NULL == p_sbi->p_gdt ||
            group >= p_sbi->groups_count)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    memset(p_gd, 0, sizeof(*p_gd));
    p = p_sbi->p_gdt + (size_t)group * p_sbi->desc_size;
    n = p_sbi->desc_size;
    if (n > sizeof(*p_gd))
    {
        n = (uint32_t)sizeof(*p_gd);
    }
    memcpy(p_gd, p, n);
    // LE -> host for multi-byte (struct is LE on disk; convert used fields)
    p_gd->bg_block_bitmap_lo = pfs_le32_to_cpu(p_gd->bg_block_bitmap_lo);
    p_gd->bg_inode_bitmap_lo = pfs_le32_to_cpu(p_gd->bg_inode_bitmap_lo);
    p_gd->bg_inode_table_lo = pfs_le32_to_cpu(p_gd->bg_inode_table_lo);
    p_gd->bg_free_blocks_count_lo = pfs_le16_to_cpu(p_gd->bg_free_blocks_count_lo);
    p_gd->bg_free_inodes_count_lo = pfs_le16_to_cpu(p_gd->bg_free_inodes_count_lo);
    p_gd->bg_used_dirs_count_lo = pfs_le16_to_cpu(p_gd->bg_used_dirs_count_lo);
    p_gd->bg_flags = pfs_le16_to_cpu(p_gd->bg_flags);
    if (p_sbi->desc_size >= PFS_EXT4_MIN_DESC_SIZE_64BIT)
    {
        p_gd->bg_block_bitmap_hi = pfs_le32_to_cpu(p_gd->bg_block_bitmap_hi);
        p_gd->bg_inode_bitmap_hi = pfs_le32_to_cpu(p_gd->bg_inode_bitmap_hi);
        p_gd->bg_inode_table_hi = pfs_le32_to_cpu(p_gd->bg_inode_table_hi);
        p_gd->bg_free_blocks_count_hi = pfs_le16_to_cpu(p_gd->bg_free_blocks_count_hi);
        p_gd->bg_free_inodes_count_hi = pfs_le16_to_cpu(p_gd->bg_free_inodes_count_hi);
        p_gd->bg_used_dirs_count_hi = pfs_le16_to_cpu(p_gd->bg_used_dirs_count_hi);
    }
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_gd_put(pfs_ext4_sbi_t *p_sbi, uint32_t group, const pfs_ext4_group_desc_t *p_gd)
{
    pfs_ext4_group_desc_t tmp;
    uint32_t desc_per_block;
    uint64_t gdb0;
    uint64_t gdb_block;
    uint32_t idx;
    uint8_t *blk;

    if (NULL == p_sbi || NULL == p_gd || NULL == p_sbi->p_gdt ||
            group >= p_sbi->groups_count)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    tmp = *p_gd;
    tmp.bg_block_bitmap_lo = pfs_cpu_to_le32(tmp.bg_block_bitmap_lo);
    tmp.bg_inode_bitmap_lo = pfs_cpu_to_le32(tmp.bg_inode_bitmap_lo);
    tmp.bg_inode_table_lo = pfs_cpu_to_le32(tmp.bg_inode_table_lo);
    tmp.bg_free_blocks_count_lo = pfs_cpu_to_le16(tmp.bg_free_blocks_count_lo);
    tmp.bg_free_inodes_count_lo = pfs_cpu_to_le16(tmp.bg_free_inodes_count_lo);
    tmp.bg_used_dirs_count_lo = pfs_cpu_to_le16(tmp.bg_used_dirs_count_lo);
    tmp.bg_flags = pfs_cpu_to_le16(tmp.bg_flags);
    if (p_sbi->desc_size >= PFS_EXT4_MIN_DESC_SIZE_64BIT)
    {
        tmp.bg_block_bitmap_hi = pfs_cpu_to_le32(tmp.bg_block_bitmap_hi);
        tmp.bg_inode_bitmap_hi = pfs_cpu_to_le32(tmp.bg_inode_bitmap_hi);
        tmp.bg_inode_table_hi = pfs_cpu_to_le32(tmp.bg_inode_table_hi);
        tmp.bg_free_blocks_count_hi = pfs_cpu_to_le16(tmp.bg_free_blocks_count_hi);
        tmp.bg_free_inodes_count_hi = pfs_cpu_to_le16(tmp.bg_free_inodes_count_hi);
        tmp.bg_used_dirs_count_hi = pfs_cpu_to_le16(tmp.bg_used_dirs_count_hi);
    }

    memcpy(p_sbi->p_gdt + (size_t)group * p_sbi->desc_size, &tmp, p_sbi->desc_size);

    desc_per_block = p_sbi->block_size / p_sbi->desc_size;
    gdb0 = (1024u == p_sbi->block_size) ? 2ull : 1ull;
    gdb_block = gdb0 + group / desc_per_block;
    idx = group % desc_per_block;

    blk = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == blk)
    {
        return PFS_EXT4_RET_ENOMEM;
    }
    int rc = pfs_ext4_read_block(p_sbi, gdb_block, blk);
    if (PFS_EXT4_RET_OK != rc)
    {
        pfs_free(blk);
        return rc;
    }
    memcpy(blk + (size_t)idx * p_sbi->desc_size, &tmp, p_sbi->desc_size);
    rc = pfs_ext4_write_block(p_sbi, gdb_block, blk);
    pfs_free(blk);
    return rc;
}

uint64_t pfs_ext4_gd_block_bitmap(const pfs_ext4_sbi_t *p_sbi, const pfs_ext4_group_desc_t *p_gd)
{
    uint64_t b = p_gd->bg_block_bitmap_lo;
    if (p_sbi->desc_size >= PFS_EXT4_MIN_DESC_SIZE_64BIT)
    {
        b |= ((uint64_t)p_gd->bg_block_bitmap_hi << 32);
    }
    return b;
}

uint64_t pfs_ext4_gd_inode_bitmap(const pfs_ext4_sbi_t *p_sbi, const pfs_ext4_group_desc_t *p_gd)
{
    uint64_t b = p_gd->bg_inode_bitmap_lo;
    if (p_sbi->desc_size >= PFS_EXT4_MIN_DESC_SIZE_64BIT)
    {
        b |= ((uint64_t)p_gd->bg_inode_bitmap_hi << 32);
    }
    return b;
}

uint64_t pfs_ext4_gd_inode_table(const pfs_ext4_sbi_t *p_sbi, const pfs_ext4_group_desc_t *p_gd)
{
    uint64_t b = p_gd->bg_inode_table_lo;
    if (p_sbi->desc_size >= PFS_EXT4_MIN_DESC_SIZE_64BIT)
    {
        b |= ((uint64_t)p_gd->bg_inode_table_hi << 32);
    }
    return b;
}

int pfs_ext4_init(pfs_ext4_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    memset(p_sbi, 0, sizeof(*p_sbi));
    p_sbi->mount_state = PFS_EXT4_UNMOUNTED;
    return PFS_EXT4_RET_OK;
}

void pfs_ext4_deinit(pfs_ext4_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    (void)pfs_ext4_umount(p_sbi);
    memset(p_sbi, 0, sizeof(*p_sbi));
}

int pfs_ext4_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    pfs_ext4_sb_raw_t sb;
    pfs_ext4_sbi_t tmp;

    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EXT4_RET_EBUSY;
    }
    int rc = pfs_ext4_read_sb(p_blkio, part_offset, sb);
    if (PFS_EXT4_RET_OK != rc)
    {
        return (PFS_EXT4_RET_EIO == rc) ? PFS_EXT4_RET_ENOEXEC : rc;
    }
    memset(&tmp, 0, sizeof(tmp));
    memcpy(tmp.sb_raw, sb, sizeof(sb));
    rc = pfs_ext4_parse_sb(&tmp);
    if (PFS_EXT4_RET_OK != rc)
    {
        return (PFS_EXT4_RET_ENOEXEC == rc || PFS_EXT4_RET_EOPNOTSUPP == rc) ?
                rc : PFS_EXT4_RET_ENOEXEC;
    }
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_mount(pfs_ext4_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset,
        pfs_ext4_mount_state_e mode)
{

    if (NULL == p_sbi || NULL == p_blkio || part_offset < 0)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (PFS_EXT4_MOUNT_RDONLY != mode && PFS_EXT4_MOUNT_RDWR != mode)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (PFS_EXT4_UNMOUNTED != p_sbi->mount_state)
    {
        return PFS_EXT4_RET_EBUSY;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EXT4_RET_EBUSY;
    }

    int rc = pfs_ext4_read_sb(p_blkio, part_offset, p_sbi->sb_raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ext4_parse_sb(p_sbi);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    // 需 journal 回放时拒绝 RDWR
    if (PFS_EXT4_MOUNT_RDWR == mode &&
            0u != (p_sbi->feature_incompat & PFS_EXT4_FEATURE_INCOMPAT_RECOVER))
    {
        return PFS_EXT4_RET_EPERM;
    }
    // 未实现 metadata/GDT 校验和更新, 有校验和特征时仅允许 RO
    if (PFS_EXT4_MOUNT_RDWR == mode &&
            0u != (p_sbi->feature_ro_compat &
                    (PFS_EXT4_FEATURE_RO_COMPAT_METADATA_CSUM |
                     PFS_EXT4_FEATURE_RO_COMPAT_GDT_CSUM)))
    {
        return PFS_EXT4_RET_EPERM;
    }

    p_sbi->p_blkio = p_blkio;
    p_sbi->part_offset = part_offset;
    rc = pfs_ext4_gd_load(p_sbi);
    if (PFS_EXT4_RET_OK != rc)
    {
        p_sbi->p_blkio = NULL;
        return rc;
    }

    p_sbi->mount_state = mode;
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_remount(pfs_ext4_sbi_t *p_sbi, pfs_ext4_mount_state_e mode)
{
    if (NULL == p_sbi)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (PFS_EXT4_UNMOUNTED == p_sbi->mount_state)
    {
        return PFS_EXT4_RET_EBUSY;
    }
    if (PFS_EXT4_MOUNT_RDONLY != mode && PFS_EXT4_MOUNT_RDWR != mode)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (PFS_EXT4_MOUNT_RDWR == mode &&
            0u != (p_sbi->feature_incompat & PFS_EXT4_FEATURE_INCOMPAT_RECOVER))
    {
        return PFS_EXT4_RET_EPERM;
    }
    if (PFS_EXT4_MOUNT_RDWR == mode &&
            0u != (p_sbi->feature_ro_compat &
                    (PFS_EXT4_FEATURE_RO_COMPAT_METADATA_CSUM |
                     PFS_EXT4_FEATURE_RO_COMPAT_GDT_CSUM)))
    {
        return PFS_EXT4_RET_EPERM;
    }
    p_sbi->mount_state = mode;
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_umount(pfs_ext4_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    pfs_ext4_gd_free(p_sbi);
    p_sbi->p_blkio = NULL;
    p_sbi->part_offset = 0;
    p_sbi->mount_state = PFS_EXT4_UNMOUNTED;
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_sbi_is_mounted(const pfs_ext4_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_EXT4_UNMOUNTED != p_sbi->mount_state) ? 1 : 0;
}

int pfs_ext4_sbi_can_write(const pfs_ext4_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return 0;
    }
    return (PFS_EXT4_MOUNT_RDWR == p_sbi->mount_state) ? 1 : 0;
}

// end
