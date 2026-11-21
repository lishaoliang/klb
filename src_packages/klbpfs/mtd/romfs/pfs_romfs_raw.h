// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_romfs_raw.h
/// @brief   romfs on-disk 布局
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    对照 include/uapi/linux/romfs_fs.h; 全 big-endian; 16 字节对齐
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_ROMFS_RAW_H_
#define _PFS_ROMFS_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_ROMFS_MAGIC                 0x7275u         ///< statfs 类型; 非盘上 ASCII
#define PFS_ROMFS_MAGIC_WORD            "-rom1fs-"
#define PFS_ROMFS_MAXFN                 128
#define PFS_ROMFS_FH_SIZE               16
#define PFS_ROMFS_FH_PAD                15
#define PFS_ROMFS_FH_MASK               (~15u)
#define PFS_ROMFS_CHECKSUM_BYTES        512

#define PFS_ROMFH_TYPE                  7
#define PFS_ROMFH_HRD                   0
#define PFS_ROMFH_DIR                   1
#define PFS_ROMFH_REG                   2
#define PFS_ROMFH_SYM                   3
#define PFS_ROMFH_BLK                   4
#define PFS_ROMFH_CHR                   5
#define PFS_ROMFH_SCK                   6
#define PFS_ROMFH_FIF                   7
#define PFS_ROMFH_EXEC                  8

/// @brief 解码后的 filehdr (pos 为镜像内偏移)
typedef struct _pfs_romfs_fh_t {
    uint32_t                pos;
    uint32_t                next; ///< next & ~15
    uint8_t                 type; ///< next & 7
    uint32_t                spec;
    uint32_t                size;
    uint32_t                meta_sz; ///< align16(16 + namelen + 1)
    uint32_t                data_off; ///< pos + meta_sz
} pfs_romfs_fh_t;

static inline uint32_t pfs_romfs_align16(uint32_t x)
{
    return (x + PFS_ROMFS_FH_PAD) & PFS_ROMFS_FH_MASK;
}

static inline uint32_t pfs_romfs_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

#ifdef __cplusplus
}
#endif

#endif // _PFS_ROMFS_RAW_H_

// end
