///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/04/11
//
/// @file    klua_data.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/04/11 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_DATA_H__
#define __KLUA_DATA_H__

#include "klb_type.h"
#include "klua/klua.h"
#include "klbthird/sds.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klua_data_t
/// @brief  基础可变数据类型
typedef struct klua_data_t_
{
#define KLUA_DATA_TNIL              LUA_TNIL                ///< nil 空
#define KLUA_DATA_TBOOLEAN          LUA_TBOOLEAN            ///< boolean 布尔
#define KLUA_DATA_TLIGHTUSERDATA    LUA_TLIGHTUSERDATA      ///< void* 指针
#define KLUA_DATA_TNUMBER           LUA_TNUMBER             ///< lua_Number 浮点类型
#define KLUA_DATA_TSTRING           LUA_TSTRING             ///< char* 字符串
#define KLUA_DATA_TINTEGER          LUA_NUMTYPES + 10       ///< lua_Integer 数值型
    int                 type;       ///< 类型: KLUA_DATA_TNIL, KLUA_DATA_TBOOLEAN, KLUA_DATA_TLIGHTUSERDATA

    union
    {
        lua_Integer     integer;    ///< 整数
        lua_Number      number;     ///< 浮点数
        sds             str;        ///< 字符串
        void*           ptr;        ///< C指针
    };
}klua_data_t;


/// @brief 清空可变数据
/// @param [in] *p_data     可变数据
/// @return 无
/// @note 清空后, 类型变为: KLUA_DATA_TNIL
void klua_emptydata(klua_data_t* p_data);


/// @brief 检查为通用类型
/// @param [in]  *L         L
/// @param [out] *p_data    可变数据
/// @param [in]  idx        序号
/// @return int 数据类型: KLUA_DATA_TNIL
/// @note 参考 luaL_checkstring 等系列函数
int  klua_checkdata(lua_State* L, klua_data_t* p_data, int idx);


/// @brief 放入通用类型
/// @param [in]  *L         L
/// @param [in]  *p_data    可变数据
/// @return 无
/// @note 参考 lua_pushstring 等系列函数
void klua_pushdata(lua_State* L, const klua_data_t* p_data);


typedef struct klua_msg_t_ klua_msg_t;


/// @brief 销毁消息
/// @param [in] *p_msg      lpc消息
/// @return 无
typedef void(*klua_msg_destroy_cb)(klua_msg_t* p_msg);


/// @def   KLUA_MSG_COMMON_HEADER
/// @brief 消息部分公共头域
#define KLUA_MSG_COMMON_HEADER  int size; sds ex_name; klua_msg_destroy_cb cb_destroy


/// @struct klua_msg_t
/// @brief  消息基础类型
typedef struct klua_msg_t_
{
    KLUA_MSG_COMMON_HEADER;

    // int size;                        ///< 结构体尺寸大小
    // sds ex_name;                     ///< 目标扩展名称
    // klua_msg_destroy_cb cb_destroy;  ///< 销毁函数
}klua_msg_t;


/// @struct klua_ctrlex_msg_t
/// @brief  对扩展进行控制的消息
typedef struct klua_ctrlex_msg_t_
{
    KLUA_MSG_COMMON_HEADER;

    int         data_num;           ///< *.data区域数目
    klua_data_t data[];             ///< 附加数据信息
}klua_ctrlex_msg_t;


/// @brief 销毁控制消息
/// @param [in] *p_msg      lpc消息
/// @return 无
void klua_ctrlex_msg_destroy(klua_msg_t* p_msg);


/// @struct klua_lpc_msg_t
/// @brief  lpc模块消息
typedef struct klua_lpc_msg_t_
{
    KLUA_MSG_COMMON_HEADER;

#define KLUA_LPC_MSG_POST       0   ///< POST消息: 无响应
#define KLUA_LPC_MSG_REQUEST    1   ///< 请求消息
#define KLUA_LPC_MSG_RESPONSE   2   ///< 响应消息
    int         msg;                ///< 消息类型

    sds         src_name;           ///< 源调用者名称: REQUEST有效
    sds         func_name;          ///< 函数名称: REQUEST有效
    sds         sequence;           ///< 序列号(唯一标记): RESPONSE有效

    int         data_num;           ///< *.data区域数目
    klua_data_t data[];             ///< 附加数据信息
}klua_lpc_msg_t;


/// @brief 销毁lpc消息
/// @param [in] *p_msg      lpc消息
/// @return 无
void klua_lpc_msg_destroy(klua_msg_t* p_msg);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_DATA_H__
//end
