///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_app.h
/// @brief   klb app, 主应用框架流程
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_APP_H__
#define __KLB_APP_H__

#include "klb_type.h"
#include "klbapp/klb_app_extension.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_app_t_ klb_app_t;


/// @brief app main
/// @return int 0
KLB_API int klb_app_main(int argc, char** argv);


/// @brief 获取app实例
/// @return klb_app_t* 实例指针
KLB_API klb_app_t* klb_app_instance();








#if defined(__cplusplus)
}
#endif

#endif // __KLB_APP_H__
//end
