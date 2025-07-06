#ifndef __KLBAPPEX_PLUGINS_IN_H__
#define __KLBAPPEX_PLUGINS_IN_H__


#include "klb_type.h"
#include "klbapp/klbappex_plugins.h"
#include "klbapp/klb_app_extension.h"
#include "klbapp/klbappex_klua.h"
#include "klbutil/klb_nlist.h"
#include "klbplatform/klb_dynamic_link.h"
#include "klbplatform/klb_mutex.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbappex_plugins_t
/// @brief  klb app extension plugins 管理插件的扩展
typedef struct klbappex_plugins_dl_t_
{
    klb_dl_t*       p_dl;           ///< 打开的动态库 句柄
    sds             path;           ///< 动态库 路径

    int             open_count;     ///< 打开 的有效 扩展数
    int             prelib_count;   ///< 打开 的klua 有效预加载函数 数

    struct
    {
        klbapp_init_extension_cb    cb_init;        ///< init函数
        klbapp_quit_extension_cb    cb_quit;        ///< quit函数   
        klbapp_extension_count_cb   cb_count;       ///< count函数
        klbapp_open_extension_cb    cb_open;        ///< open函数

        klbapp_kluaprelib_count_cb  cb_prelib_count;///< klua预加载库 数目
        klbapp_open_kluaprelib_cb   cb_open_prelib; ///< 打开 klua预加载函数
    };
}klbappex_plugins_dl_t;


/// @struct klbappex_plugins_t
/// @brief  klb app extension plugins 管理插件的扩展
///    注意: app 扩展, 是唯一的
typedef struct klbappex_plugins_t_
{
    bool                    enable;             ///< 是否启用 动态库插件

    struct
    {
        klb_app_t*          p_app;              ///< app指针
        klbappex_klua_t*    p_kluaex;           ///< klua appex
    };

    struct
    {
        klb_mutex_t*        p_path_mutex;       ///< 锁: p_path_nlist
        klb_nlist_t*        p_path_nlist;       ///< 动态库插件 加载路径 列表: sds
    };

    struct
    {
        klb_nlist_t*        p_dl_nlist;         ///< 已成功加载的 动态 列表: klbappex_plugins_dl_t*
    };
}klbappex_plugins_t;


/// @brief 注册 klua 扩展
/// @return int 0.成功; 非0.失败
int klbappex_register_plugins(klb_app_t* p_app);


/// @brief 设置 klua ex 扩展指针
void klbappex_plugins_set_kluaex(klbappex_plugins_t* p_appex, klbappex_klua_t* p_kluaex);


/// @brief 设置是否启用 (动态库)插件plugins
///   默认: 未启用
void klbappex_plugins_enable(klbappex_plugins_t* p_appex, bool enable);


/// @brief 若启用(动态库)插件, 加载动态库插件的路径
void klbappex_plugins_push_path(klbappex_plugins_t* p_appex, const char* p_path_plugins);


/// @brief 预加载(动态库)插件
void klbappex_plugins_preload(klbappex_plugins_t* p_appex);


#if defined(__cplusplus)
}
#endif

#endif // __KLBAPPEX_PLUGINS_IN_H__
//end
