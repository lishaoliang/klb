///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_dynamic_link.h
/// @brief   动态链接库
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_DYNAMIC_LINK_H__
#define __KLB_DYNAMIC_LINK_H__


#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_dl_t_ klb_dl_t;


/// @brief 打开动态库
/// @param [in] *p_file_name        动态库路径
/// @return klb_dl_t* 动态库对象
klb_dl_t* klb_dlopen(const char* p_path_name);


/// @brief 关闭动态库
/// @param [in] *p_dl               动态库对象
/// @return 无
void klb_dlclose(klb_dl_t* p_dl);


/// @brief 获取动态库函数
/// @param [in] *p_dl               动态库对象
/// @param [in] *p_procname         函数名称
/// @return void*   函数指针
void* klb_dlsym(klb_dl_t* p_dl, const char* p_procname);


#ifdef __cplusplus
}
#endif

#endif // __KLB_DYNAMIC_LINK_H__
//end
