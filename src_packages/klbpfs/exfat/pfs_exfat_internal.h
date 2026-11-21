// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_exfat_internal.h
/// @brief   exFAT 子模块内部辅助 (对齐 linux-7.1 fs/exfat)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 仅 pfs_exfat_*.c 使用, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_EXFAT_INTERNAL_H_
#define _PFS_EXFAT_INTERNAL_H_

#include "pfs_exfat.h"
#include "pfs_endian.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define PFS_EXFAT_SBI_BS(p_sbi)             (&(p_sbi)->bs)

#define PFS_EXFAT_MAX_NAME_LENGTH           EXFAT_MAX_FILE_LEN

#define PFS_EXFAT_ES_IDX_FILE               0
#define PFS_EXFAT_ES_IDX_STREAM             1
#define PFS_EXFAT_ES_IDX_FIRST_FILENAME     2

#define PFS_EXFAT_FILENAME_ENTRY_NUM(name_len) \
            (((name_len) + EXFAT_FILE_NAME_LEN - 1U) / EXFAT_FILE_NAME_LEN)
#define PFS_EXFAT_ES_IDX_LAST_FILENAME(name_len) \
            (PFS_EXFAT_ES_IDX_FIRST_FILENAME + PFS_EXFAT_FILENAME_ENTRY_NUM(name_len) - 1)

// dentry logical type (linux-7.1 exfat_fs.h TYPE_*)
#define PFS_EXFAT_TYPE_UNUSED               0x0000
#define PFS_EXFAT_TYPE_DELETED              0x0001
#define PFS_EXFAT_TYPE_INVALID              0x0002
#define PFS_EXFAT_TYPE_BITMAP               0x0101
#define PFS_EXFAT_TYPE_UPCASE               0x0102
#define PFS_EXFAT_TYPE_VOLUME               0x0103
#define PFS_EXFAT_TYPE_DIR                  0x0104
#define PFS_EXFAT_TYPE_FILE                 0x011F
#define PFS_EXFAT_TYPE_STREAM               0x0201
#define PFS_EXFAT_TYPE_EXTEND               0x0202

#define PFS_EXFAT_MAX_DENTRIES              8388608u
#define PFS_EXFAT_CS_DEFAULT                2

// dentry 索引 <-> 字节偏移 (linux EXFAT_B_TO_DEN / EXFAT_DEN_TO_B)
#define PFS_EXFAT_B_TO_DEN(b)               ((uint32_t)((b) >> EXFAT_DENTRY_SIZE_BITS))
#define PFS_EXFAT_DEN_TO_B(b)               ((uint64_t)(b) << EXFAT_DENTRY_SIZE_BITS)

// dentry 索引 <-> 簇 (linux EXFAT_CLU_TO_DEN / EXFAT_DEN_TO_CLU)
#define PFS_EXFAT_CLU_TO_DEN(clu, p_bs) \
            ((uint32_t)((clu) << ((p_bs)->cluster_size_bits - EXFAT_DENTRY_SIZE_BITS)))
#define PFS_EXFAT_DEN_TO_CLU(dentry, p_bs) \
            ((uint32_t)((dentry) >> ((p_bs)->cluster_size_bits - EXFAT_DENTRY_SIZE_BITS)))

#define PFS_EXFAT_B_TO_CLU(b, p_bs)         ((uint32_t)((b) >> (p_bs)->cluster_size_bits))
#define PFS_EXFAT_B_TO_CLU_ROUND_UP(b, p_bs) \
            ((uint32_t)(((b) + ((uint64_t)(p_bs)->cluster_size - 1ULL)) >> (p_bs)->cluster_size_bits))

// directory chain (linux exfat_chain)
typedef struct _pfs_exfat_chain_t {
    uint32_t            dir;                              ///< [单位:簇] 当前簇号
    uint32_t            size;                             ///< [单位:簇] 链上剩余簇数
    uint8_t             flags;                            ///< stream.flags 子集
} pfs_exfat_chain_t;

// UTF-16 文件名 (linux exfat_uni_name)
typedef struct _pfs_exfat_uni_name_t {
    uint16_t            name[PFS_EXFAT_MAX_NAME_LENGTH + 3];
    uint16_t            name_hash;
    uint8_t             name_len;
} pfs_exfat_uni_name_t;

// 解析后的目录项 (linux exfat_dir_entry 子集)
typedef struct _pfs_exfat_dir_entry_t {
    pfs_exfat_chain_t   dir;                              ///< 所在目录簇链
    int                 entry;                            ///< 目录项索引
    uint32_t            type;                             ///< PFS_EXFAT_TYPE_*
    uint32_t            start_clu;                        ///< [单位:簇] 数据起始簇号
    uint8_t             flags;
    uint16_t            attr;
    int64_t             size;                             ///< [单位:字节] 逻辑大小
    int64_t             valid_size;                       ///< [单位:字节] 有效数据长度
    uint32_t            num_subdirs;
} pfs_exfat_dir_entry_t;


// 是否已经挂载
static inline bool pfs_exfat_sbi_is_mounted(const pfs_exfat_sbi_t *p_sbi)
{
    // RDONLY / RDWR 均为已挂载
    return (PFS_EXFAT_UNMOUNTED != p_sbi->mount_state);
}

// 是否只读
static inline bool pfs_exfat_sbi_is_rdonly(const pfs_exfat_sbi_t *p_sbi)
{
    return (PFS_EXFAT_MOUNT_RDONLY == p_sbi->mount_state);
}

// 是否可写
static inline bool pfs_exfat_sbi_can_write(const pfs_exfat_sbi_t *p_sbi)
{
    return (PFS_EXFAT_MOUNT_RDWR == p_sbi->mount_state);
}

// + 初始偏移 p_sbi->part_offset
static inline int64_t pfs_exfat_part_byte_offset(const pfs_exfat_sbi_t *p_sbi, int64_t vol_byte)
{
    return p_sbi->part_offset + vol_byte;
}

// 扇区 对应的 字节偏移
static inline int64_t pfs_exfat_sector_to_byte(const pfs_exfat_sbi_t *p_sbi, int64_t sector)
{
    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    return pfs_exfat_part_byte_offset(p_sbi, sector << p_bs->boot.sect_size_bits);
}

// 簇号 对应的 扇区
static inline int64_t pfs_exfat_cluster_to_sector(const pfs_exfat_sbi_t *p_sbi, uint32_t clu)
{
    // 需要减去 保留的扇区号(0,1) EXFAT_RESERVED_CLUSTERS
    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    return (int64_t)((uint64_t)(clu - EXFAT_RESERVED_CLUSTERS) << p_bs->sectors_per_clu_bits) + (int64_t)p_bs->data_start_sector;
}

// 簇号 对应的 偏移
static inline int64_t pfs_exfat_cluster_to_byte(const pfs_exfat_sbi_t *p_sbi, uint32_t clu)
{
    int64_t sector = pfs_exfat_cluster_to_sector(p_sbi, clu); ///< 扇区号
    return pfs_exfat_sector_to_byte(p_sbi, sector); ///< 相对偏移字节
}

// 簇号 是否有效
static inline bool pfs_exfat_is_valid_cluster(const pfs_exfat_sbi_t *p_sbi, uint32_t clu)
{
    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    return (clu >= EXFAT_FIRST_CLUSTER && clu < p_bs->num_clusters);
}

// --- super.c ---
// boot 读/解析见 pfs_exfat_super.c

// --- misc.c ---
void pfs_exfat_chain_set(pfs_exfat_chain_t *p_ec, uint32_t dir, uint32_t size, uint8_t flags);
void pfs_exfat_chain_dup(pfs_exfat_chain_t *p_dup, const pfs_exfat_chain_t *p_ec);
int pfs_exfat_cluster_walk(pfs_exfat_sbi_t *p_sbi, uint32_t *p_clu, uint32_t step, uint8_t flags);
int pfs_exfat_chain_advance(pfs_exfat_sbi_t *p_sbi, pfs_exfat_chain_t *p_chain, uint32_t step);

// --- fatent.c ---
int pfs_exfat_ent_get(pfs_exfat_sbi_t *p_sbi, uint32_t loc, uint32_t *p_content);
int pfs_exfat_ent_set(pfs_exfat_sbi_t *p_sbi, uint32_t loc, uint32_t content);

// --- balloc.c ---
int pfs_exfat_load_bitmap(pfs_exfat_sbi_t *p_sbi);
void pfs_exfat_free_bitmap(pfs_exfat_sbi_t *p_sbi);
int pfs_exfat_set_bitmap(pfs_exfat_sbi_t *p_sbi, uint32_t clu);
int pfs_exfat_clear_bitmap(pfs_exfat_sbi_t *p_sbi, uint32_t clu);
int pfs_exfat_test_bitmap(pfs_exfat_sbi_t *p_sbi, uint32_t clu, int *p_used);
uint32_t pfs_exfat_find_free_bitmap(pfs_exfat_sbi_t *p_sbi, uint32_t hint);
int pfs_exfat_alloc_cluster(pfs_exfat_sbi_t *p_sbi, uint32_t *p_clu);
// num_clu: FAT_CHAIN 时为上限 (走到 EOF 可提前结束); NO_FAT_CHAIN 时为连续簇数
int pfs_exfat_free_cluster_chain(pfs_exfat_sbi_t *p_sbi, uint32_t start, uint32_t num_clu,
        uint8_t flags);
int pfs_exfat_zero_cluster(pfs_exfat_sbi_t *p_sbi, uint32_t clu);

// --- dir.c ---
unsigned int pfs_exfat_get_entry_type(const exfat_dentry_t *p_entry);
int pfs_exfat_get_uniname_from_ext_entry(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_clu,
        int local_entry, uint16_t *uniname);
int pfs_exfat_read_dentry_at(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        uint32_t abs_entry, exfat_dentry_t *p_out);
int pfs_exfat_write_dentry_at(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        uint32_t abs_entry, const exfat_dentry_t *p_in);
int pfs_exfat_load_dir_entry_info(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        int entry, pfs_exfat_dir_entry_t *p_info);
int pfs_exfat_readdir(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir_chain,
        uint8_t dir_flags_unused, int64_t *p_cpos, pfs_exfat_dir_entry_t *p_de,
        char *utf8_name, int utf8_cap);

// --- nls.c ---
uint16_t pfs_exfat_toupper(const pfs_exfat_sbi_t *p_sbi, uint16_t a);
int pfs_exfat_uniname_ncmp(const pfs_exfat_sbi_t *p_sbi, const uint16_t *a,
        const uint16_t *b, unsigned int len);
uint16_t pfs_exfat_calc_chksum16(const void *data, int len, uint16_t chksum);
// type: EXFAT_CS_DIR_ENTRY 时跳过首项 checksum 字段字节 (i==2,3); 其余同 CS_DEFAULT
uint16_t pfs_exfat_calc_chksum16_type(const void *data, int len, uint16_t chksum, int type);
int pfs_exfat_utf16_to_utf8(const uint16_t *uniname, int max_uni, char *out, int out_cap);
int pfs_exfat_utf8_to_uniname(pfs_exfat_sbi_t *p_sbi, const char *utf8, int len,
        pfs_exfat_uni_name_t *p_uni);

// --- nls.c (upcase) ---

// 默认压缩 Up-case Table (对齐 linux-7.1 uni_def_upcase / 规范 Recommended Up-case Table)
#define PFS_EXFAT_NUM_UPCASE            2918

// 默认表目录项 checksum (对齐常见 mkfs / exfatprogs)
#define PFS_EXFAT_UPCASE_CHECKSUM       0xE619D30Du

// 压缩格式 upcase 表 (定义在 pfs_exfat_nls.c); 展开见 linux-7.1 exfat_load_default_upcase_table
extern const uint16_t g_pfs_exfat_def_upcase[PFS_EXFAT_NUM_UPCASE];

int pfs_exfat_create_upcase_table(pfs_exfat_sbi_t *p_sbi); ///< mount: 分配+加载展开表; 成功后只读
void pfs_exfat_free_upcase_table(pfs_exfat_sbi_t *p_sbi);  ///< umount / 失败回滚

// --- namei.c ---
int pfs_exfat_find_dir_entry(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        const pfs_exfat_uni_name_t *p_uni, pfs_exfat_dir_entry_t *p_info);
int pfs_exfat_lookup_path(pfs_exfat_sbi_t *p_sbi, const char *path, pfs_exfat_dir_entry_t *p_info);
// 在 path 建空普通文件 (已存在返回 EEXIST); 不扩展目录簇, 无空槽则 ENOSPC
int pfs_exfat_create_file(pfs_exfat_sbi_t *p_sbi, const char *path, pfs_exfat_dir_entry_t *p_info);
int pfs_exfat_mkdir(pfs_exfat_sbi_t *p_sbi, const char *path);
int pfs_exfat_unlink(pfs_exfat_sbi_t *p_sbi, const char *path);
int pfs_exfat_rmdir(pfs_exfat_sbi_t *p_sbi, const char *path);
int pfs_exfat_rename(pfs_exfat_sbi_t *p_sbi, const char *oldpath, const char *newpath);
// 回写 File+Stream 的 start/size/valid/flags 并重算 checksum
int pfs_exfat_update_stream_meta(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_dir_entry_t *p_loc,
        uint32_t start_clu, uint8_t flags, int64_t size, int64_t valid_size);
// 将 File 项集合标为已删除 (type &= ~0x80)
int pfs_exfat_remove_entry_set(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_dir_entry_t *p_info);

// --- file.c --- (只读; 对齐 linux-7.1 exfat_map_cluster / get_block + VDL)
#define PFS_EXFAT_SEC_BUF_MAX               4096

typedef struct _pfs_exfat_file_meta_t {
    uint32_t            start_clu;                      ///< [单位:簇] 数据起始
    uint8_t             flags;                          ///< EXFAT_ALLOC_*
    int64_t             size;                           ///< [单位:字节] DataLength
    int64_t             valid_size;                     ///< [单位:字节] ValidDataLength
    uint32_t            hint_clu_off;                   ///< [单位:簇] 提示: 文件内簇偏移
    uint32_t            hint_clu;                       ///< [单位:簇] 提示: 对应盘簇
} pfs_exfat_file_meta_t;

// Win 卷设备要求扇区对齐读; 从任意字节偏移读 len 字节 (len 须落在同一逻辑扇区内)
static inline int pfs_exfat_pread_at(pfs_exfat_sbi_t *p_sbi, void *dst, int len, int64_t off)
{
    const pfs_exfat_bs_t *p_bs;
    uint8_t sec[PFS_EXFAT_SEC_BUF_MAX];
    uint32_t sector_size;
    int64_t sec_off;
    uint32_t byte_in_sec;
    int rc;

    if (NULL == p_sbi || NULL == dst || len <= 0 || NULL == p_sbi->p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    sector_size = 1u << p_bs->boot.sect_size_bits;
    if (0 == sector_size || sector_size > PFS_EXFAT_SEC_BUF_MAX)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    sec_off = off - (off % (int64_t)sector_size);
    byte_in_sec = (uint32_t)(off - sec_off);
    if ((uint32_t)len > sector_size || byte_in_sec + (uint32_t)len > sector_size)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    rc = pfs_blkio_pread(p_sbi->p_blkio, sec, (int)sector_size, sec_off);
    if (rc < 0)
    {
        return PFS_EXFAT_RET_EIO;
    }
    if (rc != (int)sector_size)
    {
        return PFS_EXFAT_RET_EIO;
    }
    memcpy(dst, sec + byte_in_sec, (size_t)len);
    return PFS_EXFAT_RET_OK;
}

// Win 卷设备要求扇区对齐写; 从任意字节偏移写 len 字节 (len 须落在同一逻辑扇区内)
static inline int pfs_exfat_pwrite_at(pfs_exfat_sbi_t *p_sbi, const void *src, int len, int64_t off)
{
    const pfs_exfat_bs_t *p_bs;
    uint8_t sec[PFS_EXFAT_SEC_BUF_MAX];
    uint32_t sector_size;
    int64_t sec_off;
    uint32_t byte_in_sec;
    int rc;

    if (NULL == p_sbi || NULL == src || len <= 0 || NULL == p_sbi->p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }
    p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    sector_size = 1u << p_bs->boot.sect_size_bits;
    if (0 == sector_size || sector_size > PFS_EXFAT_SEC_BUF_MAX)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    sec_off = off - (off % (int64_t)sector_size);
    byte_in_sec = (uint32_t)(off - sec_off);
    if ((uint32_t)len > sector_size || byte_in_sec + (uint32_t)len > sector_size)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    rc = pfs_blkio_pread(p_sbi->p_blkio, sec, (int)sector_size, sec_off);
    if (rc < 0 || rc != (int)sector_size)
    {
        return PFS_EXFAT_RET_EIO;
    }
    memcpy(sec + byte_in_sec, src, (size_t)len);
    rc = pfs_blkio_pwrite(p_sbi->p_blkio, sec, (int)sector_size, sec_off);
    if (rc < 0 || rc != (int)sector_size)
    {
        return PFS_EXFAT_RET_EIO;
    }
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_map_cluster(pfs_exfat_sbi_t *p_sbi, pfs_exfat_file_meta_t *p_meta,
        uint32_t clu_off, uint32_t *p_clu);
int pfs_exfat_file_read(pfs_exfat_sbi_t *p_sbi, pfs_exfat_file_meta_t *p_meta,
        int64_t pos, void *buf, int len, int *p_got);
int pfs_exfat_file_write(pfs_exfat_sbi_t *p_sbi, pfs_exfat_file_meta_t *p_meta,
        int64_t pos, const void *buf, int len, int *p_put);
// 扩展/收缩逻辑大小; 可能改 start_clu/flags/size/valid_size
int pfs_exfat_file_truncate(pfs_exfat_sbi_t *p_sbi, pfs_exfat_file_meta_t *p_meta,
        int64_t new_size);

// --- cache.c --- (dir/bitmap/FAT: init 只分配内存, get 才读盘)
// init: 分配槽缓冲, 不读盘; get: 未命中则 pfs_blkio_pread
int pfs_exfat_dir_cache_init(pfs_cache_cluster_t *p_dir_cache, pfs_exfat_sbi_t *p_sbi,
        uint32_t cluster_size);
int pfs_exfat_dir_cache_deinit(pfs_cache_cluster_t *p_dir_cache);
pfs_cache_cluster_slot_t *pfs_exfat_dir_cache_get(pfs_cache_cluster_t *p_dir_cache, uint32_t clu_id);
void pfs_exfat_dir_cache_invalidate(pfs_cache_cluster_t *p_dir_cache, uint32_t clu_id);
void pfs_exfat_dir_cache_invalidate_all(pfs_cache_cluster_t *p_dir_cache);

int pfs_exfat_bmp_cache_init(pfs_cache_cluster_t *p_bmp_cache, pfs_exfat_sbi_t *p_sbi,
        uint32_t cluster_size);
int pfs_exfat_bmp_cache_deinit(pfs_cache_cluster_t *p_bmp_cache);
pfs_cache_cluster_slot_t *pfs_exfat_bmp_cache_get(pfs_cache_cluster_t *p_bmp_cache, uint32_t clu_id);
void pfs_exfat_bmp_cache_invalidate(pfs_cache_cluster_t *p_bmp_cache, uint32_t clu_id);
void pfs_exfat_bmp_cache_invalidate_all(pfs_cache_cluster_t *p_bmp_cache);

// init: 填 sbi->fat 几何并分配扇区槽; get: sector_id 为卷内相对 LBA
int pfs_exfat_fat_cache_init(pfs_cache_sector_t *p_fat_cache, pfs_exfat_sbi_t *p_sbi);
int pfs_exfat_fat_cache_deinit(pfs_cache_sector_t *p_fat_cache);
pfs_cache_sector_slot_t *pfs_exfat_fat_cache_get(pfs_cache_sector_t *p_fat_cache, uint64_t sector_id);
void pfs_exfat_fat_cache_invalidate(pfs_cache_sector_t *p_fat_cache, uint64_t sector_id);
void pfs_exfat_fat_cache_invalidate_all(pfs_cache_sector_t *p_fat_cache);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_EXFAT_INTERNAL_H_

// end
