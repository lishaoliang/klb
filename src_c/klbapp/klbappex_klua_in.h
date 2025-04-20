#ifndef __KLBAPPEX_KLUA_IN_H__
#define __KLBAPPEX_KLUA_IN_H__


#include "klb_type.h"
#include "klbapp/klbappex_klua.h"
#include "klua/klua_env.h"
#include "klua/klua.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbappex_klua_t
/// @brief  klb app extension klua 扩展
///    注意: app 扩展, 是唯一的
typedef struct klbappex_klua_t_
{
    klua_env_t*             p_env;              ///< klua 环境

    bool                    is_load_entry;      ///< 是否成功 加载了 入口脚本

    klua_openlibs_cb        cb_pre_load;        ///< Lua 需要初始 预加载的 所有非标准库
}klbappex_klua_t;


// lua 环境 在 loop 之前的 准备流程
int klbappex_klua_do_preinit(klbappex_klua_t* p_appex, int argc, char** argv);


// lua 环境 在 loop 之后的 清理流程
int klbappex_klua_do_prequit(klbappex_klua_t* p_appex);


#if defined(__cplusplus)
}
#endif

#endif // __KLBAPPEX_KLUA_IN_H__
//end
