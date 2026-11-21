#ifndef __KLBAPPEX_KLUA_IN_H__
#define __KLBAPPEX_KLUA_IN_H__


#include "klbapp/klbappex_klua.h"
#include "klua/klua_env.h"
#include "klbutil/klb_nlist.h"
#include "klbplatform/klb_rwlock.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbappex_preload_item_t
/// @brief  klua 预加载项
typedef struct klbappex_preload_item_t_
{
    lua_CFunction       cb;                 ///< openlib 或 wrapper 回调
    const char*         name;               ///< NULL 表示 wrapper 回调; 非 NULL 则 klua_loadlib
}klbappex_preload_item_t;


/// @struct klbappex_klua_t
/// @brief  klb app extension klua 扩展
///    注意: app 扩展, 是唯一的
typedef struct klbappex_klua_t_
{
    klua_env_t*             p_env;              ///< klua 环境
    bool                    is_load_entry;      ///< 是否成功 加载了 入口脚本

    struct
    {
        klb_rwlock_t*       p_preload_rwlock;   ///< p_preload_nlist 的锁
        klb_nlist_t*        p_preload_nlist;    ///< 所有 lua 环境需要 预加载的函数列表
    };
}klbappex_klua_t;


/// @brief 注册 klua 扩展
/// @return int 0.成功; 非0.失败
int klbappex_register_klua(klb_app_t* p_app);


/// @brief lua 环境在 loop 之前的准备流程
/// @param [in]  *p_appex       klua 扩展
/// @param [in]  argc           原始参数个数
/// @param [in]  **argv         原始参数数组
/// @param [in]  entry_idx      入口脚本在 argv 中的下标 (通常为 klb_optind)
/// @return int 0.成功; 非0.失败
/// @note  写入 env 的 get_args 为: argv[0] + argv[entry_idx..]; 与无壳选项时 argv[1]=脚本 兼容
int klbappex_klua_do_preinit(klbappex_klua_t* p_appex, int argc, char** argv, int entry_idx);


// lua 环境 在 loop 之后的 清理流程
int klbappex_klua_do_prequit(klbappex_klua_t* p_appex);


#if defined(__cplusplus)
}
#endif

#endif // __KLBAPPEX_KLUA_IN_H__
//end
