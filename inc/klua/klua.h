///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/30
//
/// @file    klua.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/06/30 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_H__
#define __KLUA_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif


#define KLUA_HELP_TOP_B(L_)     int klua_tb_ = lua_gettop(L_);
#define KLUA_HELP_TOP_E(L_)     int klua_te_ = lua_gettop(L_); assert(klua_tb_ == klua_te_);


/// @brief 自定义预加载Lua库
/// @param [in] *L              Lua状态
/// @return int 0
typedef int(*klua_openlibs_cb)(lua_State* L);


/// @brief 对lua.c中main简易修改
/// @param [in] argc        命令行参数个数
/// @param [in] **argv      命令行参数
/// @param [in] cb          预加载库函数
/// @return int 同main函数
KLB_API int klua_main(int argc, char** argv, klua_openlibs_cb cb);


/// @brief 预加载库函数
/// @param [in] *L          Lua状态
/// @param [in] openlib     库函数列表
/// @param [in] p_name      库名称
/// @return 无
KLB_API void klua_loadlib(lua_State* L, lua_CFunction openlib, const char* p_name);


/// @brief check boolean
/// @return int 0.false; 1.true
KLB_API bool luaL_checkboolean(lua_State* L, int arg);


/// @brief check lightuserdata
/// @return void*
KLB_API void* luaL_checklightuserdata(lua_State* L, int arg);


/// @brief setfield
/// 返回 lua_newtable(L);
KLB_API void klua_setfield_boolean(lua_State* L, const char* p_key, bool b);
KLB_API void klua_setfield_integer(lua_State* L, const char* p_key, lua_Integer n);
KLB_API void klua_setfield_string(lua_State* L, const char* p_key, const char* p_value);
KLB_API void klua_setfield_lstring(lua_State* L, const char* p_key, const char* p_value, size_t v_len);

/// @brief 在LUA_REGISTRYINDEX上引用
/// @param [in] *L          Lua状态
/// @param [in] arg         第几个参数[1,N]
/// @return int 大于0
KLB_API int klua_ref_registryindex(lua_State* L, int arg);


/// @brief 在LUA_REGISTRYINDEX上解除引用
/// @param [in] *L          Lua状态
/// @param [in] ref         klua_ref_registryindex函数的返回值
/// @return 无
KLB_API void klua_unref_registryindex(lua_State* L, int reg);


/// @brief 扩展库"cjson"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_cjson(lua_State* L);


/// @brief 扩展库"cjson.safe"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_cjson_safe(lua_State* L);


/// @brief 扩展库"lpeg"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_lpeg(lua_State *L);


/// @brief 扩展库"lfs"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_lfs(lua_State *L);


/// @brief 扩展库"kos"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kos(lua_State* L);


/// @brief 扩展库"ktime"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_ktime(lua_State* L);


/// @brief 扩展库"kthread"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kthread(lua_State* L);


/// @brief 扩展库"kmnp_dev"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kmnp_dev(lua_State* L);


/// @brief 扩展库"kgui"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kgui(lua_State* L);


/// @brief 扩展库"kwnd"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kwnd(lua_State* L);


/// @brief 扩展库"kurl"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kurl(lua_State* L);


/// @brief 扩展库"khttp"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_khttp(lua_State* L);


/// @brief 扩展库"kwebsocket"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kwebsocket(lua_State* L);


/// @brief 扩展库"kncm": 网络媒体长链接管理模块
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kncm(lua_State* L);


/// @brief 预加载 宏KLUA_LOADLIBS 定义的所有库
/// @param [in] *L          Lua状态
/// @return int 返回0
KLB_API int klua_loadlib_all(lua_State* L);


/// @def   KLB_KLUA_LOADLIBS
/// @brief 预加载扩展库
#define KLUA_LOADLIBS(L) {                                  \
    klua_loadlib(L, klua_open_cjson,        "cjson");       \
    klua_loadlib(L, klua_open_cjson_safe,   "cjson.safe");  \
    klua_loadlib(L, klua_open_lpeg,         "lpeg");        \
    klua_loadlib(L, klua_open_lfs,          "lfs");         \
    klua_loadlib(L, klua_open_kos,          "kos");         \
    klua_loadlib(L, klua_open_ktime,        "ktime");       \
    klua_loadlib(L, klua_open_kthread,      "kthread");     \
    klua_loadlib(L, klua_open_kmnp_dev,     "kmnp_dev");    \
    klua_loadlib(L, klua_open_kgui,         "kgui");        \
    klua_loadlib(L, klua_open_kwnd,         "kwnd");        \
    klua_loadlib(L, klua_open_kurl,         "kurl");        \
    klua_loadlib(L, klua_open_khttp,        "khttp");       \
    klua_loadlib(L, klua_open_kwebsocket,   "kwebsocket");  \
    klua_loadlib(L, klua_open_kncm,         "kncm");        \
}

#ifdef __cplusplus
}
#endif

#endif // __KLUA_H__
//end
