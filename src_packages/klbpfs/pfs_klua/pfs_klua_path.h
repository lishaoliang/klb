// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_klua_path.h
/// @brief   kpfs 盘符路径解析与 ctx 定位
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    盘符语法: "盘名[分区号]:/相对路径"; 仅 src_klua 内使用
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_KLUA_PATH_H_
#define _PFS_KLUA_PATH_H_

#include "pfs.h"
#include "klua/klua.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief 盘符路径解析结果
typedef struct pfs_klua_drive_path_t {
    char            disk_name[32];      ///< 盘名 (仅字母)
    int             part_slot;        ///< 分区槽 0-based
    char            vfs_path[512];    ///< 分区内路径; 以 '/' 开头
} pfs_klua_drive_path_t;

/// @brief 解析盘符路径 "盘名[槽]:/相对路径"
/// @return PFS_OK; PFS_EINVAL
int pfs_klua_path_parse(const char *drive_path, pfs_klua_drive_path_t *p_out);

/// @brief 将解析结果定位到已 mount 分区 ctx
/// @return PFS_OK; PFS_EINVAL / PFS_ENOENT
int pfs_klua_path_resolve_ctx(lua_State *L, const pfs_klua_drive_path_t *p_parsed, pfs_ctx_t **p_out_ctx);

/// @brief 解析盘符路径并定位 ctx (parse + resolve)
/// @return PFS_OK; PFS_EINVAL / PFS_ENOENT
int pfs_klua_path_resolve_drive(lua_State *L, const char *drive_path,
        pfs_klua_drive_path_t *p_parsed, pfs_ctx_t **p_out_ctx);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_KLUA_PATH_H_

// end
