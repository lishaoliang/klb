#ifndef __KLUA_EX_COROUTINE_H__
#define __KLUA_EX_COROUTINE_H__


#include "klb_type.h"
#include "klua/klua_env.h"
#include "klua/klua_coroutine.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLUA_EX_COROUTINE_NAME      "_KLUA_EX_COROUTINE_"

// lua_State.uname[16]
#define KLUA_EX_STATE_NAME_LEN      12 


typedef struct klua_ex_coroutine_t_ klua_ex_coroutine_t;


typedef struct klua_coroutine_env_t_
{
    lua_State*  p_main;     ///< 主程
    lua_State*  p_co;       ///< 协程

    int         param_num;  ///< 附加参数数目
    int         co_reg;     ///< 协程 ref函数

    // 一个协程, 只可能有一处，处于 yield
    klua_coroutine_yield_cb     cb_wakeup;  ///< 唤醒 yield 的回调函数 
    void*                       ptr;        ///< 附加指针
}klua_coroutine_env_t;


/// @brief 注册协程
/// @param [in]  *p_env     Lua环境 
/// @return int 0
int klua_ex_register_coroutine(klua_env_t* p_env);


/// @brief 获取
/// @param [in]  *p_env         Lua环境 
/// @return klua_ex_coroutine_t* 
klua_ex_coroutine_t* klua_ex_get_coroutine(klua_env_t* p_env);
klua_ex_coroutine_t* klua_ex_get_coroutine_by_L(lua_State* L);

int klua_ex_coroutine_exit(klua_env_t* p_env, int64_t now);

int klua_ex_coroutine_push(klua_ex_coroutine_t* p_ex, klua_coroutine_env_t* p_co_env);
int klua_ex_coroutine_remove(klua_ex_coroutine_t* p_ex, klua_coroutine_env_t* p_co_env);

int klua_ex_coroutine_wakeup(klua_ex_coroutine_t* p_ex, lua_State* p_co);

int klua_ex_coroutine_wakeup_timeout(klua_ex_coroutine_t* p_ex, lua_State* p_co, int64_t tc);


lua_State* klua_ex_coroutine_rawgeti(klua_ex_coroutine_t* p_ex, lua_State* p_co);


// Bug. 直接使用 lua_yield, 当退出时, 若未满足唤醒条件, 则没有时机去唤醒, 协程一直被阻塞
// 使用 klua_ex_coroutine_yield 替代, 当退出时, 触发强制唤醒协程时机
// 因唤醒协程, 可能有上下文需要处理, 故使用回调函数, 交由调用者处理相关情况
// 对于一个协程而言, 同一时间内, 只可能有一个位置处于 yield 
int klua_ex_coroutine_yield(klua_ex_coroutine_t* p_ex, lua_State* p_co, klua_coroutine_yield_cb cb, void* ptr);


// 检查 co
int klua_ex_coroutine_debug_check(klua_ex_coroutine_t* p_ex, lua_State* p_co);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_COROUTINE_H__
//end
