// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_jffs2_raw.h
/// @brief   jffs2 on-disk 常量/布局
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    对照 include/uapi/linux/jffs2.h; 假定 LE
///  \n Linux dirent type (dirent.type); 0 时按 REG
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_JFFS2_RAW_H_
#define _PFS_JFFS2_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_JFFS2_SUPER_MAGIC           0x72b6u         ///< magic.h JFFS2_SUPER_MAGIC (statfs; 非 flash sb)

#define PFS_JFFS2_OLD_MAGIC_BITMASK     0x1984u
#define PFS_JFFS2_MAGIC_BITMASK         0x1985u
#define PFS_JFFS2_CIGAM_BITMASK         0x8519u         ///< wrong-endian
#define PFS_JFFS2_EMPTY_BITMASK         0xffffu
#define PFS_JFFS2_DIRTY_BITMASK         0x0000u

#define PFS_JFFS2_MAX_NAME_LEN          254
#define PFS_JFFS2_ROOT_INO              1u

#define PFS_JFFS2_COMPR_NONE            0x00u
#define PFS_JFFS2_COMPR_ZERO            0x01u
#define PFS_JFFS2_COMPR_ZLIB            0x06u

#define PFS_JFFS2_COMPAT_MASK           0xc000u
#define PFS_JFFS2_NODE_ACCURATE         0x2000u
#define PFS_JFFS2_FEATURE_INCOMPAT      0xc000u
#define PFS_JFFS2_FEATURE_ROCOMPAT      0x8000u
#define PFS_JFFS2_FEATURE_RWCOMPAT_COPY 0x4000u
#define PFS_JFFS2_FEATURE_RWCOMPAT_DELETE 0x0000u

#define PFS_JFFS2_NODETYPE_DIRENT       (PFS_JFFS2_FEATURE_INCOMPAT | PFS_JFFS2_NODE_ACCURATE | 1)
#define PFS_JFFS2_NODETYPE_INODE        (PFS_JFFS2_FEATURE_INCOMPAT | PFS_JFFS2_NODE_ACCURATE | 2)
#define PFS_JFFS2_NODETYPE_CLEANMARKER  (PFS_JFFS2_FEATURE_RWCOMPAT_DELETE | PFS_JFFS2_NODE_ACCURATE | 3)
#define PFS_JFFS2_NODETYPE_PADDING      (PFS_JFFS2_FEATURE_RWCOMPAT_DELETE | PFS_JFFS2_NODE_ACCURATE | 4)
#define PFS_JFFS2_NODETYPE_SUMMARY      (PFS_JFFS2_FEATURE_RWCOMPAT_DELETE | PFS_JFFS2_NODE_ACCURATE | 6)
#define PFS_JFFS2_NODETYPE_XATTR        (PFS_JFFS2_FEATURE_INCOMPAT | PFS_JFFS2_NODE_ACCURATE | 8)
#define PFS_JFFS2_NODETYPE_XREF         (PFS_JFFS2_FEATURE_INCOMPAT | PFS_JFFS2_NODE_ACCURATE | 9)

#define PFS_JFFS2_PAD(x)                (((uint32_t)(x) + 3u) & ~3u)

#define PFS_JFFS2_DT_UNKNOWN            0
#define PFS_JFFS2_DT_DIR                4
#define PFS_JFFS2_DT_REG                8
#define PFS_JFFS2_DT_LNK                10

#define PFS_JFFS2_S_IFMT                0170000
#define PFS_JFFS2_S_IFDIR               0040000
#define PFS_JFFS2_S_IFREG               0100000
#define PFS_JFFS2_S_IFLNK               0120000

#pragma pack(push, 1)

typedef struct _pfs_jffs2_unknown_node_t {
    uint16_t                magic;
    uint16_t                nodetype;
    uint32_t                totlen;
    uint32_t                hdr_crc;
} pfs_jffs2_unknown_node_t;

typedef struct _pfs_jffs2_raw_dirent_t {
    uint16_t                magic;
    uint16_t                nodetype;
    uint32_t                totlen;
    uint32_t                hdr_crc;
    uint32_t                pino;
    uint32_t                version;
    uint32_t                ino;
    uint32_t                mctime;
    uint8_t                 nsize;
    uint8_t                 type;
    uint8_t                 unused[2];
    uint32_t                node_crc;
    uint32_t                name_crc;
    ///< name[] (可变长, 不在此结构体内)
} pfs_jffs2_raw_dirent_t;

typedef struct _pfs_jffs2_raw_inode_t {
    uint16_t                magic;
    uint16_t                nodetype;
    uint32_t                totlen;
    uint32_t                hdr_crc;
    uint32_t                ino;
    uint32_t                version;
    uint32_t                mode;
    uint16_t                uid;
    uint16_t                gid;
    uint32_t                isize;
    uint32_t                atime;
    uint32_t                mtime;
    uint32_t                ctime;
    uint32_t                offset;
    uint32_t                csize;
    uint32_t                dsize;
    uint8_t                 compr;
    uint8_t                 usercompr;
    uint16_t                flags;
    uint32_t                data_crc;
    uint32_t                node_crc;
    ///< data[] (可变长, 不在此结构体内)
} pfs_jffs2_raw_inode_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_JFFS2_RAW_H_

// end
