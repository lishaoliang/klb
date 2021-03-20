///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_time.h
/// @brief   时间/定时器相关扩展
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_TIME_H__
#define __KLUA_EX_TIME_H__

#include "klb_type.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define KLUA_EX_TIME_NAME       "_KLUA_EX_TIME_"

typedef struct klua_ex_time_t_ klua_ex_time_t;

int klua_ex_register_time(klua_env_t* p_env);

klua_ex_time_t* klua_ex_get_time(klua_env_t* p_env);


int klua_ex_time_new_timer_once(klua_ex_time_t* p_time, lua_State* L, lua_Integer wait, int index);

int klua_ex_time_new_ticker(klua_ex_time_t* p_time, lua_State* L, const char* p_name, size_t name_len, lua_Integer interval, int index);
int klua_ex_time_stop_ticker(klua_ex_time_t* p_time, const char* p_name, size_t name_len);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_TIME_H__
//end
