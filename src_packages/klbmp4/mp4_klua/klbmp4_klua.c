// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbmp4/klbmp4.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbbase/klb_mnp.h"
#include "klua/klua.h"
#include <assert.h>


#define KLUA_KMP4_HANDLE    "KLUA_KMP4_HANDLE*"


/// @struct klua_kmp4_t
/// @brief  MP4 Lua 包装
typedef struct klua_kmp4_t_
{
    klbmp4_t*   p_mp4;      ///< C 句柄
    bool        write;      ///< true=只写; false=只读
}klua_kmp4_t;


//////////////////////////////////////////////////////////////////////////


static klua_kmp4_t* new_klua_kmp4(lua_State* L)
{
    klua_kmp4_t* p_obj = (klua_kmp4_t*)lua_newuserdata(L, sizeof(klua_kmp4_t));

    KLB_MEMSET(p_obj, 0, sizeof(klua_kmp4_t));
    luaL_setmetatable(L, KLUA_KMP4_HANDLE);

    return p_obj;
}


static klua_kmp4_t* to_klua_kmp4(lua_State* L, int index)
{
    klua_kmp4_t* p_obj = (klua_kmp4_t*)luaL_checkudata(L, index, KLUA_KMP4_HANDLE);

    luaL_argcheck(L, NULL != p_obj, index, "'kmp4' expected");

    return p_obj;
}


static int close_klua_kmp4(klua_kmp4_t* p_obj)
{
    if (NULL == p_obj || NULL == p_obj->p_mp4)
    {
        return 0;
    }

    int rc = klbmp4_close(p_obj->p_mp4);

    p_obj->p_mp4 = NULL;

    return rc;
}


//////////////////////////////////////////////////////////////////////////


static int klua_kmp4_read(lua_State* L)
{
    klua_kmp4_t* p_obj = to_klua_kmp4(L, 1);
    klb_buf_t* p_frame = NULL;
    int rc = 0;

    if (NULL == p_obj->p_mp4 || p_obj->write)
    {
        lua_pushnil(L);
        lua_pushinteger(L, KLBMP4_ERR_ARG);
        return 2;
    }

    rc = klbmp4_read(p_obj->p_mp4, &p_frame);

    if (KLBMP4_OK == rc)
    {
        lua_pushlightuserdata(L, p_frame);
        lua_pushinteger(L, KLBMP4_OK);
        return 2;
    }

    if (KLBMP4_EOF == rc)
    {
        lua_pushnil(L);
        lua_pushinteger(L, KLBMP4_EOF);
        return 2;
    }

    lua_pushnil(L);
    lua_pushinteger(L, KLB_BASECODE_ERR);
    return 2;
}


static int klua_kmp4_write(lua_State* L)
{
    klua_kmp4_t* p_obj = to_klua_kmp4(L, 1);
    klb_buf_t* p_frame = (klb_buf_t*)luaL_checklightuserdata(L, 2);
    int rc = 0;

    if (NULL == p_obj->p_mp4 || !p_obj->write || NULL == p_frame)
    {
        lua_pushinteger(L, KLBMP4_ERR_ARG);
        return 1;
    }

    rc = klbmp4_write(p_obj->p_mp4, p_frame);
    lua_pushinteger(L, (0 == rc) ? KLBMP4_OK : KLBMP4_ERR_WRITE);
    return 1;
}


static int klua_kmp4_free_frame(lua_State* L)
{
    klb_buf_t* p_frame = (klb_buf_t*)luaL_checklightuserdata(L, 2);

    KLB_FREE(p_frame);
    return 0;
}


/// @brief 取帧载荷 (Annex-B; 跳过 klb_mnp_media_t 头)
/// @note  kmp4:frame_payload(frame) -> string payload, int size
static int klua_kmp4_frame_payload(lua_State* L)
{
    klb_buf_t* p_frame = (klb_buf_t*)luaL_checklightuserdata(L, 2);
    const char* p_data = NULL;
    int payload = 0;

    if (NULL == p_frame || NULL == p_frame->p_buf
        || p_frame->end <= (int)sizeof(klb_mnp_media_t))
    {
        lua_pushnil(L);
        lua_pushinteger(L, 0);
        return 2;
    }

    p_data = p_frame->p_buf + sizeof(klb_mnp_media_t);
    payload = p_frame->end - (int)sizeof(klb_mnp_media_t);

    lua_pushlstring(L, p_data, (size_t)payload);
    lua_pushinteger(L, payload);
    return 2;
}


/// @brief 取帧元信息
/// @note  kmp4:frame_info(frame) -> table {pts_us, key}
static int klua_kmp4_frame_info(lua_State* L)
{
    klb_buf_t* p_frame = (klb_buf_t*)luaL_checklightuserdata(L, 2);
    klb_mnp_media_t* p_media = NULL;

    if (NULL == p_frame || NULL == p_frame->p_buf)
    {
        lua_pushnil(L);
        return 1;
    }

    p_media = (klb_mnp_media_t*)p_frame->p_buf;

    lua_createtable(L, 0, 2);
    lua_pushinteger(L, (lua_Integer)p_media->time);
    lua_setfield(L, -2, "pts_us");
    lua_pushboolean(L, KLB_MNP_VTYPE_I == p_media->vtype);
    lua_setfield(L, -2, "key");
    return 1;
}


static int klua_kmp4_info(lua_State* L)
{
    klua_kmp4_t* p_obj = to_klua_kmp4(L, 1);
    klbmp4_info_t info;

    if (NULL == p_obj->p_mp4 || 0 != klbmp4_info(p_obj->p_mp4, &info))
    {
        lua_pushnil(L);
        lua_pushinteger(L, KLBMP4_ERR_ARG);
        return 2;
    }

    lua_newtable(L);

    lua_pushinteger(L, info.width);
    lua_setfield(L, -2, "width");

    lua_pushinteger(L, info.height);
    lua_setfield(L, -2, "height");

    lua_pushinteger(L, info.profile);
    lua_setfield(L, -2, "profile");

    lua_pushinteger(L, info.level);
    lua_setfield(L, -2, "level");

    lua_pushinteger(L, info.sample_count);
    lua_setfield(L, -2, "sample_count");

    lua_pushnumber(L, (lua_Number)info.fps);
    lua_setfield(L, -2, "fps");

    lua_pushinteger(L, (lua_Integer)info.timescale);
    lua_setfield(L, -2, "timescale");

    lua_pushinteger(L, (lua_Integer)info.duration_ms);
    lua_setfield(L, -2, "duration_ms");

    lua_pushinteger(L, KLBMP4_OK);
    return 2;
}


static int klua_kmp4_seek(lua_State* L)
{
    klua_kmp4_t* p_obj = to_klua_kmp4(L, 1);
    lua_Integer time_ms = luaL_checkinteger(L, 2);
    int rc = 0;

    if (NULL == p_obj->p_mp4 || p_obj->write)
    {
        lua_pushinteger(L, KLBMP4_ERR_ARG);
        return 1;
    }

    rc = klbmp4_seek(p_obj->p_mp4, (int64_t)time_ms);
    lua_pushinteger(L, (0 == rc) ? KLBMP4_OK : KLB_BASECODE_ERR);
    return 1;
}


static int klua_kmp4_close(lua_State* L)
{
    klua_kmp4_t* p_obj = to_klua_kmp4(L, 1);
    int rc = close_klua_kmp4(p_obj);

    lua_pushinteger(L, rc);
    return 1;
}


static int klua_kmp4_gc(lua_State* L)
{
    klua_kmp4_t* p_obj = to_klua_kmp4(L, 1);

    close_klua_kmp4(p_obj);
    return 0;
}


static int klua_kmp4_tostring(lua_State* L)
{
    klua_kmp4_t* p_obj = to_klua_kmp4(L, 1);

    lua_pushfstring(L, "kmp4:%p", p_obj);
    return 1;
}


static void klua_kmp4_createmeta(lua_State* L)
{
    static luaL_Reg meth[] =
    {
        { "read",           klua_kmp4_read },
        { "write",          klua_kmp4_write },
        { "info",           klua_kmp4_info },
        { "seek",           klua_kmp4_seek },
        { "free_frame",     klua_kmp4_free_frame },
        { "frame_payload",  klua_kmp4_frame_payload },
        { "frame_info",     klua_kmp4_frame_info },
        { "close",          klua_kmp4_close },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] =
    {
        { "__index",        NULL },
        { "__gc",           klua_kmp4_gc },
        { "__close",        klua_kmp4_gc },
        { "__tostring",     klua_kmp4_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KMP4_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


//////////////////////////////////////////////////////////////////////////


static int open_klua_kmp4(lua_State* L, bool write)
{
    const char* p_path = luaL_checkstring(L, 1);
    klbmp4_t* p_mp4 = NULL;
    klua_kmp4_t* p_obj = NULL;
    int err = KLBMP4_OK;

    if (write)
    {
        p_mp4 = klbmp4_open_write_ex(p_path, &err);
    }
    else
    {
        p_mp4 = klbmp4_open_read_ex(p_path, &err);
    }

    if (NULL == p_mp4)
    {
        lua_pushnil(L);
        lua_pushinteger(L, err);
        lua_pushfstring(L, "kmp4 open %s failed: path=[%s]", write ? "write" : "read", p_path);
        return 3;
    }

    p_obj = new_klua_kmp4(L);
    p_obj->p_mp4 = p_mp4;
    p_obj->write = write;

    return 1;
}


static int klua_kmp4_open_read(lua_State* L)
{
    return open_klua_kmp4(L, false);
}


static int klua_kmp4_open_write(lua_State* L)
{
    return open_klua_kmp4(L, true);
}


//////////////////////////////////////////////////////////////////////////


int klua_open_kmp4(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "open_read",      klua_kmp4_open_read },
        { "open_write",     klua_kmp4_open_write },
        { NULL,             NULL }
    };

    klua_kmp4_createmeta(L);
    luaL_newlib(L, lib);
    return 1;
}


// end
