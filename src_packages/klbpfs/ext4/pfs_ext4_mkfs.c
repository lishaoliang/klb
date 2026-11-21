// Doc Encode : UTF-8 BOM, Unix(LF)
// ext2 / ext3 / ext4 mkfs: 单超块 + GDT + 各组 bitmap/inode 表 + 根目录
#include "ext4/pfs_ext4.h"
#include "ext4/pfs_ext4_internal.h"
#include "pfs_ops.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// sb 字段偏移 (与 pfs_ext4_super.c 一致)
#define OFF_INODES_COUNT        0x000u
#define OFF_BLOCKS_COUNT_LO     0x004u
#define OFF_R_BLOCKS_LO         0x008u
#define OFF_FREE_BLOCKS_LO      0x00Cu
#define OFF_FREE_INODES         0x010u
#define OFF_FIRST_DATA_BLOCK    0x014u
#define OFF_LOG_BLOCK_SIZE      0x018u
#define OFF_BLOCKS_PER_GROUP    0x020u
#define OFF_CLUSTERS_PER_GROUP  0x024u
#define OFF_INODES_PER_GROUP    0x028u
#define OFF_MTIME               0x02Cu
#define OFF_WTIME               0x030u
#define OFF_MAGIC               0x038u
#define OFF_STATE               0x03Au
#define OFF_ERRORS              0x03Cu
#define OFF_CREATOR_OS          0x048u
#define OFF_REV_LEVEL           0x04Cu
#define OFF_FIRST_INO           0x054u
#define OFF_INODE_SIZE          0x058u
#define OFF_BLOCK_GROUP_NR      0x05Au
#define OFF_FEATURE_COMPAT      0x05Cu
#define OFF_FEATURE_INCOMPAT    0x060u
#define OFF_FEATURE_RO_COMPAT   0x064u
#define OFF_UUID                0x068u
#define OFF_JOURNAL_INUM        0x0E0u
#define OFF_MKFS_TIME           0x108u
#define OFF_WANT_EXTRA_ISIZE    0x15Cu
#define OFF_MIN_EXTRA_ISIZE     0x15Eu

#define PFS_EXT_MK_MIN_VOL_BYTES    (1024u * 1024u)
#define PFS_EXT_MK_MAX_GROUPS       1024u
#define PFS_EXT_CREATOR_LINUX       0u
#define PFS_EXT_ERRORS_CONTINUE     1u
#define PFS_EXT_REV_DYNAMIC         1u
#define PFS_EXT_MK_EXT_INLINE_MAX   4u

typedef struct _pfs_ext_mk_layout_t {
    uint32_t block_size;
    uint32_t log_block_size;
    uint32_t first_data_block;
    uint32_t blocks_count;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    uint32_t inode_size;
    uint32_t groups_count;
    uint32_t inodes_count;
    uint32_t desc_size;
    uint32_t gdb_count;
    uint32_t itable_blocks;
    uint32_t feature_compat;
    uint32_t feature_incompat;
    uint32_t feature_ro_compat;
    uint32_t use_extents;
    uint32_t has_journal;
} pfs_ext_mk_layout_t;

typedef struct _pfs_ext_mk_group_t {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t used_dirs;
    uint32_t meta_blocks;
} pfs_ext_mk_group_t;

static int pfs_ext_mk_pwrite(pfs_blkio_t *p_blkio, int64_t part_offset,
        uint32_t block, uint32_t block_size, const void *buf)
{
    int rc = pfs_blkio_pwrite(p_blkio, buf, (int)block_size,
            part_offset + (int64_t)block * (int64_t)block_size);
    if (0 > rc)
    {
        return pfs_ext4_map_blkio_rc(rc);
    }
    else if (rc != (int)block_size)
    {
        return PFS_EXT4_RET_EIO;
    }
    return PFS_EXT4_RET_OK;
}

static void pfs_ext_mk_bitmap_set(uint8_t *bm, uint32_t bit)
{
    bm[bit / 8u] |= (uint8_t)(1u << (bit % 8u));
}

static bool pfs_ext_mk_group_has_sb(uint32_t group)
{

    if (0u == group || 1u == group)
    {
        return true;
    }
    // sparse_super: 3/5/7 的幂
    for (uint32_t i = 3; i <= group; i *= 3u)
    {
        if (i == group)
        {
            return true;
        }
        if (i > group / 3u)
        {
            break;
        }
    }
    for (int i = 5; i <= group; i *= 5u)
    {
        if (i == group)
        {
            return true;
        }
        if (i > group / 5u)
        {
            break;
        }
    }
    for (int i = 7; i <= group; i *= 7u)
    {
        if (i == group)
        {
            return true;
        }
        if (i > group / 7u)
        {
            break;
        }
    }
    return false;
}

static int pfs_ext_mk_calc(pfs_fs_type_e fs_type, uint64_t vol_bytes, pfs_ext_mk_layout_t *p_lay)
{
    memset(p_lay, 0, sizeof(*p_lay));

    if (vol_bytes < PFS_EXT_MK_MIN_VOL_BYTES)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (0u != (vol_bytes % 512ull))
    {
        return PFS_EXT4_RET_EINVAL;
    }

    if (vol_bytes >= (16ull * 1024ull * 1024ull))
    {
        p_lay->block_size = 4096u;
        p_lay->log_block_size = 2u;
        p_lay->first_data_block = 0u;
    }
    else
    {
        p_lay->block_size = 1024u;
        p_lay->log_block_size = 0u;
        p_lay->first_data_block = 1u;
    }

    uint64_t blocks_u = vol_bytes / (uint64_t)p_lay->block_size;
    if (blocks_u < 64ull || blocks_u > (uint64_t)UINT32_MAX)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    p_lay->blocks_count = (uint32_t)blocks_u;
    p_lay->blocks_per_group = p_lay->block_size * 8u;
    p_lay->desc_size = PFS_EXT4_MIN_DESC_SIZE;

    if (PFS_EXT4 == fs_type)
    {
        p_lay->inode_size = 256u;
        p_lay->use_extents = 1u;
        p_lay->feature_incompat = PFS_EXT4_FEATURE_INCOMPAT_FILETYPE |
                PFS_EXT4_FEATURE_INCOMPAT_EXTENTS;
        p_lay->feature_ro_compat = PFS_EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER |
                PFS_EXT4_FEATURE_RO_COMPAT_LARGE_FILE |
                PFS_EXT4_FEATURE_RO_COMPAT_DIR_NLINK |
                PFS_EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE;
        p_lay->feature_compat = 0u;
        p_lay->has_journal = 0u;
    }
    else if (PFS_EXT3 == fs_type)
    {
        p_lay->inode_size = PFS_EXT4_GOOD_OLD_INODE_SIZE;
        p_lay->use_extents = 0u;
        p_lay->has_journal = 1u;
        p_lay->feature_incompat = PFS_EXT4_FEATURE_INCOMPAT_FILETYPE;
        p_lay->feature_ro_compat = PFS_EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER |
                PFS_EXT4_FEATURE_RO_COMPAT_LARGE_FILE;
        p_lay->feature_compat = PFS_EXT4_FEATURE_COMPAT_HAS_JOURNAL;
    }
    else if (PFS_EXT2 == fs_type)
    {
        p_lay->inode_size = PFS_EXT4_GOOD_OLD_INODE_SIZE;
        p_lay->use_extents = 0u;
        p_lay->has_journal = 0u;
        p_lay->feature_incompat = PFS_EXT4_FEATURE_INCOMPAT_FILETYPE;
        p_lay->feature_ro_compat = PFS_EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER |
                PFS_EXT4_FEATURE_RO_COMPAT_LARGE_FILE;
        p_lay->feature_compat = 0u;
    }
    else
    {
        return PFS_EXT4_RET_EINVAL;
    }

    uint32_t ipg = p_lay->blocks_per_group / 4u;
    if (ipg < 16u)
    {
        ipg = 16u;
    }
    if (ipg > p_lay->blocks_per_group)
    {
        ipg = p_lay->blocks_per_group;
    }
    while (0u != ((ipg * p_lay->inode_size) % p_lay->block_size) && ipg > 16u)
    {
        ipg--;
    }
    p_lay->inodes_per_group = ipg;
    uint32_t itb = (ipg * p_lay->inode_size + p_lay->block_size - 1u) / p_lay->block_size;
    p_lay->itable_blocks = itb;

    uint32_t groups = (p_lay->blocks_count - p_lay->first_data_block + p_lay->blocks_per_group - 1u) /
            p_lay->blocks_per_group;
    if (0u == groups || groups > PFS_EXT_MK_MAX_GROUPS)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    p_lay->groups_count = groups;
    p_lay->inodes_count = groups * ipg;

    p_lay->gdb_count = (groups * p_lay->desc_size + p_lay->block_size - 1u) / p_lay->block_size;

    uint32_t overhead0 = 1u + p_lay->gdb_count + 2u + itb + 1u;
    uint32_t min_blocks = p_lay->first_data_block + overhead0;
    if (p_lay->blocks_count < min_blocks)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    return PFS_EXT4_RET_OK;
}

static void pfs_ext_mk_fill_group(const pfs_ext_mk_layout_t *p_lay, uint32_t group,
        pfs_ext_mk_group_t *p_g)
{
    uint32_t start = p_lay->first_data_block + group * p_lay->blocks_per_group;
    uint32_t group_blocks;
    if (group + 1u == p_lay->groups_count)
    {
        group_blocks = p_lay->blocks_count - start;
    }
    else
    {
        group_blocks = p_lay->blocks_per_group;
    }

    uint32_t has_sb = pfs_ext_mk_group_has_sb(group) ? 1u : 0u;
    uint32_t used = 0u;
    if (has_sb)
    {
        // 组0 / 备份组: 超块块 + GDT
        used = 1u + p_lay->gdb_count;
    }

    p_g->block_bitmap = start + used;
    p_g->inode_bitmap = p_g->block_bitmap + 1u;
    p_g->inode_table = p_g->inode_bitmap + 1u;
    used += 2u + p_lay->itable_blocks;
    p_g->meta_blocks = used;

    if (used >= group_blocks)
    {
        p_g->free_blocks = 0u;
    }
    else
    {
        p_g->free_blocks = group_blocks - used;
    }

    // 组0: 预留 inode 1..10; 根 = 2, journal = 8 (若有)
    if (0u == group)
    {
        p_g->free_inodes = p_lay->inodes_per_group - (PFS_EXT4_GOOD_OLD_FIRST_INO - 1u);
        p_g->used_dirs = 1u;
    }
    else
    {
        p_g->free_inodes = p_lay->inodes_per_group;
        p_g->used_dirs = 0u;
    }
}

static void pfs_ext_mk_fill_sb(pfs_ext4_sb_raw_t sb, const pfs_ext_mk_layout_t *p_lay,
        uint32_t free_blocks, uint32_t free_inodes, uint32_t now)
{
    memset(sb, 0, PFS_EXT4_SUPER_SIZE);
    pfs_ext4_sb_set_u32(sb, OFF_INODES_COUNT, p_lay->inodes_count);
    pfs_ext4_sb_set_u32(sb, OFF_BLOCKS_COUNT_LO, p_lay->blocks_count);
    pfs_ext4_sb_set_u32(sb, OFF_R_BLOCKS_LO, 0u);
    pfs_ext4_sb_set_u32(sb, OFF_FREE_BLOCKS_LO, free_blocks);
    pfs_ext4_sb_set_u32(sb, OFF_FREE_INODES, free_inodes);
    pfs_ext4_sb_set_u32(sb, OFF_FIRST_DATA_BLOCK, p_lay->first_data_block);
    pfs_ext4_sb_set_u32(sb, OFF_LOG_BLOCK_SIZE, p_lay->log_block_size);
    pfs_ext4_sb_set_u32(sb, OFF_BLOCKS_PER_GROUP, p_lay->blocks_per_group);
    pfs_ext4_sb_set_u32(sb, OFF_CLUSTERS_PER_GROUP, p_lay->blocks_per_group);
    pfs_ext4_sb_set_u32(sb, OFF_INODES_PER_GROUP, p_lay->inodes_per_group);
    pfs_ext4_sb_set_u32(sb, OFF_MTIME, now);
    pfs_ext4_sb_set_u32(sb, OFF_WTIME, now);
    pfs_ext4_sb_set_u16(sb, OFF_MAGIC, PFS_EXT4_SUPER_MAGIC);
    pfs_ext4_sb_set_u16(sb, OFF_STATE, PFS_EXT4_VALID_FS);
    pfs_ext4_sb_set_u16(sb, OFF_ERRORS, PFS_EXT_ERRORS_CONTINUE);
    pfs_ext4_sb_set_u32(sb, OFF_CREATOR_OS, PFS_EXT_CREATOR_LINUX);
    pfs_ext4_sb_set_u32(sb, OFF_REV_LEVEL, PFS_EXT_REV_DYNAMIC);
    pfs_ext4_sb_set_u32(sb, OFF_FIRST_INO, PFS_EXT4_GOOD_OLD_FIRST_INO);
    pfs_ext4_sb_set_u16(sb, OFF_INODE_SIZE, (uint16_t)p_lay->inode_size);
    pfs_ext4_sb_set_u16(sb, OFF_BLOCK_GROUP_NR, 0u);
    pfs_ext4_sb_set_u32(sb, OFF_FEATURE_COMPAT, p_lay->feature_compat);
    pfs_ext4_sb_set_u32(sb, OFF_FEATURE_INCOMPAT, p_lay->feature_incompat);
    pfs_ext4_sb_set_u32(sb, OFF_FEATURE_RO_COMPAT, p_lay->feature_ro_compat);
    pfs_ext4_sb_set_u32(sb, OFF_MKFS_TIME, now);
    if (0u != p_lay->has_journal)
    {
        pfs_ext4_sb_set_u32(sb, OFF_JOURNAL_INUM, PFS_EXT4_JOURNAL_INO);
    }
    if (p_lay->inode_size > PFS_EXT4_GOOD_OLD_INODE_SIZE)
    {
        pfs_ext4_sb_set_u16(sb, OFF_WANT_EXTRA_ISIZE, 32u);
        pfs_ext4_sb_set_u16(sb, OFF_MIN_EXTRA_ISIZE, 32u);
    }
    pfs_write_le32(&sb[OFF_UUID], now);
    pfs_write_le32(&sb[OFF_UUID + 4], now ^ 0xA5A5A5A5u);
}

static void pfs_ext_mk_pack_gd(uint8_t *p_dst, const pfs_ext_mk_group_t *p_g)
{
    memset(p_dst, 0, PFS_EXT4_MIN_DESC_SIZE);
    pfs_write_le32(p_dst + 0, p_g->block_bitmap);
    pfs_write_le32(p_dst + 4, p_g->inode_bitmap);
    pfs_write_le32(p_dst + 8, p_g->inode_table);
    pfs_write_le16(p_dst + 12, (uint16_t)p_g->free_blocks);
    pfs_write_le16(p_dst + 14, (uint16_t)p_g->free_inodes);
    pfs_write_le16(p_dst + 16, (uint16_t)p_g->used_dirs);
}

// 字段按本栈惯例写主机序 (LE 主机上等同 on-disk)
static void pfs_ext_mk_init_inode(pfs_ext4_inode_raw_t *p_raw, uint16_t mode, uint16_t links,
        uint32_t now, uint32_t blocks_512, uint32_t flags, uint64_t size)
{
    memset(p_raw, 0, sizeof(*p_raw));
    p_raw->i_mode = mode;
    p_raw->i_links_count = links;
    p_raw->i_atime = now;
    p_raw->i_ctime = now;
    p_raw->i_mtime = now;
    p_raw->i_blocks_lo = blocks_512;
    p_raw->i_flags = flags;
    p_raw->i_size_lo = (uint32_t)size;
    p_raw->i_size_high = (uint32_t)(size >> 32);
}

static void pfs_ext_mk_root_dir_block(uint8_t *blk, uint32_t block_size)
{
    memset(blk, 0, block_size);

    pfs_ext4_dir_entry_2_t *p_de = (pfs_ext4_dir_entry_2_t *)(void *)blk;
    p_de->inode = PFS_EXT4_ROOT_INO;
    p_de->rec_len = 12;
    p_de->name_len = 1;
    p_de->file_type = PFS_EXT4_FT_DIR;
    p_de->name[0] = '.';

    p_de = (pfs_ext4_dir_entry_2_t *)(void *)(blk + 12);
    p_de->inode = PFS_EXT4_ROOT_INO;
    p_de->rec_len = (uint16_t)(block_size - 12u);
    p_de->name_len = 2;
    p_de->file_type = PFS_EXT4_FT_DIR;
    p_de->name[0] = '.';
    p_de->name[1] = '.';
}

static void pfs_ext_mk_root_extents(pfs_ext4_inode_raw_t *p_root, uint32_t root_data)
{
    uint8_t *p = (uint8_t *)(void *)p_root->i_block;

    memset(p_root->i_block, 0, sizeof(p_root->i_block));
    pfs_write_le16(p + 0, PFS_EXT4_EXT_MAGIC);
    pfs_write_le16(p + 2, 1u);
    pfs_write_le16(p + 4, PFS_EXT_MK_EXT_INLINE_MAX);
    pfs_write_le16(p + 6, 0u);
    pfs_write_le32(p + 8, 0u);
    pfs_write_le32(p + 12, 0u);                 // ee_block
    pfs_write_le16(p + 16, 1u);                 // ee_len
    pfs_write_le16(p + 18, 0u);                 // ee_start_hi
    pfs_write_le32(p + 20, root_data);          // ee_start_lo
}

static int pfs_ext_mkfs_common(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes,
        pfs_fs_type_e fs_type)
{
    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EXT4_RET_EINVAL;
    }

    pfs_ext_mk_layout_t lay;
    int rc = pfs_ext_mk_calc(fs_type, vol_bytes, &lay);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    pfs_ext_mk_group_t *groups = NULL;
    pfs_ext4_sb_raw_t sb;
    uint8_t *p_blk = NULL;
    uint8_t *p_gdt = NULL;
    uint8_t *p_bm = NULL;
    uint8_t *p_itable = NULL;

    groups = (pfs_ext_mk_group_t *)pfs_mallocz((size_t)lay.groups_count * sizeof(*groups));
    p_blk = (uint8_t *)pfs_malloc(lay.block_size);
    p_gdt = (uint8_t *)pfs_mallocz((size_t)lay.gdb_count * lay.block_size);
    p_bm = (uint8_t *)pfs_malloc(lay.block_size);
    if (NULL == groups || NULL == p_blk || NULL == p_gdt || NULL == p_bm)
    {
        rc = PFS_EXT4_RET_ENOMEM;
        goto out;
    }

    uint32_t free_blocks = 0u;
    uint32_t free_inodes = 0u;
    for (uint32_t g = 0; g < lay.groups_count; g++)
    {
        pfs_ext_mk_fill_group(&lay, g, &groups[g]);
        if (groups[g].meta_blocks >
                (lay.blocks_count - (lay.first_data_block + g * lay.blocks_per_group)))
        {
            rc = PFS_EXT4_RET_ENOSPC;
            goto out;
        }
        free_blocks += groups[g].free_blocks;
        free_inodes += groups[g].free_inodes;
        pfs_ext_mk_pack_gd(p_gdt + (size_t)g * lay.desc_size, &groups[g]);
    }

    if (groups[0].free_blocks < 1u)
    {
        rc = PFS_EXT4_RET_ENOSPC;
        goto out;
    }
    uint32_t root_data = groups[0].inode_table + lay.itable_blocks;
    groups[0].free_blocks--;
    free_blocks--;
    pfs_ext_mk_pack_gd(p_gdt, &groups[0]);

    uint32_t now = (uint32_t)time(NULL);
    pfs_ext_mk_fill_sb(sb, &lay, free_blocks, free_inodes, now);

    memset(p_blk, 0, lay.block_size);
    if (1024u == lay.block_size)
    {
        memcpy(p_blk, sb, PFS_EXT4_SUPER_SIZE);
        rc = pfs_ext_mk_pwrite(p_blkio, part_offset, 1u, lay.block_size, p_blk);
    }
    else
    {
        memcpy(p_blk + PFS_EXT4_SUPER_OFFSET, sb, PFS_EXT4_SUPER_SIZE);
        rc = pfs_ext_mk_pwrite(p_blkio, part_offset, 0u, lay.block_size, p_blk);
    }
    if (PFS_EXT4_RET_OK != rc)
    {
        goto out;
    }

    {
        uint32_t gdb0 = (1024u == lay.block_size) ? 2u : 1u;
        for (uint32_t i = 0; i < lay.gdb_count; i++)
        {
            rc = pfs_ext_mk_pwrite(p_blkio, part_offset, gdb0 + i, lay.block_size,
                    p_gdt + (size_t)i * lay.block_size);
            if (PFS_EXT4_RET_OK != rc)
            {
                goto out;
            }
        }
    }

    for (uint32_t g = 0; g < lay.groups_count; g++)
    {
        uint32_t start = lay.first_data_block + g * lay.blocks_per_group;
        uint32_t group_blocks;
        uint32_t it_bytes;

        if (g + 1u == lay.groups_count)
        {
            group_blocks = lay.blocks_count - start;
        }
        else
        {
            group_blocks = lay.blocks_per_group;
        }

        memset(p_bm, 0, lay.block_size);
        for (uint32_t b = 0; b < groups[g].meta_blocks && b < group_blocks; b++)
        {
            pfs_ext_mk_bitmap_set(p_bm, b);
        }
        if (0u == g)
        {
            pfs_ext_mk_bitmap_set(p_bm, root_data - start);
        }
        for (uint32_t b = group_blocks; b < lay.blocks_per_group && (b / 8u) < lay.block_size; b++)
        {
            pfs_ext_mk_bitmap_set(p_bm, b);
        }
        rc = pfs_ext_mk_pwrite(p_blkio, part_offset, groups[g].block_bitmap, lay.block_size, p_bm);
        if (PFS_EXT4_RET_OK != rc)
        {
            goto out;
        }

        memset(p_bm, 0, lay.block_size);
        if (0u == g)
        {
            for (uint32_t b = 1u; b < PFS_EXT4_GOOD_OLD_FIRST_INO; b++)
            {
                pfs_ext_mk_bitmap_set(p_bm, b - 1u);
            }
        }
        rc = pfs_ext_mk_pwrite(p_blkio, part_offset, groups[g].inode_bitmap, lay.block_size, p_bm);
        if (PFS_EXT4_RET_OK != rc)
        {
            goto out;
        }

        it_bytes = lay.itable_blocks * lay.block_size;
        p_itable = (uint8_t *)pfs_mallocz(it_bytes);
        if (NULL == p_itable)
        {
            rc = PFS_EXT4_RET_ENOMEM;
            goto out;
        }

        if (0u == g)
        {
            pfs_ext4_inode_raw_t *p_root;
            uint32_t flags = 0u;
            uint32_t blocks_512 = lay.block_size / 512u;

            if (0u != lay.use_extents)
            {
                flags = PFS_EXT4_EXTENTS_FL;
            }
            p_root = (pfs_ext4_inode_raw_t *)(void *)(p_itable +
                    (size_t)(PFS_EXT4_ROOT_INO - 1u) * lay.inode_size);
            pfs_ext_mk_init_inode(p_root,
                    (uint16_t)(PFS_EXT4_S_IFDIR | 0755), 2u, now, blocks_512, flags,
                    lay.block_size);
            if (0u != lay.use_extents)
            {
                pfs_ext_mk_root_extents(p_root, root_data);
            }
            else
            {
                p_root->i_block[0] = root_data;
            }
            if (lay.inode_size >= 256u)
            {
                pfs_write_le16(p_itable + (size_t)(PFS_EXT4_ROOT_INO - 1u) * lay.inode_size + 128u,
                        32u);
            }

            if (0u != lay.has_journal)
            {
                pfs_ext4_inode_raw_t *p_jnl;
                p_jnl = (pfs_ext4_inode_raw_t *)(void *)(p_itable +
                        (size_t)(PFS_EXT4_JOURNAL_INO - 1u) * lay.inode_size);
                // 空 journal inode (compat 已标 HAS_JOURNAL); 本栈不回放
                pfs_ext_mk_init_inode(p_jnl,
                        (uint16_t)(PFS_EXT4_S_IFREG | 0600), 1u, now, 0u, 0u, 0ull);
            }
        }

        for (int i = 0; i < lay.itable_blocks; i++)
        {
            rc = pfs_ext_mk_pwrite(p_blkio, part_offset, groups[g].inode_table + i,
                    lay.block_size, p_itable + (size_t)i * lay.block_size);
            if (PFS_EXT4_RET_OK != rc)
            {
                pfs_free(p_itable);
                p_itable = NULL;
                goto out;
            }
        }
        pfs_free(p_itable);
        p_itable = NULL;
    }

    pfs_ext_mk_root_dir_block(p_blk, lay.block_size);
    rc = pfs_ext_mk_pwrite(p_blkio, part_offset, root_data, lay.block_size, p_blk);
    if (PFS_EXT4_RET_OK != rc)
    {
        goto out;
    }

    rc = pfs_blkio_fsync(p_blkio);
    if (0 > rc)
    {
        rc = pfs_ext4_map_blkio_rc(rc);
        goto out;
    }
    rc = PFS_EXT4_RET_OK;

out:
    pfs_free(groups);
    pfs_free(p_blk);
    pfs_free(p_gdt);
    pfs_free(p_bm);
    pfs_free(p_itable);
    return rc;
}

int pfs_ext2_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_ext_mkfs_common(p_blkio, part_offset, vol_bytes, PFS_EXT2);
}

int pfs_ext3_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_ext_mkfs_common(p_blkio, part_offset, vol_bytes, PFS_EXT3);
}

int pfs_ext4_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_ext_mkfs_common(p_blkio, part_offset, vol_bytes, PFS_EXT4);
}

// end
