// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_klua_internal.h
/// @brief   libkpfs 绑定层跨文件声明 (src_klua 内部)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    仅 src_klua 内使用; klua 预加载 openlib 见 **pfs_klua.h**
///  \n 其余符号在各自 .c 内 static
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_KLUA_INTERNAL_H_
#define _PFS_KLUA_INTERNAL_H_

#include "klua/klua.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief kpfs.vfs opendir + kpfs.dir* userdata
void pfs_klua_dir_setup(lua_State *L);
int pfs_klua_dir_opendir(lua_State *L);

/// @brief kpfs.vfs open + kpfs.file* userdata
void pfs_klua_file_setup(lua_State *L);
int pfs_klua_file_open(lua_State *L);

/// @brief kpfs.vfs path 级 API (access/mkdir/...)
void pfs_klua_vfs_path_register(lua_State *L);

/// @brief kpfs.disk.mkpt
int pfs_klua_lua_mkpt(lua_State *L);

/// @brief kpfs.disk.mkfs
int pfs_klua_lua_mkfs(lua_State *L);

/// @brief kpfs.disk.mkvol
int pfs_klua_lua_mkvol(lua_State *L);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_KLUA_INTERNAL_H_

// end
