// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    svn_ra.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   alua 应用支撑包 svn_ra: 对外仅 ksvnra (Lua preload)
/// @version 0.8
/// @history 修改历史
///  \n [2026] 创建文件
///  \n [2026-10] core 拆分为 md5/wire/session/log/commit
///  \n [2026-11] 新增流式 cat (cat_begin/cat_next/cat_close) 供大文件分块输出
///  \n [2026-11] commit 的 textdelta-chunk 按 1MiB 分块发送, 修大文件提交被 svnserve 断连
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __SVN_RA_H__
#define __SVN_RA_H__

#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 扩展库 "ksvnra"
/// @param [in] *L  Lua 状态
/// @return int 返回 1
/// @note 供 alua 预加载; open/youngest/uuid/list/list_r/cat/log/commit; 非 libklb 导出
int klua_open_ksvnra(lua_State* L);


#if defined(__cplusplus)
}
#endif

#endif // __SVN_RA_H__

// end
