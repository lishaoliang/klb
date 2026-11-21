// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_udf_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_UDF_RAW_H_
#define _PFS_UDF_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_UDF_VSD_STD_ID_LEN          5

#define PFS_UDF_TAG_IDENT_PVD           0x0001
#define PFS_UDF_TAG_IDENT_AVDP          0x0002
#define PFS_UDF_TAG_IDENT_VDP           0x0003
#define PFS_UDF_TAG_IDENT_IUVD          0x0004
#define PFS_UDF_TAG_IDENT_PD            0x0005
#define PFS_UDF_TAG_IDENT_LVD           0x0006
#define PFS_UDF_TAG_IDENT_USD           0x0007
#define PFS_UDF_TAG_IDENT_TD            0x0008
#define PFS_UDF_TAG_IDENT_LVID          0x0009
#define PFS_UDF_TAG_IDENT_FSD           0x0100
#define PFS_UDF_TAG_IDENT_FID           0x0101
#define PFS_UDF_TAG_IDENT_AED           0x0102
#define PFS_UDF_TAG_IDENT_IE            0x0103
#define PFS_UDF_TAG_IDENT_TE            0x0104
#define PFS_UDF_TAG_IDENT_FE            0x0105
#define PFS_UDF_TAG_IDENT_EAHD          0x0106
#define PFS_UDF_TAG_IDENT_USE           0x0107
#define PFS_UDF_TAG_IDENT_SBD           0x0108
#define PFS_UDF_TAG_IDENT_PIE           0x0109
#define PFS_UDF_TAG_IDENT_EFE           0x010A

#define PFS_UDF_EXT_LENGTH_MASK         0x3FFFFFFFu
#define PFS_UDF_EXT_TYPE_MASK           0xC0000000u
#define PFS_UDF_EXT_RECORDED_ALLOC      0x00000000u
#define PFS_UDF_EXT_NOTREC_ALLOC        0x40000000u
#define PFS_UDF_EXT_NOTREC_NOTALLOC     0x80000000u
#define PFS_UDF_EXT_NEXT_ALLOCDESCS     0xC0000000u

#define PFS_UDF_PD_ACCESS_RDONLY        0x00000001u
#define PFS_UDF_PD_ACCESS_WRITE_ONCE    0x00000002u
#define PFS_UDF_PD_ACCESS_REWRITABLE    0x00000003u
#define PFS_UDF_PD_ACCESS_OVERWRITABLE  0x00000004u

#define PFS_UDF_ICBTAG_AD_SHORT         0x0000
#define PFS_UDF_ICBTAG_AD_LONG          0x0001
#define PFS_UDF_ICBTAG_AD_EXTENDED      0x0002
#define PFS_UDF_ICBTAG_AD_IN_ICB        0x0003

#define PFS_UDF_ICBTAG_FILE_DIRECTORY   0x04
#define PFS_UDF_ICBTAG_FILE_REGULAR     0x05
#define PFS_UDF_ICBTAG_FILE_SYMLINK     0x0C
#define PFS_UDF_ICBTAG_FILE_VAT20       0xF8
#define PFS_UDF_ICBTAG_FILE_MAIN        0xFA
#define PFS_UDF_ICBTAG_FILE_MIRROR      0xFB

#define PFS_UDF_ID_VIRTUAL              "*UDF Virtual Partition"
#define PFS_UDF_ID_SPARABLE             "*UDF Sparable Partition"
#define PFS_UDF_ID_METADATA             "*UDF Metadata Partition"

#define PFS_UDF_FID_CHAR_HIDDEN         0x01
#define PFS_UDF_FID_CHAR_DIRECTORY      0x02
#define PFS_UDF_FID_CHAR_DELETED        0x04
#define PFS_UDF_FID_CHAR_PARENT         0x08
#define PFS_UDF_FID_CHAR_METADATA       0x10

#define PFS_UDF_GP_PARTMAP_TYPE1        0x01
#define PFS_UDF_GP_PARTMAP_TYPE2        0x02

#pragma pack(push, 1)

typedef struct _pfs_udf_tag_t {
    uint16_t tagIdent;
    uint16_t descVersion;
    uint8_t  tagChecksum;
    uint8_t  reserved;
    uint16_t tagSerialNum;
    uint16_t descCRC;
    uint16_t descCRCLength;
    uint32_t tagLocation;
} pfs_udf_tag_t;

typedef struct _pfs_udf_extent_ad_t {
    uint32_t extLength;
    uint32_t extLocation;
} pfs_udf_extent_ad_t;

typedef struct _pfs_udf_lb_addr_t {
    uint32_t logicalBlockNum;
    uint16_t partitionReferenceNum;
} pfs_udf_lb_addr_t;

typedef struct _pfs_udf_short_ad_t {
    uint32_t extLength;
    uint32_t extPosition;
} pfs_udf_short_ad_t;

typedef struct _pfs_udf_long_ad_t {
    uint32_t extLength;
    pfs_udf_lb_addr_t extLocation;
    uint8_t  impUse[6];
} pfs_udf_long_ad_t;

typedef struct _pfs_udf_ext_ad_t {
    uint32_t extLength;
    uint32_t recordedLength;
    uint32_t informationLength;
    pfs_udf_lb_addr_t extLocation;
} pfs_udf_ext_ad_t;

typedef struct _pfs_udf_regid_t {
    uint8_t flags;
    uint8_t ident[23];
    uint8_t identSuffix[8];
} pfs_udf_regid_t;

typedef struct _pfs_udf_charspec_t {
    uint8_t charSetType;
    uint8_t charSetInfo[63];
} pfs_udf_charspec_t;

typedef struct _pfs_udf_generic_desc_t {
    pfs_udf_tag_t descTag;
    uint32_t volDescSeqNum;
    uint8_t  reserved[492];
} pfs_udf_generic_desc_t;

typedef struct _pfs_udf_anchor_vol_desc_ptr_t {
    pfs_udf_tag_t       descTag;
    pfs_udf_extent_ad_t mainVolDescSeqExt;
    pfs_udf_extent_ad_t reserveVolDescSeqExt;
    uint8_t             reserved[480];
} pfs_udf_anchor_vol_desc_ptr_t;

typedef struct _pfs_udf_primary_vol_desc_t {
    pfs_udf_tag_t       descTag;
    uint32_t            volDescSeqNum;
    uint32_t            primaryVolDescNum;
    uint8_t             volIdent[32];
    uint16_t            volSeqNum;
    uint16_t            maxVolSeqNum;
    uint16_t            interchangeLvl;
    uint16_t            maxInterchangeLvl;
    uint32_t            charSetList;
    uint32_t            maxCharSetList;
    uint8_t             volSetIdent[128];
    pfs_udf_charspec_t  descCharSet;
    pfs_udf_charspec_t  explanatoryCharSet;
    pfs_udf_extent_ad_t volAbstract;
    pfs_udf_extent_ad_t volCopyright;
    pfs_udf_regid_t     appIdent;
    uint8_t             recordingDateAndTime[12];
    pfs_udf_regid_t     impIdent;
    uint8_t             impUse[64];
    uint32_t            predecessorVolDescSeqLocation;
    uint16_t            flags;
    uint8_t             reserved[22];
} pfs_udf_primary_vol_desc_t;

typedef struct _pfs_udf_partition_desc_t {
    pfs_udf_tag_t   descTag;
    uint32_t        volDescSeqNum;
    uint16_t        partitionFlags;
    uint16_t        partitionNumber;
    pfs_udf_regid_t partitionContents;
    uint8_t         partitionContentsUse[128];
    uint32_t        accessType;
    uint32_t        partitionStartingLocation;
    uint32_t        partitionLength;
    pfs_udf_regid_t impIdent;
    uint8_t         impUse[128];
    uint8_t         reserved[156];
} pfs_udf_partition_desc_t;

typedef struct _pfs_udf_partmap1_t {
    uint8_t  partitionMapType;
    uint8_t  partitionMapLength;
    uint16_t volSeqNum;
    uint16_t partitionNum;
} pfs_udf_partmap1_t;

typedef struct _pfs_udf_partmap2_t {
    uint8_t         partitionMapType;
    uint8_t         partitionMapLength;
    uint8_t         reserved1[2];
    pfs_udf_regid_t partIdent;
    uint16_t        volSeqNum;
    uint16_t        partitionNum;
} pfs_udf_partmap2_t;

typedef struct _pfs_udf_metadata_partmap_t {
    uint8_t         partitionMapType;
    uint8_t         partitionMapLength;
    uint8_t         reserved1[2];
    pfs_udf_regid_t partIdent;
    uint16_t        volSeqNum;
    uint16_t        partitionNum;
    uint32_t        metadataFileLoc;
    uint32_t        metadataMirrorFileLoc;
    uint32_t        metadataBitmapFileLoc;
    uint32_t        allocUnitSize;
    uint16_t        alignUnitSize;
    uint8_t         flags;
    uint8_t         reserved2[5];
} pfs_udf_metadata_partmap_t;

typedef struct _pfs_udf_vat20_hdr_t {
    uint16_t lengthHeader;
    uint16_t lengthImpUse;
    uint8_t  logicalVolIdent[128];
    uint32_t previousVATICBLoc;
    uint32_t numFiles;
    uint32_t numDirs;
    uint16_t minUDFReadRev;
    uint16_t minUDFWriteRev;
    uint16_t maxUDFWriteRev;
    uint16_t reserved;
} pfs_udf_vat20_hdr_t;

typedef struct _pfs_udf_logical_vol_desc_t {
    pfs_udf_tag_t       descTag;
    uint32_t            volDescSeqNum;
    pfs_udf_charspec_t  descCharSet;
    uint8_t             logicalVolIdent[128];
    uint32_t            logicalBlockSize;
    pfs_udf_regid_t     domainIdent;
    uint8_t             logicalVolContentsUse[16];
    uint32_t            mapTableLength;
    uint32_t            numPartitionMaps;
    pfs_udf_regid_t     impIdent;
    uint8_t             impUse[128];
    pfs_udf_extent_ad_t integritySeqExt;
    /* uint8_t partitionMaps[]; */
} pfs_udf_logical_vol_desc_t;

typedef struct _pfs_udf_vol_desc_ptr_t {
    pfs_udf_tag_t       descTag;
    uint32_t            volDescSeqNum;
    pfs_udf_extent_ad_t nextVolDescSeqExt;
    uint8_t             reserved[484];
} pfs_udf_vol_desc_ptr_t;

typedef struct _pfs_udf_terminating_desc_t {
    pfs_udf_tag_t descTag;
    uint8_t       reserved[496];
} pfs_udf_terminating_desc_t;

typedef struct _pfs_udf_file_set_desc_t {
    pfs_udf_tag_t       descTag;
    uint8_t             recordingDateAndTime[12];
    uint16_t            interchangeLvl;
    uint16_t            maxInterchangeLvl;
    uint32_t            charSetList;
    uint32_t            maxCharSetList;
    uint32_t            fileSetNum;
    uint32_t            fileSetDescNum;
    pfs_udf_charspec_t  logicalVolIdentCharSet;
    uint8_t             logicalVolIdent[128];
    pfs_udf_charspec_t  fileSetCharSet;
    uint8_t             fileSetIdent[32];
    uint8_t             copyrightFileIdent[32];
    uint8_t             abstractFileIdent[32];
    pfs_udf_long_ad_t   rootDirectoryICB;
    pfs_udf_regid_t     domainIdent;
    pfs_udf_long_ad_t   nextExt;
    pfs_udf_long_ad_t   streamDirectoryICB;
    uint8_t             reserved[32];
} pfs_udf_file_set_desc_t;

typedef struct _pfs_udf_icbtag_t {
    uint32_t           priorRecordedNumDirectEntries;
    uint16_t           strategyType;
    uint16_t           strategyParameter;
    uint16_t           numEntries;
    uint8_t            reserved;
    uint8_t            fileType;
    pfs_udf_lb_addr_t  parentICBLocation;
    uint16_t           flags;
} pfs_udf_icbtag_t;

typedef struct _pfs_udf_file_entry_t {
    pfs_udf_tag_t     descTag;
    pfs_udf_icbtag_t  icbTag;
    uint32_t          uid;
    uint32_t          gid;
    uint32_t          permissions;
    uint16_t          fileLinkCount;
    uint8_t           recordFormat;
    uint8_t           recordDisplayAttr;
    uint32_t          recordLength;
    uint64_t          informationLength;
    uint64_t          logicalBlocksRecorded;
    uint8_t           accessTime[12];
    uint8_t           modificationTime[12];
    uint8_t           attrTime[12];
    uint32_t          checkpoint;
    pfs_udf_long_ad_t extendedAttrICB;
    pfs_udf_regid_t   impIdent;
    uint64_t          uniqueID;
    uint32_t          lengthExtendedAttr;
    uint32_t          lengthAllocDescs;
    /* uint8_t extendedAttr[]; allocDescs[]; */
} pfs_udf_file_entry_t;

typedef struct _pfs_udf_extended_file_entry_t {
    pfs_udf_tag_t     descTag;
    pfs_udf_icbtag_t  icbTag;
    uint32_t          uid;
    uint32_t          gid;
    uint32_t          permissions;
    uint16_t          fileLinkCount;
    uint8_t           recordFormat;
    uint8_t           recordDisplayAttr;
    uint32_t          recordLength;
    uint64_t          informationLength;
    uint64_t          objectSize;
    uint64_t          logicalBlocksRecorded;
    uint8_t           accessTime[12];
    uint8_t           modificationTime[12];
    uint8_t           createTime[12];
    uint8_t           attrTime[12];
    uint32_t          checkpoint;
    uint32_t          reserved;
    pfs_udf_long_ad_t extendedAttrICB;
    pfs_udf_long_ad_t streamDirectoryICB;
    pfs_udf_regid_t   impIdent;
    uint64_t          uniqueID;
    uint32_t          lengthExtendedAttr;
    uint32_t          lengthAllocDescs;
    /* uint8_t extendedAttr[]; allocDescs[]; */
} pfs_udf_extended_file_entry_t;

typedef struct _pfs_udf_file_ident_desc_t {
    pfs_udf_tag_t   descTag;
    uint16_t        fileVersionNum;
    uint8_t         fileCharacteristics;
    uint8_t         lengthFileIdent;
    pfs_udf_long_ad_t icb;
    uint16_t        lengthOfImpUse;
    /* uint8_t impUse[]; fileIdent[]; padding */
} pfs_udf_file_ident_desc_t;

typedef struct _pfs_udf_alloc_ext_desc_t {
    pfs_udf_tag_t descTag;
    uint32_t      previousAllocExtLocation;
    uint32_t      lengthAllocDescs;
} pfs_udf_alloc_ext_desc_t;

typedef struct _pfs_udf_indirect_entry_t {
    pfs_udf_tag_t     descTag;
    pfs_udf_icbtag_t  icbTag;
    pfs_udf_long_ad_t indirectICB;
} pfs_udf_indirect_entry_t;

typedef struct _pfs_udf_partition_header_desc_t {
    pfs_udf_short_ad_t  unallocSpaceTable;
    pfs_udf_short_ad_t  unallocSpaceBitmap;
    pfs_udf_short_ad_t  partitionIntegrityTable;
    pfs_udf_short_ad_t  freedSpaceTable;
    pfs_udf_short_ad_t  freedSpaceBitmap;
    uint8_t             reserved[88];
} pfs_udf_partition_header_desc_t;

typedef struct _pfs_udf_space_bitmap_desc_t {
    pfs_udf_tag_t       descTag;
    uint32_t            numOfBits;
    uint32_t            numOfBytes;
    /* uint8_t bitmap[]; */
} pfs_udf_space_bitmap_desc_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_UDF_RAW_H_

// end
