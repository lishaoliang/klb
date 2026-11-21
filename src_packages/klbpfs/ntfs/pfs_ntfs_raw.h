// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ntfs_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * All NTFS associated on-disk structures.
 *
 * Copyright (c) 2001-2005 Anton Altaparmakov
 * Copyright (c) 2002 Richard Russon
 *
 * copy from linux-7.1.x kernel fs/ntfs/layout.h.
 * Types: kernel __leN / uN / sN mapped to uintN_t / intN_t; on-disk little endian.
 * Endian helpers: pfs_cpu_to_leN / pfs_leN_to_cpu (pfs_endian.h).
 */

#ifndef _PFS_NTFS_RAW_H_
#define _PFS_NTFS_RAW_H_

#include <stdint.h>
#include <stdbool.h>

#include "pfs_endian.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#pragma pack(push, 1)

/* The NTFS oem_id "NTFS    " */
#define NTFS_OEM_ID                     pfs_cpu_to_le64(0x202020205346544eULL) ///< OEM "NTFS    "

/*
 * Location of bootsector on partition:
 *    The standard ntfs_boot_sector_t is on sector 0 of the partition.
 *    On NT4 and above there is one backup copy of the boot sector to
 *    be found on the last sector of the partition (not normally accessible
 *    from within Windows as the bootsector contained number of sectors
 *    value is one less than the actual value!).
 *    On versions of NT 3.51 and earlier, the backup copy was located at
 *    number of sectors/2 (integer divide), i.e. in the middle of the volume.
 */

/*
 * BIOS parameter block (bpb) structure.
 *
 * @bytes_per_sector:       Size of a sector in bytes (usually 512).
 *                          Matches the logical sector size of the underlying device.
 * @sectors_per_cluster:    Size of a cluster in sectors (NTFS cluster size / sector size).
 * @reserved_sectors:       Number of reserved sectors at the beginning of the volume.
 *                          Always set to 0 in NTFS.
 * @fats:                   Number of FAT tables.
 *                          Always 0 in NTFS (no FAT tables exist).
 * @root_entries:           Number of entries in the root directory.
 *                          Always 0 in NTFS.
 * @sectors:                Total number of sectors on the volume.
 *                          Always 0 in NTFS (use @large_sectors instead).
 * @media_type:             Media descriptor byte.
 *                          0xF8 for hard disk (fixed media) in NTFS.
 * @sectors_per_fat:        Number of sectors per FAT table.
 *                          Always 0 in NTFS.
 * @sectors_per_track:      Number of sectors per track.
 *                          Irrelevant for NTFS.
 * @heads:                  Number of heads (CHS geometry).
 *                          Irrelevant for NTFS.
 * @hidden_sectors:         Number of hidden sectors before the start of the partition.
 *                          Always 0 in NTFS boot sector.
 * @large_sectors:          Total number of sectors on the volume.
 */
typedef struct _ntfs_bios_parameter_block_t {
    uint16_t bytes_per_sector;                          ///< 逻辑扇区字节数 (常见 512)
    uint8_t  sectors_per_cluster;                       ///< 每簇扇区数
    uint16_t reserved_sectors;                          ///< 保留扇区数 (NTFS 常为 0)
    uint8_t  fats;                                      ///< FAT 表份数 (NTFS 为 0)
    uint16_t root_entries;                              ///< 根目录项数 (NTFS 为 0)
    uint16_t sectors;                                   ///< 总扇区 (小字段; NTFS 为 0)
    uint8_t  media_type;                                ///< 介质描述符 (硬盘常 0xF8)
    uint16_t sectors_per_fat;                           ///< 每 FAT 扇区数 (NTFS 为 0)
    uint16_t sectors_per_track;                         ///< 每磁道扇区数 (CHS, 无关)
    uint16_t heads;                                     ///< 磁头数 (CHS, 无关)
    uint32_t hidden_sectors;                            ///< 隐藏扇区数 (NTFS Boot 常为 0)
    uint32_t large_sectors;                             ///< 总扇区 (兼容字段; NTFS 常为 0)
} ntfs_bios_parameter_block_t;

/*
 * NTFS boot sector structure.
 *
 * @jump:               3-byte jump instruction to boot code (irrelevant for NTFS).
 *                      Typically 0xEB 0x52 0x90 or similar.
 * @oem_id:             OEM identifier string (8 bytes).
 *                      Always "NTFS    " (with trailing spaces) in NTFS volumes.
 * @bpb:                Legacy BIOS Parameter Block (see ntfs_bios_parameter_block_t).
 *                      Mostly zeroed or set to fixed values for NTFS compatibility.
 * @unused:             4 bytes, reserved/unused.
 *                      NTFS disk editors show it as:
 *                        - physical_drive (0x80 for fixed disk)
 *                        - current_head (0)
 *                        - extended_boot_signature (0x80 or 0x28)
 *                        - unused (0)
 *                      Always zero in practice for NTFS.
 * @number_of_sectors:  Number of sectors in volume. Gives maximum volume
 *                      size of 2^63 sectors. Assuming standard sector
 *                      size of 512 bytes, the maximum byte size is
 *                      approx. 4.7x10^21 bytes. (-;
 * @mft_lcn:            Logical cluster number (LCN) of the $MFT data attribute.
 *                      Location of the Master File Table.
 * @mftmirr_lcn:        LCN of the $MFTMirr (first 3-4 MFT records copy).
 *                      Mirror for boot-time recovery.
 * @clusters_per_mft_record:
 *                      Size of each MFT record in clusters.
 * @reserved0:          3 bytes, reserved/zero.
 * @clusters_per_index_record:
 *                      Size of each index block/record in clusters.
 * @reserved1:          3 bytes, reserved/zero.
 * @volume_serial_number:
 *                      64-bit volume serial number.
 *                      Used for identification (irrelevant for NTFS operation).
 * @checksum:           32-bit checksum of the boot sector (excluding this field).
 *                      Used to detect boot sector corruption.
 * @bootstrap:          426 bytes of bootstrap code.
 *                      Irrelevant for NTFS (contains x86 boot loader stub).
 * @end_of_sector_marker:
 *                      2-byte end-of-sector signature.
 *                      Always 0xAA55 (little-endian magic number).
 */
typedef struct _ntfs_boot_sector_t {
    uint8_t                     jump[3];                    ///< 引导跳转指令
    uint64_t                    oem_id;                     ///< OEM 标识, 须为 "NTFS    "
    ntfs_bios_parameter_block_t bpb;                        ///< 兼容 BPB (多数 FAT 字段为 0)
    uint8_t                     unused[4];                  ///< 保留/未用
    uint64_t                    number_of_sectors;          ///< 卷扇区总数
    uint64_t                    mft_lcn;                    ///< $MFT 起始 LCN
    uint64_t                    mftmirr_lcn;                ///< $MFTMirr 起始 LCN
    int8_t                      clusters_per_mft_record;    ///< MFT 记录大小编码 (>0=簇数; <0=2^|n| 字节)
    uint8_t                     reserved0[3];               ///< 保留
    int8_t                      clusters_per_index_record;  ///< 索引块大小编码 (同上)
    uint8_t                     reserved1[3];               ///< 保留
    uint64_t                    volume_serial_number;       ///< 卷序列号
    uint32_t                    checksum;                   ///< Boot 校验和 (不含本字段)
    uint8_t                     bootstrap[426];             ///< 引导代码
    uint16_t                    end_of_sector_marker;       ///< 扇区尾标 0xAA55
} ntfs_boot_sector_t;

typedef char pfs_ntfs_sa_1[(sizeof(ntfs_boot_sector_t) == 512) ? 1 : -1];

/*
 * Magic identifiers present at the beginning of all ntfs record containing
 * records (like mft records for example).
 *
 * NTFS_MAGIC_FILE:      MFT entry header ("FILE" in ASCII).
 *                  Used in $MFT/$DATA for all master file table records.
 * NTFS_MAGIC_INDX:      Index buffer header ("INDX" in ASCII).
 *                  Used in $INDEX_ALLOCATION attributes (directories, $I30 indexes).
 * NTFS_MAGIC_HOLE:      Hole marker ("HOLE" in ASCII).
 *                  Introduced in NTFS 3.0+, used for sparse/hole regions in some contexts.
 * NTFS_MAGIC_RSTR:      Restart page header ("RSTR" in ASCII).
 *                  Used in LogFile for restart pages (transaction log recovery).
 * NTFS_MAGIC_RCRD:      Log record page header ("RCRD" in ASCII).
 *                  Used in LogFile for individual log record pages.
 * NTFS_MAGIC_CHKD:      Chkdsk modified marker ("CHKD" in ASCII).
 *                  Set by chkdsk when it modifies a record; indicates repair was done.
 * NTFS_MAGIC_BAAD:      Bad record marker ("BAAD" in ASCII).
 *                  Indicates a multi-sector transfer failure was detected.
 *                  The record is corrupted/unusable; often set during I/O errors.
 * NTFS_MAGIC_EMPTY:     Empty/uninitialized page marker (0xffffffff).
 *                  Used in LogFile when a page is filled with 0xff bytes
 *                  and has not yet been initialized. Must be formatted before use.
 */
enum {
    NTFS_MAGIC_FILE  = pfs_cpu_to_le32(0x454c4946),     ///< MFT 记录 "FILE"
    NTFS_MAGIC_INDX  = pfs_cpu_to_le32(0x58444e49),     ///< 索引块 "INDX"
    NTFS_MAGIC_HOLE  = pfs_cpu_to_le32(0x454c4f48),     ///< 稀疏洞 "HOLE"
    NTFS_MAGIC_RSTR  = pfs_cpu_to_le32(0x52545352),     ///< LogFile 重启页 "RSTR"
    NTFS_MAGIC_RCRD  = pfs_cpu_to_le32(0x44524352),     ///< LogFile 记录页 "RCRD"
    NTFS_MAGIC_CHKD  = pfs_cpu_to_le32(0x444b4843),     ///< chkdsk 已修改 "CHKD"
    NTFS_MAGIC_BAAD  = pfs_cpu_to_le32(0x44414142),     ///< 多扇区传输损坏 "BAAD"
    NTFS_MAGIC_EMPTY = pfs_cpu_to_le32(0xffffffff)      ///< 未初始化页
};

/*
 * Generic magic comparison macros. Finally found a use for the ## preprocessor
 * operator! (-8
 */

static inline bool __ntfs_is_magic(uint32_t x, uint32_t r)
{
    return (x == r);
}
#define ntfs_is_magic(x, m)             __ntfs_is_magic(x, NTFS_MAGIC_##m)  ///< 按魔数比较

static inline bool __ntfs_is_magicp(uint32_t *p, uint32_t r)
{
    return (*p == r);
}
#define ntfs_is_magicp(p, m)            __ntfs_is_magicp(p, NTFS_MAGIC_##m) ///< 按魔数指针比较

/*
 * Specialised magic comparison macros for the NTFS_RECORD_TYPEs defined above.
 */
#define ntfs_is_file_record(x)          (ntfs_is_magic(x, FILE))            ///< 是否 FILE/MFT 记录
#define ntfs_is_file_recordp(p)         (ntfs_is_magicp(p, FILE))           ///< 指针版 FILE
#define ntfs_is_mft_record(x)           (ntfs_is_file_record(x))            ///< 同 FILE
#define ntfs_is_mft_recordp(p)          (ntfs_is_file_recordp(p))           ///< 指针版 MFT
#define ntfs_is_indx_record(x)          (ntfs_is_magic(x, INDX))            ///< 是否 INDX
#define ntfs_is_indx_recordp(p)         (ntfs_is_magicp(p, INDX))           ///< 指针版 INDX
#define ntfs_is_hole_record(x)          (ntfs_is_magic(x, HOLE))            ///< 是否 HOLE
#define ntfs_is_hole_recordp(p)         (ntfs_is_magicp(p, HOLE))           ///< 指针版 HOLE

#define ntfs_is_rstr_record(x)          (ntfs_is_magic(x, RSTR))            ///< 是否 RSTR
#define ntfs_is_rstr_recordp(p)         (ntfs_is_magicp(p, RSTR))           ///< 指针版 RSTR
#define ntfs_is_rcrd_record(x)          (ntfs_is_magic(x, RCRD))            ///< 是否 RCRD
#define ntfs_is_rcrd_recordp(p)         (ntfs_is_magicp(p, RCRD))           ///< 指针版 RCRD

#define ntfs_is_chkd_record(x)          (ntfs_is_magic(x, CHKD))            ///< 是否 CHKD
#define ntfs_is_chkd_recordp(p)         (ntfs_is_magicp(p, CHKD))           ///< 指针版 CHKD

#define ntfs_is_baad_record(x)          (ntfs_is_magic(x, BAAD))            ///< 是否 BAAD
#define ntfs_is_baad_recordp(p)         (ntfs_is_magicp(p, BAAD))           ///< 指针版 BAAD

#define ntfs_is_empty_record(x)         (ntfs_is_magic(x, EMPTY))           ///< 是否 EMPTY
#define ntfs_is_empty_recordp(p)        (ntfs_is_magicp(p, EMPTY))          ///< 指针版 EMPTY

/*
 * ntfs_record_t - Common header for all multi-sector protected NTFS records
 *
 * @magic:      4-byte magic identifier for the record type and/or status.
 *              Common values are defined in the magic_* enum (FILE, INDX, RSTR,
 *              RCRD, CHKD, BAAD, HOLE, EMPTY).
 *              - "FILE" = MFT record
 *              - "INDX" = Index allocation block
 *              - "BAAD" = Record corrupted (multi-sector fixup failed)
 *              - 0xffffffff = Uninitialized/empty page
 * @usa_ofs:    Offset (in bytes) from the start of this record to the Update
 *              Sequence Array (USA).
 *              The USA is located at record + usa_ofs.
 * @usa_count:  Number of 16-bit entries in the USA array (including the Update
 *              Sequence Number itself).
 *              - Number of fixup locations = usa_count - 1
 *              - Each fixup location is a 16-bit value in the record that needs
 *                protection against torn writes.
 *
 * The Update Sequence Array (usa) is an array of the uint16_t values which belong
 * to the end of each sector protected by the update sequence record in which
 * this array is contained. Note that the first entry is the Update Sequence
 * Number (usn), a cyclic counter of how many times the protected record has
 * been written to disk. The values 0 and -1 (ie. 0xffff) are not used. All
 * last le16's of each sector have to be equal to the usn (during reading) or
 * are set to it (during writing). If they are not, an incomplete multi sector
 * transfer has occurred when the data was written.
 * The maximum size for the update sequence array is fixed to:
 *    maximum size = usa_ofs + (usa_count * 2) = 510 bytes
 * The 510 bytes comes from the fact that the last uint16_t in the array has to
 * (obviously) finish before the last uint16_t of the first 512-byte sector.
 * This formula can be used as a consistency check in that usa_ofs +
 * (usa_count * 2) has to be less than or equal to 510.
 */
typedef struct _ntfs_record_t {
    uint32_t magic;                                     ///< 记录魔数 (FILE/INDX/RSTR 等)
    uint16_t usa_ofs;                                   ///< USA 相对本记录起始的字节偏移
    uint16_t usa_count;                                 ///< USA 项数 (含 USN; 保护区=usa_count-1)
} ntfs_record_t;

/*
 * System files mft record numbers. All these files are always marked as used
 * in the bitmap attribute of the mft; presumably in order to avoid accidental
 * allocation for random other mft records. Also, the sequence number for each
 * of the system files is always equal to their mft record number and it is
 * never modified.
 *
 * NTFS_FILE_MFT:        Master File Table (MFT) itself.
 *                  Data attribute contains all MFT entries;
 *                  Bitmap attribute tracks which records are in use (bit==1).
 * NTFS_FILE_MFTMirr:    MFT mirror: copy of the first four (or more) MFT records
 *                  in its data attribute.
 *                  If cluster size > 4 KiB, copies first N records where
 *                  N = cluster_size / mft_record_size.
 * NTFS_FILE_LogFile:    Journaling log (LogFile) in data attribute.
 *                  Used for transaction logging and recovery.
 * NTFS_FILE_Volume:     Volume information and name.
 *                  Contains $VolumeName (label) and $VolumeInformation
 *                  (flags, NTFS version). Windows calls this the volume DASD.
 * NTFS_FILE_AttrDef:    Attribute definitions array in data attribute.
 *                  Defines all possible attribute types and their properties.
 * NTFS_FILE_root:       Root directory ($Root).
 *                  The top-level directory of the filesystem.
 * NTFS_FILE_Bitmap:     Cluster allocation bitmap ($Bitmap) in data attribute.
 *                  Tracks free/used clusters (LCNs) on the volume.
 * NTFS_FILE_Boot:       Boot sector ($Boot) in data attribute.
 *                  Always located at cluster 0; contains BPB and NTFS parameters.
 * NTFS_FILE_BadClus:    Bad cluster list ($BadClus) in non-resident data attribute.
 *                  Marks all known bad clusters.
 * NTFS_FILE_Secure:     Security descriptors ($Secure).
 *                  Contains shared $SDS (security descriptors) and two indexes
 *                  ($SDH, $SII). Introduced in Windows 2000.
 *                  Before that, it was called $Quota but was unused.
 * NTFS_FILE_UpCase:     Uppercase table ($UpCase) in data attribute.
 *                  Maps all 65536 Unicode characters to their uppercase forms.
 * NTFS_FILE_Extend:     System directory ($Extend).
 *                  Contains additional system files ($ObjId, $Quota, $Reparse,
 *                  $UsnJrnl, etc.). Introduced in NTFS 3.0 (Windows 2000).
 * NTFS_FILE_reserved12: Reserved for future use (MFT records 12-15).
 * NTFS_FILE_reserved13: Reserved.
 * NTFS_FILE_reserved14: Reserved.
 * NTFS_FILE_reserved15: Reserved.
 * NTFS_FILE_first_user: First possible user-created file MFT record number.
 *                  Used as a boundary to distinguish system files from user files.
 */
enum {
    NTFS_FILE_MFT        = 0,                           ///< $MFT
    NTFS_FILE_MFTMirr    = 1,                           ///< $MFTMirr
    NTFS_FILE_LogFile    = 2,                           ///< $LogFile
    NTFS_FILE_Volume     = 3,                           ///< $Volume
    NTFS_FILE_AttrDef    = 4,                           ///< $AttrDef
    NTFS_FILE_root       = 5,                           ///< 根目录
    NTFS_FILE_Bitmap     = 6,                           ///< $Bitmap
    NTFS_FILE_Boot       = 7,                           ///< $Boot
    NTFS_FILE_BadClus    = 8,                           ///< $BadClus
    NTFS_FILE_Secure     = 9,                           ///< $Secure
    NTFS_FILE_UpCase     = 10,                          ///< $UpCase
    NTFS_FILE_Extend     = 11,                          ///< $Extend
    NTFS_FILE_reserved12 = 12,                          ///< 保留
    NTFS_FILE_reserved13 = 13,                          ///< 保留
    NTFS_FILE_reserved14 = 14,                          ///< 保留
    NTFS_FILE_reserved15 = 15,                          ///< 保留
    NTFS_FILE_first_user = 16,                          ///< 首个用户文件记录号
};

/*
 * enum - Flags for MFT record header
 *
 * These are the so far known NTFS_MFT_RECORD_* flags (16-bit) which contain
 * information about the mft record in which they are present.
 *
 * NTFS_MFT_RECORD_IN_USE:        This MFT record is allocated and in use.
 *                           (bit set = record is valid/used; clear = free)
 * NTFS_MFT_RECORD_IS_DIRECTORY:  This MFT record represents a directory.
 *                           (Used to quickly distinguish files from directories)
 * NTFS_MFT_RECORD_IS_4:          Indicates the record is a special "record 4" type.
 *                           (Rarely used; related to NTFS internal special cases,
 *                           often for $AttrDef or early system files)
 * NTFS_MFT_RECORD_IS_VIEW_INDEX: This MFT record is used as a view index.
 *                           (Specific to NTFS indexed views or object ID indexes)
 * NTFS_MFT_REC_SPACE_FILLER:     Dummy value to force the enum to be 16-bit wide.
 *                           (Not a real flag; just a sentinel to ensure the type
 *                           is uint16_t and no higher bits are accidentally used)
 */
enum {
    NTFS_MFT_RECORD_IN_USE        = pfs_cpu_to_le16(0x0001), ///< 记录在用
    NTFS_MFT_RECORD_IS_DIRECTORY  = pfs_cpu_to_le16(0x0002), ///< 目录
    NTFS_MFT_RECORD_IS_4          = pfs_cpu_to_le16(0x0004), ///< 特殊 record 4
    NTFS_MFT_RECORD_IS_VIEW_INDEX = pfs_cpu_to_le16(0x0008), ///< 视图索引
    NTFS_MFT_REC_SPACE_FILLER     = pfs_cpu_to_le16(0xffff), ///< 占位使枚举为 16 bit
};

/*
 * mft references (aka file references or file record segment references) are
 * used whenever a structure needs to refer to a record in the mft.
 *
 * A reference consists of a 48-bit index into the mft and a 16-bit sequence
 * number used to detect stale references.
 *
 * For error reporting purposes we treat the 48-bit index as a signed quantity.
 *
 * The sequence number is a circular counter (skipping 0) describing how many
 * times the referenced mft record has been (re)used. This has to match the
 * sequence number of the mft record being referenced, otherwise the reference
 * is considered stale and removed.
 *
 * If the sequence number is zero it is assumed that no sequence number
 * consistency checking should be performed.
 */

/*
 * Define two unpacking macros to get to the reference (NTFS_MREF) and
 * sequence number (NTFS_MSEQNO) respectively.
 * The _LE versions are to be applied on little endian MFT_REFs.
 * Note: The _LE versions will return a CPU endian formatted value!
 */
#define NTFS_MFT_REF_MASK_CPU           0x0000ffffffffffffULL               ///< MFT 号 48 bit 掩码 (CPU)
#define NTFS_MFT_REF_MASK_LE            pfs_cpu_to_le64(NTFS_MFT_REF_MASK_CPU) ///< 同上 (LE)

#define NTFS_MK_MREF(m, s)              ((uint64_t)(((uint64_t)(s) << 48) | \
                                        ((uint64_t)(m) & NTFS_MFT_REF_MASK_CPU))) ///< 拼 MFT 引用
#define NTFS_MK_LE_MREF(m, s)           pfs_cpu_to_le64(NTFS_MK_MREF(m, s)) ///< 拼 LE MFT 引用

#define NTFS_MREF(x)                    ((uint64_t)((x) & NTFS_MFT_REF_MASK_CPU)) ///< 取 MFT 号
#define NTFS_MSEQNO(x)                  ((uint16_t)(((x) >> 48) & 0xffff))  ///< 取序列号
#define NTFS_MREF_LE(x)                 ((uint64_t)(pfs_le64_to_cpu(x) & NTFS_MFT_REF_MASK_CPU)) ///< LE 取号
#define NTFS_MREF_INO(x)                ((uint64_t)NTFS_MREF_LE(x))         ///< LE 取号别名
#define NTFS_MSEQNO_LE(x)               ((uint16_t)((pfs_le64_to_cpu(x) >> 48) & 0xffff)) ///< LE 取序列号

#define NTFS_IS_ERR_MREF(x)             (((x) & 0x0000800000000000ULL) ? true : false) ///< 是否错误引用
#define NTFS_ERR_MREF(x)                ((uint64_t)((int64_t)(x)))          ///< 错误引用转无符号
#define NTFS_MREF_ERR(x)                ((int)((int64_t)(x)))               ///< 错误引用转 int

/*
 * ntfs_mft_record_t - NTFS Master File Table (MFT) record header
 *
 * The mft record header present at the beginning of every record in the mft.
 * This is followed by a sequence of variable length attribute records which
 * is terminated by an attribute of type NTFS_AT_END which is a truncated attribute
 * in that it only consists of the attribute type code NTFS_AT_END and none of the
 * other members of the attribute structure are present.
 *
 * magic:               Record magic ("FILE" for valid MFT entries).
 *                      See ntfs_record magic enum for other values.
 * usa_ofs:             Offset to Update Sequence Array (see ntfs_record).
 * usa_count:           Number of entries in USA (see ntfs_record).
 * lsn:                 Log sequence number (LSN) from LogFile.
 *                      Incremented on every modification to this record.
 * sequence_number:     Reuse count of this MFT record slot.
 *                      Incremented (skipping zero) when the file is deleted.
 *                      Zero means never reused or special case.
 *                      Part of MFT reference (together with record number).
 * link_count:          Number of hard links (directory entries) to this file.
 *                      Only meaningful in base MFT records.
 *                      When deleting a directory entry:
 *                        - If link_count == 1, delete the whole file
 *                        - Else remove only the $FILE_NAME attribute and decrement
 * attrs_offset:        Byte offset from start of MFT record to first attribute.
 *                      Must be 8-byte aligned.
 * flags:               Bit array of NTFS_MFT_RECORD_* flags (see NTFS_MFT_RECORD_IN_USE enum).
 *                      NTFS_MFT_RECORD_IN_USE cleared when record is freed/deleted.
 * bytes_in_use:        Number of bytes actually used in this MFT record.
 *                      Must be 8-byte aligned.
 *                      Includes header + all attributes + padding.
 * bytes_allocated:     Total allocated size of this MFT record.
 *                      Usually equal to MFT record size (1024 bytes or cluster size).
 * base_mft_record:     MFT reference to the base record.
 *                      0 for base records.
 *                      Non-zero for extension records -> points to base record
 *                      containing the $ATTRIBUTE_LIST that describes this extension.
 * next_attr_instance:  Next attribute instance number to assign.
 *                      Incremented after each use.
 *                      Reset to 0 when MFT record is reused.
 *                      First instance is always 0.
 * reserved:            Reserved for alignment (NTFS 3.1+).
 * mft_record_number:   This MFT record's number (index in $MFT).
 *                      Only present in NTFS 3.1+ (Windows XP and above).
 */
typedef struct _ntfs_mft_record_t {
    uint32_t magic;                                     ///< 魔数 "FILE"
    uint16_t usa_ofs;                                   ///< USA 字节偏移
    uint16_t usa_count;                                 ///< USA 项数

    uint64_t lsn;                                       ///< LogFile 日志序号 (LSN)
    uint16_t sequence_number;                           ///< 记录重用序号 (参与 MFT 引用)
    uint16_t link_count;                                ///< 硬链接数 (基记录有效)
    uint16_t attrs_offset;                              ///< 首属性相对本记录的字节偏移 (8 对齐)
    uint16_t flags;                                     ///< NTFS_MFT_RECORD_* 标志
    uint32_t bytes_in_use;                              ///< 已用字节数 (8 对齐)
    uint32_t bytes_allocated;                           ///< 本记录分配大小
    uint64_t base_mft_record;                           ///< 基记录 MFT 引用 (基记录为 0)
    uint16_t next_attr_instance;                        ///< 下一属性实例号
    uint16_t reserved;                                  ///< 对齐保留 (NTFS 3.1+)
    uint32_t mft_record_number;                         ///< 本记录号 (NTFS 3.1+)
} ntfs_mft_record_t;

typedef char pfs_ntfs_sa_2[(sizeof(ntfs_mft_record_t) == 48) ? 1 : -1];

/*
 * ntfs_mft_record_old_t - Old NTFS MFT record header (pre-NTFS 3.1 / Windows XP)
 *
 * This is the older version of the MFT record header used in NTFS versions
 * prior to 3.1 (Windows XP and later). It lacks the additional fields
 * @reserved and @mft_record_number that were added in NTFS 3.1+.
 *
 * @magic:              Record magic ("FILE" for valid MFT entries).
 *                      See ntfs_record magic enum for other values.
 * @usa_ofs:            Offset to Update Sequence Array (see ntfs_record).
 * @usa_count:          Number of entries in USA (see ntfs_record).
 * @lsn:                Log sequence number (LSN) from LogFile.
 *                      Incremented on every modification to this record.
 * @sequence_number:    Reuse count of this MFT record slot.
 *                      Incremented (skipping zero) when the file is deleted.
 *                      Zero means never reused or special case.
 *                      Part of MFT reference (together with record number).
 * @link_count:         Number of hard links (directory entries) to this file.
 *                      Only meaningful in base MFT records.
 *                      When deleting a directory entry:
 *                        - If link_count == 1, delete the whole file
 *                        - Else remove only the $FILE_NAME attribute and decrement
 * @attrs_offset:       Byte offset from start of MFT record to first attribute.
 *                      Must be 8-byte aligned.
 * @flags:              Bit array of NTFS_MFT_RECORD_* flags (see NTFS_MFT_RECORD_IN_USE enum).
 *                      NTFS_MFT_RECORD_IN_USE cleared when record is freed/deleted.
 * @bytes_in_use:       Number of bytes actually used in this MFT record.
 *                      Must be 8-byte aligned.
 *                      Includes header + all attributes + padding.
 * @bytes_allocated:    Total allocated size of this MFT record.
 *                      Usually equal to MFT record size (1024 bytes or cluster size).
 * @base_mft_record:    MFT reference to the base record.
 *                      0 for base records.
 *                      Non-zero for extension records -> points to base record
 *                      containing the $ATTRIBUTE_LIST that describes this extension.
 * @next_attr_instance: Next attribute instance number to assign.
 *                      Incremented after each use.
 *                      Reset to 0 when MFT record is reused.
 *                      First instance is always 0.
 */
typedef struct _ntfs_mft_record_old_t {
    uint32_t magic;                                     ///< 魔数 "FILE"
    uint16_t usa_ofs;                                   ///< USA 字节偏移
    uint16_t usa_count;                                 ///< USA 项数

    uint64_t lsn;                                       ///< LogFile 日志序号 (LSN)
    uint16_t sequence_number;                           ///< 记录重用序号
    uint16_t link_count;                                ///< 硬链接数
    uint16_t attrs_offset;                              ///< 首属性字节偏移 (8 对齐)
    uint16_t flags;                                     ///< NTFS_MFT_RECORD_* 标志
    uint32_t bytes_in_use;                              ///< 已用字节数
    uint32_t bytes_allocated;                           ///< 本记录分配大小
    uint64_t base_mft_record;                           ///< 基记录 MFT 引用 (基记录为 0)
    uint16_t next_attr_instance;                        ///< 下一属性实例号
} ntfs_mft_record_old_t;

typedef char pfs_ntfs_sa_3[(sizeof(ntfs_mft_record_old_t) == 42) ? 1 : -1];

/*
 * System defined attributes (32-bit).  Each attribute type has a corresponding
 * attribute name (Unicode string of maximum 64 character length) as described
 * by the attribute definitions present in the data attribute of the $AttrDef
 * system file.  On NTFS 3.0 volumes the names are just as the types are named
 * in the below defines exchanging AT_ for the dollar sign ($).  If that is not
 * a revealing choice of symbol I do not know what is... (-;
 */
enum {
    NTFS_AT_UNUSED                       = pfs_cpu_to_le32(0),          ///< 未用
    NTFS_AT_STANDARD_INFORMATION         = pfs_cpu_to_le32(0x10),       ///< $STANDARD_INFORMATION
    NTFS_AT_ATTRIBUTE_LIST               = pfs_cpu_to_le32(0x20),       ///< $ATTRIBUTE_LIST
    NTFS_AT_FILE_NAME                    = pfs_cpu_to_le32(0x30),       ///< $FILE_NAME
    NTFS_AT_OBJECT_ID                    = pfs_cpu_to_le32(0x40),       ///< $OBJECT_ID
    NTFS_AT_SECURITY_DESCRIPTOR          = pfs_cpu_to_le32(0x50),       ///< $SECURITY_DESCRIPTOR
    NTFS_AT_VOLUME_NAME                  = pfs_cpu_to_le32(0x60),       ///< $VOLUME_NAME
    NTFS_AT_VOLUME_INFORMATION           = pfs_cpu_to_le32(0x70),       ///< $VOLUME_INFORMATION
    NTFS_AT_DATA                         = pfs_cpu_to_le32(0x80),       ///< $DATA
    NTFS_AT_INDEX_ROOT                   = pfs_cpu_to_le32(0x90),       ///< $INDEX_ROOT
    NTFS_AT_INDEX_ALLOCATION             = pfs_cpu_to_le32(0xa0),       ///< $INDEX_ALLOCATION
    NTFS_AT_BITMAP                       = pfs_cpu_to_le32(0xb0),       ///< $BITMAP
    NTFS_AT_REPARSE_POINT                = pfs_cpu_to_le32(0xc0),       ///< $REPARSE_POINT
    NTFS_AT_EA_INFORMATION               = pfs_cpu_to_le32(0xd0),       ///< $EA_INFORMATION
    NTFS_AT_EA                           = pfs_cpu_to_le32(0xe0),       ///< $EA
    NTFS_AT_PROPERTY_SET                 = pfs_cpu_to_le32(0xf0),       ///< $PROPERTY_SET
    NTFS_AT_LOGGED_UTILITY_STREAM        = pfs_cpu_to_le32(0x100),      ///< $LOGGED_UTILITY_STREAM
    NTFS_AT_FIRST_USER_DEFINED_ATTRIBUTE = pfs_cpu_to_le32(0x1000),     ///< 用户自定义属性起始
    NTFS_AT_END                          = pfs_cpu_to_le32(0xffffffff)  ///< 属性列表结束标记
};

/*
 * The collation rules for sorting views/indexes/etc (32-bit).
 *
 * NTFS_COLLATION_BINARY - Collate by binary compare where the first byte is most
 *    significant.
 * NTFS_COLLATION_UNICODE_STRING - Collate Unicode strings by comparing their binary
 *    Unicode values, except that when a character can be uppercased, the
 *    upper case value collates before the lower case one.
 * NTFS_COLLATION_FILE_NAME - Collate file names as Unicode strings. The collation
 *    is done very much like NTFS_COLLATION_UNICODE_STRING. In fact I have no idea
 *    what the difference is. Perhaps the difference is that file names
 *    would treat some special characters in an odd way (see
 *    unistr.c::ntfs_collate_names() and unistr.c::legal_ansi_char_array[]
 *    for what I mean but NTFS_COLLATION_UNICODE_STRING would not give any special
 *    treatment to any characters at all, but this is speculation.
 * NTFS_COLLATION_NTOFS_ULONG - Sorting is done according to ascending uint32_t key
 *    values. E.g. used for $SII index in NTFS_FILE_Secure, which sorts by
 *    security_id (le32).
 * NTFS_COLLATION_NTOFS_SID - Sorting is done according to ascending SID values.
 *    E.g. used for $O index in NTFS_FILE_Extend/$Quota.
 * NTFS_COLLATION_NTOFS_SECURITY_HASH - Sorting is done first by ascending hash
 *    values and second by ascending security_id values. E.g. used for $SDH
 *    index in NTFS_FILE_Secure.
 * NTFS_COLLATION_NTOFS_ULONGS - Sorting is done according to a sequence of ascending
 *    uint32_t key values. E.g. used for $O index in NTFS_FILE_Extend/$ObjId, which
 *    sorts by object_id (16-byte), by splitting up the object_id in four
 *    uint32_t values and using them as individual keys. E.g. take the following
 *    two security_ids, stored as follows on disk:
 *        1st: a1 61 65 b7 65 7b d4 11 9e 3d 00 e0 81 10 42 59
 *        2nd: 38 14 37 d2 d2 f3 d4 11 a5 21 c8 6b 79 b1 97 45
 *    To compare them, they are split into four uint32_t values each, like so:
 *        1st: 0xb76561a1 0x11d47b65 0xe0003d9e 0x59421081
 *        2nd: 0xd2371438 0x11d4f3d2 0x6bc821a5 0x4597b179
 *    Now, it is apparent why the 2nd object_id collates after the 1st: the
 *    first uint32_t value of the 1st object_id is less than the first uint32_t of
 *    the 2nd object_id. If the first uint32_t values of both object_ids were
 *    equal then the second uint32_t values would be compared, etc.
 */
enum {
    NTFS_COLLATION_BINARY              = pfs_cpu_to_le32(0x00), ///< 按字节比较
    NTFS_COLLATION_FILE_NAME           = pfs_cpu_to_le32(0x01), ///< 文件名排序
    NTFS_COLLATION_UNICODE_STRING      = pfs_cpu_to_le32(0x02), ///< Unicode 字符串
    NTFS_COLLATION_NTOFS_ULONG         = pfs_cpu_to_le32(0x10), ///< 升序 uint32 (如 $SII)
    NTFS_COLLATION_NTOFS_SID           = pfs_cpu_to_le32(0x11), ///< 按 SID 排序
    NTFS_COLLATION_NTOFS_SECURITY_HASH = pfs_cpu_to_le32(0x12), ///< 先哈希再 security_id ($SDH)
    NTFS_COLLATION_NTOFS_ULONGS        = pfs_cpu_to_le32(0x13), ///< 多段 uint32 (如 $ObjId)
};

/*
 * enum - Attribute definition flags
 *
 * The flags (32-bit) describing attribute properties in the attribute
 * definition structure.
 * The INDEXABLE flag is fairly certainly correct as only the file
 * name attribute has this flag set and this is the only attribute indexed in
 * NT4.
 *
 * NTFS_ATTR_DEF_INDEXABLE:      Attribute can be indexed.
 *                          (Used for creating indexes like $I30, $SDH, etc.)
 * NTFS_ATTR_DEF_MULTIPLE:       Attribute type can be present multiple times
 *                          in the MFT record of an inode.
 *                          (e.g., multiple $FILE_NAME, $DATA streams)
 * NTFS_ATTR_DEF_NOT_ZERO:       Attribute value must contain at least one non-zero byte.
 *                          (Prevents empty or all-zero values)
 * NTFS_ATTR_DEF_INDEXED_UNIQUE: Attribute must be indexed and the value must be unique
 *                          for this attribute type across all MFT records of an inode.
 *                          (e.g., security descriptor IDs in $Secure)
 * NTFS_ATTR_DEF_NAMED_UNIQUE:   Attribute must be named and the name must be unique
 *                          for this attribute type across all MFT records of an inode.
 *                          (e.g., named $DATA streams or alternate data streams)
 * NTFS_ATTR_DEF_RESIDENT:       Attribute must be resident (stored in MFT record).
 *                          (Cannot be non-resident/sparse/compressed)
 * NTFS_ATTR_DEF_ALWAYS_LOG:     Always log modifications to this attribute in LogFile,
 *                          regardless of whether it is resident or non-resident.
 *                          Without this flag, modifications are logged only if resident.
 *                          (Used for critical metadata attributes)
 */
enum {
    NTFS_ATTR_DEF_INDEXABLE      = pfs_cpu_to_le32(0x02), ///< 可建索引
    NTFS_ATTR_DEF_MULTIPLE       = pfs_cpu_to_le32(0x04), ///< 同类型可出现多次
    NTFS_ATTR_DEF_NOT_ZERO       = pfs_cpu_to_le32(0x08), ///< 值不得全零
    NTFS_ATTR_DEF_INDEXED_UNIQUE = pfs_cpu_to_le32(0x10), ///< 索引且值唯一
    NTFS_ATTR_DEF_NAMED_UNIQUE   = pfs_cpu_to_le32(0x20), ///< 命名且名唯一
    NTFS_ATTR_DEF_RESIDENT       = pfs_cpu_to_le32(0x40), ///< 必须常驻
    NTFS_ATTR_DEF_ALWAYS_LOG     = pfs_cpu_to_le32(0x80), ///< 修改必记 LogFile
};

/*
 * ntfs_attr_def_t - Attribute definition entry ($AttrDef array)
 *
 * The data attribute of NTFS_FILE_AttrDef contains a sequence of attribute
 * definitions for the NTFS volume. With this, it is supposed to be safe for an
 * older NTFS driver to mount a volume containing a newer NTFS version without
 * damaging it (that's the theory. In practice it's: not damaging it too much).
 * Entries are sorted by attribute type. The flags describe whether the
 * attribute can be resident/non-resident and possibly other things, but the
 * actual bits are unknown.
 *
 * @name:           Unicode (UTF-16LE) name of the attribute (e.g. "$DATA", "$FILE_NAME").
 *                  Zero-terminated string, maximum 0x40 characters (128 bytes).
 *                  Used for human-readable display and debugging.
 * @type:           Attribute type code (NTFS_AT_* constants).
 *                  Defines which attribute this entry describes.
 * @display_rule:   Default display rule (usually 0; rarely used in modern NTFS).
 *                  Controls how the attribute is displayed in tools (legacy).
 * @collation_rule: Default collation rule for indexing this attribute.
 *                  Determines sort order when indexed (e.g. CASE_SENSITIVE, UNICODE).
 *                  Used in $I30, $SDH, $SII, etc.
 * @flags:          Bit array of attribute constraints (NTFS_ATTR_DEF_* flags).
 *                  See NTFS_ATTR_DEF_INDEXABLE, NTFS_ATTR_DEF_MULTIPLE, etc.
 *                  Defines whether the attribute can be indexed, multiple, resident-only, etc.
 * @min_size:       Optional minimum size of the attribute value (in bytes).
 *                  0 means no minimum enforced.
 * @max_size:       Maximum allowed size of the attribute value (in bytes).
 */
typedef struct _ntfs_attr_def_t {
    uint16_t name[0x40];                                ///< 属性名 UTF-16LE (最长 0x40 字符)
    uint32_t type;                                      ///< 属性类型码 (NTFS_AT_*)
    uint32_t display_rule;                              ///< 显示规则 (常为 0)
    uint32_t collation_rule;                            ///< 索引排序规则
    uint32_t flags;                                     ///< NTFS_ATTR_DEF_* 约束标志
    uint64_t min_size;                                  ///< 值最小字节数 (0=不限)
    uint64_t max_size;                                  ///< 值最大字节数
} ntfs_attr_def_t;

typedef char pfs_ntfs_sa_4[(sizeof(ntfs_attr_def_t) == 160) ? 1 : -1];

/*
 * enum - Attribute flags (16-bit) for non-resident attributes
 *
 * NTFS_ATTR_IS_COMPRESSED:      Attribute is compressed.
 *                          If set, data is compressed using the method in
 *                          NTFS_ATTR_COMPRESSION_MASK.
 * NTFS_ATTR_COMPRESSION_MASK:   Mask for compression method.
 *                          Valid values are defined in NTFS compression types
 *                          (e.g., 0x02 = LZNT1, etc.).
 *                          Also serves as the first illegal value for method.
 * NTFS_ATTR_IS_ENCRYPTED:       Attribute is encrypted.
 *                          Data is encrypted using EFS (Encrypting File System).
 * NTFS_ATTR_IS_SPARSE:          Attribute is sparse.
 *                          Contains holes (unallocated regions) that read as zeros.
 */
enum {
    NTFS_ATTR_IS_COMPRESSED    = pfs_cpu_to_le16(0x0001), ///< 已压缩
    NTFS_ATTR_COMPRESSION_MASK = pfs_cpu_to_le16(0x00ff), ///< 压缩方法掩码
    NTFS_ATTR_IS_ENCRYPTED     = pfs_cpu_to_le16(0x4000), ///< 已加密 (EFS)
    NTFS_ATTR_IS_SPARSE        = pfs_cpu_to_le16(0x8000), ///< 稀疏
};

/*
 * Attribute compression.
 *
 * Only the data attribute is ever compressed in the current ntfs driver in
 * Windows. Further, compression is only applied when the data attribute is
 * non-resident. Finally, to use compression, the maximum allowed cluster size
 * on a volume is 4kib.
 *
 * The compression method is based on independently compressing blocks of X
 * clusters, where X is determined from the compression_unit value found in the
 * non-resident attribute record header (more precisely: X = 2^compression_unit
 * clusters). On Windows NT/2k, X always is 16 clusters (compression_unit = 4).
 *
 * There are three different cases of how a compression block of X clusters
 * can be stored:
 *
 *   1) The data in the block is all zero (a sparse block):
 *      This is stored as a sparse block in the runlist, i.e. the runlist
 *      entry has length = X and lcn = -1. The mapping pairs array actually
 *      uses a delta_lcn value length of 0, i.e. delta_lcn is not present at
 *      all, which is then interpreted by the driver as lcn = -1.
 *      NOTE: Even uncompressed files can be sparse on NTFS 3.0 volumes, then
 *      the same principles apply as above, except that the length is not
 *      restricted to being any particular value.
 *
 *   2) The data in the block is not compressed:
 *      This happens when compression doesn't reduce the size of the block
 *      in clusters. I.e. if compression has a small effect so that the
 *      compressed data still occupies X clusters, then the uncompressed data
 *      is stored in the block.
 *      This case is recognised by the fact that the runlist entry has
 *      length = X and lcn >= 0. The mapping pairs array stores this as
 *      normal with a run length of X and some specific delta_lcn, i.e.
 *      delta_lcn has to be present.
 *
 *   3) The data in the block is compressed:
 *      The common case. This case is recognised by the fact that the run
 *      list entry has length L < X and lcn >= 0. The mapping pairs array
 *      stores this as normal with a run length of X and some specific
 *      delta_lcn, i.e. delta_lcn has to be present. This runlist entry is
 *      immediately followed by a sparse entry with length = X - L and
 *      lcn = -1. The latter entry is to make up the vcn counting to the
 *      full compression block size X.
 *
 * In fact, life is more complicated because adjacent entries of the same type
 * can be coalesced. This means that one has to keep track of the number of
 * clusters handled and work on a basis of X clusters at a time being one
 * block. An example: if length L > X this means that this particular runlist
 * entry contains a block of length X and part of one or more blocks of length
 * L - X. Another example: if length L < X, this does not necessarily mean that
 * the block is compressed as it might be that the lcn changes inside the block
 * and hence the following runlist entry describes the continuation of the
 * potentially compressed block. The block would be compressed if the
 * following runlist entry describes at least X - L sparse clusters, thus
 * making up the compression block length as described in point 3 above. (Of
 * course, there can be several runlist entries with small lengths so that the
 * sparse entry does not follow the first data containing entry with
 * length < X.)
 *
 * NOTE: At the end of the compressed attribute value, there most likely is not
 * just the right amount of data to make up a compression block, thus this data
 * is not even attempted to be compressed. It is just stored as is, unless
 * the number of clusters it occupies is reduced when compressed in which case
 * it is stored as a compressed compression block, complete with sparse
 * clusters at the end.
 */

/*
 * enum - Flags for resident attributes (8-bit)
 *
 * NTFS_RESIDENT_ATTR_IS_INDEXED: Attribute is referenced in an index.
 *                           (e.g., part of an index key or entry)
 *                           Has implications for deletion and modification:
 *                            - Cannot be freely removed if indexed
 *                            - Index must be updated when value changes
 *                            - Used for attributes like $FILE_NAME in directories
 */
enum {
    NTFS_RESIDENT_ATTR_IS_INDEXED = 0x01,               ///< 常驻属性被索引引用
};

/*
 * ntfs_attr_record_t - NTFS attribute record header
 *
 * Common header for both resident and non-resident attributes.
 * Always aligned to an 8-byte boundary on disk.
 * Located at attrs_offset in the MFT record (see ntfs_mft_record_t).
 *
 * @type:           32-bit attribute type (NTFS_AT_* constants).
 *                  Identifies the attribute
 *                  (e.g. 0x10 = $STANDARD_INFORMATION).
 * @length:         Total byte size of this attribute record (resident).
 *                  8-byte aligned; used to locate the next attribute.
 * @non_resident:   0 = resident attribute
 *                  1 = non-resident attribute
 * @name_length:    Number of Unicode characters in the attribute name.
 *                  0 if unnamed (most system attributes are unnamed).
 * @name_offset:    Byte offset from start of attribute record to the name.
 *                  8-byte aligned; when creating, place at end of header.
 * @flags:          Attribute flags (see NTFS_ATTR_IS_COMPRESSED,
 *                  NTFS_ATTR_IS_ENCRYPTED, etc.).
 *                  For resident: see RESIDENT_ATTR_* flags.
 * @instance:       Unique instance number within this MFT record.
 *                  Incremented via next_attr_instance; unique per record.
 *
 * Resident attributes (when @non_resident == 0):
 * @data.resident.value_length:     Byte size of the attribute value.
 * @data.resident.value_offset:     Byte offset from start of attribute
 *                                  record to the value data.
 *                                  8-byte aligned if name present.
 * @data.resident.flags:            Resident-specific flags
 * @data.resident.reserved:         Reserved/alignment to 8 bytes.
 *
 * Non-resident attributes (when @non_resident == 1):
 * @data.non_resident.lowest_vcn:   Lowest valid VCN in this extent.
 *                                  Usually 0 unless attribute list is used.
 * @data.non_resident.highest_vcn:  Highest valid VCN in this extent.
 *                                  -1 for zero-length, 0 for single extent.
 * @data.non_resident.mapping_pairs_offset:
 *                                  Byte offset to mapping pairs array
 *                                  (VCN -> LCN mappings).
 *                                  8-byte aligned when creating.
 * @data.non_resident.compression_unit:
 *                                  Log2 of clusters per compression unit.
 *                                  0 = not compressed.
 *                                  WinNT4 used 4; sparse files use 0
 *                                  on XP SP2+.
 * @data.non_resident.reserved:     5 bytes for 8-byte alignment.
 * @data.non_resident.allocated_size:
 *                                  Allocated disk space in bytes.
 *                                  For compressed: logical allocated size.
 * @data.non_resident.data_size:    Logical attribute value size in bytes.
 *                                  Can be larger than allocated_size if
 *                                  compressed/sparse.
 * @data.non_resident.initialized_size:
 *                                  Initialized portion size in bytes.
 *                                  Usually equals data_size.
 * @data.non_resident.compressed_size:
 *                                  Compressed on-disk size in bytes.
 *                                  Only present when compressed or sparse.
 *                                  Actual disk usage.
 */
typedef struct _ntfs_attr_record_t {
    uint32_t type;                                      ///< 属性类型 (NTFS_AT_*)
    uint32_t length;                                    ///< 本属性记录总字节数 (8 对齐)
    uint8_t  non_resident;                              ///< 0=常驻; 1=非常驻
    uint8_t  name_length;                               ///< 属性名 UTF-16 字符数 (0=无名)
    uint16_t name_offset;                               ///< 属性名相对本记录的字节偏移
    uint16_t flags;                                     ///< NTFS_ATTR_IS_* 等
    uint16_t instance;                                  ///< 本 MFT 记录内实例号
    union {
        struct {                                        ///< 常驻 (non_resident==0)
            uint32_t value_length;                      ///< 属性值字节数
            uint16_t value_offset;                      ///< 值相对本记录的字节偏移
            uint8_t  flags;                             ///< 常驻标志 (如 INDEXED)
            int8_t   reserved;                          ///< 对齐保留
        } resident;
        struct {                                        ///< 非常驻 (non_resident==1)
            uint64_t lowest_vcn;                        ///< 本区段最低 VCN
            uint64_t highest_vcn;                       ///< 本区段最高 VCN
            uint16_t mapping_pairs_offset;              ///< mapping pairs 字节偏移
            uint8_t  compression_unit;                  ///< 压缩单元 log2(簇数); 0=未压缩
            uint8_t  reserved[5];                       ///< 对齐保留
            uint64_t allocated_size;                    ///< 已分配磁盘字节数
            uint64_t data_size;                         ///< 逻辑数据大小
            uint64_t initialized_size;                  ///< 已初始化大小
            uint64_t compressed_size;                   ///< 压缩/稀疏实际占用 (有条件出现)
        } non_resident;
    } data;
} ntfs_attr_record_t;

/*
 * enum - NTFS file attribute flags (32-bit)
 *
 * File attribute flags (32-bit) appearing in the file_attributes fields of the
 * STANDARD_INFORMATION attribute of MFT_RECORDs and the FILENAME_ATTR
 * attributes of MFT_RECORDs and directory index entries.
 *
 * All of the below flags appear in the directory index entries but only some
 * appear in the STANDARD_INFORMATION attribute whilst only some others appear
 * in the FILENAME_ATTR attribute of MFT_RECORDs.  Unless otherwise stated the
 * flags appear in all of the above.
 *
 * NTFS_FILE_ATTR_READONLY:         File is read-only.
 * NTFS_FILE_ATTR_HIDDEN:           File is hidden (not shown by default).
 * NTFS_FILE_ATTR_SYSTEM:           System file (protected by OS).
 * NTFS_FILE_ATTR_DIRECTORY:        Directory flag (reserved in NT; use MFT flag instead).
 * NTFS_FILE_ATTR_ARCHIVE:          File needs archiving (backup flag).
 * NTFS_FILE_ATTR_DEVICE:           Device file (rarely used).
 * NTFS_FILE_ATTR_NORMAL:           Normal file (no special attributes).
 * NTFS_FILE_ATTR_TEMPORARY:        Temporary file (delete on close).
 * NTFS_FILE_ATTR_SPARSE_FILE:      Sparse file (contains holes).
 * NTFS_FILE_ATTR_REPARSE_POINT:    Reparse point (junction, symlink, mount point).
 * NTFS_FILE_ATTR_COMPRESSED:       File is compressed.
 * NTFS_FILE_ATTR_OFFLINE:          File data is offline (not locally available).
 * NTFS_FILE_ATTR_NOT_CONTENT_INDEXED:
 *                              File is excluded from content indexing.
 * NTFS_FILE_ATTR_ENCRYPTED:        File is encrypted (EFS).
 * NTFS_FILE_ATTR_VALID_FLAGS:      Mask of all valid flags for reading.
 * NTFS_FILE_ATTR_VALID_SET_FLAGS:  Mask of flags that can be set by user.
 * NTFS_FILE_ATTRIBUTE_RECALL_ON_OPEN:
 *                              Recall data on open (cloud/HSM related).
 * NTFS_FILE_ATTR_DUP_FILE_NAME_INDEX_PRESENT:
 *                              $FILE_NAME has duplicate index entry.
 * NTFS_FILE_ATTR_DUP_VIEW_INDEX_PRESENT:
 *                              Duplicate view index present (object ID, quota, etc.).
 */
enum {
    NTFS_FILE_ATTR_READONLY                     = pfs_cpu_to_le32(0x00000001), ///< 只读
    NTFS_FILE_ATTR_HIDDEN                       = pfs_cpu_to_le32(0x00000002), ///< 隐藏
    NTFS_FILE_ATTR_SYSTEM                       = pfs_cpu_to_le32(0x00000004), ///< 系统
    /* Old DOS volid. Unused in NT. = pfs_cpu_to_le32(0x00000008), */
    NTFS_FILE_ATTR_DIRECTORY                    = pfs_cpu_to_le32(0x00000010), ///< 目录 (DOS 遗留)
    NTFS_FILE_ATTR_ARCHIVE                      = pfs_cpu_to_le32(0x00000020), ///< 归档
    NTFS_FILE_ATTR_DEVICE                       = pfs_cpu_to_le32(0x00000040), ///< 设备
    NTFS_FILE_ATTR_NORMAL                       = pfs_cpu_to_le32(0x00000080), ///< 普通

    NTFS_FILE_ATTR_TEMPORARY                    = pfs_cpu_to_le32(0x00000100), ///< 临时
    NTFS_FILE_ATTR_SPARSE_FILE                  = pfs_cpu_to_le32(0x00000200), ///< 稀疏文件
    NTFS_FILE_ATTR_REPARSE_POINT                = pfs_cpu_to_le32(0x00000400), ///< 重解析点
    NTFS_FILE_ATTR_COMPRESSED                   = pfs_cpu_to_le32(0x00000800), ///< 压缩

    NTFS_FILE_ATTR_OFFLINE                      = pfs_cpu_to_le32(0x00001000), ///< 离线
    NTFS_FILE_ATTR_NOT_CONTENT_INDEXED          = pfs_cpu_to_le32(0x00002000), ///< 不参与内容索引
    NTFS_FILE_ATTR_ENCRYPTED                    = pfs_cpu_to_le32(0x00004000), ///< 加密

    NTFS_FILE_ATTR_VALID_FLAGS                  = pfs_cpu_to_le32(0x00007fb7), ///< 合法标志掩码
    NTFS_FILE_ATTR_VALID_SET_FLAGS              = pfs_cpu_to_le32(0x000031a7), ///< 用户可设掩码
    NTFS_FILE_ATTRIBUTE_RECALL_ON_OPEN          = pfs_cpu_to_le32(0x00040000), ///< 打开时召回
    NTFS_FILE_ATTR_DUP_FILE_NAME_INDEX_PRESENT  = pfs_cpu_to_le32(0x10000000), ///< 有文件名索引副本
    NTFS_FILE_ATTR_DUP_VIEW_INDEX_PRESENT       = pfs_cpu_to_le32(0x20000000), ///< 有视图索引副本
};

/*
 * NOTE on times in NTFS: All times are in MS standard time format, i.e. they
 * are the number of 100-nanosecond intervals since 1st January 1601, 00:00:00
 * universal coordinated time (UTC). (In Linux time starts 1st January 1970,
 * 00:00:00 UTC and is stored as the number of 1-second intervals since then.)
 */

/*
 * ntfs_standard_information_t - $STANDARD_INFORMATION attribute content
 *
 * NOTE: Always resident.
 * NOTE: Present in all base file records on a volume.
 * NOTE: There is conflicting information about the meaning of each of the time
 *     fields but the meaning as defined below has been verified to be
 *     correct by practical experimentation on Windows NT4 SP6a and is hence
 *     assumed to be the one and only correct interpretation.
 *
 * @creation_time:          File creation time (NTFS timestamp).
 *                          Updated on filename change(?).
 * @last_data_change_time:  Last modification time of data streams.
 * @last_mft_change_time:   Last modification time of this MFT record.
 * @last_access_time:       Last access time (approximate).
 *                          Not updated on read-only volumes; can be disabled.
 * @file_attributes:        File attribute flags (NTFS_FILE_ATTR_* bits).
 *
 * Union (version-specific fields):
 * @ver.v1.reserved12:      12 bytes reserved/alignment (NTFS 1.2 only).
 *
 * @ver.v3 (NTFS 3.x / Windows 2000+):
 * @maximum_versions:       Max allowed file versions (0 = disabled).
 * @version_number:         Current version number (0 if disabled).
 * @class_id:               Class ID (from bidirectional index?).
 * @owner_id:               Owner ID (maps to $Quota via $Q index).
 * @security_id:            Security ID (maps to $Secure $SII/$SDS).
 * @quota_charged:          Quota charge in bytes (0 if quotas disabled).
 * @usn:                    Last USN from $UsnJrnl (0 if disabled).
 */
typedef struct _ntfs_standard_information_t {
    uint64_t creation_time;                             ///< 创建时间 (100ns since 1601)
    uint64_t last_data_change_time;                     ///< 数据最后修改时间
    uint64_t last_mft_change_time;                      ///< MFT 记录最后修改时间
    uint64_t last_access_time;                          ///< 最后访问时间
    uint32_t file_attributes;                           ///< NTFS_FILE_ATTR_* 文件属性
    union {
        struct {                                        ///< NTFS 1.2
            uint8_t reserved12[12];                     ///< 保留/对齐
        } v1;
        struct {                                        ///< NTFS 3.x
            uint32_t maximum_versions;                  ///< 最大版本数 (0=禁用)
            uint32_t version_number;                    ///< 当前版本号
            uint32_t class_id;                          ///< 类 ID
            uint32_t owner_id;                          ///< 所有者 ID ($Quota)
            uint32_t security_id;                       ///< 安全描述符 ID ($Secure)
            uint64_t quota_charged;                     ///< 配额占用字节
            uint64_t usn;                               ///< 最近 USN ($UsnJrnl)
        } v3;
    } ver;
} ntfs_standard_information_t;

/*
 * ntfs_attr_list_entry_t - Entry in $ATTRIBUTE_LIST attribute.
 *
 * @type:           Attribute type code (NTFS_AT_*).
 * @length:         Byte size of this entry (8-byte aligned).
 * @name_length:    Unicode char count of attribute name (0 if unnamed).
 * @name_offset:    Byte offset from start of entry to name (always set).
 * @lowest_vcn:     Lowest VCN of this attribute extent (usually 0).
 *                  Signed value; non-zero when attribute spans extents.
 * @mft_reference:  MFT record reference holding this attribute extent.
 * @instance:       Attribute instance number (if lowest_vcn == 0); else 0.
 * @name:           Variable Unicode name (use @name_offset when reading).
 *
 * - Can be either resident or non-resident.
 * - Value consists of a sequence of variable length, 8-byte aligned,
 * ATTR_LIST_ENTRY records.
 * - The list is not terminated by anything at all! The only way to know when
 * the end is reached is to keep track of the current offset and compare it to
 * the attribute value size.
 * - The attribute list attribute contains one entry for each attribute of
 * the file in which the list is located, except for the list attribute
 * itself. The list is sorted: first by attribute type, second by attribute
 * name (if present), third by instance number. The extents of one
 * non-resident attribute (if present) immediately follow after the initial
 * extent. They are ordered by lowest_vcn and have their instance set to zero.
 * It is not allowed to have two attributes with all sorting keys equal.
 * - Further restrictions:
 *    - If not resident, the vcn to lcn mapping array has to fit inside the
 *      base mft record.
 *    - The attribute list attribute value has a maximum size of 256kb. This
 *      is imposed by the Windows cache manager.
 * - Attribute lists are only used when the attributes of mft record do not
 * fit inside the mft record despite all attributes (that can be made
 * non-resident) having been made non-resident. This can happen e.g. when:
 *    - File has a large number of hard links (lots of file name
 *      attributes present).
 *    - The mapping pairs array of some non-resident attribute becomes so
 *      large due to fragmentation that it overflows the mft record.
 *    - The security descriptor is very complex (not applicable to
 *      NTFS 3.0 volumes).
 *    - There are many named streams.
 */
typedef struct _ntfs_attr_list_entry_t {
    uint32_t type;                                      ///< 属性类型 (NTFS_AT_*)
    uint16_t length;                                    ///< 本项字节数 (8 对齐)
    uint8_t  name_length;                               ///< 属性名字符数
    uint8_t  name_offset;                               ///< 名相对本项的字节偏移
    uint64_t lowest_vcn;                                ///< 本区段最低 VCN
    uint64_t mft_reference;                             ///< 存放该属性的 MFT 引用
    uint16_t instance;                                  ///< 属性实例号 (lowest_vcn==0 时有效)
    uint16_t name[];                                    ///< 可变长 UTF-16 属性名
} ntfs_attr_list_entry_t;

/*
 * The maximum allowed length for a file name.
 */
#define NTFS_MAXIMUM_FILE_NAME_LENGTH   255             ///< 文件名最大 UTF-16 字符数

/*
 * enum - Possible namespaces for filenames in ntfs (8-bit).
 *
 * NTFS_FILE_NAME_POSIX        POSIX namespace (case sensitive, most permissive).
 *                        Allows all Unicode except '\0' and '/'.
 *                        WinNT/2k/2003 default utilities ignore case
 *                        differences. SFU (Services For Unix) enables true
 *                        case sensitivity.
 *                        SFU restricts some chars: '"', '/', '<', '>', '\'.
 * NTFS_FILE_NAME_WIN32        Standard WinNT/2k long filename namespace
 *                        (case insensitive).
 *                        Disallows '\0', '"', '*', '/', ':', '<', '>', '?',
 *                        '\', '|'. Names cannot end with '.' or space.
 * NTFS_FILE_NAME_DOS          DOS 8.3 namespace (uppercase only).
 *                        Allows 8-bit chars > space except '"', '*', '+',
 *                        ',', '/', ':', ';', '<', '=', '>', '?', '\'.
 * NTFS_FILE_NAME_WIN32_AND_DOS
 *                        Win32 and DOS names are identical (single record).
 *                        Value 0x03 indicates both are stored in one entry.
 */
enum {
    NTFS_FILE_NAME_POSIX         = 0x00,                ///< POSIX 命名空间 (大小写敏感)
    NTFS_FILE_NAME_WIN32         = 0x01,                ///< Win32 长名
    NTFS_FILE_NAME_DOS           = 0x02,                ///< DOS 8.3
    NTFS_FILE_NAME_WIN32_AND_DOS = 0x03,                ///< Win32 与 DOS 名相同
};

/*
 * ntfs_file_name_attr_t - $FILE_NAME attribute content
 *
 * NOTE: Always resident.
 * NOTE: All fields, except the parent_directory, are only updated when the
 *     filename is changed. Until then, they just become out of sync with
 *     reality and the more up to date values are present in the standard
 *     information attribute.
 * NOTE: There is conflicting information about the meaning of each of the time
 *     fields but the meaning as defined below has been verified to be
 *     correct by practical experimentation on Windows NT4 SP6a and is hence
 *     assumed to be the one and only correct interpretation.
 *
 * @parent_directory:   MFT reference to parent directory.
 * @creation_time:      File creation time (NTFS timestamp).
 * @last_data_change_time:
 *                      Last data modification time.
 * @last_mft_change_time:
 *                      Last MFT record modification time.
 * @last_access_time:   Last access time (approximate; may not
 *                      update always).
 * @allocated_size:     On-disk allocated size for unnamed $DATA.
 *                      Equals compressed_size if compressed/sparse.
 *                      0 for directories or no $DATA.
 *                      Multiple of cluster size.
 * @data_size:          Logical size of unnamed $DATA.
 *                      0 for directories or no $DATA.
 * @file_attributes:    File attribute flags (NTFS_FILE_ATTR_* bits).
 * @type.ea.packed_ea_size:
 *                      Size needed to pack EAs (if present).
 * @type.ea.reserved:   Alignment padding.
 * @type.rp.reparse_point_tag:
 *                      Reparse point type (if reparse point, no EAs).
 * @file_name_length:   Length of filename in Unicode characters.
 * @file_name_type:     Namespace (NTFS_FILE_NAME_POSIX, WIN32, DOS, etc.).
 * @file_name:          Variable-length Unicode filename.
 */
typedef struct _ntfs_file_name_attr_t {
    uint64_t parent_directory;                          ///< 父目录 MFT 引用
    uint64_t creation_time;                             ///< 创建时间
    uint64_t last_data_change_time;                     ///< 数据最后修改时间
    uint64_t last_mft_change_time;                      ///< MFT 最后修改时间
    uint64_t last_access_time;                          ///< 最后访问时间
    uint64_t allocated_size;                            ///< 未命名 $DATA 分配大小
    uint64_t data_size;                                 ///< 未命名 $DATA 逻辑大小
    uint32_t file_attributes;                           ///< NTFS_FILE_ATTR_* 
    union {
        struct {
            uint16_t packed_ea_size;                    ///< 打包 EA 所需大小
            uint16_t reserved;                          ///< 对齐保留
        } ea;
        struct {
            uint32_t reparse_point_tag;                 ///< 重解析点标签
        } rp;
    } type;
    uint8_t  file_name_length;                          ///< 文件名 UTF-16 字符数
    uint8_t  file_name_type;                            ///< 命名空间 (POSIX/WIN32/DOS)
    uint16_t file_name[];                               ///< 可变长 UTF-16 文件名
} ntfs_file_name_attr_t;

/*
 * ntfs_guid_t - Globally Unique Identifier (GUID) structure
 *
 * GUID structures store globally unique identifiers (GUID). A GUID is a
 * 128-bit value consisting of one group of eight hexadecimal digits, followed
 * by three groups of four hexadecimal digits each, followed by one group of
 * twelve hexadecimal digits. GUIDs are Microsoft's implementation of the
 * distributed computing environment (DCE) universally unique identifier (UUID).
 * Example of a GUID:
 *    1F010768-5A73-BC91-0010A52216A7
 *
 * @data1:      First 32 bits (first 8 hex digits).
 * @data2:      Next 16 bits (first group of 4 hex digits).
 * @data3:      Next 16 bits (second group of 4 hex digits).
 * @data4:      Final 64 bits (third group of 4 + last 12 hex digits).
 *              data4[0-1]: third group; data4[2-7]: remaining part.
 */
typedef struct _ntfs_guid_t {
    uint32_t data1;                                     ///< GUID 前 32 bit
    uint16_t data2;                                     ///< 随后 16 bit
    uint16_t data3;                                     ///< 再 16 bit
    uint8_t  data4[8];                                  ///< 末 64 bit
} ntfs_guid_t;

/*
 * ntfs_object_id_attr_t - $OBJECT_ID attribute content (NTFS 3.0+)
 *
 * NOTE: Always resident.
 *
 * @object_id:          Unique 128-bit GUID assigned to the file.
 *                      Core identifier; always present.
 *
 * Optional extended info (union; total value size 16-64 bytes):
 * @extended_info.birth_volume_id:
 *                      Birth volume GUID (where file was first created).
 * @extended_info.birth_object_id:
 *                      Birth object GUID (original ID before copy/move).
 * @extended_info.domain_id:
 *                      Domain GUID (usually zero; reserved).
 */
typedef struct _ntfs_object_id_attr_t {
    ntfs_guid_t object_id;                              ///< 对象 GUID (必有)
    union {
        struct {
            ntfs_guid_t birth_volume_id;                ///< 出生卷 GUID
            ntfs_guid_t birth_object_id;                ///< 出生对象 GUID
            ntfs_guid_t domain_id;                      ///< 域 GUID (常为 0)
        };
        uint8_t extended_info[48];                      ///< 扩展信息原始 48 字节
    };
} ntfs_object_id_attr_t;

/*
 * enum - RIDs (Relative Identifiers) in Windows/NTFS security
 *
 * These relative identifiers (RIDs) are used with the above identifier
 * authorities to make up universal well-known SIDs.
 *
 * NTFS_SECURITY_NULL_RID              S-1-0 (Null authority)
 * NTFS_SECURITY_WORLD_RID             S-1-1 (World/Everyone)
 * NTFS_SECURITY_LOCAL_RID             S-1-2 (Local)
 * NTFS_SECURITY_CREATOR_OWNER_RID     S-1-3-0 (Creator Owner)
 * NTFS_SECURITY_CREATOR_GROUP_RID     S-1-3-1 (Creator Group)
 * NTFS_SECURITY_CREATOR_OWNER_SERVER_RID S-1-3-2 (Server Creator Owner)
 * NTFS_SECURITY_CREATOR_GROUP_SERVER_RID S-1-3-3 (Server Creator Group)
 * NTFS_SECURITY_DIALUP_RID            S-1-5-1 (Dialup)
 * NTFS_SECURITY_NETWORK_RID           S-1-5-2 (Network)
 * NTFS_SECURITY_BATCH_RID             S-1-5-3 (Batch)
 * NTFS_SECURITY_INTERACTIVE_RID       S-1-5-4 (Interactive)
 * NTFS_SECURITY_SERVICE_RID           S-1-5-6 (Service)
 * NTFS_SECURITY_ANONYMOUS_LOGON_RID   S-1-5-7 (Anonymous Logon)
 * NTFS_SECURITY_PROXY_RID             S-1-5-8 (Proxy)
 * NTFS_SECURITY_ENTERPRISE_CONTROLLERS_RID S-1-5-9 (Enterprise DCs)
 * NTFS_SECURITY_SERVER_LOGON_RID      S-1-5-9 (Server Logon alias)
 * NTFS_SECURITY_PRINCIPAL_SELF_RID    S-1-5-10 (Self/PrincipalSelf)
 * NTFS_SECURITY_AUTHENTICATED_USER_RID S-1-5-11 (Authenticated Users)
 * NTFS_SECURITY_RESTRICTED_CODE_RID   S-1-5-12 (Restricted Code)
 * NTFS_SECURITY_TERMINAL_SERVER_RID   S-1-5-13 (Terminal Server)
 * NTFS_SECURITY_LOGON_IDS_RID         S-1-5-5 (Logon session IDs base)
 * NTFS_SECURITY_LOCAL_SYSTEM_RID      S-1-5-18 (Local System)
 * NTFS_SECURITY_NT_NON_UNIQUE         S-1-5-21 (NT non-unique authority)
 * NTFS_SECURITY_BUILTIN_DOMAIN_RID    S-1-5-32 (Built-in domain)
 *
 * Built-in domain relative RIDs (S-1-5-32-...):
 * Users:
 * NTFS_DOMAIN_USER_RID_ADMIN          Administrator
 * NTFS_DOMAIN_USER_RID_GUEST          Guest
 * NTFS_DOMAIN_USER_RID_KRBTGT         krbtgt (Kerberos ticket-granting)
 *
 * Groups:
 * NTFS_DOMAIN_GROUP_RID_ADMINS        Administrators
 * NTFS_DOMAIN_GROUP_RID_USERS         Users
 * NTFS_DOMAIN_GROUP_RID_GUESTS        Guests
 * NTFS_DOMAIN_GROUP_RID_COMPUTERS     Computers
 * NTFS_DOMAIN_GROUP_RID_CONTROLLERS   Domain Controllers
 * NTFS_DOMAIN_GROUP_RID_CERT_ADMINS   Cert Publishers
 * NTFS_DOMAIN_GROUP_RID_SCHEMA_ADMINS Schema Admins
 * NTFS_DOMAIN_GROUP_RID_ENTERPRISE_ADMINS Enterprise Admins
 * NTFS_DOMAIN_GROUP_RID_POLICY_ADMINS Policy Admins (if present)
 *
 * Aliases:
 * NTFS_DOMAIN_ALIAS_RID_ADMINS        Administrators alias
 * NTFS_DOMAIN_ALIAS_RID_USERS         Users alias
 * NTFS_DOMAIN_ALIAS_RID_GUESTS        Guests alias
 * NTFS_DOMAIN_ALIAS_RID_POWER_USERS   Power Users
 * NTFS_DOMAIN_ALIAS_RID_ACCOUNT_OPS   Account Operators
 * NTFS_DOMAIN_ALIAS_RID_SYSTEM_OPS    Server Operators
 * NTFS_DOMAIN_ALIAS_RID_PRINT_OPS     Print Operators
 * NTFS_DOMAIN_ALIAS_RID_BACKUP_OPS    Backup Operators
 * NTFS_DOMAIN_ALIAS_RID_REPLICATOR    Replicator
 * NTFS_DOMAIN_ALIAS_RID_RAS_SERVERS   RAS Servers
 * NTFS_DOMAIN_ALIAS_RID_PREW2KCOMPACCESS Pre-Windows 2000 Compatible Access
 *
 * Note: The relative identifier (RID) refers to the portion of a SID, which
 * identifies a user or group in relation to the authority that issued the SID.
 * For example, the universal well-known SID Creator Owner ID (S-1-3-0) is
 * made up of the identifier authority SECURITY_CREATOR_SID_AUTHORITY (3) and
 * the relative identifier NTFS_SECURITY_CREATOR_OWNER_RID (0).
 */
enum {                                                      ///< RID / 标识机构相对 ID
    NTFS_SECURITY_NULL_RID                   = 0,       ///< S-1-0
    NTFS_SECURITY_WORLD_RID                  = 0,       ///< S-1-1 Everyone
    NTFS_SECURITY_LOCAL_RID                  = 0,       ///< S-1-2

    NTFS_SECURITY_CREATOR_OWNER_RID          = 0,       ///< S-1-3-0 Creator Owner
    NTFS_SECURITY_CREATOR_GROUP_RID          = 1,       ///< S-1-3-1 Creator Group

    NTFS_SECURITY_CREATOR_OWNER_SERVER_RID   = 2,       ///< S-1-3-2
    NTFS_SECURITY_CREATOR_GROUP_SERVER_RID   = 3,       ///< S-1-3-3

    NTFS_SECURITY_DIALUP_RID                 = 1,       ///< Dialup
    NTFS_SECURITY_NETWORK_RID                = 2,       ///< Network
    NTFS_SECURITY_BATCH_RID                  = 3,       ///< Batch
    NTFS_SECURITY_INTERACTIVE_RID            = 4,       ///< Interactive
    NTFS_SECURITY_SERVICE_RID                = 6,       ///< Service
    NTFS_SECURITY_ANONYMOUS_LOGON_RID        = 7,       ///< Anonymous
    NTFS_SECURITY_PROXY_RID                  = 8,       ///< Proxy
    NTFS_SECURITY_ENTERPRISE_CONTROLLERS_RID = 9,       ///< Enterprise DCs
    NTFS_SECURITY_SERVER_LOGON_RID           = 9,       ///< Server Logon
    NTFS_SECURITY_PRINCIPAL_SELF_RID         = 0xa,     ///< Self
    NTFS_SECURITY_AUTHENTICATED_USER_RID     = 0xb,     ///< Authenticated Users
    NTFS_SECURITY_RESTRICTED_CODE_RID        = 0xc,     ///< Restricted
    NTFS_SECURITY_TERMINAL_SERVER_RID        = 0xd,     ///< Terminal Server

    NTFS_SECURITY_LOGON_IDS_RID              = 5,       ///< Logon IDs 基
    NTFS_SECURITY_LOGON_IDS_RID_COUNT        = 3,       ///< Logon IDs 子权限个数

    NTFS_SECURITY_LOCAL_SYSTEM_RID           = 0x12,    ///< Local System

    NTFS_SECURITY_NT_NON_UNIQUE              = 0x15,    ///< NT non-unique

    NTFS_SECURITY_BUILTIN_DOMAIN_RID         = 0x20,    ///< Built-in domain

    /* Users. */
    NTFS_DOMAIN_USER_RID_ADMIN               = 0x1f4,   ///< Administrator
    NTFS_DOMAIN_USER_RID_GUEST               = 0x1f5,   ///< Guest
    NTFS_DOMAIN_USER_RID_KRBTGT              = 0x1f6,   ///< krbtgt

    /* Groups. */
    NTFS_DOMAIN_GROUP_RID_ADMINS             = 0x200,   ///< Domain Admins
    NTFS_DOMAIN_GROUP_RID_USERS              = 0x201,   ///< Domain Users
    NTFS_DOMAIN_GROUP_RID_GUESTS             = 0x202,   ///< Domain Guests
    NTFS_DOMAIN_GROUP_RID_COMPUTERS          = 0x203,   ///< Domain Computers
    NTFS_DOMAIN_GROUP_RID_CONTROLLERS        = 0x204,   ///< Domain Controllers
    NTFS_DOMAIN_GROUP_RID_CERT_ADMINS        = 0x205,   ///< Cert Publishers
    NTFS_DOMAIN_GROUP_RID_SCHEMA_ADMINS      = 0x206,   ///< Schema Admins
    NTFS_DOMAIN_GROUP_RID_ENTERPRISE_ADMINS  = 0x207,   ///< Enterprise Admins
    NTFS_DOMAIN_GROUP_RID_POLICY_ADMINS      = 0x208,   ///< Policy Admins

    /* Aliases. */
    NTFS_DOMAIN_ALIAS_RID_ADMINS             = 0x220,   ///< Administrators
    NTFS_DOMAIN_ALIAS_RID_USERS              = 0x221,   ///< Users
    NTFS_DOMAIN_ALIAS_RID_GUESTS             = 0x222,   ///< Guests
    NTFS_DOMAIN_ALIAS_RID_POWER_USERS        = 0x223,   ///< Power Users

    NTFS_DOMAIN_ALIAS_RID_ACCOUNT_OPS        = 0x224,   ///< Account Operators
    NTFS_DOMAIN_ALIAS_RID_SYSTEM_OPS         = 0x225,   ///< Server Operators
    NTFS_DOMAIN_ALIAS_RID_PRINT_OPS          = 0x226,   ///< Print Operators
    NTFS_DOMAIN_ALIAS_RID_BACKUP_OPS         = 0x227,   ///< Backup Operators

    NTFS_DOMAIN_ALIAS_RID_REPLICATOR         = 0x228,   ///< Replicator
    NTFS_DOMAIN_ALIAS_RID_RAS_SERVERS        = 0x229,   ///< RAS Servers
    NTFS_DOMAIN_ALIAS_RID_PREW2KCOMPACCESS   = 0x22a,   ///< Pre-Windows 2000 Compatible Access
};

/*
 * The universal well-known SIDs:
 *
 *    NULL_SID            S-1-0-0
 *    WORLD_SID            S-1-1-0
 *    LOCAL_SID            S-1-2-0
 *    CREATOR_OWNER_SID        S-1-3-0
 *    CREATOR_GROUP_SID        S-1-3-1
 *    CREATOR_OWNER_SERVER_SID    S-1-3-2
 *    CREATOR_GROUP_SERVER_SID    S-1-3-3
 *
 *    (Non-unique IDs)        S-1-4
 *
 * NT well-known SIDs:
 *
 *    NT_AUTHORITY_SID    S-1-5
 *    DIALUP_SID        S-1-5-1
 *
 *    NETWORD_SID        S-1-5-2
 *    BATCH_SID        S-1-5-3
 *    INTERACTIVE_SID        S-1-5-4
 *    SERVICE_SID        S-1-5-6
 *    ANONYMOUS_LOGON_SID    S-1-5-7        (aka null logon session)
 *    PROXY_SID        S-1-5-8
 *    SERVER_LOGON_SID    S-1-5-9        (aka domain controller account)
 *    SELF_SID        S-1-5-10    (self RID)
 *    AUTHENTICATED_USER_SID    S-1-5-11
 *    RESTRICTED_CODE_SID    S-1-5-12    (running restricted code)
 *    TERMINAL_SERVER_SID    S-1-5-13    (running on terminal server)
 *
 *    (Logon IDs)        S-1-5-5-X-Y
 *
 *    (NT non-unique IDs)    S-1-5-0x15-...
 *
 *    (Built-in domain)    S-1-5-0x20
 */

/*
 * ntfs_sid_t - Security Identifier (SID) structure
 *
 * @revision:            SID revision level (usually 1).
 * @sub_authority_count: Number of sub-authorities (1 or more).
 * @identifier_authority:
 *                       48-bit identifier authority (S-1-x-...).
 *                       @parts.high_part: high 16 bits.
 *                       @parts.low_part: low 32 bits.
 *                       @value: raw 6-byte array.
 * @sub_authority:       Variable array of 32-bit RIDs.
 *                       At least one; defines the SID relative to authority.
 *
 * The SID structure is a variable-length structure used to uniquely identify
 * users or groups. SID stands for security identifier.
 *
 * The standard textual representation of the SID is of the form:
 *    S-R-I-S-S...
 * Where:
 *    - The first "S" is the literal character 'S' identifying the following
 *    digits as a SID.
 *    - R is the revision level of the SID expressed as a sequence of digits
 *    either in decimal or hexadecimal (if the later, prefixed by "0x").
 *    - I is the 48-bit identifier_authority, expressed as digits as R above.
 *    - S... is one or more sub_authority values, expressed as digits as above.
 *
 * Example SID; the domain-relative SID of the local Administrators group on
 * Windows NT/2k:
 *    S-1-5-32-544
 * This translates to a SID with:
 *    revision = 1,
 *    sub_authority_count = 2,
 *    identifier_authority = {0,0,0,0,0,5},    // SECURITY_NT_AUTHORITY
 *    sub_authority[0] = 32,            // NTFS_SECURITY_BUILTIN_DOMAIN_RID
 *    sub_authority[1] = 544            // NTFS_DOMAIN_ALIAS_RID_ADMINS
 */
typedef struct _ntfs_sid_t {
    uint8_t revision;                                   ///< SID 修订号 (常为 1)
    uint8_t sub_authority_count;                        ///< 子权限个数
    union {
        struct {
            uint16_t high_part;                         ///< 标识机构高 16 bit
            uint32_t low_part;                          ///< 标识机构低 32 bit
        } parts;
        uint8_t value[6];                               ///< 标识机构原始 6 字节
    } identifier_authority;
    uint32_t sub_authority[];                           ///< 可变长 RID 数组
} ntfs_sid_t;

/*
 * enum - Predefined ACE types (8-bit) for NTFS security descriptors
 *
 * NTFS_ACCESS_MIN_MS_ACE_TYPE:         Minimum MS ACE type (0).
 * NTFS_ACCESS_ALLOWED_ACE_TYPE:        Allow access (standard ACE).
 * NTFS_ACCESS_DENIED_ACE_TYPE:         Deny access (standard ACE).
 * NTFS_SYSTEM_AUDIT_ACE_TYPE:          Audit successful/failed access.
 * NTFS_SYSTEM_ALARM_ACE_TYPE:          Alarm on access (not in Win2k+).
 * NTFS_ACCESS_MAX_MS_V2_ACE_TYPE:      Max for V2 ACE types.
 * NTFS_ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
 *                                 Compound ACE (legacy).
 * NTFS_ACCESS_MAX_MS_V3_ACE_TYPE:      Max for V3 ACE types.
 * NTFS_ACCESS_MIN_MS_OBJECT_ACE_TYPE:  Min for object ACE types (Win2k+).
 * NTFS_ACCESS_ALLOWED_OBJECT_ACE_TYPE: Allow with object-specific rights.
 * NTFS_ACCESS_DENIED_OBJECT_ACE_TYPE:  Deny with object-specific rights.
 * NTFS_SYSTEM_AUDIT_OBJECT_ACE_TYPE:   Audit with object-specific rights.
 * NTFS_SYSTEM_ALARM_OBJECT_ACE_TYPE:   Alarm with object-specific rights.
 * NTFS_ACCESS_MAX_MS_OBJECT_ACE_TYPE:  Max for object ACE types.
 * NTFS_ACCESS_MAX_MS_V4_ACE_TYPE:      Max for V4 ACE types.
 * NTFS_ACCESS_MAX_MS_ACE_TYPE:         Overall max ACE type (WinNT/2k).
 */
enum {
    NTFS_ACCESS_MIN_MS_ACE_TYPE           = 0,          ///< MS ACE 类型下限
    NTFS_ACCESS_ALLOWED_ACE_TYPE          = 0,          ///< 允许访问
    NTFS_ACCESS_DENIED_ACE_TYPE           = 1,          ///< 拒绝访问
    NTFS_SYSTEM_AUDIT_ACE_TYPE            = 2,          ///< 系统审计
    NTFS_SYSTEM_ALARM_ACE_TYPE            = 3,          ///< 系统告警
    NTFS_ACCESS_MAX_MS_V2_ACE_TYPE        = 3,          ///< V2 ACE 上限

    NTFS_ACCESS_ALLOWED_COMPOUND_ACE_TYPE = 4,          ///< 复合允许 (遗留)
    NTFS_ACCESS_MAX_MS_V3_ACE_TYPE        = 4,          ///< V3 ACE 上限
    NTFS_ACCESS_MIN_MS_OBJECT_ACE_TYPE    = 5,          ///< 对象 ACE 下限
    NTFS_ACCESS_ALLOWED_OBJECT_ACE_TYPE   = 5,          ///< 对象允许
    NTFS_ACCESS_DENIED_OBJECT_ACE_TYPE    = 6,          ///< 对象拒绝
    NTFS_SYSTEM_AUDIT_OBJECT_ACE_TYPE     = 7,          ///< 对象审计
    NTFS_SYSTEM_ALARM_OBJECT_ACE_TYPE     = 8,          ///< 对象告警
    NTFS_ACCESS_MAX_MS_OBJECT_ACE_TYPE    = 8,          ///< 对象 ACE 上限

    NTFS_ACCESS_MAX_MS_V4_ACE_TYPE        = 8,          ///< V4 ACE 上限
    NTFS_ACCESS_MAX_MS_ACE_TYPE           = 8,          ///< MS ACE 总上限
};

/*
 * enum - ACE inheritance and audit flags (8-bit)
 *
 * NTFS_OBJECT_INHERIT_ACE:         Object inherit (files inherit this ACE).
 * NTFS_CONTAINER_INHERIT_ACE:      Container inherit (subdirectories inherit).
 * NTFS_NO_PROPAGATE_INHERIT_ACE:   No propagation (stop inheritance after this level).
 * NTFS_INHERIT_ONLY_ACE:           Inherit only (not applied to current object).
 * NTFS_INHERITED_ACE:              ACE was inherited (Win2k+ only).
 * NTFS_VALID_INHERIT_FLAGS:        Mask of all valid inheritance flags (0x1f).
 * NTFS_SUCCESSFUL_ACCESS_ACE_FLAG: Audit successful access (system audit ACE).
 * NTFS_FAILED_ACCESS_ACE_FLAG:     Audit failed access (system audit ACE).
 *
 * NTFS_SUCCESSFUL_ACCESS_ACE_FLAG is only used with system audit and alarm ACE
 * types to indicate that a message is generated (in Windows!) for successful
 * accesses.
 *
 * NTFS_FAILED_ACCESS_ACE_FLAG is only used with system audit and alarm ACE types
 * to indicate that a message is generated (in Windows!) for failed accesses.
 */
enum {
    NTFS_OBJECT_INHERIT_ACE         = 0x01,             ///< 对象继承 (文件继承)
    NTFS_CONTAINER_INHERIT_ACE      = 0x02,             ///< 容器继承 (子目录继承)
    NTFS_NO_PROPAGATE_INHERIT_ACE   = 0x04,             ///< 不向更深层传播
    NTFS_INHERIT_ONLY_ACE           = 0x08,             ///< 仅继承, 不作用于当前对象
    NTFS_INHERITED_ACE              = 0x10,             ///< 已继承而来
    NTFS_VALID_INHERIT_FLAGS        = 0x1f,             ///< 合法继承标志掩码
    NTFS_SUCCESSFUL_ACCESS_ACE_FLAG = 0x40,             ///< 审计成功访问
    NTFS_FAILED_ACCESS_ACE_FLAG     = 0x80,             ///< 审计失败访问
};

/*
 * enum - NTFS access rights masks (32-bit)
 *
 * NTFS_FILE_READ_DATA / NTFS_FILE_LIST_DIRECTORY:  Read file data / list dir contents.
 * NTFS_FILE_WRITE_DATA / NTFS_FILE_ADD_FILE:       Write file data / create file in dir.
 * NTFS_FILE_APPEND_DATA / NTFS_FILE_ADD_SUBDIRECTORY: Append data / create subdir.
 * NTFS_FILE_READ_EA:                          Read extended attributes.
 * NTFS_FILE_WRITE_EA:                         Write extended attributes.
 * NTFS_FILE_EXECUTE / NTFS_FILE_TRAVERSE:          Execute file / traverse dir.
 * NTFS_FILE_DELETE_CHILD:                     Delete children in dir.
 * NTFS_FILE_READ_ATTRIBUTES:                  Read attributes.
 * NTFS_FILE_WRITE_ATTRIBUTES:                 Write attributes.
 *
 * Standard rights (object-independent):
 * NTFS_DELETE:                                Delete object.
 * NTFS_READ_CONTROL:                          Read security descriptor/owner.
 * NTFS_WRITE_DAC:                             Modify DACL.
 * NTFS_WRITE_OWNER:                           Change owner.
 * NTFS_SYNCHRONIZE:                           Wait on object signal state.
 *
 * Combinations:
 * NTFS_STANDARD_RIGHTS_READ / WRITE / EXECUTE: Aliases for NTFS_READ_CONTROL.
 * NTFS_STANDARD_RIGHTS_REQUIRED:              NTFS_DELETE + NTFS_READ_CONTROL +
 *                                        NTFS_WRITE_DAC + NTFS_WRITE_OWNER.
 * NTFS_STANDARD_RIGHTS_ALL:                   Above + NTFS_SYNCHRONIZE.
 *
 * System/access types:
 * NTFS_ACCESS_SYSTEM_SECURITY:                Access system ACL.
 * NTFS_MAXIMUM_ALLOWED:                       Maximum allowed access.
 *
 * Generic rights (high bits, map to specific/standard):
 * NTFS_GENERIC_ALL:                           Full access.
 * NTFS_GENERIC_EXECUTE:                       Execute/traverse.
 * NTFS_GENERIC_WRITE:                         Write (append, attrs, data, EA, etc.).
 * NTFS_GENERIC_READ:                          Read (attrs, data, EA, etc.).
 *
 * The specific rights (bits 0 to 15).  These depend on the type of the object
 * being secured by the ACE.
 */
enum {
    NTFS_FILE_READ_DATA            = pfs_cpu_to_le32(0x00000001), ///< 读文件数据
    NTFS_FILE_LIST_DIRECTORY       = pfs_cpu_to_le32(0x00000001), ///< 枚举目录
    NTFS_FILE_WRITE_DATA           = pfs_cpu_to_le32(0x00000002), ///< 写文件数据
    NTFS_FILE_ADD_FILE             = pfs_cpu_to_le32(0x00000002), ///< 在目录中建文件
    NTFS_FILE_APPEND_DATA          = pfs_cpu_to_le32(0x00000004), ///< 追加数据
    NTFS_FILE_ADD_SUBDIRECTORY     = pfs_cpu_to_le32(0x00000004), ///< 建子目录
    NTFS_FILE_READ_EA              = pfs_cpu_to_le32(0x00000008), ///< 读扩展属性
    NTFS_FILE_WRITE_EA             = pfs_cpu_to_le32(0x00000010), ///< 写扩展属性
    NTFS_FILE_EXECUTE              = pfs_cpu_to_le32(0x00000020), ///< 执行文件
    NTFS_FILE_TRAVERSE             = pfs_cpu_to_le32(0x00000020), ///< 遍历目录
    NTFS_FILE_DELETE_CHILD         = pfs_cpu_to_le32(0x00000040), ///< 删除子项
    NTFS_FILE_READ_ATTRIBUTES      = pfs_cpu_to_le32(0x00000080), ///< 读属性
    NTFS_FILE_WRITE_ATTRIBUTES     = pfs_cpu_to_le32(0x00000100), ///< 写属性
    NTFS_DELETE                    = pfs_cpu_to_le32(0x00010000), ///< 删除对象
    NTFS_READ_CONTROL              = pfs_cpu_to_le32(0x00020000), ///< 读安全描述符
    NTFS_WRITE_DAC                 = pfs_cpu_to_le32(0x00040000), ///< 改 DACL
    NTFS_WRITE_OWNER               = pfs_cpu_to_le32(0x00080000), ///< 改所有者
    NTFS_SYNCHRONIZE               = pfs_cpu_to_le32(0x00100000), ///< 同步等待
    NTFS_STANDARD_RIGHTS_READ      = pfs_cpu_to_le32(0x00020000), ///< 标准读 (= READ_CONTROL)
    NTFS_STANDARD_RIGHTS_WRITE     = pfs_cpu_to_le32(0x00020000), ///< 标准写
    NTFS_STANDARD_RIGHTS_EXECUTE   = pfs_cpu_to_le32(0x00020000), ///< 标准执行
    NTFS_STANDARD_RIGHTS_REQUIRED  = pfs_cpu_to_le32(0x000f0000), ///< 标准必需集合
    NTFS_STANDARD_RIGHTS_ALL       = pfs_cpu_to_le32(0x001f0000), ///< 标准全部 (+ SYNCHRONIZE)
    NTFS_ACCESS_SYSTEM_SECURITY    = pfs_cpu_to_le32(0x01000000), ///< 访问系统 ACL
    NTFS_MAXIMUM_ALLOWED           = pfs_cpu_to_le32(0x02000000), ///< 最大允许
    NTFS_GENERIC_ALL               = pfs_cpu_to_le32(0x10000000), ///< 通用全部
    NTFS_GENERIC_EXECUTE           = pfs_cpu_to_le32(0x20000000), ///< 通用执行
    NTFS_GENERIC_WRITE             = pfs_cpu_to_le32(0x40000000), ///< 通用写
    NTFS_GENERIC_READ              = pfs_cpu_to_le32(0x80000000), ///< 通用读
};

/*
 * ntfs_ace_t - Access Control Entry (ACE) structure
 *
 * @type: ACE type (NTFS_ACCESS_ALLOWED_ACE_TYPE, NTFS_ACCESS_DENIED_ACE_TYPE, etc.).
 * @flags: Inheritance and audit flags (NTFS_OBJECT_INHERIT_ACE, etc.).
 * @size: Total byte size of this ACE (header + SID + variable data).
 * @mask: Access rights mask (NTFS_FILE_READ_DATA, NTFS_DELETE, NTFS_GENERIC_ALL, etc.).
 * @sid: Security Identifier (SID) this ACE applies to.
 */
typedef struct _ntfs_ace_t {
    uint8_t    type;                                    ///< ACE 类型 (ALLOW/DENY 等)
    uint8_t    flags;                                   ///< 继承/审计标志
    uint16_t   size;                                    ///< 本 ACE 总字节数
    uint32_t   mask;                                    ///< 访问权限掩码
    ntfs_sid_t sid;                                     ///< 适用的 SID
} ntfs_ace_t;

/*
 * The object ACE flags (32-bit).
 */
enum {
    NTFS_ACE_OBJECT_TYPE_PRESENT           = pfs_cpu_to_le32(1), ///< 含 ObjectType GUID
    NTFS_ACE_INHERITED_OBJECT_TYPE_PRESENT = pfs_cpu_to_le32(2), ///< 含 InheritedObjectType GUID
};

/*
 * ntfs_acl_t - NTFS Access Control List (ACL) header
 *
 * An ACL is an access-control list (ACL).
 * An ACL starts with an ACL header structure, which specifies the size of
 * the ACL and the number of ACEs it contains. The ACL header is followed by
 * zero or more access control entries (ACEs). The ACL as well as each ACE
 * are aligned on 4-byte boundaries.
 *
 * @revision:           ACL revision level (usually 2 or 4).
 * @alignment1:         Padding/alignment byte (zero).
 * @size:               Total allocated size in bytes (header + all ACEs +
 *                      free space).
 * @ace_count:          Number of ACE entries following the header.
 * @alignment2:         Padding/alignment (zero).
 */
typedef struct _ntfs_acl_t {
    uint8_t  revision;                                  ///< ACL 修订号 (常 2 或 4)
    uint8_t  alignment1;                                ///< 对齐填充
    uint16_t size;                                      ///< ACL 总分配字节数
    uint16_t ace_count;                                 ///< ACE 个数
    uint16_t alignment2;                                ///< 对齐填充
} ntfs_acl_t;

typedef char pfs_ntfs_sa_5[(sizeof(ntfs_acl_t) == 8) ? 1 : -1];

/*
 * The security descriptor control flags (16-bit).
 *
 * NTFS_SE_OWNER_DEFAULTED - This boolean flag, when set, indicates that the SID
 *    pointed to by the Owner field was provided by a defaulting mechanism
 *    rather than explicitly provided by the original provider of the
 *    security descriptor.  This may affect the treatment of the SID with
 *    respect to inheritance of an owner.
 *
 * NTFS_SE_GROUP_DEFAULTED - This boolean flag, when set, indicates that the SID in
 *    the Group field was provided by a defaulting mechanism rather than
 *    explicitly provided by the original provider of the security
 *    descriptor.  This may affect the treatment of the SID with respect to
 *    inheritance of a primary group.
 *
 * NTFS_SE_DACL_PRESENT - This boolean flag, when set, indicates that the security
 *    descriptor contains a discretionary ACL.  If this flag is set and the
 *    Dacl field of the SECURITY_DESCRIPTOR is null, then a null ACL is
 *    explicitly being specified.
 *
 * NTFS_SE_DACL_DEFAULTED - This boolean flag, when set, indicates that the ACL
 *    pointed to by the Dacl field was provided by a defaulting mechanism
 *    rather than explicitly provided by the original provider of the
 *    security descriptor.  This may affect the treatment of the ACL with
 *    respect to inheritance of an ACL.  This flag is ignored if the
 *    DaclPresent flag is not set.
 *
 * NTFS_SE_SACL_PRESENT - This boolean flag, when set,  indicates that the security
 *    descriptor contains a system ACL pointed to by the Sacl field.  If this
 *    flag is set and the Sacl field of the SECURITY_DESCRIPTOR is null, then
 *    an empty (but present) ACL is being specified.
 *
 * NTFS_SE_SACL_DEFAULTED - This boolean flag, when set, indicates that the ACL
 *    pointed to by the Sacl field was provided by a defaulting mechanism
 *    rather than explicitly provided by the original provider of the
 *    security descriptor.  This may affect the treatment of the ACL with
 *    respect to inheritance of an ACL.  This flag is ignored if the
 *    SaclPresent flag is not set.
 *
 * NTFS_SE_SELF_RELATIVE - This boolean flag, when set, indicates that the security
 *    descriptor is in self-relative form.  In this form, all fields of the
 *    security descriptor are contiguous in memory and all pointer fields are
 *    expressed as offsets from the beginning of the security descriptor.
 */
enum {
    NTFS_SE_OWNER_DEFAULTED       = pfs_cpu_to_le16(0x0001), ///< Owner 来自默认
    NTFS_SE_GROUP_DEFAULTED       = pfs_cpu_to_le16(0x0002), ///< Group 来自默认
    NTFS_SE_DACL_PRESENT          = pfs_cpu_to_le16(0x0004), ///< 含 DACL
    NTFS_SE_DACL_DEFAULTED        = pfs_cpu_to_le16(0x0008), ///< DACL 来自默认

    NTFS_SE_SACL_PRESENT          = pfs_cpu_to_le16(0x0010), ///< 含 SACL
    NTFS_SE_SACL_DEFAULTED        = pfs_cpu_to_le16(0x0020), ///< SACL 来自默认

    NTFS_SE_DACL_AUTO_INHERIT_REQ = pfs_cpu_to_le16(0x0100), ///< 请求 DACL 自动继承
    NTFS_SE_SACL_AUTO_INHERIT_REQ = pfs_cpu_to_le16(0x0200), ///< 请求 SACL 自动继承
    NTFS_SE_DACL_AUTO_INHERITED   = pfs_cpu_to_le16(0x0400), ///< DACL 已自动继承
    NTFS_SE_SACL_AUTO_INHERITED   = pfs_cpu_to_le16(0x0800), ///< SACL 已自动继承

    NTFS_SE_DACL_PROTECTED        = pfs_cpu_to_le16(0x1000), ///< DACL 受保护
    NTFS_SE_SACL_PROTECTED        = pfs_cpu_to_le16(0x2000), ///< SACL 受保护
    NTFS_SE_RM_CONTROL_VALID      = pfs_cpu_to_le16(0x4000), ///< 资源管理器控制有效
    NTFS_SE_SELF_RELATIVE         = pfs_cpu_to_le16(0x8000)  ///< 自相对形式
};

/*
 * ntfs_security_descriptor_relative_t - Relative security descriptor
 *
 * Self-relative security descriptor. Contains the owner and group SIDs as well
 * as the sacl and dacl ACLs inside the security descriptor itself.
 *
 * @revision:          Security descriptor revision (usually 1).
 * @alignment:         Padding/alignment byte (zero).
 * @control:           Control flags (NTFS_SE_OWNER_DEFAULTED, NTFS_SE_DACL_PRESENT,
 *                     NTFS_SE_SACL_PRESENT, NTFS_SE_SACL_AUTO_INHERITED, etc.).
 * @owner:             Byte offset to owner SID (from start of descriptor).
 *                     0 if no owner SID present.
 * @group:             Byte offset to primary group SID.
 *                     0 if no group SID present.
 * @sacl:              Byte offset to System ACL (SACL).
 *                     Valid only if NTFS_SE_SACL_PRESENT in @control.
 *                     0 means NULL SACL.
 * @dacl:              Byte offset to Discretionary ACL (DACL).
 *                     Valid only if NTFS_SE_DACL_PRESENT in @control.
 *                     0 means NULL DACL (full access granted).
 */
typedef struct _ntfs_security_descriptor_relative_t {
    uint8_t  revision;                                  ///< 安全描述符修订号 (常为 1)
    uint8_t  alignment;                                 ///< 对齐填充
    uint16_t control;                                   ///< NTFS_SE_* 控制标志
    uint32_t owner;                                     ///< 所有者 SID 相对偏移 (0=无)
    uint32_t group;                                     ///< 主组 SID 相对偏移 (0=无)
    uint32_t sacl;                                      ///< SACL 相对偏移 (须 SE_SACL_PRESENT)
    uint32_t dacl;                                      ///< DACL 相对偏移 (须 SE_DACL_PRESENT)
} ntfs_security_descriptor_relative_t;

typedef char pfs_ntfs_sa_6[(sizeof(ntfs_security_descriptor_relative_t) == 20) ? 1 : -1];

/*
 * On NTFS 3.0+, all security descriptors are stored in NTFS_FILE_Secure. Only one
 * referenced instance of each unique security descriptor is stored.
 *
 * NTFS_FILE_Secure contains no unnamed data attribute, i.e. it has zero length. It
 * does, however, contain two indexes ($SDH and $SII) as well as a named data
 * stream ($SDS).
 *
 * Every unique security descriptor is assigned a unique security identifier
 * (security_id, not to be confused with a SID). The security_id is unique for
 * the NTFS volume and is used as an index into the $SII index, which maps
 * security_ids to the security descriptor's storage location within the $SDS
 * data attribute. The $SII index is sorted by ascending security_id.
 *
 * A simple hash is computed from each security descriptor. This hash is used
 * as an index into the $SDH index, which maps security descriptor hashes to
 * the security descriptor's storage location within the $SDS data attribute.
 * The $SDH index is sorted by security descriptor hash and is stored in a B+
 * tree. When searching $SDH (with the intent of determining whether or not a
 * new security descriptor is already present in the $SDS data stream), if a
 * matching hash is found, but the security descriptors do not match, the
 * search in the $SDH index is continued, searching for a next matching hash.
 *
 * When a precise match is found, the security_id coresponding to the security
 * descriptor in the $SDS attribute is read from the found $SDH index entry and
 * is stored in the $STANDARD_INFORMATION attribute of the file/directory to
 * which the security descriptor is being applied. The $STANDARD_INFORMATION
 * attribute is present in all base mft records (i.e. in all files and
 * directories).
 *
 * If a match is not found, the security descriptor is assigned a new unique
 * security_id and is added to the $SDS data attribute. Then, entries
 * referencing the this security descriptor in the $SDS data attribute are
 * added to the $SDH and $SII indexes.
 *
 * Note: Entries are never deleted from NTFS_FILE_Secure, even if nothing
 * references an entry any more.
 */

/*
 * ntfs_sii_index_key_t - Key for $SII index in $Secure file
 *
 * The index entry key used in the $SII index. The collation type is
 * NTFS_COLLATION_NTOFS_ULONG.
 *
 * @security_id:    32-bit security identifier.
 *                  Unique ID assigned to a security descriptor.
 */
typedef struct _ntfs_sii_index_key_t {
    uint32_t security_id;                               ///< 安全描述符 ID ($SII 键)
} ntfs_sii_index_key_t;

/*
 * ntfs_sdh_index_key_t - Key for $SDH index in $Secure file
 *
 * The index entry key used in the $SDH index. The keys are sorted first by
 * hash and then by security_id. The collation rule is
 * NTFS_COLLATION_NTOFS_SECURITY_HASH.
 *
 * @hash:           32-bit hash of the security descriptor.
 *                  Used for quick collision checks and indexing.
 * @security_id:    32-bit security identifier.
 *                  Unique ID assigned to the descriptor.
 */
typedef struct _ntfs_sdh_index_key_t {
    uint32_t hash;                                      ///< 安全描述符哈希
    uint32_t security_id;                               ///< 安全描述符 ID
} ntfs_sdh_index_key_t;

/*
 * enum - NTFS volume flags (16-bit)
 *
 * These flags are stored in $VolumeInformation attribute.
 * They indicate volume state and required actions.
 *
 * NTFS_VOLUME_IS_DIRTY:                Volume is dirty (needs chkdsk).
 * NTFS_VOLUME_RESIZE_LOG_FILE:         Resize LogFile on next mount.
 * NTFS_VOLUME_UPGRADE_ON_MOUNT:        Upgrade volume on mount (old NTFS).
 * NTFS_VOLUME_MOUNTED_ON_NT4:          Mounted on NT4 (compatibility flag).
 * NTFS_VOLUME_DELETE_USN_UNDERWAY:     USN journal deletion in progress.
 * NTFS_VOLUME_REPAIR_OBJECT_ID:        Repair $ObjId on next mount.
 * NTFS_VOLUME_CHKDSK_UNDERWAY:         Chkdsk is running.
 * NTFS_VOLUME_MODIFIED_BY_CHKDSK:      Modified by chkdsk.
 * NTFS_VOLUME_FLAGS_MASK:              Mask of all valid flags (0xc03f).
 * NTFS_VOLUME_MUST_MOUNT_RO_MASK:      Flags forcing read-only mount (0xc027).
 *                                 If any set, mount read-only.
 */
enum {
    NTFS_VOLUME_IS_DIRTY            = pfs_cpu_to_le16(0x0001), ///< 卷脏
    NTFS_VOLUME_RESIZE_LOG_FILE     = pfs_cpu_to_le16(0x0002), ///< 下次挂载调整 LogFile
    NTFS_VOLUME_UPGRADE_ON_MOUNT    = pfs_cpu_to_le16(0x0004), ///< 挂载时升级
    NTFS_VOLUME_MOUNTED_ON_NT4      = pfs_cpu_to_le16(0x0008), ///< 曾挂于 NT4

    NTFS_VOLUME_DELETE_USN_UNDERWAY = pfs_cpu_to_le16(0x0010), ///< 正在删 USN
    NTFS_VOLUME_REPAIR_OBJECT_ID    = pfs_cpu_to_le16(0x0020), ///< 下次修复 $ObjId

    NTFS_VOLUME_CHKDSK_UNDERWAY     = pfs_cpu_to_le16(0x4000), ///< chkdsk 进行中
    NTFS_VOLUME_MODIFIED_BY_CHKDSK  = pfs_cpu_to_le16(0x8000), ///< 已被 chkdsk 修改

    NTFS_VOLUME_FLAGS_MASK          = pfs_cpu_to_le16(0xc03f), ///< 合法卷标志掩码

    NTFS_VOLUME_MUST_MOUNT_RO_MASK  = pfs_cpu_to_le16(0xc027), ///< 任一置位则须只读挂载
};

/*
 * ntfs_volume_information_t - $VOLUME_INFORMATION (0x70)
 *
 * @reserved:       Reserved 64-bit field (currently unused).
 * @major_ver:      Major NTFS version number (e.g., 3 for NTFS 3.1).
 * @minor_ver:      Minor NTFS version number (e.g., 1 for NTFS 3.1).
 * @flags:          Volume flags (NTFS_VOLUME_IS_DIRTY, NTFS_VOLUME_CHKDSK_UNDERWAY, etc.).
 *                  See volume flags enum for details.
 *
 * NOTE: Always resident.
 * NOTE: Present only in NTFS_FILE_Volume.
 * NOTE: Windows 2000 uses NTFS 3.0 while Windows NT4 service pack 6a uses
 *     NTFS 1.2. I haven't personally seen other values yet.
 */
typedef struct _ntfs_volume_information_t {
    uint64_t reserved;                                  ///< 保留
    uint8_t  major_ver;                                 ///< NTFS 主版本 (如 3)
    uint8_t  minor_ver;                                 ///< NTFS 次版本 (如 1)
    uint16_t flags;                                     ///< NTFS_VOLUME_* 卷标志
} ntfs_volume_information_t;

/*
 * enum - Index header flags
 *
 * These flags are stored in the index header (INDEX_HEADER.flags) for both
 * index root ($INDEX_ROOT) and index allocation blocks ($INDEX_ALLOCATION).
 *
 * For index root ($INDEX_ROOT attribute):
 * NTFS_SMALL_INDEX: Index fits entirely in root attribute (no $INDEX_ALLOCATION).
 * NTFS_LARGE_INDEX: Index too large for root; $INDEX_ALLOCATION present.
 *
 * For index blocks ($INDEX_ALLOCATION):
 * NTFS_LEAF_NODE:   Leaf node (no child nodes; contains actual entries).
 * NTFS_INDEX_NODE:  Internal node (indexes other nodes; contains keys/pointers).
 *
 * NTFS_NODE_MASK:   Mask to extract node type bits (0x01).
 */
enum {
    NTFS_SMALL_INDEX = 0,                               ///< 根索引: 仅在 INDEX_ROOT 内
    NTFS_LARGE_INDEX = 1,                               ///< 根索引: 有 INDEX_ALLOCATION
    NTFS_LEAF_NODE   = 0,                               ///< 索引块: 叶节点
    NTFS_INDEX_NODE  = 1,                               ///< 索引块: 内部节点
    NTFS_NODE_MASK   = 1,                               ///< 节点类型掩码
};

/*
 * ntfs_index_header_t - Common header for index root and index blocks
 *
 * entries_offset:     Byte offset to first INDEX_ENTRY (8-byte aligned).
 * index_length:       Bytes used by index entries (8-byte aligned).
 *                     From entries_offset to end of used data.
 * allocated_size:     Total allocated bytes for this index block.
 *                     Fixed size in index allocation; dynamic in root.
 * flags:              Index flags (NTFS_SMALL_INDEX, NTFS_LARGE_INDEX, NTFS_LEAF_NODE, etc.).
 *                     See INDEX_HEADER_FLAGS enum.
 * reserved:           3 bytes reserved/padding (zero, 8-byte aligned).
 *
 * This is the header for indexes, describing the INDEX_ENTRY records, which
 * follow the index_header. Together the index header and the index entries
 * make up a complete index.
 *
 * IMPORTANT NOTE: The offset, length and size structure members are counted
 * relative to the start of the index header structure and not relative to the
 * start of the index root or index allocation structures themselves.
 *
 * For the index root attribute, the above two numbers are always
 * equal, as the attribute is resident and it is resized as needed. In
 * the case of the index allocation attribute the attribute is not
 * resident and hence the allocated_size is a fixed value and must
 * equal the index_block_size specified by the INDEX_ROOT attribute
 * corresponding to the INDEX_ALLOCATION attribute this INDEX_BLOCK
 * belongs to.
 */
typedef struct _ntfs_index_header_t {
    uint32_t entries_offset;                            ///< 首 INDEX_ENTRY 相对本头的偏移
    uint32_t index_length;                              ///< 已用索引数据字节数
    uint32_t allocated_size;                            ///< 本索引区分配字节数
    uint8_t  flags;                                     ///< SMALL/LARGE 或 LEAF/NODE
    uint8_t  reserved[3];                               ///< 对齐保留
} ntfs_index_header_t;

/*
 * ntfs_index_root_t - $INDEX_ROOT attribute (0x90).
 *
 * @type:               Indexed attribute type ($FILE_NAME for dirs,
 *                      0 for view indexes).
 * @collation_rule:     Collation rule for sorting entries
 *                      (NTFS_COLLATION_FILE_NAME for $FILE_NAME).
 * @index_block_size:   Size of each index block in bytes
 *                      (in $INDEX_ALLOCATION).
 * @clusters_per_index_block:
 *                      Clusters per index block (or log2(bytes)
 *                      if < cluster).
 *                      Power of 2; used for encoding block size.
 * @reserved:           3 bytes reserved/alignment (zero).
 * @index:              Index header for root entries (entries follow
 *                      immediately).
 *
 * NOTE: Always resident.
 *
 * This is followed by a sequence of index entries (INDEX_ENTRY structures)
 * as described by the index header.
 *
 * When a directory is small enough to fit inside the index root then this
 * is the only attribute describing the directory. When the directory is too
 * large to fit in the index root, on the other hand, two additional attributes
 * are present: an index allocation attribute, containing sub-nodes of the B+
 * directory tree (see below), and a bitmap attribute, describing which virtual
 * cluster numbers (vcns) in the index allocation attribute are in use by an
 * index block.
 *
 * NOTE: The root directory (NTFS_FILE_root) contains an entry for itself. Other
 * directories do not contain entries for themselves, though.
 */
typedef struct _ntfs_index_root_t {
    uint32_t            type;                           ///< 被索引属性类型 ($FILE_NAME 等)
    uint32_t            collation_rule;                 ///< 排序规则 (NTFS_COLLATION_*)
    uint32_t            index_block_size;               ///< 索引块字节大小
    uint8_t             clusters_per_index_block;       ///< 每块簇数编码 (或 log2 字节)
    uint8_t             reserved[3];                    ///< 对齐保留
    ntfs_index_header_t index;                          ///< 根内索引头 (后接 INDEX_ENTRY)
} ntfs_index_root_t;

/*
 * ntfs_index_block_t - Index allocation (0xa0).
 *
 * @magic:              Magic value "INDX" (see NTFS_MAGIC_INDX).
 * @usa_ofs:            Offset to Update Sequence Array (see ntfs_record).
 * @usa_count:          Number of USA entries (see ntfs_record).
 * @lsn:                Log sequence number of last modification.
 * @index_block_vcn:    VCN of this index block.
 *                      Units: clusters if cluster_size <= index_block_size;
 *                      sectors otherwise.
 * @index:              Index header describing entries in this block.
 *
 * When creating the index block, we place the update sequence array at this
 * offset, i.e. before we start with the index entries. This also makes sense,
 * otherwise we could run into problems with the update sequence array
 * containing in itself the last two bytes of a sector which would mean that
 * multi sector transfer protection wouldn't work. As you can't protect data
 * by overwriting it since you then can't get it back...
 * When reading use the data from the ntfs record header.
 *
 * NOTE: Always non-resident (doesn't make sense to be resident anyway!).
 *
 * This is an array of index blocks. Each index block starts with an
 * index_block structure containing an index header, followed by a sequence of
 * index entries (INDEX_ENTRY structures), as described by the ntfs_index_header_t.
 */
typedef struct _ntfs_index_block_t {
    uint32_t            magic;                          ///< 魔数 "INDX"
    uint16_t            usa_ofs;                        ///< USA 字节偏移
    uint16_t            usa_count;                      ///< USA 项数
    uint64_t            lsn;                            ///< 最后修改 LSN
    uint64_t            index_block_vcn;                ///< 本索引块 VCN
    ntfs_index_header_t index;                          ///< 块内索引头 (后接 INDEX_ENTRY)
} ntfs_index_block_t;

typedef char pfs_ntfs_sa_7[(sizeof(ntfs_index_block_t) == 40) ? 1 : -1];

/*
 * ntfs_reparse_index_key_t - Key for $R reparse index in $Extend/$Reparse
 *
 * @reparse_tag:    Reparse point type (including flags, NTFS_IO_REPARSE_TAG_*).
 * @file_id:        MFT record number of the file with $REPARSE_POINT
 *                  attribute.
 *
 * The system file NTFS_FILE_Extend/$Reparse contains an index named $R listing
 * all reparse points on the volume. The index entry keys are as defined
 * below. Note, that there is no index data associated with the index entries.
 *
 * The index entries are sorted by the index key file_id. The collation rule is
 * NTFS_COLLATION_NTOFS_ULONGS.
 */
typedef struct _ntfs_reparse_index_key_t {
    uint32_t reparse_tag;                               ///< 重解析点标签
    uint64_t file_id;                                   ///< 含 $REPARSE_POINT 的文件 MFT 号
} ntfs_reparse_index_key_t;

/*
 * enum - Quota entry flags (32-bit) in $Quota/$Q
 *
 * These flags are stored in quota control entries ($Quota file).
 * They control quota tracking, limits, and state.
 *
 * User quota flags (mask 0x00000007):
 * @NTFS_QUOTA_FLAG_DEFAULT_LIMITS:   Use default limits.
 * @NTFS_QUOTA_FLAG_LIMIT_REACHED:    Quota limit reached.
 * @NTFS_QUOTA_FLAG_ID_DELETED:       Quota ID deleted.
 * @NTFS_QUOTA_FLAG_USER_MASK:        Mask for user quota flags (0x00000007).
 *
 * Default entry flags (owner_id = NTFS_QUOTA_DEFAULTS_ID):
 * @NTFS_QUOTA_FLAG_TRACKING_ENABLED:    Quota tracking enabled.
 * @NTFS_QUOTA_FLAG_ENFORCEMENT_ENABLED: Quota enforcement enabled.
 * @NTFS_QUOTA_FLAG_TRACKING_REQUESTED:  Tracking requested (pending).
 * @NTFS_QUOTA_FLAG_LOG_THRESHOLD:       Log when threshold reached.
 * @NTFS_QUOTA_FLAG_LOG_LIMIT:           Log when limit reached.
 * @NTFS_QUOTA_FLAG_OUT_OF_DATE:         Quota data out of date.
 * @NTFS_QUOTA_FLAG_CORRUPT:             Quota entry corrupt.
 * @NTFS_QUOTA_FLAG_PENDING_DELETES:     Pending quota deletes.
 *
 */
enum {
    NTFS_QUOTA_FLAG_DEFAULT_LIMITS      = pfs_cpu_to_le32(0x00000001), ///< 使用默认限额
    NTFS_QUOTA_FLAG_LIMIT_REACHED       = pfs_cpu_to_le32(0x00000002), ///< 已达限额
    NTFS_QUOTA_FLAG_ID_DELETED          = pfs_cpu_to_le32(0x00000004), ///< 配额 ID 已删

    NTFS_QUOTA_FLAG_USER_MASK           = pfs_cpu_to_le32(0x00000007), ///< 用户配额标志掩码
    NTFS_QUOTA_FLAG_TRACKING_ENABLED    = pfs_cpu_to_le32(0x00000010), ///< 启用跟踪
    NTFS_QUOTA_FLAG_ENFORCEMENT_ENABLED = pfs_cpu_to_le32(0x00000020), ///< 启用强制
    NTFS_QUOTA_FLAG_TRACKING_REQUESTED  = pfs_cpu_to_le32(0x00000040), ///< 已请求跟踪
    NTFS_QUOTA_FLAG_LOG_THRESHOLD       = pfs_cpu_to_le32(0x00000080), ///< 达阈值记日志

    NTFS_QUOTA_FLAG_LOG_LIMIT           = pfs_cpu_to_le32(0x00000100), ///< 达限额记日志
    NTFS_QUOTA_FLAG_OUT_OF_DATE         = pfs_cpu_to_le32(0x00000200), ///< 数据过期
    NTFS_QUOTA_FLAG_CORRUPT             = pfs_cpu_to_le32(0x00000400), ///< 项损坏
    NTFS_QUOTA_FLAG_PENDING_DELETES     = pfs_cpu_to_le32(0x00000800), ///< 有待删项
};

/*
 * ntfs_quota_control_entry_t - Quota entry in $Quota/$Q
 *
 * @version:        Currently 2.
 * @flags:          Quota flags (NTFS_QUOTA_FLAG_* bits).
 * @bytes_used:     Current quota usage in bytes.
 * @change_time:    Last modification time (NTFS timestamp).
 * @threshold:      Soft quota limit (-1 = unlimited).
 * @limit:          Hard quota limit (-1 = unlimited).
 * @exceeded_time:  Time soft quota has been exceeded.
 * @sid:            SID of user/object (zero for defaults entry).
 *
 * The system file NTFS_FILE_Extend/$Quota contains two indexes $O and $Q. Quotas
 * are on a per volume and per user basis.
 *
 * The $Q index contains one entry for each existing user_id on the volume. The
 * index key is the user_id of the user/group owning this quota control entry,
 * i.e. the key is the owner_id. The user_id of the owner of a file, i.e. the
 * owner_id, is found in the standard information attribute. The collation rule
 * for $Q is NTFS_COLLATION_NTOFS_ULONG.
 *
 * The $O index contains one entry for each user/group who has been assigned
 * a quota on that volume. The index key holds the SID of the user_id the
 * entry belongs to, i.e. the owner_id. The collation rule for $O is
 * NTFS_COLLATION_NTOFS_SID.
 *
 * The $O index entry data is the user_id of the user corresponding to the SID.
 * This user_id is used as an index into $Q to find the quota control entry
 * associated with the SID.
 *
 * The $Q index entry data is the quota control entry and is defined below.
 */
typedef struct _ntfs_quota_control_entry_t {
    uint32_t   version;                                 ///< 配额项版本 (当前为 2)
    uint32_t   flags;                                   ///< NTFS_QUOTA_FLAG_* 
    uint64_t   bytes_used;                              ///< 已用配额字节
    uint64_t   change_time;                             ///< 最后修改时间
    uint64_t   threshold;                               ///< 软限额 (-1=不限)
    uint64_t   limit;                                   ///< 硬限额 (-1=不限)
    uint64_t   exceeded_time;                           ///< 超额持续时间
    ntfs_sid_t sid;                                     ///< 用户/对象 SID (默认项可为 0)
} ntfs_quota_control_entry_t;

/*
 * Predefined owner_id values (32-bit).
 */
enum {
    NTFS_QUOTA_INVALID_ID    = pfs_cpu_to_le32(0x00000000), ///< 无效 owner_id
    NTFS_QUOTA_DEFAULTS_ID   = pfs_cpu_to_le32(0x00000001), ///< 默认配额项
    NTFS_QUOTA_FIRST_USER_ID = pfs_cpu_to_le32(0x00000100), ///< 首个用户配额 ID
};

/*
 * Current constants for quota control entries.
 */
enum {
    NTFS_QUOTA_VERSION = 2,                             ///< 配额控制项当前版本
};

/*
 * enum - Index entry flags (16-bit)
 *
 * These flags are in INDEX_ENTRY.flags (after key data).
 * They describe entry type and status in index blocks/root.
 *
 * @NTFS_INDEX_ENTRY_NODE:     Entry points to a sub-node (index block VCN).
 *                        (Not a leaf entry; internal node reference.)
 *                        i.e. a reference to an index block in form of
 *                        a virtual cluster number
 * @NTFS_INDEX_ENTRY_END:      Last entry in index block/root.
 *                        Does not represent a real file; can point to sub-node.
 * @NTFS_INDEX_ENTRY_SPACE_FILLER:
 *                        Dummy value to force enum to 16-bit width.
 */
enum {
    NTFS_INDEX_ENTRY_NODE         = pfs_cpu_to_le16(1), ///< 指向子节点 VCN
    NTFS_INDEX_ENTRY_END          = pfs_cpu_to_le16(2), ///< 本块/根末项
    NTFS_INDEX_ENTRY_SPACE_FILLER = pfs_cpu_to_le16(0xffff), ///< 占位使枚举为 16 bit
};

/*
 * ntfs_index_entry_header_t - Common header for all NTFS index entries
 *
 * This is the fixed header at the start of every INDEX_ENTRY in index
 * blocks or index root. It is followed by the variable key, data, and
 * sub-node VCN.
 *
 * Union @data:
 * - When NTFS_INDEX_ENTRY_END is not set:
 *   @data.dir.indexed_file: MFT reference of the file described by
 *                           this entry. Used in directory indexes ($I30).
 * - When NTFS_INDEX_ENTRY_END is set or for view indexes:
 *   @data.vi.data_offset: Byte offset from end of this header to
 *                         entry data.
 *   @data.vi.data_length: Length of data in bytes.
 *   @data.vi.reservedV:   Reserved (zero).
 *
 * @length:         Total byte size of this index entry
 *                  (multiple of 8 bytes).
 * @key_length:     Byte size of the key (not multiple of 8 bytes).
 *                  Key follows the header immediately.
 * @flags:          Bit field of NTFS_INDEX_ENTRY_* flags (NTFS_INDEX_ENTRY_NODE, etc.).
 * @reserved:       Reserved/padding (zero; align to 8 bytes).
 */
typedef struct _ntfs_index_entry_header_t {
    union {
        struct {
            uint64_t indexed_file;                      ///< 目录索引: 目标文件 MFT 引用
        } dir;
        struct {
            uint16_t data_offset;                       ///< 视图索引: 数据相对偏移
            uint16_t data_length;                       ///< 数据字节长度
            uint32_t reservedV;                         ///< 保留
        } vi;
    } data;
    uint16_t length;                                    ///< 本 INDEX_ENTRY 总字节数 (8 对齐)
    uint16_t key_length;                                ///< 键字节长度
    uint16_t flags;                                     ///< NTFS_INDEX_ENTRY_* 
    uint16_t reserved;                                  ///< 对齐保留
} ntfs_index_entry_header_t;

typedef char pfs_ntfs_sa_8[(sizeof(ntfs_index_entry_header_t) == 16) ? 1 : -1];

/*
 * ntfs_index_entry_t - NTFS index entry structure
 *
 * This is an index entry. A sequence of such entries follows each index_header
 * structure. Together they make up a complete index. The index follows either
 * an index root attribute or an index allocation attribute.
 *
 * Union @data (valid when NTFS_INDEX_ENTRY_END not set):
 * @data.dir.indexed_file: MFT ref of file (for directory indexes).
 * @data.vi.data_offset:   Offset to data after key.
 * @data.vi.data_length:   Length of data in bytes.
 * @data.vi.reservedV:     Reserved (zero).
 *
 * Fields:
 * @length:         Total byte size of entry (multiple of 8 bytes).
 * @key_length:     Byte size of key (not multiple of 8).
 * @flags:          NTFS_INDEX_ENTRY_* flags (NODE, END, etc.).
 * @reserved:       Reserved/padding (zero).
 *
 * Union @key (valid when NTFS_INDEX_ENTRY_END not set)
 * The key of the indexed attribute. NOTE: Only present
 * if NTFS_INDEX_ENTRY_END bit in flags is not set. NOTE: On
 * NTFS versions before 3.0 the only valid key is the
 * ntfs_file_name_attr_t. On NTFS 3.0+ the following
 * additional index keys are defined:
 * @key.file_name:  $FILE_NAME attr (for $I30 directory indexes).
 * @key.sii:        $SII key (for $Secure $SII index).
 * @key.sdh:        $SDH key (for $Secure $SDH index).
 * @key.object_id:  GUID (for $ObjId $O index).
 * @key.reparse:    Reparse tag + file ID (for $Reparse $R).
 * @key.sid:        SID (for $Quota $O index).
 * @key.owner_id:   User ID (for $Quota $Q index).
 *
 * The (optional) index data is inserted here when creating.
 * uint64_t vcn;     If NTFS_INDEX_ENTRY_NODE bit in flags is set, the last
 *                 eight bytes of this index entry contain the virtual
 *                 cluster number of the index block that holds the
 *                 entries immediately preceding the current entry (the
 *                 vcn references the corresponding cluster in the data
 *                 of the non-resident index allocation attribute). If
 *                 the key_length is zero, then the vcn immediately
 *                 follows the INDEX_ENTRY_HEADER. Regardless of
 *                 key_length, the address of the 8-byte boundary
 *                 aligned vcn of INDEX_ENTRY{_HEADER} *ie is given by
 *                 (char*)ie + pfs_le16_to_cpu(ie*)->length) - sizeof(VCN),
 *                 where sizeof(VCN) can be hardcoded as 8 if wanted.
 *
 * NOTE: Before NTFS 3.0 only filename attributes were indexed.
 */
typedef struct _ntfs_index_entry_t {
    union {
        struct {
            uint64_t indexed_file;                      ///< 目录索引: 目标文件 MFT 引用
        } dir;
        struct {
            uint16_t data_offset;                       ///< 视图索引: 数据相对偏移
            uint16_t data_length;                       ///< 数据字节长度
            uint32_t reservedV;                         ///< 保留
        } vi;
    } data;
    uint16_t length;                                    ///< 本项总字节数 (8 对齐)
    uint16_t key_length;                                ///< 键字节长度
    uint16_t flags;                                     ///< NTFS_INDEX_ENTRY_* 
    uint16_t reserved;                                  ///< 对齐保留
    union {
        ntfs_file_name_attr_t    file_name;             ///< $I30: $FILE_NAME 键
        ntfs_sii_index_key_t     sii;                   ///< $Secure $SII 键
        ntfs_sdh_index_key_t     sdh;                   ///< $Secure $SDH 键
        ntfs_guid_t              object_id;             ///< $ObjId $O 键
        ntfs_reparse_index_key_t reparse;               ///< $Reparse $R 键
        ntfs_sid_t               sid;                   ///< $Quota $O 键
        uint32_t                 owner_id;              ///< $Quota $Q 键
    } key;
} ntfs_index_entry_t;

/*
 * The reparse point tag defines the type of the reparse point. It also
 * includes several flags, which further describe the reparse point.
 *
 * The reparse point tag is an unsigned 32-bit value divided in three parts:
 *
 * 1. The least significant 16 bits (i.e. bits 0 to 15) specify the type of
 *    the reparse point.
 * 2. The 12 bits after this (i.e. bits 16 to 27) are reserved for future use.
 * 3. The most significant four bits are flags describing the reparse point.
 *    They are defined as follows:
 *    bit 28: Directory bit. If set, the directory is not a surrogate
 *        and can be used the usual way.
 *    bit 29: Name surrogate bit. If set, the filename is an alias for
 *        another object in the system.
 *    bit 30: High-latency bit. If set, accessing the first byte of data will
 *        be slow. (E.g. the data is stored on a tape drive.)
 *    bit 31: Microsoft bit. If set, the tag is owned by Microsoft. User
 *        defined tags have to use zero here.
 * 4. Moreover, on Windows 10 :
 *    Some flags may be used in bits 12 to 15 to further describe the
 *    reparse point.
 */
enum {
    NTFS_IO_REPARSE_TAG_DIRECTORY       = pfs_cpu_to_le32(0x10000000), ///< 目录位
    NTFS_IO_REPARSE_TAG_IS_ALIAS        = pfs_cpu_to_le32(0x20000000), ///< 名称替身
    NTFS_IO_REPARSE_TAG_IS_HIGH_LATENCY = pfs_cpu_to_le32(0x40000000), ///< 高延迟
    NTFS_IO_REPARSE_TAG_IS_MICROSOFT    = pfs_cpu_to_le32(0x80000000), ///< Microsoft 标签

    NTFS_IO_REPARSE_TAG_RESERVED_ZERO   = pfs_cpu_to_le32(0x00000000), ///< 保留 0
    NTFS_IO_REPARSE_TAG_RESERVED_ONE    = pfs_cpu_to_le32(0x00000001), ///< 保留 1
    NTFS_IO_REPARSE_TAG_RESERVED_RANGE  = pfs_cpu_to_le32(0x00000001), ///< 保留范围

    NTFS_IO_REPARSE_TAG_CSV             = pfs_cpu_to_le32(0x80000009), ///< CSV
    NTFS_IO_REPARSE_TAG_DEDUP           = pfs_cpu_to_le32(0x80000013), ///< 重复数据删除
    NTFS_IO_REPARSE_TAG_DFS             = pfs_cpu_to_le32(0x8000000A), ///< DFS
    NTFS_IO_REPARSE_TAG_DFSR            = pfs_cpu_to_le32(0x80000012), ///< DFSR
    NTFS_IO_REPARSE_TAG_HSM             = pfs_cpu_to_le32(0xC0000004), ///< HSM
    NTFS_IO_REPARSE_TAG_HSM2            = pfs_cpu_to_le32(0x80000006), ///< HSM2
    NTFS_IO_REPARSE_TAG_MOUNT_POINT     = pfs_cpu_to_le32(0xA0000003), ///< 挂载点
    NTFS_IO_REPARSE_TAG_NFS             = pfs_cpu_to_le32(0x80000014), ///< NFS
    NTFS_IO_REPARSE_TAG_SIS             = pfs_cpu_to_le32(0x80000007), ///< SIS
    NTFS_IO_REPARSE_TAG_SYMLINK         = pfs_cpu_to_le32(0xA000000C), ///< 符号链接
    NTFS_IO_REPARSE_TAG_WIM             = pfs_cpu_to_le32(0x80000008), ///< WIM
    NTFS_IO_REPARSE_TAG_DFM             = pfs_cpu_to_le32(0x80000016), ///< DFM
    NTFS_IO_REPARSE_TAG_WOF             = pfs_cpu_to_le32(0x80000017), ///< WOF
    NTFS_IO_REPARSE_TAG_WCI             = pfs_cpu_to_le32(0x80000018), ///< WCI
    NTFS_IO_REPARSE_TAG_CLOUD           = pfs_cpu_to_le32(0x9000001A), ///< Cloud
    NTFS_IO_REPARSE_TAG_APPEXECLINK     = pfs_cpu_to_le32(0x8000001B), ///< AppExecLink
    NTFS_IO_REPARSE_TAG_GVFS            = pfs_cpu_to_le32(0x9000001C), ///< GVFS
    NTFS_IO_REPARSE_TAG_LX_SYMLINK      = pfs_cpu_to_le32(0xA000001D), ///< WSL 符号链接
    NTFS_IO_REPARSE_TAG_AF_UNIX         = pfs_cpu_to_le32(0x80000023), ///< AF_UNIX
    NTFS_IO_REPARSE_TAG_LX_FIFO         = pfs_cpu_to_le32(0x80000024), ///< WSL FIFO
    NTFS_IO_REPARSE_TAG_LX_CHR          = pfs_cpu_to_le32(0x80000025), ///< WSL 字符设备
    NTFS_IO_REPARSE_TAG_LX_BLK          = pfs_cpu_to_le32(0x80000026), ///< WSL 块设备

    NTFS_IO_REPARSE_TAG_VALID_VALUES    = pfs_cpu_to_le32(0xf000ffff), ///< 合法标签掩码
    NTFS_IO_REPARSE_PLUGIN_SELECT       = pfs_cpu_to_le32(0xffff0fff), ///< 插件选择掩码
};

/*
 * ntfs_reparse_point_t - $REPARSE_POINT attribute content (0xc0)\
 *
 * @reparse_tag:        Reparse point type (with flags; NTFS_IO_REPARSE_TAG_*).
 * @reparse_data_length: Byte size of @reparse_data.
 * @reserved:           Reserved/padding (zero; 8-byte alignment).
 * @reparse_data:       Variable reparse data (meaning depends on @reparse_tag).
 *                      - Symbolic link/junction: tag-specific reparse data (no dedicated typedef in layout.h)
 *                      - Mount point: similar symlink layout
 *                      - Other tags: vendor-specific or extended data
 *
 * NOTE: Can be resident or non-resident.
 */
typedef struct _ntfs_reparse_point_t {
    uint32_t reparse_tag;                               ///< 重解析标签 (NTFS_IO_REPARSE_TAG_*)
    uint16_t reparse_data_length;                       ///< reparse_data 字节长度
    uint16_t reserved;                                  ///< 对齐保留
    uint8_t  reparse_data[];                            ///< 可变长重解析数据
} ntfs_reparse_point_t;

/*
 * ntfs_ea_information_t - $EA_INFORMATION attribute content (0xd0)
 *
 * @ea_length:      Byte size of packed EAs.
 * @need_ea_count:  Number of EAs with NTFS_NEED_EA bit set.
 * @ea_query_length: Byte size needed to unpack/query EAs via ZwQueryEaFile().
 *                   (Unpacked format size.)
 *
 * NOTE: Always resident. (Is this true???)
 */
typedef struct _ntfs_ea_information_t {
    uint16_t ea_length;                                 ///< 打包 EA 总字节数
    uint16_t need_ea_count;                             ///< 带 NEED_EA 的 EA 个数
    uint32_t ea_query_length;                           ///< 解包查询所需字节数
} ntfs_ea_information_t;

/*
 * enum - Extended attribute flags (8-bit)
 *
 * These flags are stored in the EA header of each extended attribute
 * (in $EA attribute, type 0xe0).
 *
 * @NTFS_NEED_EA:        If set, the file cannot be properly interpreted
 *                  without understanding its associated EAs.
 *                  (Critical EA; applications must process it.)
 */
enum {
    NTFS_NEED_EA = 0x80                                 ///< 关键 EA, 不解则无法正确解释文件
};

/*
 * ntfs_ea_attr_t - Extended attribute (EA) entry (0xe0)
 *
 * @next_entry_offset:  Byte offset to the next EA_ATTR entry.
 *                      (From start of current entry.)
 * @flags:              EA flags (NTFS_NEED_EA = 0x80 if critical).
 * @ea_name_length:     Length of @ea_name in bytes (excluding '\0').
 * @ea_value_length:    Byte size of the EA value.
 * @ea_name:            ASCII name of the EA (zero-terminated).
 *                      Value immediately follows the name.
 * uint8_t ea_value[];       The value of the EA.  Immediately follows the name.
 *
 * This is one variable-length record in the $EA attribute value.
 * The attribute can be resident or non-resident.
 * Sequence of these entries forms the packed EA list.
 *
 * NOTE: Can be resident or non-resident.
 */
typedef struct _ntfs_ea_attr_t {
    uint32_t next_entry_offset;                         ///< 下一项相对本项起始的偏移
    uint8_t  flags;                                     ///< EA 标志 (如 NTFS_NEED_EA)
    uint8_t  ea_name_length;                            ///< 名长度 (不含结尾 0)
    uint16_t ea_value_length;                           ///< 值字节长度
    uint8_t  ea_name[];                                 ///< ASCII 名 (零结尾; 值紧随其后)
} ntfs_ea_attr_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_NTFS_RAW_H_

// end
