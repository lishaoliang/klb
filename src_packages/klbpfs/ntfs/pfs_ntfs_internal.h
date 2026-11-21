// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ntfs_internal.h
/// @brief   NTFS 子模块内部辅助 (对齐 linux-7.1 fs/ntfs)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 仅 pfs_ntfs_*.c 使用, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_NTFS_INTERNAL_H_
#define _PFS_NTFS_INTERNAL_H_

#include "ntfs/pfs_ntfs.h"
#include "pfs_endian.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define PFS_NTFS_SBI_BS(p_sbi)              (&(p_sbi)->bs)

// USA/MST 块大小固定 512 (对齐 NTFS_BLOCK_SIZE; 与逻辑扇区无关)
#define PFS_NTFS_MST_BLOCK_SIZE             512u

// 文件名比较 (对齐 ntfs.h CASE_SENSITIVE / IGNORE_CASE)
#define PFS_NTFS_CASE_SENSITIVE             0
#define PFS_NTFS_IGNORE_CASE                1

// $I30 属性名 (主机序 UTF-16 码元)
#define PFS_NTFS_ATTR_NAME_I30_LEN          4u
extern const uint16_t g_pfs_ntfs_attr_name_i30[PFS_NTFS_ATTR_NAME_I30_LEN];

static inline bool pfs_ntfs_sbi_is_mounted(const pfs_ntfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return false;
    }
    // RDONLY / RDWR 均为已挂载
    return (PFS_NTFS_UNMOUNTED != p_sbi->mount_state);
}

static inline bool pfs_ntfs_sbi_is_rdonly(const pfs_ntfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return false;
    }
    return (PFS_NTFS_MOUNT_RDONLY == p_sbi->mount_state);
}

static inline bool pfs_ntfs_sbi_can_write(const pfs_ntfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return false;
    }
    return (PFS_NTFS_MOUNT_RDWR == p_sbi->mount_state);
}

static inline int pfs_ntfs_map_blkio_rc(int rc)
{
    if (rc >= 0)
    {
        return PFS_NTFS_RET_OK;
    }
    if (PFS_BLKIO_RET_PARAM == rc || PFS_BLKIO_RET_STATE == rc)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    return PFS_NTFS_RET_EIO;
}

// mst.c: 读后 USA 还原; 成功 0; 损坏 PFS_NTFS_RET_EBADMSG
int pfs_ntfs_post_read_mst_fixup(void *p_buf, uint32_t size);
int pfs_ntfs_pre_write_mst_fixup(void *p_buf, uint32_t size);
void pfs_ntfs_post_write_mst_fixup(void *p_buf);

// runlist.c
void pfs_ntfs_runlist_reset(pfs_ntfs_runlist_t *p_rl);
int pfs_ntfs_runlist_decompress(pfs_ntfs_runlist_t *p_out, const ntfs_attr_record_t *p_attr);
int64_t pfs_ntfs_runlist_vcn_to_lcn(const pfs_ntfs_runlist_t *p_rl, int64_t vcn);
int pfs_ntfs_runlist_merge(pfs_ntfs_runlist_t *p_dst, pfs_ntfs_runlist_t *p_src);
int pfs_ntfs_runlist_truncate(pfs_ntfs_runlist_t *p_rl, int64_t new_vcn_count);
int pfs_ntfs_mapping_pairs_build(const pfs_ntfs_runlist_t *p_rl, uint8_t *p_dst, uint32_t dst_cap,
        uint32_t *p_built);

// attrib.c
ntfs_attr_record_t *pfs_ntfs_attr_find_unnamed(ntfs_mft_record_t *p_mrec, uint32_t type_le);
ntfs_attr_record_t *pfs_ntfs_attr_find(ntfs_mft_record_t *p_mrec, uint32_t type_le,
        const uint16_t *name_host_utf16, uint32_t name_len);
int pfs_ntfs_attr_update_data(pfs_ntfs_sbi_t *p_sbi, ntfs_mft_record_t *p_mrec,
        pfs_ntfs_inode_t *p_ino);
int pfs_ntfs_mft_add_resident(ntfs_mft_record_t *p_mrec, uint32_t rec_size, uint32_t type,
        const uint16_t *name, uint32_t name_len, const void *val, uint32_t val_len);
// 追加非常驻属性 (单段 runlist; data_size 可为 allocated)
int pfs_ntfs_mft_add_nonres(ntfs_mft_record_t *p_mrec, uint32_t rec_size, uint32_t type,
        const uint16_t *name, uint32_t name_len, const pfs_ntfs_runlist_t *p_rl,
        uint32_t cluster_size, uint64_t data_size);
int pfs_ntfs_mft_attr_remove(pfs_ntfs_sbi_t *p_sbi, ntfs_mft_record_t *p_mrec, uint32_t rec_size,
        uint32_t type, const uint16_t *name, uint32_t name_len,
        const pfs_ntfs_uni_name_t *p_fn_match);

// mft.c
void pfs_ntfs_mft_reset(pfs_ntfs_mft_t *p_mft);
void pfs_ntfs_mft_rec_reset(pfs_ntfs_mft_rec_t *p_rec);
void pfs_ntfs_index_reset(pfs_ntfs_index_t *p_idx);
void pfs_ntfs_inode_reset(pfs_ntfs_inode_t *p_ino);
int pfs_ntfs_mft_load_bootstrap(pfs_ntfs_sbi_t *p_sbi);
int pfs_ntfs_mft_read_record(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, uint8_t *p_buf);
int pfs_ntfs_inode_load(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, pfs_ntfs_inode_t *p_ino);
int pfs_ntfs_rl_pread(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_runlist_t *p_rl, int64_t data_size,
        int64_t pos, void *buf, int len, int *p_got);
int pfs_ntfs_rl_pwrite(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_runlist_t *p_rl, int64_t data_size,
        int64_t pos, const void *buf, int len, int *p_put);
int pfs_ntfs_mft_write_record(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, uint8_t *p_buf);

// upcase.c
void pfs_ntfs_upcase_reset(pfs_ntfs_upcase_t *p_up);
int pfs_ntfs_upcase_load(pfs_ntfs_sbi_t *p_sbi);

// misc.c
uint64_t pfs_ntfs_current_time_le(void);
int pfs_ntfs_utf8_to_uni(pfs_ntfs_sbi_t *p_sbi, const char *utf8, int len, pfs_ntfs_uni_name_t *p_uni);
int pfs_ntfs_utf16_to_utf8(const uint16_t *p_uni, uint32_t uni_len, char *out, int out_cap);
uint16_t pfs_ntfs_upcase_char(const pfs_ntfs_upcase_t *p_up, uint16_t ch);
int pfs_ntfs_are_names_equal(const uint16_t *s1, uint32_t s1_len, const uint16_t *s2, uint32_t s2_len,
        int ic, const pfs_ntfs_upcase_t *p_up);
int pfs_ntfs_collate_names(const uint16_t *name1, uint32_t name1_len, const uint16_t *name2,
        uint32_t name2_len, int err_val, int ic, const pfs_ntfs_upcase_t *p_up);
int pfs_ntfs_uni_is_dos_compatible(const pfs_ntfs_uni_name_t *p_uni);
// 长名生成唯一 DOS 8.3 短名 (大写; 冲突用 ~n); 需可读父目录 $I30
int pfs_ntfs_uni_make_dos_name(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir_ino,
        const pfs_ntfs_uni_name_t *p_long, pfs_ntfs_uni_name_t *p_dos);

// index.c
int pfs_ntfs_index_read_block(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_index_t *p_idx, int64_t vcn,
        uint8_t *p_buf);
int pfs_ntfs_index_write_block(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_index_t *p_idx, int64_t vcn,
        uint8_t *p_buf);
int pfs_ntfs_index_insert_filename(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir_ino,
        const ntfs_file_name_attr_t *p_fn, uint64_t mref);
int pfs_ntfs_index_remove_filename(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir_ino,
        const pfs_ntfs_uni_name_t *p_uni);
int pfs_ntfs_index_remove_by_mref(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir_ino,
        uint64_t mft_no);
int pfs_ntfs_index_update_filename_dup(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir_ino,
        const ntfs_file_name_attr_t *p_fn);
// 父目录 $I30 是否已有同名项 (含 DOS 短名; 忽略大小写)
int pfs_ntfs_index_has_filename(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_inode_t *p_dir_ino,
        const pfs_ntfs_uni_name_t *p_uni);
int pfs_ntfs_index_dir_is_empty(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_inode_t *p_dir_ino);

// namei.c
void pfs_ntfs_lookup_reset(pfs_ntfs_lookup_t *p_lu);
void pfs_ntfs_uni_name_reset(pfs_ntfs_uni_name_t *p_name);
void pfs_ntfs_create_reset(pfs_ntfs_create_t *p_cr);
int pfs_ntfs_lookup_iname(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_inode_t *p_dir_ino,
        const pfs_ntfs_uni_name_t *p_uni_name, pfs_ntfs_lookup_t *p_lu);
int pfs_ntfs_lookup_path(pfs_ntfs_sbi_t *p_sbi, const char *path, pfs_ntfs_lookup_t *p_lu);
int pfs_ntfs_root_reload(pfs_ntfs_sbi_t *p_sbi);
int pfs_ntfs_create_file(pfs_ntfs_sbi_t *p_sbi, const char *path);
int pfs_ntfs_mkdir(pfs_ntfs_sbi_t *p_sbi, const char *path);
int pfs_ntfs_unlink(pfs_ntfs_sbi_t *p_sbi, const char *path);
int pfs_ntfs_rmdir(pfs_ntfs_sbi_t *p_sbi, const char *path);
int pfs_ntfs_rename(pfs_ntfs_sbi_t *p_sbi, const char *oldpath, const char *newpath);

// dir.c
void pfs_ntfs_dir_meta_reset(pfs_ntfs_dir_meta_t *p_meta);
int pfs_ntfs_dir_open(pfs_ntfs_sbi_t *p_sbi, const char *path, pfs_ntfs_dir_meta_t *p_meta);
int pfs_ntfs_readdir(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_dir_meta_t *p_meta, uint8_t *p_d_type,
        char *utf8_name, int name_cap);

// file.c
void pfs_ntfs_file_meta_reset(pfs_ntfs_file_meta_t *p_meta);
int pfs_ntfs_file_open(pfs_ntfs_sbi_t *p_sbi, const char *path, pfs_ntfs_file_meta_t *p_meta);
int pfs_ntfs_file_read(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_file_meta_t *p_meta, int64_t pos,
        void *buf, int len, int *p_got);
int pfs_ntfs_file_write(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_file_meta_t *p_meta, int64_t pos,
        const void *buf, int len, int *p_put);
int pfs_ntfs_file_truncate(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_file_meta_t *p_meta, int64_t size);
int pfs_ntfs_file_sync(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_file_meta_t *p_meta);

// bitmap.c (写路径; mount 装载后填充)
void pfs_ntfs_lcn_bitmap_reset(pfs_ntfs_lcn_bitmap_t *p_bmp);
void pfs_ntfs_mft_bitmap_reset(pfs_ntfs_mft_bitmap_t *p_bmp);
int pfs_ntfs_lcn_bitmap_load(pfs_ntfs_sbi_t *p_sbi);
int pfs_ntfs_mft_bitmap_load(pfs_ntfs_sbi_t *p_sbi);
int pfs_ntfs_mft_bitmap_set_bit(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, int set);
int pfs_ntfs_mft_record_alloc(pfs_ntfs_sbi_t *p_sbi, int is_dir, uint64_t *p_mft_no,
        uint16_t *p_seq_no, uint8_t **p_rec_buf);
int pfs_ntfs_mft_record_free(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, uint8_t *p_rec);
int pfs_ntfs_bitmap_set_run(pfs_ntfs_sbi_t *p_sbi, int64_t lcn, int64_t count, int set);
int pfs_ntfs_cluster_alloc(pfs_ntfs_sbi_t *p_sbi, int64_t count, pfs_ntfs_alloc_zone_e zone,
        pfs_ntfs_runlist_t *p_out_rl);
int pfs_ntfs_cluster_free_from_rl(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_runlist_t *p_rl,
        int64_t start_vcn, int64_t count);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_NTFS_INTERNAL_H_

// end
