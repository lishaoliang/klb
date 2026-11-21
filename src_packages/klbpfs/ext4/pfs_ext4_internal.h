// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ext4_internal.h
/// @brief   ext4 模块内部辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_EXT4_INTERNAL_H_
#define _PFS_EXT4_INTERNAL_H_

#include "ext4/pfs_ext4.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- misc / sb field ---
uint32_t pfs_ext4_sb_u32(const pfs_ext4_sb_raw_t sb, unsigned off);
uint16_t pfs_ext4_sb_u16(const pfs_ext4_sb_raw_t sb, unsigned off);
void pfs_ext4_sb_set_u32(pfs_ext4_sb_raw_t sb, unsigned off, uint32_t v);
void pfs_ext4_sb_set_u16(pfs_ext4_sb_raw_t sb, unsigned off, uint16_t v);
int pfs_ext4_map_blkio_rc(int rc);
int64_t pfs_ext4_block_to_off(const pfs_ext4_sbi_t *p_sbi, uint64_t block);
int pfs_ext4_read_block(pfs_ext4_sbi_t *p_sbi, uint64_t block, void *buf);
int pfs_ext4_write_block(pfs_ext4_sbi_t *p_sbi, uint64_t block, const void *buf);
int pfs_ext4_sync_sb(pfs_ext4_sbi_t *p_sbi);

// --- group desc ---
int pfs_ext4_gd_load(pfs_ext4_sbi_t *p_sbi);
void pfs_ext4_gd_free(pfs_ext4_sbi_t *p_sbi);
int pfs_ext4_gd_get(const pfs_ext4_sbi_t *p_sbi, uint32_t group, pfs_ext4_group_desc_t *p_gd);
int pfs_ext4_gd_put(pfs_ext4_sbi_t *p_sbi, uint32_t group, const pfs_ext4_group_desc_t *p_gd);
uint64_t pfs_ext4_gd_block_bitmap(const pfs_ext4_sbi_t *p_sbi, const pfs_ext4_group_desc_t *p_gd);
uint64_t pfs_ext4_gd_inode_bitmap(const pfs_ext4_sbi_t *p_sbi, const pfs_ext4_group_desc_t *p_gd);
uint64_t pfs_ext4_gd_inode_table(const pfs_ext4_sbi_t *p_sbi, const pfs_ext4_group_desc_t *p_gd);

// --- inode ---
int pfs_ext4_inode_read(pfs_ext4_sbi_t *p_sbi, uint32_t ino, pfs_ext4_inode_raw_t *p_raw);
int pfs_ext4_inode_write(pfs_ext4_sbi_t *p_sbi, uint32_t ino, const pfs_ext4_inode_raw_t *p_raw);
void pfs_ext4_inode_to_meta(const pfs_ext4_inode_raw_t *p_raw, pfs_ext4_file_meta_t *p_meta);
uint64_t pfs_ext4_inode_size(const pfs_ext4_inode_raw_t *p_raw);

// --- map file logical block -> physical ---
// *p_pblk=0 表示空洞; 成功 OK
int pfs_ext4_map_block(pfs_ext4_sbi_t *p_sbi, const uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t flags, uint32_t logical, uint64_t *p_pblk);
// 写路径: 确保 logical 有物理块 (可新分配); 更新 i_block[] 与 *p_blocks_512
int pfs_ext4_map_block_alloc(pfs_ext4_sbi_t *p_sbi, uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t *p_flags, uint32_t *p_blocks_512, uint32_t logical, uint64_t *p_pblk);

int pfs_ext4_blockmap_get(pfs_ext4_sbi_t *p_sbi, const uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t logical, uint64_t *p_pblk);
int pfs_ext4_extents_get(pfs_ext4_sbi_t *p_sbi, const uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t logical, uint64_t *p_pblk);
int pfs_ext4_blockmap_alloc(pfs_ext4_sbi_t *p_sbi, uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t *p_blocks_512, uint32_t logical, uint64_t *p_pblk);
int pfs_ext4_extents_alloc(pfs_ext4_sbi_t *p_sbi, uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t *p_blocks_512, uint32_t logical, uint64_t *p_pblk);

// --- balloc / ialloc ---
int pfs_ext4_balloc_new(pfs_ext4_sbi_t *p_sbi, uint64_t *p_block);
int pfs_ext4_balloc_free(pfs_ext4_sbi_t *p_sbi, uint64_t block);
int pfs_ext4_ialloc_new(pfs_ext4_sbi_t *p_sbi, uint32_t *p_ino);
int pfs_ext4_ialloc_free(pfs_ext4_sbi_t *p_sbi, uint32_t ino);

// --- dir / namei / file ---
int pfs_ext4_dir_lookup(pfs_ext4_sbi_t *p_sbi, uint32_t dir_ino,
        const uint32_t dir_i_block[PFS_EXT4_N_BLOCKS], uint32_t dir_flags, uint64_t dir_size,
        const char *name, uint32_t *p_ino, uint8_t *p_ftype);
int pfs_ext4_dir_readdir(pfs_ext4_sbi_t *p_sbi, pfs_ext4_dir_meta_t *p_dir,
        char *name, int name_cap, pfs_ext4_type_e *p_type, uint32_t *p_child_ino);
int pfs_ext4_dir_add(pfs_ext4_sbi_t *p_sbi, uint32_t dir_ino, pfs_ext4_inode_raw_t *p_dir_raw,
        const char *name, uint32_t child_ino, uint8_t ftype);
int pfs_ext4_dir_remove(pfs_ext4_sbi_t *p_sbi, uint32_t dir_ino, pfs_ext4_inode_raw_t *p_dir_raw,
        const char *name);

int pfs_ext4_path_lookup(pfs_ext4_sbi_t *p_sbi, const char *path, uint32_t *p_ino,
        pfs_ext4_inode_raw_t *p_raw);
int pfs_ext4_path_parent(pfs_ext4_sbi_t *p_sbi, const char *path, uint32_t *p_dir_ino,
        pfs_ext4_inode_raw_t *p_dir_raw, char *leaf, size_t leaf_cap);

int pfs_ext4_file_open(pfs_ext4_sbi_t *p_sbi, const char *path, int mode,
        pfs_ext4_file_meta_t *p_meta);
int pfs_ext4_file_read(pfs_ext4_sbi_t *p_sbi, pfs_ext4_file_meta_t *p_meta,
        int64_t pos, void *buf, int size, int *p_got);
int pfs_ext4_file_write(pfs_ext4_sbi_t *p_sbi, pfs_ext4_file_meta_t *p_meta,
        int64_t pos, const void *buf, int size, int *p_put);
int pfs_ext4_file_truncate(pfs_ext4_sbi_t *p_sbi, pfs_ext4_file_meta_t *p_meta, int64_t size);
int pfs_ext4_file_sync(pfs_ext4_sbi_t *p_sbi, pfs_ext4_file_meta_t *p_meta);

int pfs_ext4_mkdir(pfs_ext4_sbi_t *p_sbi, const char *path);
int pfs_ext4_rmdir(pfs_ext4_sbi_t *p_sbi, const char *path);
int pfs_ext4_unlink(pfs_ext4_sbi_t *p_sbi, const char *path);
int pfs_ext4_rename(pfs_ext4_sbi_t *p_sbi, const char *oldpath, const char *newpath);
int pfs_ext4_access(pfs_ext4_sbi_t *p_sbi, const char *path, int mode);

#ifdef __cplusplus
}
#endif

#endif // _PFS_EXT4_INTERNAL_H_

// end
