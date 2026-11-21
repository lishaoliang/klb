// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_erofs_raw.h
/// @brief   erofs on-disk 布局
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    对照 ../linux-7.1.2/fs/erofs/erofs_fs.h; LE
///  \n 首版拒绝的 incompat: 多设备/metabox; 压缩镜像可读未压缩 inode 仍可挂
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_EROFS_RAW_H_
#define _PFS_EROFS_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_EROFS_MAGIC                 0xE0F5E1E2u     ///< EROFS_SUPER_MAGIC_V1
#define PFS_EROFS_SUPER_OFFSET          1024            ///< [单位:字节]

#define PFS_EROFS_ISLOTBITS             5               ///< inode slot = 32B
#define PFS_EROFS_NAME_LEN              255
#define PFS_EROFS_NULL_ADDR             ((uint64_t)-1)

#define PFS_EROFS_INODE_FLAT_PLAIN      0
#define PFS_EROFS_INODE_COMPRESSED_FULL 1
#define PFS_EROFS_INODE_FLAT_INLINE     2
#define PFS_EROFS_INODE_COMPRESSED_COMPACT 3
#define PFS_EROFS_INODE_CHUNK_BASED     4

#define PFS_EROFS_INODE_LAYOUT_COMPACT  0
#define PFS_EROFS_INODE_LAYOUT_EXTENDED 1

#define PFS_EROFS_I_VERSION_BIT         0
#define PFS_EROFS_I_DATALAYOUT_BIT      1
#define PFS_EROFS_I_NLINK_1_BIT         4
#define PFS_EROFS_I_DOT_OMITTED_BIT     4

#define PFS_EROFS_FT_UNKNOWN            0
#define PFS_EROFS_FT_REG_FILE           1
#define PFS_EROFS_FT_DIR                2
#define PFS_EROFS_FT_SYMLINK            7

#define PFS_EROFS_S_IFMT                0170000
#define PFS_EROFS_S_IFDIR               0040000
#define PFS_EROFS_S_IFREG               0100000
#define PFS_EROFS_S_IFLNK               0120000

#define PFS_EROFS_INCOMPAT_DEVICE_TABLE 0x00000008u
#define PFS_EROFS_INCOMPAT_METABOX      0x00000100u
#define PFS_EROFS_INCOMPAT_48BIT        0x00000080u

#pragma pack(push, 1)

typedef struct _pfs_erofs_super_block_t {
    uint32_t                magic;
    uint32_t                checksum;
    uint32_t                feature_compat;
    uint8_t                 blkszbits;
    uint8_t                 sb_extslots;
    uint16_t                rootnid_2b; ///< 无 48BIT 时为 root nid
    uint64_t                inos;
    uint64_t                epoch;
    uint32_t                fixed_nsec;
    uint32_t                blocks_lo;
    uint32_t                meta_blkaddr;
    uint32_t                xattr_blkaddr;
    uint8_t                 uuid[16];
    uint8_t                 volume_name[16];
    uint32_t                feature_incompat;
    uint16_t                available_compr_algs;
    uint16_t                extra_devices;
    uint16_t                devt_slotoff;
    uint8_t                 dirblkbits;
    uint8_t                 xattr_prefix_count;
    uint32_t                xattr_prefix_start;
    uint64_t                packed_nid;
    uint8_t                 xattr_filter_reserved;
    uint8_t                 ishare_xattr_prefix_id;
    uint8_t                 reserved[2];
    uint32_t                build_time;
    uint64_t                rootnid_8b;
    uint64_t                reserved2;
    uint64_t                metabox_nid;
    uint64_t                reserved3;
} pfs_erofs_super_block_t;

typedef struct _pfs_erofs_inode_compact_t {
    uint16_t                i_format;
    uint16_t                i_xattr_icount;
    uint16_t                i_mode;
    uint16_t                i_nb; ///< nlink 或 startblk_hi
    uint32_t                i_size;
    uint32_t                i_mtime;
    uint32_t                i_u; ///< startblk_lo / rdev / ...
    uint32_t                i_ino;
    uint16_t                i_uid;
    uint16_t                i_gid;
    uint32_t                i_reserved;
} pfs_erofs_inode_compact_t;

typedef struct _pfs_erofs_inode_extended_t {
    uint16_t                i_format;
    uint16_t                i_xattr_icount;
    uint16_t                i_mode;
    uint16_t                i_nb;
    uint64_t                i_size;
    uint32_t                i_u;
    uint32_t                i_ino;
    uint32_t                i_uid;
    uint32_t                i_gid;
    uint64_t                i_mtime;
    uint32_t                i_mtime_nsec;
    uint32_t                i_nlink;
    uint8_t                 i_reserved2[16];
} pfs_erofs_inode_extended_t;

typedef struct _pfs_erofs_dirent_t {
    uint64_t                nid;
    uint16_t                nameoff;
    uint8_t                 file_type;
    uint8_t                 reserved;
} pfs_erofs_dirent_t;

typedef struct _pfs_erofs_xattr_ibody_header_t {
    uint32_t                h_name_filter;
    uint8_t                 h_shared_count;
    uint8_t                 h_reserved2[7];
} pfs_erofs_xattr_ibody_header_t;

#pragma pack(pop)

static inline unsigned int pfs_erofs_xattr_ibody_size(uint16_t icount)
{
    if (0 == icount)
    {
        return 0;
    }
    return (unsigned int)sizeof(pfs_erofs_xattr_ibody_header_t) + 4u * ((unsigned int)icount - 1u);
}

#ifdef __cplusplus
}
#endif

#endif // _PFS_EROFS_RAW_H_

// end
