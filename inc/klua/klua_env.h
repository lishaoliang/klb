///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_env.h
/// @brief   lua_State简易封装
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_ENV_H__
#define __KLUA_ENV_H__

#include "klb_type.h"
#include "klbthird/sds.h"
#include "klbmem/klb_buf.h"
#include "klua/klua_env_extension.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


typedef struct klua_env_t_ klua_env_t;


/// @brief 创建一个lua环境
/// @param [in] cb_pre_load       预加载额外库函数
/// @return klua_env_t* lua环境
KLB_API klua_env_t* klua_env_create(lua_CFunction cb_pre_load);


/// @brief 销毁lua环境
/// @param [in] *p_env             lua环境 
/// @return 无
KLB_API void klua_env_destroy(klua_env_t* p_env);


/// @brief 设置用户数据指针(user data)
/// @param [in] *p_env             lua环境
/// @param [in] *p_udata           用户数据指针(user data)
/// @return 无
KLB_API void klua_env_set_udata(klua_env_t* p_env, void* p_udata);


/// @brief 获取用户数据指针(user data)
/// @param [in] *p_env             lua环境
/// @return void* 用户数据指针
KLB_API void* klua_env_get_udata(klua_env_t* p_env);


/// @brief 获取当前lua环境滴答数
/// @param [in] *p_env             lua环境 
/// @return int64_t tc
KLB_API int64_t klua_env_get_tick_count(klua_env_t* p_env);


/// @brief 立即更新lua环境滴答数
/// @param [in] *p_env             lua环境 
/// @return 无
KLB_API void klua_env_update_tick_count(klua_env_t* p_env);


/// @brief 按路径方式加载一个脚本文件
/// @param [in] *p_env             lua环境
/// @param [in] *p_entry           入口lua文件路径; eg. "./test.lua"
/// @return int 0.成功; 非0.失败
KLB_API int klua_env_dofile(klua_env_t* p_env, const char* p_entry);


/// @brief 按库方式加载一个脚本文件
/// @param [in] *p_env             lua环境
/// @param [in] *p_entry           入口lua文件; eg. "server.entry_gui"
/// @return int 0.成功; 非0.失败
KLB_API int klua_env_dolibrary(klua_env_t* p_env, const char* p_entry);


/// @brief 结束dofile/dolibrary
/// @param [in] *p_env             lua环境
/// @return int 0.成功; 非0.失败
KLB_API int klua_env_doend(klua_env_t* p_env);


/// @brief 从lua_State*获取klua_env_t*指针
/// @param [in] *L             L
/// @return klua_env_t* lua环境
KLB_API klua_env_t* klua_env_get_by_L(lua_State* L);


/// @brief 获取原始lua_State*
/// @param [in] *p_env             lua环境
/// @return lua_State* L
KLB_API lua_State* klua_env_get_L(klua_env_t* p_env);


/// @brief 报错
/// @param [in] *p_env              lua环境
/// @param [in] status              非0时, 报错
/// @return int status
KLB_API int klua_env_report(klua_env_t* p_env, int status);

/// @brief 报错
/// @param [in] *p_env              lua环境
/// @param [in] status              非0时, 报错
/// @return int status
KLB_API int klua_env_report_by_L(lua_State* L, int status);


#define KLUA_LPC_NAME_LEN           15
#define KLUA_LPC_NAME_BUF           16


/// @struct klua_msg_t
/// @brief  消息
typedef struct klua_msg_t_
{
#define KLUA_LPC_POST                   10                  ///< POST消息: 无响应
#define KLUA_LPC_REQUEST                11                  ///< 请求消息
#define KLUA_LPC_RESPONSE               12                  ///< 响应消息
#define KLUA_LPC_NOTIFY                 13                  ///< 通知消息
    int                     type;                           ///< 消息类型

    union
    {
        // POST / REQUEST / RESPONSE / NOTIFY
        struct
        {
            char            dst_name[KLUA_LPC_NAME_BUF];    ///< 目标(模块等)名称
            char            src_name[KLUA_LPC_NAME_BUF];    ///< 来源(模块等)名称

            klb_buf_t*      p_data;                         ///< 消息数据
        };
    };
}klua_msg_t;


KLB_API void klua_msg_free(klua_msg_t* p_msg);


/// @brief 调用一次lua环境; 需要定期调用
/// @param [in] *p_env              lua环境
/// @return int 0
KLB_API int klua_env_loop_once(klua_env_t* p_env);


/// @brief 是否退出状态
/// @param [in] *p_env              lua环境
/// @return bool true.退出; false.不退出
KLB_API bool klua_env_is_exit(klua_env_t* p_env);


/// @brief 设置退出状态
/// @param [in] *p_env              lua环境
/// @return 无
KLB_API void klua_env_exit(klua_env_t* p_env);


/// @brief 设置名称
/// @param [in] *p_env              lua环境
/// @return 无
KLB_API void klua_env_set_name(klua_env_t* p_env, const char* p_name, size_t name_len);


/// @brief 获取名称
/// @param [in] *p_env              lua环境
/// @return sds 名称
KLB_API const sds klua_env_get_name(klua_env_t* p_env);


/// @brief 设置全局参数: 数据格式参考 luaseri_pack/luaseri_pack_from
/// @param [in] *p_env              lua环境
/// @return 无
KLB_API void klua_env_set_args(klua_env_t* p_env, const char* p_data, int data_len);


/// @brief 获取全局参数: 数据格式参考 luaseri_pack/luaseri_pack_from
/// @param [in] *p_env              lua环境
/// @return const klb_buf_t*
KLB_API const klb_buf_t* klua_env_get_args(klua_env_t* p_env);


/// @brief 放入消息: p_msg 交给
/// @param [in] *p_env              lua环境
/// @return sds 名称
KLB_API void klua_env_push_lpc_msg(klua_env_t* p_env, klua_msg_t* p_msg);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_ENV_H__
//end
