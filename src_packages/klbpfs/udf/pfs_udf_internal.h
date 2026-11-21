// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_udf_internal.h
/// @brief   udf 模块内部辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_UDF_INTERNAL_H_
#define _PFS_UDF_INTERNAL_H_

#include "udf/pfs_udf.h"
#include "pfs_endian.h"
#include "pfs.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint16_t pfs_udf_le16(const void *p)
{
    return pfs_read_le16(p);
}

static inline uint32_t pfs_udf_le32(const void *p)
{
    return pfs_read_le32(p);
}

static inline uint64_t pfs_udf_le64(const void *p)
{
    return pfs_read_le64(p);
}

int pfs_udf_map_blkio_rc(int rc);

uint8_t pfs_udf_tag_checksum(const pfs_udf_tag_t *p_tag);

uint16_t pfs_udf_crc16(const uint8_t *data, int len);

int pfs_udf_verify_tag(const pfs_udf_tag_t *p_tag, uint32_t tag_location,
        const void *p_desc_body, int desc_body_max);

int pfs_udf_read_sector(pfs_blkio_t *p_blkio, int64_t part_offset, uint32_t sector_size,
        uint32_t vol_sector, void *buf);

int64_t pfs_udf_vol_sector_to_off(int64_t part_offset, uint32_t sector_size, uint32_t vol_sector);

int64_t pfs_udf_part_lba_to_off(const pfs_udf_sbi_t *p_sbi, uint32_t part_lba);

int64_t pfs_udf_map_to_off(const pfs_udf_sbi_t *p_sbi, uint16_t part_ref, uint32_t lba);

int pfs_udf_read_vol_block(pfs_blkio_t *p_blkio, int64_t part_offset, uint32_t sector_size,
        uint32_t block_size, uint32_t vol_block, void *buf);

int pfs_udf_read_part_block(const pfs_udf_sbi_t *p_sbi, uint32_t part_lba, void *buf);

int pfs_udf_read_part_block_ref(const pfs_udf_sbi_t *p_sbi, uint16_t part_ref,
        uint32_t part_lba, void *buf);

int pfs_udf_read_phys_block(const pfs_udf_sbi_t *p_sbi, uint16_t type1_ref,
        uint32_t part_lba, void *buf);

uint32_t pfs_udf_part_capacity_blocks(const pfs_udf_sbi_t *p_sbi, uint16_t part_ref);

void pfs_udf_maps_free(pfs_udf_sbi_t *p_sbi);

int pfs_udf_maps_postload(pfs_udf_sbi_t *p_sbi);

int pfs_udf_find_type1_map_idx(const pfs_udf_sbi_t *p_sbi, uint16_t partition_num);

int pfs_udf_pmap_add_type1(pfs_udf_sbi_t *p_sbi, const pfs_udf_partmap1_t *p_m1);

int pfs_udf_pmap_add_type2(pfs_udf_sbi_t *p_sbi, const uint8_t *p_map_raw, uint8_t map_len);

void pfs_udf_pmap_apply_pd(pfs_udf_sbi_t *p_sbi, uint16_t partition_number,
        uint32_t partition_start, uint32_t partition_len);

int pfs_udf_pmap_have_type1_pd(const pfs_udf_sbi_t *p_sbi);

int pfs_udf_add_extent(pfs_udf_inode_t *p_out, uint32_t block, uint32_t skip,
        uint32_t len);

int pfs_udf_read_tagged(pfs_blkio_t *p_blkio, int64_t part_offset, uint32_t sector_size,
        uint32_t block_size, uint32_t vol_block, uint16_t expect_ident, void *buf);

int pfs_udf_read_volumes(pfs_blkio_t *p_blkio, int64_t part_offset, pfs_udf_sbi_t *p_out);

int pfs_udf_icb_read(pfs_udf_sbi_t *p_sbi, const pfs_udf_lb_addr_t *p_loc,
        pfs_udf_inode_t *p_out);

int pfs_udf_icb_from_fe_buf(const pfs_udf_sbi_t *p_sbi, const uint8_t *p_buf,
        int buf_len, uint32_t iblock, uint16_t part_ref, pfs_udf_inode_t *p_out);

int pfs_udf_icb_from_fe_buf_ex(const pfs_udf_sbi_t *p_sbi, const uint8_t *p_buf,
        int buf_len, uint32_t iblock, uint16_t part_ref, pfs_udf_inode_t *p_out,
        int allow_special);

int pfs_udf_unicode_to_utf8(const uint8_t *raw, int raw_len, char *name_out, int name_cap);

bool pfs_udf_name_equal(const char *a, const char *b);

int pfs_udf_find_in_dir(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_dir,
        const char *name, pfs_udf_inode_t *p_out);

int pfs_udf_lookup_path(pfs_udf_sbi_t *p_sbi, const char *path, pfs_udf_inode_t *p_out);

int pfs_udf_parent_dir(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_dir,
        pfs_udf_inode_t *p_parent);

int pfs_udf_dir_read_next(pfs_udf_sbi_t *p_sbi, pfs_udf_dir_meta_t *p_dm,
        pfs_dirent_t *p_ent);

int pfs_udf_file_read(pfs_udf_sbi_t *p_sbi, const pfs_udf_file_meta_t *p_fm,
        int64_t pos, void *buf, int size, int *p_got);

void pfs_udf_wle16(void *p, uint16_t v);
void pfs_udf_wle32(void *p, uint32_t v);
void pfs_udf_wle64(void *p, uint64_t v);
void pfs_udf_set_tag(pfs_udf_tag_t *p_tag, uint16_t ident, uint32_t tag_loc,
        void *p_body, int body_off, int body_crc_len, uint16_t desc_ver);

int pfs_udf_write_part_block_ref(pfs_udf_sbi_t *p_sbi, uint16_t part_ref,
        uint32_t part_lba, const void *buf);
int pfs_udf_write_part_block(pfs_udf_sbi_t *p_sbi, uint32_t part_lba, const void *buf);

int pfs_udf_volume_can_rw(const pfs_udf_sbi_t *p_sbi);
int pfs_udf_bitmap_load(pfs_udf_sbi_t *p_sbi);
int pfs_udf_bitmap_flush(pfs_udf_sbi_t *p_sbi);
void pfs_udf_bitmap_free(pfs_udf_sbi_t *p_sbi);
int pfs_udf_alloc_blocks(pfs_udf_sbi_t *p_sbi, uint32_t count, uint32_t *p_start);
int pfs_udf_free_blocks(pfs_udf_sbi_t *p_sbi, uint32_t start, uint32_t count);

int pfs_udf_utf8_to_cs0(const char *utf8, uint8_t *out, int out_cap, int *p_out_len);

int pfs_udf_file_write(pfs_udf_sbi_t *p_sbi, pfs_udf_file_meta_t *p_fm,
        int64_t pos, const void *buf, int size, int *p_put);
int pfs_udf_file_truncate(pfs_udf_sbi_t *p_sbi, pfs_udf_file_meta_t *p_fm, int64_t size);
int pfs_udf_create_file(pfs_udf_sbi_t *p_sbi, const char *path, pfs_udf_inode_t *p_out);
int pfs_udf_mkdir(pfs_udf_sbi_t *p_sbi, const char *path);
int pfs_udf_unlink(pfs_udf_sbi_t *p_sbi, const char *path);
int pfs_udf_rmdir(pfs_udf_sbi_t *p_sbi, const char *path);

int pfs_udf_find_fid_in_dir(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_dir,
        const char *name, pfs_udf_inode_t *p_child, uint64_t *p_fid_pos, int *p_fid_len);

#ifdef __cplusplus
}
#endif

#endif // _PFS_UDF_INTERNAL_H_

// end
