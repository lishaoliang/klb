// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_yaffs_raw.h
/// @brief   yaffs2 on-disk 常量/布局
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    出处: aleph1 YAFFS; 首版假定 LE; 镜像为 chunk||spare 拼接
///  \n mkfs 默认几何 (与 detect 表首选一致)
///  \n spare 头部 packed tags (无 ECC 段; ECC 可跟在其后)
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_YAFFS_RAW_H_
#define _PFS_YAFFS_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_YAFFS_MAX_NAME_LENGTH       255
#define PFS_YAFFS_MAX_ALIAS_LENGTH      159

#define PFS_YAFFS_OBJECTID_ROOT         1u
#define PFS_YAFFS_OBJECTID_LOSTNFOUND   2u
#define PFS_YAFFS_OBJECTID_UNLINKED     3u
#define PFS_YAFFS_OBJECTID_DELETED      4u

#define PFS_YAFFS_LOWEST_SEQUENCE_NUMBER  0x00001000u
#define PFS_YAFFS_HIGHEST_SEQUENCE_NUMBER 0xEFFFFF00u

#define PFS_YAFFS_OBJECT_TYPE_UNKNOWN   0
#define PFS_YAFFS_OBJECT_TYPE_FILE      1
#define PFS_YAFFS_OBJECT_TYPE_SYMLINK   2
#define PFS_YAFFS_OBJECT_TYPE_DIRECTORY 3
#define PFS_YAFFS_OBJECT_TYPE_HARDLINK  4
#define PFS_YAFFS_OBJECT_TYPE_SPECIAL   5

#define PFS_YAFFS_S_IFMT                0170000
#define PFS_YAFFS_S_IFDIR               0040000
#define PFS_YAFFS_S_IFREG               0100000

#define PFS_YAFFS_MKFS_CHUNK_SIZE       2048
#define PFS_YAFFS_MKFS_SPARE_SIZE       64

#pragma pack(push, 1)

typedef struct _pfs_yaffs_packed_tags2_t {
    uint32_t                seq_number;
    uint32_t                obj_id;
    uint32_t                chunk_id; ///< 0=object header; >=1 数据块
    uint32_t                n_bytes;
} pfs_yaffs_packed_tags2_t;

/// @brief object header (chunk_id==0 时的 data 区前缀; ASCII YCHAR)
typedef struct _pfs_yaffs_obj_hdr_t {
    uint32_t                type; ///< PFS_YAFFS_OBJECT_TYPE_*
    uint32_t                parent_obj_id;
    uint16_t                sum_no_longer_used;
    char                    name[PFS_YAFFS_MAX_NAME_LENGTH + 1];
    uint32_t                yst_mode;
    uint32_t                yst_uid;
    uint32_t                yst_gid;
    uint32_t                yst_atime;
    uint32_t                yst_mtime;
    uint32_t                yst_ctime;
    uint32_t                file_size_low;
    int32_t                 equiv_id;
    char                    alias[PFS_YAFFS_MAX_ALIAS_LENGTH + 1];
    uint32_t                yst_rdev;
    uint32_t                win_ctime[2];
    uint32_t                win_atime[2];
    uint32_t                win_mtime[2];
    uint32_t                inband_shadowed_obj_id;
    uint32_t                inband_is_shrink;
    uint32_t                file_size_high;
    uint32_t                reserved[1];
    int32_t                 shadows_obj;
    uint32_t                is_shrink;
} pfs_yaffs_obj_hdr_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_YAFFS_RAW_H_

// end
