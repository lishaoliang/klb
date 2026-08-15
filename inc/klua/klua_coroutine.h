///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_coroutine.h
/// @brief   Lua协程扩展: 标准扩展协程导出函数
/// @version 0.1
/// @history 修改历史
///   \n [2025-10] 提供协程调用接口; 添加调试接口
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_COROUTINE_H__
#define __KLUA_COROUTINE_H__

#include "klb_type.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klua_ex_coroutine_t_ klua_ex_coroutine_t;


/// @brief 获取协程扩展
/// @param [in]  *p_env         Lua环境 
/// @return klua_ex_coroutine_t* 
KLB_API klua_ex_coroutine_t* klua_coroutine_get(klua_env_t* p_env);


/// @brief 获取协程扩展
/// @param [in]  *L             lua_State 
/// @return klua_ex_coroutine_t* 
KLB_API klua_ex_coroutine_t* klua_coroutine_get_by_L(lua_State* L);


/// @brief 将协程入口函数放入栈顶
/// @param [in]  *p_ex         协程扩展
/// @param [in]  *p_co         当前协程
/// @return lua_State* 注册协程的主程; 可能为 NULL
KLB_API lua_State* klua_coroutine_rawgeti(klua_ex_coroutine_t* p_ex, lua_State* p_co);


/// @brief 当出现需要中断协程时的回调函数
/// @param [in] *ptr        附加对象
/// @param [in] *p_ex       协程扩展
/// @param [in] *p_co       Lua协程
/// @param [in] opt         消息: klua_env_extension_opt_e
/// @return int 0.成功
typedef int(*klua_coroutine_yield_cb)(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt);


/// @brief yield
/// @return int
/// @note 替代 lua_yield
///   Fixed Bug. [2025] 直接使用 lua_yield, 当退出时, 若未满足唤醒条件, 则没有时机去唤醒, 协程一直被阻塞
///   使用 klua_coroutine_yield 替代, 当退出时, 触发强制唤醒协程时机
///   因唤醒协程, 可能有上下文需要处理, 故使用回调函数, 交由调用者处理相关情况
///   对于一个协程而言, 同一时间内, 只可能有一个位置处于 yield 
KLB_API int klua_coroutine_yield(klua_ex_coroutine_t* p_ex, lua_State* p_co, klua_coroutine_yield_cb cb, void* ptr);


/// @brief 调试校验 协程是否存在
KLB_API int klua_coroutine_debug_check(klua_ex_coroutine_t* p_ex, lua_State* p_co);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_COROUTINE_H__
//end
