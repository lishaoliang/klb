// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_udf.h
/// @brief   UDF 子模块对外接口
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_UDF_H_
#define _PFS_UDF_H_

#include "pfs_blkio.h"
#include "udf/pfs_udf_raw.h"
#include "base/pfs_fs_ops.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _pfs_udf_ret_e {
    PFS_UDF_RET_OK            =  0,
    PFS_UDF_RET_EPERM         = -1,
    PFS_UDF_RET_ENOENT        = -2,
    PFS_UDF_RET_EIO           = -5,
    PFS_UDF_RET_ENOEXEC       = -8,
    PFS_UDF_RET_ENOMEM        = -12,
    PFS_UDF_RET_EBUSY         = -16,
    PFS_UDF_RET_EEXIST        = -17,
    PFS_UDF_RET_ENOTDIR       = -20,
    PFS_UDF_RET_EISDIR        = -21,
    PFS_UDF_RET_EINVAL        = -22,
    PFS_UDF_RET_ENOSPC        = -28,
    PFS_UDF_RET_EROFS         = -30,
    PFS_UDF_RET_ERANGE        = -34,
    PFS_UDF_RET_ENAMETOOLONG  = -36,
    PFS_UDF_RET_ENOTEMPTY     = -39,
    PFS_UDF_RET_EBADMSG       = -74,
    PFS_UDF_RET_EILSEQ        = -84,
    PFS_UDF_RET_EOPNOTSUPP    = -95,
} pfs_udf_ret_e;

typedef enum _pfs_udf_mount_state_e {
    PFS_UDF_UNMOUNTED         = 0x00,
    PFS_UDF_MOUNT_RDONLY      = 0x01,
    PFS_UDF_MOUNT_RDWR        = 0x02,
} pfs_udf_mount_state_e;

typedef enum _pfs_udf_type_e {
    PFS_UDF_TYPE_FILE         = 1,
    PFS_UDF_TYPE_DIR          = 2,
} pfs_udf_type_e;

#define PFS_UDF_MAX_EXTENTS             64
#define PFS_UDF_MAX_MAPS                8
#define PFS_UDF_MAX_VAT_ENTRIES         (1u << 20)

enum {
    PFS_UDF_PMAP_TYPE1    = 1,
    PFS_UDF_PMAP_VIRT15   = 2,
    PFS_UDF_PMAP_VIRT20   = 3,
    PFS_UDF_PMAP_META25   = 4,
    PFS_UDF_PMAP_SPAR15   = 5,
};

typedef struct _pfs_udf_extent_t {
    uint32_t block;                                     ///< logical block in partition
    uint32_t len;                                       ///< [单位:字节]
    uint32_t skip;                                      ///< skip bytes at extent start (IN_ICB)
} pfs_udf_extent_t;

typedef struct _pfs_udf_inode_t {
    pfs_udf_type_e          type;
    uint64_t                size;
    uint32_t                iblock;                     ///< FE partition-relative LBA
    uint16_t                part_ref;
    int                     alloc_type;                 ///< ICBTAG_FLAG_AD_*
    int                     n_extents;
    pfs_udf_extent_t        sec[PFS_UDF_MAX_EXTENTS];
} pfs_udf_inode_t;

typedef struct _pfs_udf_pmap_t {
    uint16_t                type;
    uint16_t                partition_num;
    uint16_t                vol_seq;
    uint32_t                phys_start;
    uint32_t                phys_len;
    uint16_t                vat_start_offset;
    uint32_t                vat_entries;
    uint32_t               *vat_entries_tbl;
    uint16_t                vat_phys_part_ref;
    uint32_t                meta_file_loc;
    uint32_t                mirror_file_loc;
    uint16_t                meta_phys_part_ref;
    pfs_udf_inode_t         meta_inode;
    int                     have_meta_inode;
} pfs_udf_pmap_t;

typedef struct _pfs_udf_sbi_t {
    pfs_udf_mount_state_e   mount_state;
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset;                ///< [单位:字节]

    uint32_t                sector_size;                ///< 512 or 2048
    uint32_t                block_size;                 ///< logical block from LVD
    uint32_t                blocks_per_sector;          ///< = block_size/sector_size (sectors per logical block)
    uint32_t                partition_start;            ///< primary type1 PD start [单位:sector_size]
    uint32_t                partition_len;              ///< primary type1 PD length [单位:block]
    uint16_t                partition_num;
    pfs_udf_pmap_t          maps[PFS_UDF_MAX_MAPS];
    int                     n_maps;
    pfs_udf_inode_t         root;
    uint32_t                pd_access_type;
    uint16_t                bitmap_part_ref;
    uint32_t                bitmap_lba;
    uint32_t                bitmap_blocks;
    uint32_t                bitmap_num_bits;
    uint32_t                bitmap_num_bytes;
    uint8_t                *bitmap;
    int                     bitmap_dirty;
    uint64_t                next_unique_id;
    int                     have_unalloc_bitmap;
} pfs_udf_sbi_t;

typedef struct _pfs_udf_file_meta_t {
    pfs_udf_inode_t         inode;
} pfs_udf_file_meta_t;

typedef struct _pfs_udf_dir_meta_t {
    pfs_udf_inode_t         dir;
    uint64_t                pos;                        ///< [单位:字节] directory stream offset
} pfs_udf_dir_meta_t;

extern const pfs_fs_ops_t g_pfs_udf_fs_ops;

int pfs_udf_init(pfs_udf_sbi_t *p_sbi);
void pfs_udf_deinit(pfs_udf_sbi_t *p_sbi);
int pfs_udf_probe(pfs_blkio_t *p_blkio, int64_t part_offset);
int pfs_udf_mount(pfs_udf_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset,
        pfs_udf_mount_state_e mode);
int pfs_udf_remount(pfs_udf_sbi_t *p_sbi, pfs_udf_mount_state_e mode);
int pfs_udf_umount(pfs_udf_sbi_t *p_sbi);
int pfs_udf_sbi_is_mounted(const pfs_udf_sbi_t *p_sbi);
int pfs_udf_sbi_can_write(const pfs_udf_sbi_t *p_sbi);
int pfs_udf_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

#define PFS_UDF_MKVOL_DEFAULT           0

/// 从宿主目录打包 UDF 2.x 映像 (供 pfs_mkvol); volume_id 可 NULL; vol_bytes 0 表示用设备剩余容量
int pfs_udf_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir,
        const char *volume_id);

#ifdef __cplusplus
}
#endif

#endif // _PFS_UDF_H_

// end
