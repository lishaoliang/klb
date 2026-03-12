///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua.h
/// @brief   文件简要描述
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_H__
#define __KLUA_H__

#include "klb_type.h"
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

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


/// @brief check 并赋予值
KLB_API bool klua_check_coroutine(lua_State* L, const char* p_tip_msg);
KLB_API bool klua_check_option_boolean(lua_State* L, int idx, bool v_default);
KLB_API lua_Integer klua_check_option_integer(lua_State* L, int idx, lua_Integer v_default);
KLB_API lua_Number  klua_check_option_number(lua_State* L, int idx, lua_Number v_default);


/// @brief is_* 探测类型
KLB_API bool klua_is_boolean(lua_State* L, int idx);
KLB_API bool klua_is_lightuserdata(lua_State* L, int idx);
KLB_API bool klua_is_integer(lua_State* L, int idx);
KLB_API bool klua_is_number(lua_State* L, int idx);
KLB_API bool klua_is_string(lua_State* L, int idx);
KLB_API bool klua_is_table(lua_State* L, int idx);
KLB_API bool klua_is_function(lua_State* L, int idx);


/// @brief setfield
/// 返回 lua_newtable(L);
KLB_API void klua_setfield_boolean(lua_State* L, const char* p_key, bool b);
KLB_API void klua_setfield_integer(lua_State* L, const char* p_key, lua_Integer n);
KLB_API void klua_setfield_number(lua_State* L, const char* p_key, lua_Number num);
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


/// @brief 是否为协程
KLB_API bool klua_is_coroutine(lua_State* L);


/// @brief 扩展库"cjson"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_cjson(lua_State* L);


/// @brief 扩展库"cjson.safe"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_cjson_safe(lua_State* L);


#if !defined(__KLB_NO_LPEG__)
/// @brief 扩展库"lpeg"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_lpeg(lua_State *L);
#endif

/// @brief 扩展库"lfs"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_lfs(lua_State *L);


#if !defined(__KLB_NO_ZLIB__)
/// @brief 扩展库"zlib"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_zlib(lua_State *L);
#endif


#if !defined(__KLB_NO_SQLITE__)
/// @brief 扩展库"lsqlite3"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_lsqlite3(lua_State* L);
#endif

/// @brief 扩展库"LuaXML_lib"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_LuaXML_lib(lua_State* L);


/// @brief 扩展库"kco", 协程库
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kco(lua_State* L);


/// @brief 扩展库"kos"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kos(lua_State* L);


/// @brief 扩展库"ksys"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_ksys(lua_State* L);


/// @brief 扩展库"kenv"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kenv(lua_State* L);


/// @brief 扩展库"krand"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_krand(lua_State* L);


/// @brief 扩展库"ktime"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_ktime(lua_State* L);


/// @brief 扩展库"kmcache"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kmcache(lua_State* L);


/// @brief 扩展库"kstore"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_klist(lua_State* L);


/// @brief 扩展库"kthread"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kthread(lua_State* L);


/// @brief 扩展库"kkpa", "*.kpa"打包/解包
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kkpa(lua_State* L);


/// @brief 扩展库"klpc", 线程间通信
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_klpc(lua_State* L);


/// @brief 扩展库"kgui"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kgui(lua_State* L);


/// @brief 扩展库"kurl"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kurl(lua_State* L);


/// @brief 扩展库"ktcp"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_ktcp(lua_State* L);


/// @brief 扩展库"kudp"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kudp(lua_State* L);


/// @brief 扩展库"khttp"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_khttp(lua_State* L);


/// @brief 扩展库"kws"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kws(lua_State* L);


/// @brief 扩展库"kmnp"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kmnp(lua_State* L);


/// @brief 扩展库"ksmp"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_ksmp(lua_State* L);


/// @brief 实时流媒体模块
KLB_API int klua_open_krtsp(lua_State* L);
KLB_API int klua_open_khttp_flv(lua_State* L);
KLB_API int klua_open_khttp_mnp(lua_State* L);
KLB_API int klua_open_kws_flv(lua_State* L);
KLB_API int klua_open_kws_mnp(lua_State* L);


/// @brief 扩展库"kh26x": h26x文件读取
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kh26x(lua_State* L);


/// @brief 预加载 宏KLUA_LOADLIBS 定义的所有库
/// @param [in] *L          Lua状态
/// @return int 返回0
KLB_API int klua_loadlib_all(lua_State* L);


//////////////////////////////////////////////////////////////////////////
// ./klb/src_packages

#if !defined(__KLB_NO_PACKAGES__)
KLB_API int klua_open_kpa_mgui(lua_State* L);
KLB_API int klua_open_kpa_http(lua_State* L);
KLB_API int klua_open_kpa_ws(lua_State* L);
KLB_API int klua_open_kpa_mnp(lua_State* L);
KLB_API int klua_open_kpa_rtsp(lua_State* L);
KLB_API int klua_open_kpa_flv(lua_State* L);
KLB_API int klua_open_kpa_sip(lua_State* L);
#endif


#ifdef __cplusplus
}
#endif

#endif // __KLUA_H__
//end
