// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_klua_ex.h
/// @brief   kpfs klua env 扩展 (每 env 单例, get 时注册+创建)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    仅 src_klua 内使用; 注册名 KLUAEX_kpfs_env, 非 require 名
///  \n env 扩展: klua_env_register_extension 注册, 随 env 创建单例
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_KLUA_EX_H_
#define _PFS_KLUA_EX_H_

#include "klua/klua_env.h"
#include "pfs_disk.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief kpfs env 扩展注册名 (klua_env_register_extension 键, 非 require); 宏 KLUAEX_kpfs_env
#define KLUAEX_kpfs_env            "_KLUAEX-kpfs-env_"

typedef struct pfs_klua_ex_t_ pfs_klua_ex_t;


/// @brief 获取 env 扩展单例; 首次 get 时注册 vtable 并 cb_create (幂等)
pfs_klua_ex_t *pfs_klua_ex_get(klua_env_t *p_env);

/// @brief 经 L 反查 env 并 get (语义同 pfs_klua_ex_get)
pfs_klua_ex_t *pfs_klua_ex_get_by_L(lua_State *L);

/// @brief 挂载镜像到盘名 (整盘登记到 pfs_klua_ex_t)
///   mount 盘名: p_name 仅字母, 如 "D", "DEF" (不含分区号)
///   路径语法: "盘名[分区号]:/" + 相对路径; 分区号 Lua **1-based**; 省略视为第 1 分区
///   目录分隔符: "/" (固定; 非 Windows 默认 "\\")
///   分区操作: "D:/"、"D1:/" 为第 1 分区; "D2:/aa/bb.txt" 为第 2 分区
///   磁盘操作: 盘名 "D" (mount/umount/remount 的 p_name; part_index 见各 API)
///   eg. mount("DEF", image); "DEF:/", "DEF1:/", "DEF2:/aa/bb.txt"
int pfs_klua_ex_mount(pfs_klua_ex_t *p_ex, const char *p_name, const char *p_image_path, const pfs_disk_mount_opts_t *p_opts);

/// @brief 重新挂载; part_index<0 全部已 mount 分区, >=0 单槽 **0-based**
/// @return int PFS_OK 至少一个分区 remount 成功; 全部失败返回末次错误码
int pfs_klua_ex_remount(pfs_klua_ex_t *p_ex, const char *p_name, int part_index, int mnt_style);

/// @brief 卸载; part_index<0 整盘并从登记表移除, >=0 单槽 **0-based**
int pfs_klua_ex_umount(pfs_klua_ex_t *p_ex, const char *p_name, int part_index);

/// @brief 按盘名查已 mount disk; name_len 为盘名字节数
pfs_disk_t *pfs_klua_ex_get_disk(pfs_klua_ex_t *p_ex, const char *p_name, int name_len);

/// @brief 已登记 disk 个数
int pfs_klua_ex_disk_count(pfs_klua_ex_t *p_ex);

/// @brief 遍历已登记 disk; cb 非 NULL
void pfs_klua_ex_foreach_disk(pfs_klua_ex_t *p_ex,
        void (*cb)(const char *p_key, int key_len, pfs_disk_t *p_disk, void *p_ctx),
        void *p_ctx);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_KLUA_EX_H_

// end
