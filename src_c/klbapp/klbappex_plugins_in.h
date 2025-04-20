#ifndef __KLBAPPEX_PLUGINS_IN_H__
#define __KLBAPPEX_PLUGINS_IN_H__


#include "klb_type.h"
#include "klbapp/klbappex_plugins.h"
#include "klbapp/klb_app_extension.h"
#include "klbutil/klb_nlist.h"
#include "klbplatform/klb_dynamic_link.h"
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

    struct
    {
        klbapp_init_extension_cb    cb_init;        ///< init函数
        klbapp_quit_extension_cb    cb_quit;        ///< quit函数   
        klbapp_extension_count_cb   cb_count;       ///< count函数
        klbapp_open_extension_cb    cb_open;        ///< open函数
    };
}klbappex_plugins_dl_t;


/// @struct klbappex_plugins_t
/// @brief  klb app extension plugins 管理插件的扩展
///    注意: app 扩展, 是唯一的
typedef struct klbappex_plugins_t_
{
    struct
    {
        klb_app_t*      p_app;              ///< app指针

        bool            enable;             ///< 是否启用
        klb_nlist_t*    p_path_nlist;       ///< 动态库插件 加载路径 列表: sds
    };

    struct
    {
        klb_nlist_t*    p_dl_nlist;         ///< 已成功加载的 动态 列表: klbappex_plugins_dl_t*
    };
}klbappex_plugins_t;


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
