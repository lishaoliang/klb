// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_mtd_raw.h
/// @brief   UBI / PFUB on-flash 常量
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    对照 ubi-media.h; PFUB 为 pfsmtd 测试用 LEB 镜像
///  \n pfsmtd 假 UBI 卷文件 (宿主机文件; 非内核 UBI 镜像)
///  \n 布局: 64B 头 + leb_count * leb_size 数据区 (按 LEB 线性拼接)
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_MTD_RAW_H_
#define _PFS_MTD_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_UBI_EC_HDR_MAGIC            0x55424923u     ///< "UBI#"
#define PFS_UBI_VID_HDR_MAGIC           0x55424921u     ///< "UBI!"

#define PFS_MTD_PFUB_MAGIC              0x42554650u     ///< "PFUB" LE
#define PFS_MTD_PFUB_VERSION            1
#define PFS_MTD_PFUB_HDR_SIZE           64

#define PFS_MTD_RAW_ERASE_BYTE          0xffu           ///< NOR 擦除态 (jffs2 等)
#define PFS_MTD_RAW_FILL_CHUNK          (64u * 1024u)   ///< raw_create 写块

#pragma pack(push, 1)

typedef struct _pfs_mtd_pfub_hdr_t {
    uint32_t                magic;
    uint32_t                version;
    uint32_t                leb_size;
    uint32_t                leb_count;
    uint8_t                 reserved[48];
} pfs_mtd_pfub_hdr_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_MTD_RAW_H_

// end
