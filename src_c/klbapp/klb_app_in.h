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


#include "klbapp/klb_app.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_nlist.h"
#include "klbthird/sds.h"
#include "klbapp/klbappex_klua_in.h"
#include "klbapp/klbappex_plugins_in.h"
#include "klbplatform/klb_rwlock.h"
#include "klbplatform/klb_mutex.h"


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
    klbappex_ioctrl_t   ioctrl;     ///< ioctrl 接口函数
}klb_app_extension_activated_t;


/// @struct klb_app_t
/// @brief  app框架
typedef struct klb_app_t_
{
    int                 run_status;                     ///< 运行状态

    // 内部临时存储数据
    struct
    {
        klb_mutex_t*    p_preload_mutex;                ///< 锁: p_preload_nlist
        klb_nlist_t*    p_preload_nlist;                ///< 预加载函数列表; klb_app_preload_cb
    };

    // 扩展
    struct
    {
        // 注册的扩展 与 已激活的扩展
        klb_rwlock_t*   p_extension_rwlock;             ///< 锁: p_extension_hlist/p_extension_activated_hlist

        klb_hlist_t*    p_extension_hlist;              ///< 注册的扩展; klb_app_extension_t*
        klb_hlist_t*    p_extension_activated_hlist;    ///< 激活的扩展; klb_app_extension_activated_t*

        // 需要处理loop的 已激活扩展
        // 将需要 loop 的扩展独立出来 处理
        klb_rwlock_t*   p_loop_rwlock;                  ///< 锁: p_loop_nlist
        klb_nlist_t*    p_loop_nlist;                   ///< 需要调用loop函数的扩展列表
    };

    // 常用扩展 指针
    struct
    {
        klbappex_klua_t*        p_klua;                 ///< klua 环境扩展
        klbappex_plugins_t*     p_plugins;              ///< plugins 插件管理 的扩展
    };
}klb_app_t;


/// @brief init/quit
int klb_app_init();
void klb_app_quit();


/// @brief 创建
klb_app_t* klb_app_create();


/// @brief 销毁
/// @return 无
void klb_app_destroy(klb_app_t* p_app);


/// @brief 运行一次
/// @return int 
int klb_app_loop_once();


#if defined(__cplusplus)
}
#endif

#endif // __KLB_APP_IN_H__
//end
