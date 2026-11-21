// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_cache_sector.h
/// @brief   扇区高热缓存
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_CACHE_SECTOR_H_
#define _PFS_CACHE_SECTOR_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// 公共扇区数据缓存 (按扇区号 LRU; slab 对齐分配)
/// fat / exfat 均可嵌入; 持有借用 p_blkio; 由调用方传入扇区字节偏移, 经 pfs_blkio_pread 加载
/// 返回值数值对齐 pfs_ret_e (0 / -5 EIO / -12 ENOMEM / -16 EBUSY / -22 EINVAL / -34 ERANGE)

#define PFS_CACHE_SECTOR_PAGE_4K                4096u     ///< [单位:字节] 页大小 / 对齐粒度
#define PFS_CACHE_SECTOR_SLOTS_PER_SLAB         8u        ///< 每 slab 容纳扇区槽数
#define PFS_CACHE_SECTOR_MAX_SLOTS              64u       ///< 总扇区槽上限
#define PFS_CACHE_SECTOR_MAX_SLAB \
            ((PFS_CACHE_SECTOR_MAX_SLOTS + PFS_CACHE_SECTOR_SLOTS_PER_SLAB - 1u) \
                / PFS_CACHE_SECTOR_SLOTS_PER_SLAB)

typedef struct _pfs_blkio_t pfs_blkio_t;

/// 单槽
typedef struct _pfs_cache_sector_slot_t {
    bool                valid;                            ///< 是否有效
    uint32_t            tick_count;                       ///< 时间戳计数

    uint64_t            sector_id;                        ///< [单位:扇区] 扇区号
    int64_t             sector_offset;                    ///< [单位:字节] 扇区相对设备偏移

    uint8_t             *p_data;                          ///< 单槽数据; 指向所属 slab 内对齐偏移
} pfs_cache_sector_slot_t;

/// 扇区缓存池 (可嵌入各栈 sbi)
typedef struct _pfs_cache_sector_t {
    pfs_blkio_t         *p_blkio;                         ///< 借用块设备句柄

    uint32_t            sector_size;                      ///< [单位:字节] I/O 读长度
    uint32_t            slot_stride;                      ///< [单位:字节] 单槽容量 (>=4K, 4K 对齐)

    uint32_t            slab_count;                       ///< 已分配 slab 个数
    uint8_t             *p_slab[PFS_CACHE_SECTOR_MAX_SLAB]; ///< 对齐分配的整块缓冲

    pfs_cache_sector_slot_t slots[PFS_CACHE_SECTOR_MAX_SLOTS]; ///< 扇区槽元数据表
} pfs_cache_sector_t;

/// 分配 slab 并绑定槽; 成功 0; -EINVAL / -EBUSY / -ENOMEM
int pfs_cache_sector_init(pfs_cache_sector_t *p_cache, pfs_blkio_t *p_blkio, uint32_t sector_size);

/// 释放 slab; 成功 0; -EINVAL
int pfs_cache_sector_deinit(pfs_cache_sector_t *p_cache);

/// 1) 命中 valid+sector_id; 2) 占无效槽并加载; 3) 替换 tick 最早者并加载; 失败 NULL
/// sector_offset: 调用方按几何换算的设备字节偏移; 合法性由调用方保证
pfs_cache_sector_slot_t *pfs_cache_sector_get(pfs_cache_sector_t *p_cache, uint64_t sector_id, int64_t sector_offset);

/// 使指定扇区槽失效 (写盘后调用); 未命中亦 OK
void pfs_cache_sector_invalidate(pfs_cache_sector_t *p_cache, uint64_t sector_id);

/// 使全部槽失效 (不释放 slab)
void pfs_cache_sector_invalidate_all(pfs_cache_sector_t *p_cache);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_CACHE_SECTOR_H_

// end
