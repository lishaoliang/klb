///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_coroutine.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   协程扩展
/// @version 0.3
/// @history 修改历史
///  \n [2022] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_COROUTINE_H__
#define __KLUA_EX_COROUTINE_H__


#include "klb_type.h"
#include "klua/klua_env.h"
#include "klua/klua_coroutine.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLUAEX_coroutine      "_KLUAEX-coroutine_"

// lua_State.uname[16]
#define KLUA_EX_STATE_NAME_LEN      12 


typedef struct klua_ex_coroutine_t_ klua_ex_coroutine_t;


typedef struct klua_coroutine_env_t_
{
    lua_State*  p_main;     ///< 主程
    lua_State*  p_co;       ///< 协程

    bool        is_first;   ///< 第一次运行

    int         param_num;  ///< 附加参数数目
    int         co_reg;     ///< 协程 ref函数

    // 一个协程, 只可能有一处，处于 yield
    klua_coroutine_yield_cb     cb_wakeup;  ///< 唤醒 yield 的回调函数 
    void*                       ptr;        ///< 附加指针

    void*                       p_handle;   ///< kco 句柄 userdata (klua_coroutine_handle_t*); 可为 NULL
}klua_coroutine_env_t;


/// @brief kco 句柄布局 (与 klua_kcoro.c userdata 一致; 结束时由 close_all/close 回写)
typedef struct klua_coroutine_handle_t_
{
    klua_coroutine_env_t*   p_co_env;   ///< 存活 env; 结束后为 NULL
    bool                    done;       ///< 已结束/已强制关闭
    int                     stat;       ///< 结束后缓存状态 (kco: dead/error)
}klua_coroutine_handle_t;


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


/// @brief 获取当前协程数目
/// @param [in]  *p_ex          协程扩展
/// @return int                 已登记协程数; p_ex 为 NULL 时为 0
/// @note 对应 p_co_hlist 大小; fork 登记后计入, 协程结束 remove 后减一
int klua_ex_coroutine_count(klua_ex_coroutine_t* p_ex);


/// @brief 关闭所有协程
/// @param [in]  *p_ex          协程扩展
/// @return int 0
/// @note quit 回调 + 丢弃计时队列 + 排空唤醒队列;
///   \n 再对 hlist 残留强制 resetthread + 释放 env (含纯 co_sleep 僵尸), 使 count 可归零;
///   \n p_ex 为 NULL 时直接返回 0; 不销毁扩展本身
int klua_ex_coroutine_close_all(klua_ex_coroutine_t* p_ex);


/// @brief 关闭单个协程的调度挂接 (摘超时/唤醒队列; 可选 quit 回调)
/// @param [in]  *p_ex          协程扩展
/// @param [in]  *p_co          目标协程
/// @return int 0
/// @note 不销毁 env/句柄; 由调用方 resetthread + free; p_ex/p_co 为 NULL 时为 0
int klua_ex_coroutine_close_one(klua_ex_coroutine_t* p_ex, lua_State* p_co);


/// @brief 协程快照压栈 (诊断用)
/// @param [in]  *p_ex          协程扩展; NULL 时压空表 count=0 list={}
/// @param [in]  *L             调用方 lua_State
/// @return int 1 (栈顶为 table: count/list)
/// @note list[i]: id/status/in_timeout/in_wakeup/has_cb; 无挂起时长字段
int klua_ex_coroutine_snapshot(klua_ex_coroutine_t* p_ex, lua_State* L);

int klua_ex_coroutine_wakeup(klua_ex_coroutine_t* p_ex, lua_State* p_co);

int klua_ex_coroutine_wakeup_timeout(klua_ex_coroutine_t* p_ex, lua_State* p_co, int64_t tc);


lua_State* klua_ex_coroutine_rawgeti(klua_ex_coroutine_t* p_ex, lua_State* p_co);


// Fixed Bug. [2025] 直接使用 lua_yield, 当退出时, 若未满足唤醒条件, 则没有时机去唤醒, 协程一直被阻塞
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
