// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_sysfilesystem.h
/// @brief   系统路径
/// @version 0.1
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SYSFILESYSTEM_H__
#define __KLB_SYSFILESYSTEM_H__


#include "klb_type.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 获取当前 主执行程序 的路径
/// @return sds 当前 主执行程序 的路径
KLB_API sds klb_get_base_path();


/// @brief 获取当前 应用 的配置文档路径
/// @return sds 路径
KLB_API sds klb_get_pref_path(const char* p_org, const char* p_app);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_SYSFILESYSTEM_H__

// end
