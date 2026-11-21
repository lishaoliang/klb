// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ntfs.h
/// @brief   NTFS 子模块对外接口 (sbi / mount / mkfs / vfs 对接)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_NTFS_H_
#define _PFS_NTFS_H_

#include "pfs_blkio.h"
#include "ntfs/pfs_ntfs_raw.h"
#include "base/pfs_cache_cluster.h"
#include "base/pfs_fs_ops.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief NTFS API 返回值 (失败时 <0; 勿透传 PFS_BLKIO_RET_*)
/// @note 命名/数值对齐 linux-7.1 -errno (与 pfs_ret_e 同值, vfs 可透传)
typedef enum _pfs_ntfs_ret_e {
    PFS_NTFS_RET_OK            =  0,                    ///< 成功
    PFS_NTFS_RET_EPERM         = -1,                    ///< 只读卷/属性禁止写 (EPERM)
    PFS_NTFS_RET_ENOENT        = -2,                    ///< 路径或目录项不存在 (ENOENT)
    PFS_NTFS_RET_EIO           = -5,                    ///< I/O 失败 (EIO; blkio 层)
    PFS_NTFS_RET_ENOEXEC       = -8,                    ///< boot/super 无效, 非 NTFS (ENOEXEC)
    PFS_NTFS_RET_ENOMEM        = -12,                   ///< 内存不足 (ENOMEM)
    PFS_NTFS_RET_EBUSY         = -16,                   ///< mount 生命周期/状态不符 (EBUSY)
    PFS_NTFS_RET_EEXIST        = -17,                   ///< 目标已存在 (EEXIST)
    PFS_NTFS_RET_ENOTDIR       = -20,                   ///< 路径组件非目录 (ENOTDIR)
    PFS_NTFS_RET_EISDIR        = -21,                   ///< 目标是目录 (EISDIR)
    PFS_NTFS_RET_EINVAL        = -22,                   ///< 参数非法 (EINVAL)
    PFS_NTFS_RET_ENOSPC        = -28,                   ///< 无空闲簇 (ENOSPC)
    PFS_NTFS_RET_EROFS         = -30,                   ///< 只读文件系统 (EROFS)
    PFS_NTFS_RET_ERANGE        = -34,                   ///< 簇号/运行列表越界 (ERANGE)
    PFS_NTFS_RET_ENAMETOOLONG  = -36,                   ///< 文件名超长 (ENAMETOOLONG)
    PFS_NTFS_RET_ENOTEMPTY     = -39,                   ///< 目录非空 (ENOTEMPTY)
    PFS_NTFS_RET_EBADMSG       = -74,                   ///< on-disk 元数据损坏 (EBADMSG)
    PFS_NTFS_RET_EILSEQ        = -84,                   ///< 路径/字符集非法 (EILSEQ)
    PFS_NTFS_RET_EOPNOTSUPP    = -95,                   ///< 未实现或不支持 (EOPNOTSUPP)
} pfs_ntfs_ret_e;

/// @brief mount 模式 (互斥; 仅 pfs_ntfs_sbi_t.mount_state; 非组合 bit)
typedef enum _pfs_ntfs_mount_state_e {
    PFS_NTFS_UNMOUNTED         = 0x00,                  ///< 未挂载
    PFS_NTFS_MOUNT_RDONLY      = 0x01,                  ///< 已挂载, 只读
    PFS_NTFS_MOUNT_RDWR        = 0x02,                  ///< 已挂载, 读写
} pfs_ntfs_mount_state_e;

/// @brief boot 解析结果 (几何对齐 linux-7.1 fs/ntfs volume.h ntfs_volume 子集)
/// @note [单位:扇区]=卷内相对扇区; [单位:字节]; [单位:簇]=簇号或簇个数
///  \n boot 多字节仍为 LE on-disk; 其下派生字段为主机序
typedef struct _pfs_ntfs_bs_t {
    ntfs_boot_sector_t  boot;                           ///< 引导扇区副本 (LE on-disk)

    uint32_t            sector_size;                    ///< [单位:字节] 逻辑扇区大小
    uint32_t            sector_size_bits;               ///< log2(sector_size)
    uint32_t            sectors_per_cluster;            ///< 每簇扇区数
    uint64_t            num_sectors;                    ///< [单位:扇区] 卷扇区总数

    uint32_t            cluster_size;                   ///< [单位:字节] 簇大小
    uint32_t            cluster_size_bits;              ///< log2(cluster_size)
    uint32_t            cluster_size_mask;              ///< cluster_size - 1
    uint64_t            num_clusters;                   ///< [单位:簇] 卷簇总数

    uint32_t            mft_record_size;                ///< [单位:字节] 单条 MFT 记录大小
    uint32_t            mft_record_size_mask;           ///< mft_record_size - 1
    uint32_t            mft_record_size_bits;           ///< log2(mft_record_size)
    uint64_t            mft_lcn;                        ///< [单位:簇] $MFT 起始 LCN
    uint64_t            mft_mirr_lcn;                   ///< [单位:簇] $MFTMirr 起始 LCN
    uint32_t            mft_mirr_size;                  ///< [单位:条] $MFTMirr 镜像记录数

    uint32_t            index_record_size;              ///< [单位:字节] 索引块 (INDX) 大小
    uint32_t            index_record_size_mask;         ///< index_record_size - 1
    uint32_t            index_record_size_bits;         ///< log2(index_record_size)

    uint64_t            serial_no;                      ///< 卷序列号
} pfs_ntfs_bs_t;

// --- 内存运行时 (对齐 linux-7.1 fs/ntfs runlist.h / inode.h 子集; 非 on-disk) ---

/// @brief runlist 特殊 LCN (主机序; 对齐 fs/ntfs/runlist.h)
enum {
    PFS_NTFS_LCN_DELALLOC      = -1,                    ///< 延迟分配
    PFS_NTFS_LCN_HOLE          = -2,                    ///< 稀疏洞 (无物理簇)
    PFS_NTFS_LCN_RL_NOT_MAPPED = -3,                    ///< 段尚未映射
    PFS_NTFS_LCN_ENOENT        = -4,                    ///< 查找越界
    PFS_NTFS_LCN_ENOMEM        = -5,                    ///< 分配失败哨兵
    PFS_NTFS_LCN_EIO           = -6,                    ///< I/O 失败哨兵
    PFS_NTFS_LCN_EINVAL        = -7,                    ///< 参数非法哨兵
};

/// @brief 单段 VCN->LCN 运行 (对齐 runlist_element; length==0 表示数组结束)
typedef struct _pfs_ntfs_run_t {
    int64_t             vcn;                            ///< [单位:簇] 本段起始 VCN
    int64_t             lcn;                            ///< [单位:簇] 本段起始 LCN; 稀疏为 PFS_NTFS_LCN_HOLE
    int64_t             length;                         ///< [单位:簇] 本段长度; 0=终止项
} pfs_ntfs_run_t;

/// @brief 运行列表 (堆上 p_rl[]; 用户态无内核 rw_semaphore)
typedef struct _pfs_ntfs_runlist_t {
    pfs_ntfs_run_t      *p_rl;                          ///< 段数组; 末项 length==0; 空则为 NULL
    uint32_t            count;                          ///< 有效段数 (不含终止项)
    int32_t             hint;                           ///< 最近命中下标; -1=无
} pfs_ntfs_runlist_t;

/// @brief 属性查找上下文 (对齐 ntfs_attr_search_ctx 子集; 不拥有记录缓冲)
typedef struct _pfs_ntfs_attr_ctx_t {
    ntfs_mft_record_t   *p_mrec;                        ///< 当前 MFT 记录 (主机可改写的缓冲)
    ntfs_attr_record_t  *p_attr;                        ///< 当前属性; 未找到可为 NULL
    uint32_t            type;                           ///< 查找类型 (NTFS_AT_* 主机序或与比较约定一致)
    const uint16_t      *p_name;                        ///< UTF-16LE 属性名; 无名则为 NULL
    uint32_t            name_len;                       ///< 属性名 UTF-16 码元个数
    bool                is_first;                       ///< true=尚未开始迭代
} pfs_ntfs_attr_ctx_t;

/// @brief $MFT 运行时 (阶段 B: 引导装载后填充; 对齐 ntfs_inode 用于 FILE_MFT 的子集)
typedef struct _pfs_ntfs_mft_t {
    bool                valid;                          ///< true=已装 $MFT/$DATA runlist
    uint64_t            mft_no;                         ///< 记录号; $MFT 恒为 0
    uint16_t            seq_no;                         ///< 序列号 (来自记录头)

    int64_t             data_size;                      ///< [单位:字节] $DATA data_size
    int64_t             initialized_size;               ///< [单位:字节] $DATA initialized_size
    int64_t             allocated_size;                 ///< [单位:字节] $DATA allocated_size

    pfs_ntfs_runlist_t  runlist;                        ///< $MFT 无名 $DATA 的 VCN->LCN
} pfs_ntfs_mft_t;

/// @def   PFS_NTFS_UPCASE_CHARS
/// @brief UTF-16 码元全表项数 ($UpCase)
#define PFS_NTFS_UPCASE_CHARS           0x10000u        ///< UTF-16 码元全表项数

/// @brief $UpCase 运行时 (对齐 exFAT upcase / linux upcase.c; 全表 0x10000 项)
typedef struct _pfs_ntfs_upcase_t {
    bool                valid;                          ///< true=表已加载可比较文件名
    uint16_t            *p_entries;                     ///< 主机序大写映射表; 长度 entries_count
    uint32_t            entries_count;                  ///< 通常 PFS_NTFS_UPCASE_CHARS
} pfs_ntfs_upcase_t;

/// @brief 目录 $I30 索引运行时 (嵌于 inode; INDEX_ROOT +/- INDEX_ALLOCATION)
typedef struct _pfs_ntfs_index_t {
    bool                valid;                          ///< true=已解析 $I30
    uint32_t            collation_rule;                 ///< 主机序 (如 FILE_NAME)
    uint32_t            block_size;                     ///< [单位:字节] INDX 块大小 (常=bs.index_record_size)
    bool                has_allocation;                 ///< true=存在 $INDEX_ALLOCATION

    uint8_t             *p_root;                        ///< $INDEX_ROOT 值副本 (堆; 可无)
    uint32_t            root_size;                      ///< [单位:字节] p_root 长度

    pfs_ntfs_runlist_t  alloc_runlist;                  ///< $INDEX_ALLOCATION 的 VCN->LCN

    uint32_t            vcn_size;                       ///< [单位:字节] INDX VCN 步长 (簇或扇区)
} pfs_ntfs_index_t;

/// @brief 通用文件/目录 inode (对齐 ntfs_inode 用户态子集; 打开或 mount 根时填充)
typedef struct _pfs_ntfs_inode_t {
    bool                valid;                          ///< true=本 inode 已装入
    uint64_t            mft_no;                         ///< MFT 记录号
    uint16_t            seq_no;                         ///< 序列号
    bool                is_dir;                         ///< true=目录 (含 $I30)

    int64_t             data_size;                      ///< [单位:字节] 无名 $DATA 逻辑大小 (目录常 0)
    int64_t             initialized_size;               ///< [单位:字节] $DATA initialized_size
    int64_t             allocated_size;                 ///< [单位:字节] $DATA allocated_size
    pfs_ntfs_runlist_t  data_runlist;                   ///< 无名 $DATA (读文件); 目录可为空

    pfs_ntfs_index_t    i30;                            ///< 目录 $I30; 文件时 valid=false

    // 写路径脏标记 (对齐 NI_Dirty / RunlistDirty 思路; 读路径保持 false)
    bool                sizes_dirty;                    ///< true=size 三元组待回写属性头
    bool                runlist_dirty;                  ///< true=data_runlist 待重建 mapping pairs

    bool                data_resident;                  ///< true=无名 $DATA 在 p_data_resident
    uint8_t             *p_data_resident;               ///< resident $DATA 副本 (堆)
    uint32_t            data_resident_size;             ///< [单位:字节] p_data_resident 长度
} pfs_ntfs_inode_t;

// --- 写路径运行时 (对齐 linux-7.1 fs/ntfs volume.h / lcnalloc.h / bitmap 子集) ---

/// @brief 簇分配区域 (对齐 ntfs ZONE_*)
typedef enum _pfs_ntfs_alloc_zone_e {
    PFS_NTFS_ZONE_MFT          = 0,                     ///< 优先自 MFT zone 分配
    PFS_NTFS_ZONE_DATA         = 1,                     ///< 数据区分配
} pfs_ntfs_alloc_zone_e;

/// @brief 卷簇空闲位图 $Bitmap (FILE#6; 对齐 vol->lcnbmp_ino + zone 游标)
/// @note bmp_cache: 写路径按需 init; 未 init 时 deinit 安全
typedef struct _pfs_ntfs_lcn_bitmap_t {
    bool                valid;                          ///< true=已装 $Bitmap/$DATA
    uint64_t            mft_no;                         ///< 恒为 FILE_Bitmap (6)
    uint16_t            seq_no;                         ///< 序列号

    int64_t             data_size;                      ///< [单位:字节] 位图逻辑大小
    int64_t             initialized_size;               ///< [单位:字节] initialized_size
    int64_t             allocated_size;                 ///< [单位:字节] allocated_size
    pfs_ntfs_runlist_t  runlist;                        ///< $Bitmap 无名 $DATA

    int64_t             free_clusters;                  ///< 空闲簇缓存; -1=未知
    int64_t             hint_lcn;                       ///< 下次扫位图提示 LCN

    // MFT / Data zone 游标 (主机序 LCN; 对齐 ntfs_volume mft_zone_* / data*_zone_pos)
    int64_t             mft_zone_start;                 ///< MFT zone 起始 LCN
    int64_t             mft_zone_end;                   ///< MFT zone 结束 LCN (不含或按实现约定)
    int64_t             mft_zone_pos;                   ///< MFT zone 分配游标
    int64_t             data_zone_pos;                  ///< Data zone 分配游标

    pfs_cache_cluster_t bmp_cache;                      ///< 位图高热簇缓存 (独立于 sbi.cache)
} pfs_ntfs_lcn_bitmap_t;

/// @brief $MFT 的 $BITMAP 属性运行时 (MFT 记录槽位图; 对齐 vol->mftbmp_ino)
/// @note 小卷常 resident; 大卷走 runlist
typedef struct _pfs_ntfs_mft_bitmap_t {
    bool                valid;                          ///< true=已装 $MFT/$BITMAP
    bool                is_resident;                    ///< true=位图在属性值内 (p_resident)

    int64_t             data_size;                      ///< [单位:字节] 位图逻辑大小
    int64_t             allocated_size;                 ///< [单位:字节] 分配大小 (非常驻有意义)
    pfs_ntfs_runlist_t  runlist;                        ///< 非常驻时 $BITMAP 的 VCN->LCN

    uint8_t             *p_resident;                    ///< resident 位图副本 (堆); 非常驻为 NULL
    uint32_t            resident_size;                  ///< [单位:字节] p_resident 长度

    int64_t             free_mft_records;               ///< 空闲 MFT 记录数; -1=未知
    uint64_t            hint_mft_no;                    ///< 下次优先分配的 MFT 号 (mft_data_pos)
} pfs_ntfs_mft_bitmap_t;

/// @brief 映射中的单条 MFT 记录缓冲 (写回/脏; 对齐 map_mft_record + mark dirty)
typedef struct _pfs_ntfs_mft_rec_t {
    bool                valid;                          ///< true=p_mrec 有效
    bool                dirty;                          ///< true=须 write_mft_record
    uint64_t            mft_no;                         ///< 记录号
    uint16_t            seq_no;                         ///< 序列号 (来自记录头)
    uint8_t             *p_mrec;                        ///< 记录缓冲 (堆; 长度=size)
    uint32_t            size;                           ///< [单位:字节] 通常=bs.mft_record_size
} pfs_ntfs_mft_rec_t;

/// @brief UTF-16 文件名 (lookup / 比较; 对齐 exFAT uni_name 思路)
typedef struct _pfs_ntfs_uni_name_t {
    uint16_t            name[NTFS_MAXIMUM_FILE_NAME_LENGTH]; ///< 主机序 UTF-16 码元
    uint32_t            len;                            ///< 码元个数 (不含 NUL)
} pfs_ntfs_uni_name_t;

/// @brief 创建/建目录暂存 (path 写 API; 非长寿命句柄)
typedef struct _pfs_ntfs_create_t {
    bool                valid;                          ///< true=参数已填
    bool                is_dir;                         ///< true=mkdir; false=creat 文件
    uint64_t            parent_mft_no;                  ///< 父目录 MFT 号
    uint16_t            parent_seq_no;                  ///< 父目录序列号
    pfs_ntfs_uni_name_t name;                           ///< 新名 (UTF-16)
} pfs_ntfs_create_t;

/// @brief namei 查找结果 (路径分量 -> MFT 引用)
typedef struct _pfs_ntfs_lookup_t {
    bool                valid;                          ///< true=已命中
    uint64_t            mft_no;                         ///< 目标记录号
    uint16_t            seq_no;                         ///< 序列号
    bool                is_dir;                         ///< 目标是否为目录
} pfs_ntfs_lookup_t;

/// @brief 打开文件元数据 (对齐 fat/exfat file_meta; 对接 pfs_fopen/fread/fwrite)
typedef struct _pfs_ntfs_file_meta_t {
    bool                valid;                          ///< true=已打开文件
    pfs_ntfs_inode_t    ino;                            ///< 文件 inode ($DATA runlist)
    int64_t             hint_vcn;                       ///< 顺序读提示: 文件内 VCN
    int64_t             hint_lcn;                       ///< 顺序读提示: 对应 LCN (hole 则为特殊 LCN)

    // 父目录定位 (写/截断回写 $FILE_NAME 等; 对齐 vfs 目录项回写)
    bool                parent_valid;                   ///< true=parent_* 有效
    uint64_t            parent_mft_no;                  ///< 父目录 MFT 号
    uint16_t            parent_seq_no;                  ///< 父目录序列号
} pfs_ntfs_file_meta_t;

/// @brief 打开目录元数据 + readdir 游标 (对齐 vfs pfs_dir 枚举需求)
typedef struct _pfs_ntfs_dir_meta_t {
    bool                valid;                          ///< true=已打开目录
    pfs_ntfs_inode_t    ino;                            ///< 目录 inode ($I30)

    bool                in_root;                        ///< true=正在扫 INDEX_ROOT
    int64_t             block_vcn;                      ///< INDEX_ALLOCATION 当前 VCN; 在 root 时无意义
    uint32_t            entry_ofs;                      ///< 当前块/根内 entries 区字节偏移
    uint8_t             *p_block;                       ///< 当前 INDX 块缓冲 (堆); in_root 时为 NULL
    bool                eof;                            ///< true=枚举结束
} pfs_ntfs_dir_meta_t;

/// @brief NTFS superblock info (用户态子集; 对齐 fat/exfat sbi 形态)
typedef struct _pfs_ntfs_sbi_t {
    pfs_ntfs_mount_state_e mount_state;                 ///< UNMOUNTED / MOUNT_RDONLY / MOUNT_RDWR

    pfs_blkio_t         *p_blkio;                       ///< 借用块设备句柄
    int64_t             part_offset;                    ///< [单位:字节] 分区相对设备偏移; 裸卷/整盘为 0

    pfs_ntfs_bs_t       bs;                             ///< boot 解析结果 (mount 时填齐几何)
    pfs_ntfs_mft_t      mft;                            ///< $MFT 运行时 (mount 引导装载)
    pfs_ntfs_upcase_t   upcase;                         ///< $UpCase (lookup/readdir 前须装)
    pfs_ntfs_inode_t    root;                           ///< 根目录 #5 (lookup/readdir 前须装)

    // 写路径 (分配簇 / 分配 MFT; 现行仅结构预留, mount 未装)
    pfs_ntfs_lcn_bitmap_t lcn_bmp;                      ///< 卷 $Bitmap (FILE#6)
    pfs_ntfs_mft_bitmap_t mft_bmp;                      ///< $MFT/$BITMAP

    pfs_cache_cluster_t cache;                          ///< 按簇缓存 (LCN 数据路径)
} pfs_ntfs_sbi_t;

/// @brief 重置为 UNMOUNTED
/// @param [in, out] *p_sbi    NTFS sbi
/// @return int PFS_NTFS_RET_OK 成功; PFS_NTFS_RET_EINVAL 失败
int pfs_ntfs_init(pfs_ntfs_sbi_t *p_sbi);

/// @brief 释放内资源; 不 close p_blkio
/// @param [in, out] *p_sbi    NTFS sbi
void pfs_ntfs_deinit(pfs_ntfs_sbi_t *p_sbi);

/// @brief 读取并解析 boot, 并引导装载 $MFT (填 mft runlist)
/// @param [in, out] *p_sbi        NTFS sbi
/// @param [in]      *p_blkio       须已 open
/// @param [in]      part_offset    分区字节偏移
/// @param [in]      mount_mode     PFS_NTFS_MOUNT_RDONLY 或 PFS_NTFS_MOUNT_RDWR
/// @return int PFS_NTFS_RET_OK 成功; 失败保持 UNMOUNTED
/// @note RDONLY: boot+$MFT+cache+$UpCase+root#5; RDWR 另装 $Bitmap 与 $MFT/$BITMAP; 读/写/path API 已通
int pfs_ntfs_mount(pfs_ntfs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, pfs_ntfs_mount_state_e mount_mode);

/// @brief 已挂载时切换只读/读写; 不重新解析 boot
/// @param [in, out] *p_sbi        NTFS sbi
/// @param [in]      mount_mode     仅 RDONLY/RDWR
/// @return int PFS_NTFS_RET_OK 成功; PFS_NTFS_RET_EBUSY / PFS_NTFS_RET_EINVAL
int pfs_ntfs_remount(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_mount_state_e mount_mode);

/// @brief 卸载; 清除 p_blkio, 置 UNMOUNTED
/// @param [in, out] *p_sbi    NTFS sbi
/// @return int PFS_NTFS_RET_OK 成功; PFS_NTFS_RET_EINVAL / PFS_NTFS_RET_EBUSY
int pfs_ntfs_umount(pfs_ntfs_sbi_t *p_sbi);

/// @brief 仅探测是否为 NTFS; 不修改 p_sbi
/// @param [in] *p_blkio       须已 open
/// @param [in] part_offset    分区字节偏移
/// @return int PFS_NTFS_RET_OK 成功; PFS_NTFS_RET_ENOEXEC 非 NTFS
int pfs_ntfs_probe(pfs_blkio_t *p_blkio, int64_t part_offset);

/// @brief 在 p_blkio 指定分区写入 NTFS 布局 (用户态 mkfs; 最小可探测子集)
/// @param [in] *p_blkio       须已 open
/// @param [in] part_offset    分区字节偏移 (裸卷为 0)
/// @param [in] vol_bytes      卷容量 [单位:字节], 须为 512 扇区整数倍且 >= 1MiB
/// @return int PFS_NTFS_RET_OK 成功; PFS_NTFS_RET_EINVAL / PFS_NTFS_RET_EIO / PFS_NTFS_RET_ENOMEM
int pfs_ntfs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

/// @brief vfs 对接表 (pfs_ntfs_ops.c; 已接线; 能力面演进中)
extern const pfs_fs_ops_t g_pfs_ntfs_fs_ops;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_NTFS_H_

// end
