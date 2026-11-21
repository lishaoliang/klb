// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ubifs_raw.h
/// @brief   ubifs on-disk 布局
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    keyed 节点布局用偏移宏 (避免大结构体 bitfield; 与 ubifs-media.h 一致)
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_UBIFS_RAW_H_
#define _PFS_UBIFS_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_UBIFS_SUPER_MAGIC           0x24051905u
#define PFS_UBIFS_NODE_MAGIC            0x06101831u
#define PFS_UBIFS_FORMAT_VERSION        5
#define PFS_UBIFS_RO_COMPAT_VERSION     0
#define PFS_UBIFS_MIN_LEB_SZ            (15 * 1024)
#define PFS_UBIFS_CRC32_INIT            0xFFFFFFFFu
#define PFS_UBIFS_ROOT_INO              1
#define PFS_UBIFS_FIRST_INO             64

#define PFS_UBIFS_INO_NODE              0
#define PFS_UBIFS_DATA_NODE             1
#define PFS_UBIFS_DENT_NODE             2
#define PFS_UBIFS_XENT_NODE             3
#define PFS_UBIFS_TRUN_NODE             4
#define PFS_UBIFS_PAD_NODE              5
#define PFS_UBIFS_SB_NODE               6
#define PFS_UBIFS_MST_NODE              7
#define PFS_UBIFS_REF_NODE              8
#define PFS_UBIFS_IDX_NODE              9
#define PFS_UBIFS_CS_NODE               10
#define PFS_UBIFS_ORPH_NODE             11

#define PFS_UBIFS_COMPR_NONE            0
#define PFS_UBIFS_COMPR_LZO             1
#define PFS_UBIFS_COMPR_ZLIB            2
#define PFS_UBIFS_COMPR_ZSTD            3

#define PFS_UBIFS_KEY_HASH_R5           0
#define PFS_UBIFS_SIMPLE_KEY_FMT        0
#define PFS_UBIFS_MAX_KEY_LEN           16
#define PFS_UBIFS_MAX_HASH_LEN          64
#define PFS_UBIFS_MAX_HMAC_LEN          64

#define PFS_UBIFS_SB_LEBS               1
#define PFS_UBIFS_MST_LEBS              2
#define PFS_UBIFS_SB_LNUM               0
#define PFS_UBIFS_MST_LNUM              1
#define PFS_UBIFS_LOG_LNUM              3

#define PFS_UBIFS_MIN_LOG_LEBS          2
#define PFS_UBIFS_MIN_LPT_LEBS          2
#define PFS_UBIFS_MIN_ORPH_LEBS         1
#define PFS_UBIFS_MIN_MAIN_LEBS         9
#define PFS_UBIFS_MIN_LEB_CNT           (PFS_UBIFS_SB_LEBS + PFS_UBIFS_MST_LEBS + \
                                         PFS_UBIFS_MIN_LOG_LEBS + PFS_UBIFS_MIN_LPT_LEBS + \
                                         PFS_UBIFS_MIN_ORPH_LEBS + PFS_UBIFS_MIN_MAIN_LEBS)

#define PFS_UBIFS_MST_DIRTY             1
#define PFS_UBIFS_MST_NO_ORPHS          2
#define PFS_UBIFS_NO_NODE_GROUP         0

#define PFS_UBIFS_CH_SZ                 24
#define PFS_UBIFS_SB_NODE_SZ            4096
#define PFS_UBIFS_MST_NODE_SZ           512
#define PFS_UBIFS_CS_NODE_SZ            32
#define PFS_UBIFS_INO_NODE_SZ           160                 ///< 无附加 data[]
#define PFS_UBIFS_DENT_NODE_SZ          56                  ///< 无 name[]
#define PFS_UBIFS_DATA_NODE_SZ          48                  ///< 无 data[]
#define PFS_UBIFS_BLOCK_SIZE            4096
#define PFS_UBIFS_MAX_NLEN              255

#define PFS_UBIFS_ITYPE_REG             0
#define PFS_UBIFS_ITYPE_DIR             1

#define PFS_UBIFS_S_IFMT                0170000
#define PFS_UBIFS_S_IFDIR               0040000
#define PFS_UBIFS_S_IFREG               0100000

#define PFS_UBIFS_ALIGN8(_x)            (((_x) + 7) & ~7)

#pragma pack(push, 1)

typedef struct _pfs_ubifs_ch_t {
    uint32_t                magic;
    uint32_t                crc;
    uint64_t                sqnum;
    uint32_t                len;
    uint8_t                 node_type;
    uint8_t                 group_type;
    uint8_t                 padding[2];
} pfs_ubifs_ch_t;

typedef struct _pfs_ubifs_sb_node_t {
    pfs_ubifs_ch_t          ch;
    uint8_t                 padding[2];
    uint8_t                 key_hash;
    uint8_t                 key_fmt;
    uint32_t                flags;
    uint32_t                min_io_size;
    uint32_t                leb_size;
    uint32_t                leb_cnt;
    uint32_t                max_leb_cnt;
    uint64_t                max_bud_bytes;
    uint32_t                log_lebs;
    uint32_t                lpt_lebs;
    uint32_t                orph_lebs;
    uint32_t                jhead_cnt;
    uint32_t                fanout;
    uint32_t                lsave_cnt;
    uint32_t                fmt_version;
    uint16_t                default_compr;
    uint8_t                 padding1[2];
    uint32_t                rp_uid;
    uint32_t                rp_gid;
    uint64_t                rp_size;
    uint32_t                time_gran;
    uint8_t                 uuid[16];
    uint32_t                ro_compat_version;
    uint8_t                 hmac[PFS_UBIFS_MAX_HMAC_LEN];
    uint8_t                 hmac_wkm[PFS_UBIFS_MAX_HMAC_LEN];
    uint16_t                hash_algo;
    uint8_t                 hash_mst[PFS_UBIFS_MAX_HASH_LEN];
    uint8_t                 padding2[3774];
} pfs_ubifs_sb_node_t;

typedef struct _pfs_ubifs_mst_node_t {
    pfs_ubifs_ch_t          ch;
    uint64_t                highest_inum;
    uint64_t                cmt_no;
    uint32_t                flags;
    uint32_t                log_lnum;
    uint32_t                root_lnum;
    uint32_t                root_offs;
    uint32_t                root_len;
    uint32_t                gc_lnum;
    uint32_t                ihead_lnum;
    uint32_t                ihead_offs;
    uint64_t                index_size;
    uint64_t                total_free;
    uint64_t                total_dirty;
    uint64_t                total_used;
    uint64_t                total_dead;
    uint64_t                total_dark;
    uint32_t                lpt_lnum;
    uint32_t                lpt_offs;
    uint32_t                nhead_lnum;
    uint32_t                nhead_offs;
    uint32_t                ltab_lnum;
    uint32_t                ltab_offs;
    uint32_t                lsave_lnum;
    uint32_t                lsave_offs;
    uint32_t                lscan_lnum;
    uint32_t                empty_lebs;
    uint32_t                idx_lebs;
    uint32_t                leb_cnt;
    uint8_t                 hash_root_idx[PFS_UBIFS_MAX_HASH_LEN];
    uint8_t                 hash_lpt[PFS_UBIFS_MAX_HASH_LEN];
    uint8_t                 hmac[PFS_UBIFS_MAX_HMAC_LEN];
    uint8_t                 padding[152];
} pfs_ubifs_mst_node_t;

typedef struct _pfs_ubifs_cs_node_t {
    pfs_ubifs_ch_t          ch;
    uint64_t                cmt_no;
} pfs_ubifs_cs_node_t;

#define PFS_UBIFS_OFF_KEY               24
#define PFS_UBIFS_INO_OFF_CREAT_SQ      40
#define PFS_UBIFS_INO_OFF_SIZE          48
#define PFS_UBIFS_INO_OFF_NLINK         96
#define PFS_UBIFS_INO_OFF_MODE          108
#define PFS_UBIFS_INO_OFF_DATA_LEN      116
#define PFS_UBIFS_INO_OFF_COMPR         132
#define PFS_UBIFS_DENT_OFF_INUM         40
#define PFS_UBIFS_DENT_OFF_TYPE         49
#define PFS_UBIFS_DENT_OFF_NLEN         50
#define PFS_UBIFS_DENT_OFF_NAME         56
#define PFS_UBIFS_DATA_OFF_SIZE         40
#define PFS_UBIFS_DATA_OFF_COMPR        44
#define PFS_UBIFS_DATA_OFF_DATA         48

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_UBIFS_RAW_H_

// end
