// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_isofs_internal.h
/// @brief   isofs 模块内部辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_ISOFS_INTERNAL_H_
#define _PFS_ISOFS_INTERNAL_H_

#include "isofs/pfs_isofs.h"
#include "pfs_endian.h"
#include "pfs.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint8_t pfs_isonum_711(const uint8_t *p)
{
    return p[0];
}

static inline uint16_t pfs_isonum_721(const uint8_t *p)
{
    return pfs_read_le16(p);
}

static inline uint16_t pfs_isonum_723(const uint8_t *p)
{
    return pfs_read_le16(p);
}

static inline uint32_t pfs_isonum_733(const uint8_t *p)
{
    return pfs_read_le32(p);
}

static inline int64_t pfs_isofs_lba_to_off(const pfs_isofs_sbi_t *p_sbi, uint32_t lba)
{
    return p_sbi->part_offset + ((int64_t)lba * (int64_t)p_sbi->logical_block_size);
}

// 数据起点 = extent + ext_attr_length (逻辑块)
static inline uint32_t pfs_isofs_data_lba(const pfs_iso_directory_record_t *p_rec)
{
    return pfs_isonum_733(p_rec->extent) + (uint32_t)pfs_isonum_711(p_rec->ext_attr_length);
}

int pfs_isofs_map_blkio_rc(int rc);

// 扫描 PVD(+Joliet SVD); 填 lbs/vol/根候选; 成功 OK; 非本 FS ENOEXEC
int pfs_isofs_read_volumes(pfs_blkio_t *p_blkio, int64_t part_offset,
        uint32_t *p_lbs, uint32_t *p_vol_blocks,
        pfs_isofs_inode_t *p_root_pri, pfs_isofs_inode_t *p_root_joliet,
        int *p_joliet_level);

// 从单条 directory record 填 inode 首节 (不含多 extent 展开)
void pfs_isofs_inode_from_rec(const pfs_iso_directory_record_t *p_rec,
        pfs_isofs_inode_t *p_out);

// 解码显示/查找名: RR NM > Joliet > ISO9660; 返回 0 有效, 1 跳过(. / .. / assoc / RE), <0 错
// p_rec_len: 本 DE 字节长度; 用于 SUSP 边界
int pfs_isofs_decode_name(pfs_isofs_sbi_t *p_sbi, const pfs_iso_directory_record_t *p_rec,
        int rec_len, char *name_out, int name_cap);

// 自目录内 byte_pos 起展开多 extent 各节到 p_out; *p_end_pos 为越过末节后的偏移
int pfs_isofs_expand_sections(pfs_isofs_sbi_t *p_sbi, const pfs_isofs_inode_t *p_dir,
        uint32_t byte_pos, pfs_isofs_inode_t *p_out, uint32_t *p_end_pos);

// Rock Ridge: 解析 SUSP; 填 NM 到 name (可空); 处理 CL 改写目录 extent; RE 返回 1(跳过)
// 返回 0 正常, 1 跳过(RE), <0 错; *p_has_nm=1 表示写出了 NM
int pfs_isofs_rock_parse(pfs_isofs_sbi_t *p_sbi, const pfs_iso_directory_record_t *p_rec,
        int rec_len, pfs_isofs_inode_t *p_inode, char *name_out, int name_cap, int *p_has_nm);

// 根目录探测 Rock Ridge (设 sbi->rock / rock_offset)
int pfs_isofs_rock_detect(pfs_isofs_sbi_t *p_sbi, const pfs_isofs_inode_t *p_root);

// Joliet: UTF-16BE -> UTF-8; 去 ;1 与尾 .
int pfs_isofs_joliet_name(const char *raw, int raw_bytes, char *name_out, int name_cap);

// ISO9660 短名规范化 (小写, 去 ;version)
int pfs_isofs_normalize_name(const char *raw, int raw_len, char *name_out, int name_cap);

// 名比较: Joliet 忽略大小写; 否则精确
bool pfs_isofs_name_equal(const pfs_isofs_sbi_t *p_sbi, const char *a, const char *b);

int pfs_isofs_find_in_dir(pfs_isofs_sbi_t *p_sbi, const pfs_isofs_inode_t *p_dir,
        const char *name, pfs_isofs_inode_t *p_out);

int pfs_isofs_lookup_path(pfs_isofs_sbi_t *p_sbi, const char *path, pfs_isofs_inode_t *p_out);

// 自当前目录读 .. DE 得父目录 inode
int pfs_isofs_parent_dir(pfs_isofs_sbi_t *p_sbi, const pfs_isofs_inode_t *p_dir,
        pfs_isofs_inode_t *p_parent);

int pfs_isofs_dir_read_next(pfs_isofs_sbi_t *p_sbi, pfs_isofs_dir_meta_t *p_dm,
        pfs_dirent_t *p_ent);

int pfs_isofs_file_read(pfs_isofs_sbi_t *p_sbi, const pfs_isofs_file_meta_t *p_fm,
        int64_t pos, void *buf, int size, int *p_got);

#ifdef __cplusplus
}
#endif

#endif // _PFS_ISOFS_INTERNAL_H_

// end
