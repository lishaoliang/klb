// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_klua.h
/// @brief   libkpfs 对外入口 (pfs klua 扩展, klb plugins)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    默认: klua/klb **不链接** libkpfs; 运行时 **plugins** dlopen `libkpfs.so`
///  \n **静态链** `libkpfs.a` 或未启用 plugins 时: 调用方 **静态插入** `pfs_klua_open_kpfs*` 至 `klbappex_klua_push_preload`
///  \n **无** `klua_open_kpfs` / `klua_loadlib` 扩展库形态
///  \n 脚本: 预加载后 `require "kpfs"` 等 (经 package.loaded, 非 _PRELOAD openlib)
///  \n klb: **plugins** 挂 `libkpfs.so`; 静态链时由扩展注册到 klua env
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_KLUA_H_
#define _PFS_KLUA_H_

#include "pfs_export.h"
#include "klua/klua.h"
#include "klua/klua_env.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


/// @brief 加载所有 kpfs 相关的 klua 库
#define PFS_KLUA_LOADALL(L)  {\
        klua_loadlib(L, pfs_klua_open_kpfs, "kpfs"); \
        klua_loadlib(L, pfs_klua_open_kpfs_vfs, "kpfs.vfs"); \
        klua_loadlib(L, pfs_klua_open_kpfs_probe, "kpfs.probe"); \
        klua_loadlib(L, pfs_klua_open_kpfs_disk, "kpfs.disk"); \
        klua_loadlib(L, pfs_klua_open_kpfs_image, "kpfs.image"); \
        klua_loadlib(L, pfs_klua_open_kpfs_mtd, "kpfs.mtd"); \
    }


/// @brief klb plugins: klua 预加载回调数目 (KLBAPPEX_DL_pre_count)
PFS_API int klbappex_pre_count(void);

/// @brief klb plugins: 打开第 idx 个 klua 预加载库 (KLBAPPEX_DL_pre_open)
/// @param [out] p_out_name 库名称(以'\0'结尾, 插件持有生命周期)
/// @return 0 成功; 非 0 失败
PFS_API int klbappex_pre_open(int idx, lua_CFunction *p_out_preload, const char **p_out_name);


/// @brief kpfs 根模块 klua 预加载 open (对齐 `klua_open_*` 命名)
/// @note  require 名 `"kpfs"`; 与 `klbappex_pre_open` 第 0 项一致
PFS_API int pfs_klua_open_kpfs(lua_State *L);

/// @brief kpfs.vfs klua 预加载 open
/// @note  require 名 `"kpfs.vfs"`
PFS_API int pfs_klua_open_kpfs_vfs(lua_State *L);

/// @brief kpfs.probe klua 预加载 open
/// @note  require 名 `"kpfs.probe"`
PFS_API int pfs_klua_open_kpfs_probe(lua_State *L);

/// @brief kpfs.disk klua 预加载 open (mkpt / mkfs / mkvol)
/// @note  require 名 `"kpfs.disk"`
PFS_API int pfs_klua_open_kpfs_disk(lua_State *L);

/// @brief kpfs.image klua 预加载 open
/// @note  require 名 `"kpfs.image"`
PFS_API int pfs_klua_open_kpfs_image(lua_State *L);

/// @brief kpfs.mtd klua 预加载 open
/// @note  require 名 `"kpfs.mtd"`
PFS_API int pfs_klua_open_kpfs_mtd(lua_State *L);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_KLUA_H_

// end
