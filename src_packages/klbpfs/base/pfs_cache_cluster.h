// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_cache_cluster.h
/// @brief   簇高热缓存
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_CACHE_CLUSTER_H_
#define _PFS_CACHE_CLUSTER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// 公共簇数据缓存 (按簇号 LRU; slab 对齐分配)
/// fat / exfat 均可嵌入; 持有借用 p_blkio; 由调用方传入簇字节偏移, 经 pfs_blkio_pread 加载
/// 返回值数值对齐 pfs_ret_e (0 / -5 EIO / -12 ENOMEM / -16 EBUSY / -22 EINVAL / -34 ERANGE)

#define PFS_CACHE_CLUSTER_PAGE_4K               4096u     ///< [单位:字节] 页大小 / 对齐粒度
#define PFS_CACHE_CLUSTER_SLOTS_PER_SLAB        8u        ///< 每 slab 容纳簇槽数
#define PFS_CACHE_CLUSTER_MAX_SLOTS             64u       ///< 总簇槽上限
#define PFS_CACHE_CLUSTER_MAX_SLAB \
            ((PFS_CACHE_CLUSTER_MAX_SLOTS + PFS_CACHE_CLUSTER_SLOTS_PER_SLAB - 1u) / PFS_CACHE_CLUSTER_SLOTS_PER_SLAB)

typedef struct _pfs_blkio_t pfs_blkio_t;

/// 单槽
typedef struct _pfs_cache_cluster_slot_t {
    bool                valid;                            ///< 是否有效
    uint32_t            tick_count;                       ///< 时间戳计数

    uint32_t            cluster_id;                       ///< [单位:簇] 簇号
    int64_t             cluster_offset;                   ///< [单位:字节] 簇相对设备偏移

    uint8_t             *p_data;                          ///< 单槽数据; 指向所属 slab 内对齐偏移
} pfs_cache_cluster_slot_t;

/// 簇缓存池 (可嵌入各栈 sbi)
typedef struct _pfs_cache_cluster_t {
    pfs_blkio_t         *p_blkio;                         ///< 借用块设备句柄

    uint32_t            cluster_size;                     ///< [单位:字节] I/O 读长度
    uint32_t            slot_stride;                      ///< [单位:字节] 单槽容量 (>=4K, 4K 对齐)

    uint32_t            slab_count;                       ///< 已分配 slab 个数
    uint8_t             *p_slab[PFS_CACHE_CLUSTER_MAX_SLAB]; ///< 对齐分配的整块缓冲

    pfs_cache_cluster_slot_t slots[PFS_CACHE_CLUSTER_MAX_SLOTS]; ///< 簇槽元数据表
} pfs_cache_cluster_t;

/// 分配 slab 并绑定槽; 成功 0; -EINVAL / -EBUSY / -ENOMEM
int pfs_cache_cluster_init(pfs_cache_cluster_t *p_cache, pfs_blkio_t *p_blkio, uint32_t cluster_size);

/// 释放 slab; 成功 0; -EINVAL
int pfs_cache_cluster_deinit(pfs_cache_cluster_t *p_cache);

/// 1) 命中 valid+cluster_id; 2) 占无效槽并加载; 3) 替换 tick 最早者并加载; 失败 NULL
/// cluster_offset: 调用方按几何换算的设备字节偏移; 合法性由调用方保证
pfs_cache_cluster_slot_t *pfs_cache_cluster_get(pfs_cache_cluster_t *p_cache, uint32_t cluster_id, int64_t cluster_offset);

/// 使指定簇槽失效 (写盘后调用); 未命中亦 OK
void pfs_cache_cluster_invalidate(pfs_cache_cluster_t *p_cache, uint32_t cluster_id);

/// 使全部槽失效 (不释放 slab)
void pfs_cache_cluster_invalidate_all(pfs_cache_cluster_t *p_cache);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_CACHE_CLUSTER_H_

// end
