///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_app_in.h
/// @brief   klb app
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_APP_IN_H__
#define __KLB_APP_IN_H__

#include "klb_type.h"
#include "klbapp/klb_app.h"
#include "klbthird/sds.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_app_extension_activated_t
/// @brief  激活的扩展
typedef struct klb_app_extension_activated_t_
{
    void*               ptr;        ///< 扩展的指针
    sds                 name;       ///< 扩展的名称
    klb_app_extension_t ex;         ///< 扩展的接口函数
}klb_app_extension_activated_t;


/// @struct klb_app_t
/// @brief  app框架
typedef struct klb_app_t_
{
    int a;
}klb_app_t;


int klb_app_init();
void klb_app_quit();


/// @brief 创建
klb_app_t* klb_app_create();


/// @brief 销毁
/// @return 无
void klb_app_destroy(klb_app_t* p_app);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_APP_IN_H__
//end
