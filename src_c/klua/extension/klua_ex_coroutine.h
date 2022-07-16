#ifndef __KLUA_EX_COROUTINE_H__
#define __KLUA_EX_COROUTINE_H__


#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLUA_EX_COROUTINE_NAME      "_KLUA_EX_COROUTINE_"


typedef struct klua_ex_coroutine_t_ klua_ex_coroutine_t;


typedef struct klua_coroutine_env_t_
{
    lua_State*  p_main;     ///< 主程
    lua_State*  p_co;       ///< 协程

    int         param_num;  ///< 附加参数数目
    int         co_reg;     ///< 协程 ref函数
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


int klua_ex_coroutine_push(klua_ex_coroutine_t* p_ex, klua_coroutine_env_t* p_co_env);
int klua_ex_coroutine_remove(klua_ex_coroutine_t* p_ex, klua_coroutine_env_t* p_co_env);

int klua_ex_coroutine_wakeup(klua_ex_coroutine_t* p_ex, lua_State* p_co);

int klua_ex_coroutine_wakeup_timeout(klua_ex_coroutine_t* p_ex, lua_State* p_co, int64_t tc);


lua_State* klua_ex_coroutine_rawgeti(klua_ex_coroutine_t* p_ex, lua_State* p_co);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_COROUTINE_H__
//end
