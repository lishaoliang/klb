// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_cramfs_raw.h
/// @brief   cramfs on-disk 布局
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    对照 include/uapi/linux/cramfs_fs.h; 与镜像同端序, 本库按 LE 解析
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_CRAMFS_RAW_H_
#define _PFS_CRAMFS_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_CRAMFS_MAGIC                0x28cd3d45u
#define PFS_CRAMFS_MAGIC_WEND           0x453dcd28u
#define PFS_CRAMFS_SIGNATURE            "Compressed ROMFS"
#define PFS_CRAMFS_PAGE_SIZE            4096
#define PFS_CRAMFS_MAXPATHLEN           252

#define PFS_CRAMFS_FLAG_FSID_VERSION_2  0x00000001u
#define PFS_CRAMFS_FLAG_SORTED_DIRS     0x00000002u
#define PFS_CRAMFS_FLAG_HOLES           0x00000100u
#define PFS_CRAMFS_FLAG_WRONG_SIGNATURE 0x00000200u
#define PFS_CRAMFS_FLAG_SHIFTED_ROOT_OFFSET 0x00000400u
#define PFS_CRAMFS_FLAG_EXT_BLOCK_POINTERS 0x00000800u

#define PFS_CRAMFS_SUPPORTED_FLAGS      (0x000000ffu \
                                        | PFS_CRAMFS_FLAG_HOLES \
                                        | PFS_CRAMFS_FLAG_WRONG_SIGNATURE \
                                        | PFS_CRAMFS_FLAG_SHIFTED_ROOT_OFFSET \
                                        | PFS_CRAMFS_FLAG_EXT_BLOCK_POINTERS)

#define PFS_CRAMFS_BLK_FLAG_UNCOMPRESSED (1u << 31)
#define PFS_CRAMFS_BLK_FLAG_DIRECT_PTR   (1u << 30)
#define PFS_CRAMFS_BLK_FLAGS            (PFS_CRAMFS_BLK_FLAG_UNCOMPRESSED | PFS_CRAMFS_BLK_FLAG_DIRECT_PTR)
#define PFS_CRAMFS_BLK_DIRECT_PTR_SHIFT 2

#define PFS_CRAMFS_S_IFMT               0170000
#define PFS_CRAMFS_S_IFDIR              0040000
#define PFS_CRAMFS_S_IFREG              0100000
#define PFS_CRAMFS_S_IFLNK              0120000

/// @brief 解码后的 inode (盘上为 3 个 u32 位域打包; 勿直接用 C bitfield 读镜像)
typedef struct _pfs_cramfs_inode_t {
    uint16_t                mode;
    uint16_t                uid;
    uint32_t                size; ///< 24-bit
    uint8_t                 gid;
    uint8_t                 namelen; ///< 名长/4 (向上对齐)
    uint32_t                offset; ///< 字节偏移/4
} pfs_cramfs_inode_t;

#pragma pack(push, 1)

typedef struct _pfs_cramfs_info_t {
    uint32_t                crc;
    uint32_t                edition;
    uint32_t                blocks;
    uint32_t                files;
} pfs_cramfs_info_t;

typedef struct _pfs_cramfs_super_t {
    uint32_t                magic;
    uint32_t                size;
    uint32_t                flags;
    uint32_t                future;
    uint8_t                 signature[16];
    pfs_cramfs_info_t       fsid;
    uint8_t                 name[16];
    uint32_t                root_w0; ///< root inode 3 words
    uint32_t                root_w1;
    uint32_t                root_w2;
} pfs_cramfs_super_t;

#pragma pack(pop)

static inline void pfs_cramfs_inode_decode(uint32_t w0, uint32_t w1, uint32_t w2, pfs_cramfs_inode_t *p_out)
{
    p_out->mode = (uint16_t)(w0 & 0xffffu);
    p_out->uid = (uint16_t)((w0 >> 16) & 0xffffu);
    p_out->size = w1 & 0x00ffffffu;
    p_out->gid = (uint8_t)((w1 >> 24) & 0xffu);
    p_out->namelen = (uint8_t)(w2 & 0x3fu);
    p_out->offset = (w2 >> 6) & 0x03ffffffu;
}

static inline uint32_t pfs_cramfs_inode_byte_off(const pfs_cramfs_inode_t *p_ino)
{
    return p_ino->offset << 2;
}

static inline unsigned int pfs_cramfs_name_padded(const pfs_cramfs_inode_t *p_ino)
{
    return (unsigned int)p_ino->namelen << 2;
}

#ifdef __cplusplus
}
#endif

#endif // _PFS_CRAMFS_RAW_H_

// end
