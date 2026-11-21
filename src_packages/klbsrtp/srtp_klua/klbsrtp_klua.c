// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbsrtp_klua.c
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klbsrtp 的 Lua 绑定 (扩展库 "ksrtp")
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.1 实现上下文工厂与 userdata 方法面
///////////////////////////////////////////////////////////////////////////
#include "klbsrtp/klbsrtp.h"
#include "klbsrtp/core/klb_srtp.h"
#include "klbnet/klb_netcode.h"
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include <string.h>


/// @def   KLUA_KSRTP_HANDLE
/// @brief Lua userdata 元表名
#define KLUA_KSRTP_HANDLE               "KSRTP_CTX_HANDLE*"

/// @def   KLUA_KSRTP_TAIL_MAX
/// @brief 保护后追加的最大尾部长度 (MKI 上限 + 认证标签)
#define KLUA_KSRTP_TAIL_MAX             (KLB_SRTP_MKI_MAX + 32)


/// @struct klua_ksrtp_t
/// @brief  ksrtp 上下文的 Lua 包装
typedef struct klua_ksrtp_t_
{
    lua_State*          L;          ///< Lua 状态
    klb_srtp_ctx_t*     p_ctx;      ///< SRTP 上下文
}klua_ksrtp_t;


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_ksrtp(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


/// @brief 统一返回: 小于 0 压入 nil, code; 否则压入 true
static int push_rc_klua_ksrtp(lua_State* L, int code)
{
    if (0 > code)
    {
        return push_err_klua_ksrtp(L, code);
    }

    lua_pushboolean(L, 1);
    return 1;
}


////////////////////////////////////////
static klua_ksrtp_t* new_klua_ksrtp(lua_State* L)
{
    klua_ksrtp_t* p_srtp = (klua_ksrtp_t*)lua_newuserdata(L, sizeof(klua_ksrtp_t));
    KLB_MEMSET(p_srtp, 0, sizeof(klua_ksrtp_t));
    luaL_setmetatable(L, KLUA_KSRTP_HANDLE);
    return p_srtp;
}


static klua_ksrtp_t* to_klua_ksrtp(lua_State* L, int index)
{
    klua_ksrtp_t* p_srtp = (klua_ksrtp_t*)luaL_checkudata(L, index, KLUA_KSRTP_HANDLE);
    luaL_argcheck(L, NULL != p_srtp, index, "'ksrtp' expected");
    return p_srtp;
}


static int klua_ksrtp_tostring(lua_State* L)
{
    klua_ksrtp_t* p_srtp = to_klua_ksrtp(L, 1);

    lua_pushfstring(L, "ksrtp:%p", p_srtp->p_ctx);
    return 1;
}


static int klua_ksrtp_close(lua_State* L)
{
    klua_ksrtp_t* p_srtp = to_klua_ksrtp(L, 1);

    if (NULL != p_srtp->p_ctx)
    {
        klb_srtp_destroy(p_srtp->p_ctx);
        p_srtp->p_ctx = NULL;
    }

    return 0;
}


////////////////////////////////////////
static int klua_ksrtp_create(lua_State* L)
{
    int profile = (int)luaL_optinteger(L, 1, KLB_SRTP_PROFILE_AES128_CM_HMAC_SHA1_80); ///< @1. 保护套件
    int code = 0;

    klb_srtp_ctx_t* p_ctx = klb_srtp_create(profile, &code);
    if (NULL == p_ctx)
    {
        return push_err_klua_ksrtp(L, (0 != code) ? code : KLB_NETCODE_ERR);
    }

    klua_ksrtp_t* p_srtp = new_klua_ksrtp(L);
    p_srtp->L = L;
    p_srtp->p_ctx = p_ctx;

    return 1;
}


static int klua_ksrtp_set_key(lua_State* L)
{
    klua_ksrtp_t* p_srtp = to_klua_ksrtp(L, 1);
    size_t key_len = 0;
    size_t salt_len = 0;
    const char* p_key = luaL_checklstring(L, 2, &key_len);     ///< @2. 主密钥
    const char* p_salt = luaL_checklstring(L, 3, &salt_len);   ///< @3. 主盐

    return push_rc_klua_ksrtp(L,
        klb_srtp_set_key(p_srtp->p_ctx, (const uint8_t*)p_key, (int)key_len,
            (const uint8_t*)p_salt, (int)salt_len));
}


static int klua_ksrtp_set_mki(lua_State* L)
{
    klua_ksrtp_t* p_srtp = to_klua_ksrtp(L, 1);

    if (lua_isnoneornil(L, 2))
    {
        return push_rc_klua_ksrtp(L, klb_srtp_set_mki(p_srtp->p_ctx, NULL, 0));
    }

    size_t mki_len = 0;
    const char* p_mki = luaL_checklstring(L, 2, &mki_len);     ///< @2. MKI 字节

    return push_rc_klua_ksrtp(L,
        klb_srtp_set_mki(p_srtp->p_ctx, (const uint8_t*)p_mki, (int)mki_len));
}


static int klua_ksrtp_protect(lua_State* L)
{
    klua_ksrtp_t* p_srtp = to_klua_ksrtp(L, 1);
    size_t len = 0;
    const char* p_pkt = luaL_checklstring(L, 2, &len);         ///< @2. RTP/RTCP 包
    int out_len = 0;

    int cap = (int)len + KLUA_KSRTP_TAIL_MAX;
    uint8_t* p_buf = KLB_MALLOC(uint8_t, cap, 0);
    if (NULL == p_buf)
    {
        return push_err_klua_ksrtp(L, KLB_NETCODE_NOMEM);
    }

    memcpy(p_buf, p_pkt, len);

    int ret = klb_srtp_protect(p_srtp->p_ctx, p_buf, (int)len, cap, &out_len);
    if (0 > ret)
    {
        KLB_FREE(p_buf);
        return push_err_klua_ksrtp(L, ret);
    }

    lua_pushlstring(L, (const char*)p_buf, (size_t)out_len);
    KLB_FREE(p_buf);

    return 1;
}


static int klua_ksrtp_unprotect(lua_State* L)
{
    klua_ksrtp_t* p_srtp = to_klua_ksrtp(L, 1);
    size_t len = 0;
    const char* p_pkt = luaL_checklstring(L, 2, &len);         ///< @2. SRTP/SRTCP 包
    int out_len = 0;

    uint8_t* p_buf = KLB_MALLOC(uint8_t, (int)len + 1, 0);
    if (NULL == p_buf)
    {
        return push_err_klua_ksrtp(L, KLB_NETCODE_NOMEM);
    }

    memcpy(p_buf, p_pkt, len);

    int ret = klb_srtp_unprotect(p_srtp->p_ctx, p_buf, (int)len, &out_len);
    if (0 > ret)
    {
        KLB_FREE(p_buf);
        return push_err_klua_ksrtp(L, ret);
    }

    lua_pushlstring(L, (const char*)p_buf, (size_t)out_len);
    KLB_FREE(p_buf);

    return 1;
}


////////////////////////////////////////
static void klua_ksrtp_createmeta(lua_State* L)
{
    static const luaL_Reg meth[] =
    {
        { "close",      klua_ksrtp_close },
        { "destroy",    klua_ksrtp_close },
        { "set_key",    klua_ksrtp_set_key },
        { "set_mki",    klua_ksrtp_set_mki },
        { "protect",    klua_ksrtp_protect },
        { "unprotect",  klua_ksrtp_unprotect },

        { NULL,         NULL }
    };

    static const luaL_Reg metameth[] =
    {
        { "__index",    NULL },     /* place holder */
        { "__gc",       klua_ksrtp_close },
        { "__close",    klua_ksrtp_close },
        { "__tostring", klua_ksrtp_tostring },
        { NULL,         NULL }
    };

    luaL_newmetatable(L, KLUA_KSRTP_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


int klua_open_ksrtp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "create",     klua_ksrtp_create },
        { NULL,         NULL }
    };

    luaL_newlib(L, lib);
    klua_ksrtp_createmeta(L);

    return 1;
}


// end
