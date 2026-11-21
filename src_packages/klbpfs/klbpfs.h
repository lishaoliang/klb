// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbpfs.h
/// @brief   klb 可裁剪包 klbpfs 对外入口 (根目录仅暴露外部接口; 实现见子目录)
/// @version 0.1
/// @history 修改历史
///  \n [2026-11] 自 portfs 迁入 src_packages; 对外暂保留 pfs_* / kpfs
/// @warning 没有警告
/// @note    包外: #include "klbpfs/klbpfs.h" (-I ./src_packages; 启用包时另加包内 -I)
///  \n C API 暂仍为 pfs_* (见 pfs.h); Lua: require "kpfs" 等 (见 pfs_klua.h)
///  \n 裁剪: no-pfs / use-pfs; 宏 __KLB_NO_PFS__
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBPFS_H__
#define __KLBPFS_H__

#include "pfs.h"
#include "pfs_klua/pfs_klua.h"

#endif // __KLBPFS_H__

// end
