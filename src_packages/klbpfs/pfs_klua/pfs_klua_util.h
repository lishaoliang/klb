// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_klua_util.h
/// @brief   kpfs Lua 绑定 opts 解析与公共辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    仅 src_klua 内使用; 逻辑对齐 pfs_tool_util, 禁止 include src_tool
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_KLUA_UTIL_H_
#define _PFS_KLUA_UTIL_H_

#include "pfs.h"
#include "pfs_part.h"
#include "pfs_disk.h"
#include "klua/klua.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief 压入 rc, msg (rc==0 成功)
int pfs_klua_push_rc(lua_State *L, int rc);

/// @brief blkio facade 返回值压入 rc, msg
int pfs_klua_push_blkio_rc(lua_State *L, int rc);

/// @brief 压入 info, rc, msg (info 为空表; rc!=0)
int pfs_klua_push_info_rc_fail(lua_State *L, int rc);

/// @brief 栈顶须为 info 表; 压入 rc, msg 后返回 3 (info, rc, msg)
int pfs_klua_push_info_rc_ok(lua_State *L);

/// @brief 容器级 info 字段 (path/container/type/sector_size/size; 对齐 kpfs_image.md / kpfs.probe)
void pfs_klua_push_blkio_container(lua_State *L, const char *path, pfs_blkio_t *p_blkio);

/// @brief 解析 path 或 opts 表: (path [, opts]) / opts{path=...}
/// @return 0 成功; -1 参数非法
int pfs_klua_parse_path_opts(lua_State *L, const char **p_path, int *p_opts_idx);

/// @brief 栈位 1 须为 opts 表; 取 path 字段; opts_idx 固定为 1
/// @return PFS_OK; PFS_EINVAL
int pfs_klua_parse_opts_table(lua_State *L, const char **p_path, int *p_opts_idx);

/// @brief opts 表取 string 字段; 缺省返回 p_default
const char *pfs_klua_opts_string(lua_State *L, int opts_idx, const char *key, const char *p_default);

/// @brief opts 表取 boolean; 缺省 v_default
int pfs_klua_opts_bool(lua_State *L, int opts_idx, const char *key, int v_default);

/// @brief opts 表取 integer; 缺省 v_default
lua_Integer pfs_klua_opts_integer(lua_State *L, int opts_idx, const char *key, lua_Integer v_default);

/// @brief opts.force 须为 true; 否则 PFS_EINVAL
int pfs_klua_opts_require_force(lua_State *L, int opts_idx);

/// @brief 大小写不敏感字符串相等
bool pfs_klua_str_ieq(const char *a, const char *b);

/// @brief 解析块 FS 类型名
int pfs_klua_parse_fs_type(const char *text, pfs_fs_type_e *p_out);

/// @brief 解析 MTD FS 类型名
int pfs_klua_parse_mtd_fs_type(const char *text, pfs_fs_type_e *p_out);

/// @brief 解析分区表 scheme
int pfs_klua_parse_part_scheme(const char *text, pfs_part_scheme_e *p_out);

/// @brief 解析容量字符串或整数 (8G / 32GiB / 十进制字节)
int pfs_klua_parse_size_bytes(lua_State *L, int opts_idx, const char *key, uint64_t *p_out, int *p_have);

/// @brief FS 类型名 (展示用)
const char *pfs_klua_fs_type_name(pfs_fs_type_e t);

/// @brief 分区 scheme 名
const char *pfs_klua_part_scheme_str(pfs_part_scheme_e scheme);

/// @brief mkfs 用卷字节数: 命中分区或整盘剩余
int pfs_klua_query_vol_bytes_at_offset(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t *p_vol);

/// @brief 解析分区偏移: 显式 offset 或 probe 首分区
int pfs_klua_resolve_format_part_offset(pfs_blkio_t *p_blkio, int64_t cli_offset, int have_cli_offset, int64_t *p_out);

/// @brief open 前探测扇区大小
int pfs_klua_resolve_open_sector_size(const char *open_path);

/// @brief Lua mount 模式串: "r" / "rw" ("w" 同 "rw"); 非法默认 r; 底层仅 r/rw 两档
#define PFS_KLUA_MNT_STYLE_R    0
#define PFS_KLUA_MNT_STYLE_RW   1

int pfs_klua_parse_mount_mode_string(const char *text, int *p_style);

/// @brief 栈位 idx 为 string 时解析 mount 模式
int pfs_klua_parse_mount_mode_lua(lua_State *L, int idx, int *p_style);

/// @brief PFS_MNT_* 转展示串 "r" / "rw"
const char *pfs_klua_mount_mode_to_string(int mount_mode);

/// @brief mount 盘名是否合法 (仅字母)
bool pfs_klua_mount_name_ok(const char *p_name);

/// @brief 压入 mount_info 分区项表 (栈顶为新表)
void pfs_klua_push_mount_part_item(lua_State *L, int lua_index, const pfs_disk_part_t *p_part);

/// @brief 压入 mount_info 单盘表 (栈顶为新表)
void pfs_klua_push_mount_disk(lua_State *L, const char *p_name, int name_len, pfs_disk_t *p_disk);

/// @brief Lua 分区序号 (1-based) 转 C 槽 (0-based); lua_part<1 返回 -1
int pfs_klua_lua_part_to_c(int lua_part);

/// @brief opts 表是否含 Lua 数组项 (rawgeti(1) 为 table)
bool pfs_klua_opts_is_part_array(lua_State *L, int opts_idx);

/// @brief 分区数组项个数 (连续 1..n 且每项为 table)
int pfs_klua_opts_part_array_count(lua_State *L, int opts_idx);

/// @brief 按 Lua 1-based 分区序号从分区表解析字节偏移; lua_part<1 失败
int pfs_klua_resolve_part_offset_by_lua_index(pfs_blkio_t *p_blkio, int lua_part_index, int64_t *p_out);

/// @brief remount 项表是否单项 ({ index=…, mode=… })
bool pfs_klua_remount_is_single_item(lua_State *L, int idx);

/// @brief remount 批量项个数
int pfs_klua_remount_item_count(lua_State *L, int idx);

/// @brief 初始化 mount opts (default r, part DFLT)
void pfs_klua_mount_opts_init(pfs_disk_mount_opts_t *p_opts);

/// @brief 从 Lua mode 参数填充 mount opts; mode_idx<=0 表示省略 (全盘 r)
int pfs_klua_mount_opts_from_lua(lua_State *L, int mode_idx, pfs_disk_mount_opts_t *p_opts);

/// @brief 从 opts 表填充 mount offset/sector_size; 可选 mode 字符串覆盖 default_mnt_style
int pfs_klua_parse_mount_extra_opts(lua_State *L, int opts_idx, pfs_disk_mount_opts_t *p_opts);

/// @brief 解析 mkpt 分区项表到 pfs_mkpt_entry_t
int pfs_klua_parse_mkpt_entry_lua(lua_State *L, int tbl_idx, int sector_size, pfs_mkpt_entry_t *p_entry);

/// @brief 解析 mkfs/mkvol 分区字节偏移 (offset / index / probe)
int pfs_klua_resolve_format_offset(lua_State *L, int tbl_idx, int array_pos, pfs_blkio_t *p_blkio, int64_t *p_offset);

/// @brief 解析 mkfs/mkvol 卷大小 (size 字段; have_vol 表示显式指定)
int pfs_klua_resolve_format_vol(lua_State *L, int tbl_idx, pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t *p_vol, int *p_have_vol);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_KLUA_UTIL_H_

// end
