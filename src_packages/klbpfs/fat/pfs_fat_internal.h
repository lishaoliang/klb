// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_fat_internal.h
/// @brief   FAT 子模块内部辅助 (对齐 linux-7.1 fs/fat/fat.h)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 仅 pfs_fat_*.c 使用, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_FAT_INTERNAL_H_
#define _PFS_FAT_INTERNAL_H_

#include "fat/pfs_fat.h"
#include "pfs_endian.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define PFS_FAT_SBI_BS(p_sbi)               (&(p_sbi)->bs)

// 单次扇区读缓冲上限 (BPB sector_size 通常 512~4096)
#define PFS_FAT_SEC_BUF_MAX                 4096

static inline bool pfs_fat_sbi_is_mounted(const pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return false;
    }
    // RDONLY / RDWR 均为已挂载
    return (PFS_FAT_UNMOUNTED != p_sbi->mount_state);
}

static inline bool pfs_fat_sbi_is_rdonly(const pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return false;
    }
    return (PFS_FAT_MOUNT_RDONLY == p_sbi->mount_state);
}

static inline bool pfs_fat_sbi_can_write(const pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return false;
    }
    return (PFS_FAT_MOUNT_RDWR == p_sbi->mount_state);
}

static inline bool pfs_fat_is_fat12(const pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return false;
    }
    return (12 == p_sbi->bs.fat_bits);
}

static inline bool pfs_fat_is_fat16(const pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return false;
    }
    return (16 == p_sbi->bs.fat_bits);
}

static inline bool pfs_fat_is_fat32(const pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return false;
    }
    return (32 == p_sbi->bs.fat_bits);
}

// Maximum cluster number for this variant (linux: max_fat)
static inline uint32_t pfs_fat_max_fat(const pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return MAX_FAT12;
    }
    if (pfs_fat_is_fat32(p_sbi))
    {
        return MAX_FAT32;
    }
    if (pfs_fat_is_fat16(p_sbi))
    {
        return MAX_FAT16;
    }
    return MAX_FAT12;
}

static inline int64_t pfs_fat_part_byte_offset(const pfs_fat_sbi_t *p_sbi, int64_t vol_byte)
{
    if (NULL == p_sbi)
    {
        return vol_byte;
    }
    return p_sbi->part_offset + vol_byte;
}

static inline int64_t pfs_fat_sector_to_byte(const pfs_fat_sbi_t *p_sbi, int64_t sector)
{
    const pfs_fat_bs_t *p_bs;

    if (NULL == p_sbi)
    {
        return 0;
    }
    p_bs = PFS_FAT_SBI_BS(p_sbi);
    return pfs_fat_part_byte_offset(p_sbi, sector * (int64_t)p_bs->sector_size);
}

// cluster -> volume-relative sector (linux: fat_clus_to_blknr)
static inline int64_t pfs_fat_cluster_to_sector(const pfs_fat_sbi_t *p_sbi, uint32_t clu)
{
    const pfs_fat_bs_t *p_bs;

    if (NULL == p_sbi)
    {
        return 0;
    }
    p_bs = PFS_FAT_SBI_BS(p_sbi);
    return (int64_t)((uint64_t)(clu - FAT_START_ENT) * (uint64_t)p_bs->sec_per_clus) +
           (int64_t)p_bs->data_start;
}

static inline bool pfs_fat_is_valid_cluster(const pfs_fat_sbi_t *p_sbi, uint32_t clu)
{
    const pfs_fat_bs_t *p_bs;

    if (NULL == p_sbi)
    {
        return false;
    }
    p_bs = PFS_FAT_SBI_BS(p_sbi);
    return (clu >= FAT_START_ENT && clu < p_bs->max_cluster);
}

// short-name checksum (linux: fat_checksum)
static inline uint8_t pfs_fat_checksum(const uint8_t *name)
{
    uint8_t s;

    if (NULL == name)
    {
        return 0;
    }
    s = name[0];
    s = (uint8_t)((s << 7) + (s >> 1) + name[1]);
    s = (uint8_t)((s << 7) + (s >> 1) + name[2]);
    s = (uint8_t)((s << 7) + (s >> 1) + name[3]);
    s = (uint8_t)((s << 7) + (s >> 1) + name[4]);
    s = (uint8_t)((s << 7) + (s >> 1) + name[5]);
    s = (uint8_t)((s << 7) + (s >> 1) + name[6]);
    s = (uint8_t)((s << 7) + (s >> 1) + name[7]);
    s = (uint8_t)((s << 7) + (s >> 1) + name[8]);
    s = (uint8_t)((s << 7) + (s >> 1) + name[9]);
    s = (uint8_t)((s << 7) + (s >> 1) + name[10]);
    return s;
}

// first cluster from short dir entry (linux: fat_get_start); de fields host order
static inline uint32_t pfs_fat_get_start(const pfs_fat_sbi_t *p_sbi, const fat_dir_entry_t *p_de)
{
    uint32_t cluster;

    if (NULL == p_de)
    {
        return 0;
    }
    cluster = p_de->start;
    if (NULL != p_sbi && pfs_fat_is_fat32(p_sbi))
    {
        cluster |= ((uint32_t)p_de->starthi << 16);
    }
    return cluster;
}

static inline void pfs_fat_set_start(fat_dir_entry_t *p_de, uint32_t cluster)
{
    if (NULL == p_de)
    {
        return;
    }
    p_de->start = (uint16_t)(cluster & 0xFFFFu);
    p_de->starthi = (uint16_t)((cluster >> 16) & 0xFFFFu);
}

static inline int64_t pfs_fat_cluster_to_byte(const pfs_fat_sbi_t *p_sbi, uint32_t clu)
{
    return pfs_fat_sector_to_byte(p_sbi, pfs_fat_cluster_to_sector(p_sbi, clu));
}

// Win 卷设备要求扇区对齐读; 从任意字节偏移读 len 字节 (len 须落在同一逻辑扇区内)
static inline int pfs_fat_pread_at(pfs_fat_sbi_t *p_sbi, void *dst, int len, int64_t off)
{
    const pfs_fat_bs_t *p_bs;
    uint8_t sec[PFS_FAT_SEC_BUF_MAX];
    uint32_t sector_size;
    int64_t sec_off;
    uint32_t byte_in_sec;
    int rc;

    if (NULL == p_sbi || NULL == dst || len <= 0 || NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }
    p_bs = PFS_FAT_SBI_BS(p_sbi);
    sector_size = p_bs->sector_size;
    if (0 == sector_size || sector_size > PFS_FAT_SEC_BUF_MAX)
    {
        return PFS_FAT_RET_EBADMSG;
    }

    sec_off = off - (off % (int64_t)sector_size);
    byte_in_sec = (uint32_t)(off - sec_off);
    if ((uint32_t)len > sector_size || byte_in_sec + (uint32_t)len > sector_size)
    {
        return PFS_FAT_RET_EINVAL;
    }

    rc = pfs_blkio_pread(p_sbi->p_blkio, sec, (int)sector_size, sec_off);
    if (rc < 0)
    {
        return PFS_FAT_RET_EIO;
    }
    if (rc != (int)sector_size)
    {
        return PFS_FAT_RET_EIO;
    }
    memcpy(dst, sec + byte_in_sec, (size_t)len);
    return PFS_FAT_RET_OK;
}

// Win 卷设备要求扇区对齐写; 从任意字节偏移写 len 字节 (len 须落在同一逻辑扇区内)
static inline int pfs_fat_pwrite_at(pfs_fat_sbi_t *p_sbi, const void *src, int len, int64_t off)
{
    const pfs_fat_bs_t *p_bs;
    uint8_t sec[PFS_FAT_SEC_BUF_MAX];
    uint32_t sector_size;
    int64_t sec_off;
    uint32_t byte_in_sec;
    int rc;

    if (NULL == p_sbi || NULL == src || len <= 0 || NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }
    p_bs = PFS_FAT_SBI_BS(p_sbi);
    sector_size = p_bs->sector_size;
    if (0 == sector_size || sector_size > PFS_FAT_SEC_BUF_MAX)
    {
        return PFS_FAT_RET_EBADMSG;
    }

    sec_off = off - (off % (int64_t)sector_size);
    byte_in_sec = (uint32_t)(off - sec_off);
    if ((uint32_t)len > sector_size || byte_in_sec + (uint32_t)len > sector_size)
    {
        return PFS_FAT_RET_EINVAL;
    }

    rc = pfs_blkio_pread(p_sbi->p_blkio, sec, (int)sector_size, sec_off);
    if (rc < 0 || rc != (int)sector_size)
    {
        return PFS_FAT_RET_EIO;
    }
    memcpy(sec + byte_in_sec, src, (size_t)len);
    rc = pfs_blkio_pwrite(p_sbi->p_blkio, sec, (int)sector_size, sec_off);
    if (rc < 0 || rc != (int)sector_size)
    {
        return PFS_FAT_RET_EIO;
    }
    return PFS_FAT_RET_OK;
}

// --- cache.c --- (目录簇缓存 + FAT16/12 根扇区缓存)
int pfs_fat_dir_cache_init(pfs_cache_cluster_t *p_dir_cache, pfs_fat_sbi_t *p_sbi, uint32_t cluster_size);
int pfs_fat_dir_cache_deinit(pfs_cache_cluster_t *p_dir_cache);
pfs_cache_cluster_slot_t *pfs_fat_dir_cache_get(pfs_cache_cluster_t *p_dir_cache, uint32_t clu_id);
void pfs_fat_dir_cache_invalidate(pfs_cache_cluster_t *p_dir_cache, uint32_t clu_id);
void pfs_fat_dir_cache_invalidate_all(pfs_cache_cluster_t *p_dir_cache);

int pfs_fat_root_sec_cache_init(pfs_cache_sector_t *p_root_sec_cache, pfs_fat_sbi_t *p_sbi);
int pfs_fat_root_sec_cache_deinit(pfs_cache_sector_t *p_root_sec_cache);
pfs_cache_sector_slot_t *pfs_fat_root_sec_cache_get(pfs_cache_sector_t *p_root_sec_cache, uint64_t sector_id);
void pfs_fat_root_sec_cache_invalidate(pfs_cache_sector_t *p_root_sec_cache, uint64_t sector_id);
void pfs_fat_root_sec_cache_invalidate_all(pfs_cache_sector_t *p_root_sec_cache);

// FAT 表扇区缓存 (嵌在 sbi->fat.fat_cache)
int pfs_fat_fat_cache_init(pfs_cache_sector_t *p_fat_cache, pfs_fat_sbi_t *p_sbi);
int pfs_fat_fat_cache_deinit(pfs_cache_sector_t *p_fat_cache);
pfs_cache_sector_slot_t *pfs_fat_fat_cache_get(pfs_cache_sector_t *p_fat_cache, uint64_t sector_id);
void pfs_fat_fat_cache_invalidate(pfs_cache_sector_t *p_fat_cache, uint64_t sector_id);
void pfs_fat_fat_cache_invalidate_all(pfs_cache_sector_t *p_fat_cache);

// --- super.c ---
// boot/BPB 读/解析见 pfs_fat_super.c

// --- mkfs (FAT12/16/32; I/O 助手在 pfs_fat_mkfs32.c) ---
#define PFS_FAT_MKFS_SECTOR_SIZE        512U

uint32_t pfs_fat_mkfs_rand_serial(void);
int pfs_fat_mkfs_map_blkio_rc(int rc);
int pfs_fat_mkfs_write_sector(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint32_t sector,
        const uint8_t *sec);
int pfs_fat_mkfs_zero_sectors(
        pfs_blkio_t *p_blkio,
        int64_t part_offset,
        uint32_t start_sector,
        uint32_t count);

// --- fatent.c ---
int pfs_fat_ent_get(pfs_fat_sbi_t *p_sbi, uint32_t loc, uint32_t *p_content);
int pfs_fat_ent_set(pfs_fat_sbi_t *p_sbi, uint32_t loc, uint32_t content);
int pfs_fat_get_next_cluster(pfs_fat_sbi_t *p_sbi, uint32_t *p_clu);
int pfs_fat_cluster_walk(pfs_fat_sbi_t *p_sbi, uint32_t *p_clu, uint32_t step);
int pfs_fat_alloc_cluster(pfs_fat_sbi_t *p_sbi, uint32_t hint, uint32_t *p_clu);
int pfs_fat_free_cluster_chain(pfs_fat_sbi_t *p_sbi, uint32_t start);
int pfs_fat_zero_cluster(pfs_fat_sbi_t *p_sbi, uint32_t clu);

// --- dir / namei (只读, 对齐现行 exFAT 目录面) ---
#define PFS_FAT_TYPE_DIR                0x0104
#define PFS_FAT_TYPE_FILE               0x011F

typedef struct _pfs_fat_dir_entry_t {
    uint32_t            type;                           ///< PFS_FAT_TYPE_*
    uint32_t            start_clu;                      ///< [单位:簇] 数据起始; 固定根可为 0
    uint16_t            attr;                           ///< FAT_ATTR_*
    uint32_t            size;                           ///< [单位:字节]
    int                 is_root;                        ///< 1=卷根
    int                 is_fixed_root;                  ///< 1=FAT12/16 固定根扇区区
    uint32_t            parent_clu;                     ///< [单位:簇] 父目录起始; 定位写回
    int                 parent_fixed_root;              ///< 1=父为 FAT12/16 固定根
    int64_t             sfn_cpos;                       ///< [单位:字节] 短名项在父目录内偏移; -1=未知
} pfs_fat_dir_entry_t;

int pfs_fat_utf16_to_utf8(const uint16_t *uniname, int max_uni, char *out, int out_cap);
int pfs_fat_readdir(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        int64_t *p_cpos, pfs_fat_dir_entry_t *p_de, char *utf8_name, int utf8_cap);
int pfs_fat_find_dir_entry(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        const char *name, pfs_fat_dir_entry_t *p_info);
// 写 32 字节目录项 (on-disk 小端布局); 写后刷盘并 invalidate 缓存
int pfs_fat_dir_write_raw(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        int64_t cpos, const void *p_raw32);
// 读 32 字节目录项 (on-disk 小端, 不转主机序)
int pfs_fat_dir_read_raw(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        int64_t cpos, void *p_raw32);
void pfs_fat_fill_root_info(pfs_fat_sbi_t *p_sbi, pfs_fat_dir_entry_t *p_info);
int pfs_fat_lookup_path(pfs_fat_sbi_t *p_sbi, const char *path, pfs_fat_dir_entry_t *p_info);
// 在 path 建空普通文件 (已存在返回 EEXIST); 不扩展目录, 无空槽则 ENOSPC
int pfs_fat_create_file(pfs_fat_sbi_t *p_sbi, const char *path, pfs_fat_dir_entry_t *p_info);
int pfs_fat_mkdir(pfs_fat_sbi_t *p_sbi, const char *path);
int pfs_fat_unlink(pfs_fat_sbi_t *p_sbi, const char *path);
int pfs_fat_rmdir(pfs_fat_sbi_t *p_sbi, const char *path);
int pfs_fat_rename(pfs_fat_sbi_t *p_sbi, const char *oldpath, const char *newpath);
// 回写短名项 size/start (sfn_cpos 须有效)
int pfs_fat_update_sfn_meta(pfs_fat_sbi_t *p_sbi, const pfs_fat_dir_entry_t *p_loc,
        uint32_t start_clu, uint32_t size);

// --- file.c --- (只读; 对齐 linux-7.1 fat_bmap / fat_get_cluster)
typedef struct _pfs_fat_file_meta_t {
    uint32_t            start_clu;                      ///< [单位:簇] 数据起始; 空文件可为 0
    int64_t             size;                           ///< [单位:字节] 逻辑文件大小
    uint32_t            hint_clu_off;                   ///< [单位:簇] 提示: 文件内簇偏移
    uint32_t            hint_clu;                       ///< [单位:簇] 提示: 对应盘簇
} pfs_fat_file_meta_t;

int pfs_fat_map_cluster(pfs_fat_sbi_t *p_sbi, pfs_fat_file_meta_t *p_meta,
        uint32_t clu_off, uint32_t *p_clu);
int pfs_fat_file_read(pfs_fat_sbi_t *p_sbi, pfs_fat_file_meta_t *p_meta,
        int64_t pos, void *buf, int len, int *p_got);
int pfs_fat_file_write(pfs_fat_sbi_t *p_sbi, pfs_fat_file_meta_t *p_meta,
        int64_t pos, const void *buf, int len, int *p_put);
int pfs_fat_file_truncate(pfs_fat_sbi_t *p_sbi, pfs_fat_file_meta_t *p_meta, int64_t new_size);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_FAT_INTERNAL_H_

// end
