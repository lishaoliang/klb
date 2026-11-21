// Doc Encode : UTF-8 BOM, Unix(LF)
#include "alua/svn_ra/svn_ra.h"
#include "alua/svn_ra/core/svn_ra_in.h"
#include "alua/svn_ra/core/svn_ra_priv.h"
#include "klbmem/klb_mem.h"
#include <string.h>


#define KLUA_KSVNRA_HANDLE  "KLUA_KSVNRA_HANDLE*"


/// @struct klua_ksvnra_t
/// @brief  ksvnra Lua 包装
typedef struct klua_ksvnra_t_
{
    svn_ra_t*   p_ra;       ///< C 会话
}klua_ksvnra_t;


//////////////////////////////////////////////////////////////////////////


static int push_ksvnra_fail(lua_State* L, svn_ra_t* p_ra);
static int klua_ksvnra_txdelta_file(lua_State* L);


static klua_ksvnra_t* new_klua_ksvnra(lua_State* L)
{
    klua_ksvnra_t* p_obj = (klua_ksvnra_t*)lua_newuserdata(L, sizeof(klua_ksvnra_t));

    KLB_MEMSET(p_obj, 0, sizeof(klua_ksvnra_t));
    luaL_setmetatable(L, KLUA_KSVNRA_HANDLE);

    return p_obj;
}


static klua_ksvnra_t* to_klua_ksvnra(lua_State* L, int index)
{
    klua_ksvnra_t* p_obj = (klua_ksvnra_t*)luaL_checkudata(L, index, KLUA_KSVNRA_HANDLE);

    luaL_argcheck(L, NULL != p_obj, index, "'ksvnra' expected");

    return p_obj;
}


static void close_klua_ksvnra(klua_ksvnra_t* p_obj)
{
    if (NULL == p_obj)
    {
        return;
    }

    if (NULL != p_obj->p_ra)
    {
        svn_ra_close(p_obj->p_ra);
        p_obj->p_ra = NULL;
    }
}


static int klua_ksvnra_gc(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);

    close_klua_ksvnra(p_obj);
    return 0;
}


static int klua_ksvnra_close(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);

    close_klua_ksvnra(p_obj);
    return 0;
}


static int klua_ksvnra_youngest(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    int64_t rev = 0;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_youngest(p_obj->p_ra, &rev))
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra youngest failed");
        return 2;
    }

    lua_pushinteger(L, (lua_Integer)rev);
    return 1;
}


static int klua_ksvnra_uuid(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    char buf[80];

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_uuid(p_obj->p_ra, buf, (int)sizeof(buf)))
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra uuid failed");
        return 2;
    }

    lua_pushstring(L, buf);
    return 1;
}


static void push_dirent(lua_State* L, const svn_ra_dirent_t* p_e)
{
    lua_createtable(L, 0, 6);
    lua_pushstring(L, p_e->name);
    lua_setfield(L, -2, "name");
    lua_pushstring(L, p_e->kind);
    lua_setfield(L, -2, "kind");
    lua_pushinteger(L, (lua_Integer)p_e->size);
    lua_setfield(L, -2, "size");
    lua_pushinteger(L, (lua_Integer)p_e->created_rev);
    lua_setfield(L, -2, "created_rev");
    lua_pushstring(L, p_e->date);
    lua_setfield(L, -2, "date");
    lua_pushstring(L, p_e->author);
    lua_setfield(L, -2, "author");
}


static int klua_ksvnra_list(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    const char* p_path = luaL_optstring(L, 2, "");
    int64_t rev = (int64_t)luaL_optinteger(L, 3, -1);
    svn_ra_dirent_t* p_ents = NULL;
    int count = 0;
    int i = 0;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_list(p_obj->p_ra, p_path, rev, &p_ents, &count))
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra list failed");
        return 2;
    }

    lua_createtable(L, count, 0);
    for (i = 0; i < count; ++i)
    {
        push_dirent(L, &p_ents[i]);
        lua_rawseti(L, -2, i + 1);
    }

    KLB_FREE(p_ents);
    return 1;
}


static int klua_ksvnra_list_r(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    const char* p_path = luaL_optstring(L, 2, "");
    int64_t rev = (int64_t)luaL_optinteger(L, 3, -1);
    svn_ra_dirent_t* p_ents = NULL;
    int count = 0;
    int i = 0;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_list_r(p_obj->p_ra, p_path, rev, &p_ents, &count))
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra list_r failed");
        return 2;
    }

    lua_createtable(L, count, 0);
    for (i = 0; i < count; ++i)
    {
        push_dirent(L, &p_ents[i]);
        lua_rawseti(L, -2, i + 1);
    }

    KLB_FREE(p_ents);
    return 1;
}


static int klua_ksvnra_dir_props(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    const char* p_path = luaL_optstring(L, 2, "");
    int64_t rev = (int64_t)luaL_optinteger(L, 3, -1);
    svn_ra_prop_t* p_props = NULL;
    int count = 0;
    int i = 0;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_get_dir_props(p_obj->p_ra, p_path, rev, &p_props, &count))
    {
        return push_ksvnra_fail(L, p_obj->p_ra);
    }

    lua_createtable(L, count, 0);
    for (i = 0; i < count; ++i)
    {
        lua_createtable(L, 0, 2);
        lua_pushstring(L, p_props[i].name);
        lua_setfield(L, -2, "name");
        if (NULL != p_props[i].p_value)
        {
            lua_pushlstring(L, p_props[i].p_value, (size_t)p_props[i].value_len);
        }
        else
        {
            lua_pushliteral(L, "");
        }
        lua_setfield(L, -2, "value");
        lua_rawseti(L, -2, i + 1);
    }

    svn_ra_props_free(p_props, count);
    return 1;
}


/// @brief ra:mergeinfo(path[, rev[, inherited]]) -> text | nil, err
/// @note text 为空串表示该路径无 svn:mergeinfo
static int klua_ksvnra_mergeinfo(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    const char* p_path = luaL_optstring(L, 2, "");
    int64_t rev = (int64_t)luaL_optinteger(L, 3, -1);
    bool inherited = (0 != lua_toboolean(L, 4));
    char* p_text = NULL;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_get_mergeinfo(p_obj->p_ra, p_path, rev, inherited, &p_text))
    {
        return push_ksvnra_fail(L, p_obj->p_ra);
    }

    lua_pushstring(L, (NULL != p_text) ? p_text : "");
    KLB_FREE(p_text);
    return 1;
}


/// @brief ra:location_segments(path[, peg[, start[, end]]]) -> { {range_start, range_end, path?} ... } | nil, err
/// @note path 缺省表示该区间内路径不存在; 路径为无前导 '/' 的仓内相对路径
static int klua_ksvnra_location_segments(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    const char* p_path = luaL_optstring(L, 2, "");
    int64_t peg_rev = (int64_t)luaL_optinteger(L, 3, -1);
    int64_t start_rev = (int64_t)luaL_optinteger(L, 4, -1);
    int64_t end_rev = (int64_t)luaL_optinteger(L, 5, -1);
    svn_ra_locseg_t* p_segs = NULL;
    int count = 0;
    int i = 0;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_get_location_segments(p_obj->p_ra, p_path, peg_rev,
                                          start_rev, end_rev, &p_segs, &count))
    {
        return push_ksvnra_fail(L, p_obj->p_ra);
    }

    lua_createtable(L, count, 0);
    for (i = 0; i < count; ++i)
    {
        lua_createtable(L, 0, 3);
        lua_pushinteger(L, (lua_Integer)p_segs[i].range_start);
        lua_setfield(L, -2, "range_start");
        lua_pushinteger(L, (lua_Integer)p_segs[i].range_end);
        lua_setfield(L, -2, "range_end");
        if (NULL != p_segs[i].p_path)
        {
            lua_pushstring(L, p_segs[i].p_path);
            lua_setfield(L, -2, "path");
        }
        lua_rawseti(L, -2, i + 1);
    }

    svn_ra_locsegs_free(p_segs, count);
    return 1;
}


/// @brief replay 编辑器事件压栈为 Lua 表
/// @note 字段: type/rev/has_rev/path/token/parent_token/name/value/checksum/
///   \n copy_path/data (data 为原始 svndiff 字节)
static void push_edit_evt(lua_State* L, const svn_ra_edit_evt_t* p_evt)
{
    lua_createtable(L, 0, 12);
    lua_pushinteger(L, p_evt->type);
    lua_setfield(L, -2, "type");
    lua_pushinteger(L, (lua_Integer)p_evt->rev);
    lua_setfield(L, -2, "rev");
    lua_pushboolean(L, (0 != p_evt->has_rev));
    lua_setfield(L, -2, "has_rev");
    if (NULL != p_evt->p_path)
    {
        lua_pushstring(L, p_evt->p_path);
        lua_setfield(L, -2, "path");
    }
    if (NULL != p_evt->p_token)
    {
        lua_pushstring(L, p_evt->p_token);
        lua_setfield(L, -2, "token");
    }
    if (NULL != p_evt->p_parent_token)
    {
        lua_pushstring(L, p_evt->p_parent_token);
        lua_setfield(L, -2, "parent_token");
    }
    if (NULL != p_evt->p_name)
    {
        lua_pushstring(L, p_evt->p_name);
        lua_setfield(L, -2, "name");
    }
    if (NULL != p_evt->p_value)
    {
        lua_pushstring(L, p_evt->p_value);
        lua_setfield(L, -2, "value");
    }
    if (NULL != p_evt->p_checksum)
    {
        lua_pushstring(L, p_evt->p_checksum);
        lua_setfield(L, -2, "checksum");
    }
    if (NULL != p_evt->p_copy_path)
    {
        lua_pushstring(L, p_evt->p_copy_path);
        lua_setfield(L, -2, "copy_path");
    }
    if (0 < p_evt->chunk_len)
    {
        lua_pushlstring(L, p_evt->p_chunk, (size_t)p_evt->chunk_len);
        lua_setfield(L, -2, "data");
    }
}


/// @brief ra:replay_begin(rev[, low[, send_deltas]]) -> true | nil, code, msg
/// @note 批量接口: 之后用 replay_next 分批取事件, 批次之间可安全 yield
///   \n (C 回调内调用 co_chunk 会触发 across C-call boundary 错误)
static int klua_ksvnra_replay_begin(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    int64_t rev = (int64_t)luaL_checkinteger(L, 2);
    int64_t low = (int64_t)luaL_optinteger(L, 3, 0);
    bool send_deltas = (0 != lua_toboolean(L, 4));

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_replay_begin(p_obj->p_ra, rev, low, send_deltas))
    {
        return push_ksvnra_fail(L, p_obj->p_ra);
    }

    lua_pushboolean(L, 1);
    return 1;
}


/// @brief ra:replay_next([max_events]) -> events, done | nil, code, msg
/// @note 最多取 max_events 条 (默认 64, 至少 1); done=true 表示 replay 已收尾
static int klua_ksvnra_replay_next(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    lua_Integer max_events = luaL_optinteger(L, 2, 64);
    bool done = false;
    int count = 0;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (max_events < 1)
    {
        max_events = 1;
    }

    lua_createtable(L, (int)max_events, 0);

    while (!done && count < (int)max_events)
    {
        svn_ra_edit_evt_t evt = { 0 };

        if (0 != svn_ra_replay_next(p_obj->p_ra, &evt, &done))
        {
            svn_ra_edit_evt_free(&evt);
            lua_settop(L, 1);
            return push_ksvnra_fail(L, p_obj->p_ra);
        }

        if (done)
        {
            svn_ra_edit_evt_free(&evt);
            break;
        }

        push_edit_evt(L, &evt);
        lua_rawseti(L, -2, count + 1);
        ++count;
        svn_ra_edit_evt_free(&evt);
    }

    lua_pushboolean(L, done);
    return 2;
}


static int klua_ksvnra_cat(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    const char* p_path = luaL_checkstring(L, 2);
    int64_t rev = (int64_t)luaL_optinteger(L, 3, -1);
    uint8_t* p_data = NULL;
    int len = 0;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_cat(p_obj->p_ra, p_path, rev, &p_data, &len))
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra cat failed");
        return 2;
    }

    lua_pushlstring(L, (const char*)p_data, (size_t)((0 < len) ? len : 0));
    KLB_FREE(p_data);
    return 1;
}


/// @brief ra:cat_open(path, rev?) -> rc, msg
/// @note 流式取文件; 成功后用 cat_next 逐块读取, 末尾必须 cat_close
static int klua_ksvnra_cat_open(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    const char* p_path = luaL_checkstring(L, 2);
    int64_t rev = (int64_t)luaL_optinteger(L, 3, -1);

    if (NULL == p_obj->p_ra)
    {
        lua_pushinteger(L, -1);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_cat_begin(p_obj->p_ra, p_path, rev))
    {
        lua_pushinteger(L, -1);
        lua_pushstring(L, "ksvnra cat_open failed");
        return 2;
    }

    lua_pushinteger(L, 0);
    lua_pushstring(L, "OK");
    return 2;
}


/// @brief ra:cat_next() -> rc, chunk, msg
/// @note rc==0 且 chunk=="" 表示正文结束; chunk 为二进制安全字符串
static int klua_ksvnra_cat_next(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    uint8_t* p_chunk = NULL;
    int chunk_len = 0;
    bool eof = false;

    if (NULL == p_obj->p_ra)
    {
        lua_pushinteger(L, -1);
        lua_pushstring(L, "");
        lua_pushstring(L, "ksvnra closed");
        return 3;
    }

    if (0 != svn_ra_cat_next(p_obj->p_ra, &p_chunk, &chunk_len, &eof))
    {
        lua_pushinteger(L, -1);
        lua_pushstring(L, "");
        lua_pushstring(L, "ksvnra cat_next failed");
        return 3;
    }

    if (eof)
    {
        lua_pushinteger(L, 0);
        lua_pushstring(L, "");
        lua_pushstring(L, "OK");
        return 3;
    }

    lua_pushinteger(L, 0);
    lua_pushlstring(L, (const char*)p_chunk, (size_t)chunk_len);
    lua_pushstring(L, "OK");
    KLB_FREE(p_chunk);
    return 3;
}


/// @brief ra:cat_close() -> rc, msg
static int klua_ksvnra_cat_close(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);

    if (NULL == p_obj->p_ra)
    {
        lua_pushinteger(L, -1);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_cat_close(p_obj->p_ra))
    {
        lua_pushinteger(L, -1);
        lua_pushstring(L, "ksvnra cat_close failed");
        return 2;
    }

    lua_pushinteger(L, 0);
    lua_pushstring(L, "OK");
    return 2;
}


/// @brief ra:log({ paths?, start?, end?, limit?, changed_paths? })
///        -> { {rev,author,date,message,changed_paths?}, ... }
static int klua_ksvnra_log(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    char** pp_paths = NULL;
    int path_count = 0;
    int64_t start_rev = -1;
    int64_t end_rev = -1;
    int limit = 0;
    int changed_paths = 0;
    svn_ra_log_entry_t* p_ents = NULL;
    int count = 0;
    int i = 0;
    int have_paths_tbl = 0;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (LUA_TTABLE == lua_type(L, 2))
    {
        lua_getfield(L, 2, "start");
        if (LUA_TNUMBER == lua_type(L, -1))
        {
            start_rev = (int64_t)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "end");
        if (LUA_TNUMBER == lua_type(L, -1))
        {
            end_rev = (int64_t)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "limit");
        if (LUA_TNUMBER == lua_type(L, -1))
        {
            limit = (int)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "changed_paths");
        if (LUA_TBOOLEAN == lua_type(L, -1))
        {
            changed_paths = lua_toboolean(L, -1) ? 1 : 0;
        }
        lua_pop(L, 1);

        // paths 表留在栈上, 保证字符串指针在 svn_ra_log 期间有效
        lua_getfield(L, 2, "paths");
        if (LUA_TTABLE == lua_type(L, -1))
        {
            path_count = (int)lua_rawlen(L, -1);
            if (path_count > 0)
            {
                pp_paths = (char**)KLB_MALLOC(char*, path_count, 0);
                for (i = 0; i < path_count; ++i)
                {
                    lua_rawgeti(L, -1, i + 1);
                    pp_paths[i] = (char*)luaL_optstring(L, -1, "");
                    lua_pop(L, 1);
                }
            }
            have_paths_tbl = 1;
        }
        else
        {
            lua_pop(L, 1);
        }
    }

    if (0 != svn_ra_log(p_obj->p_ra, (const char**)pp_paths, path_count, start_rev, end_rev,
                        limit, changed_paths, &p_ents, &count))
    {
        KLB_FREE(pp_paths);
        if (0 != have_paths_tbl)
        {
            lua_pop(L, 1);
        }
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra log failed");
        return 2;
    }

    KLB_FREE(pp_paths);
    if (0 != have_paths_tbl)
    {
        lua_pop(L, 1);
    }

    lua_createtable(L, count, 0);
    for (i = 0; i < count; ++i)
    {
        int j = 0;

        lua_createtable(L, 0, 5);
        lua_pushinteger(L, (lua_Integer)p_ents[i].rev);
        lua_setfield(L, -2, "rev");
        lua_pushstring(L, p_ents[i].author);
        lua_setfield(L, -2, "author");
        lua_pushstring(L, p_ents[i].date);
        lua_setfield(L, -2, "date");
        lua_pushstring(L, p_ents[i].message);
        lua_setfield(L, -2, "message");

        if (NULL != p_ents[i].p_changed_paths && p_ents[i].changed_path_count > 0)
        {
            lua_createtable(L, p_ents[i].changed_path_count, 0);
            for (j = 0; j < p_ents[i].changed_path_count; ++j)
            {
                const svn_ra_log_changed_path_t* p_cp = &p_ents[i].p_changed_paths[j];
                char action[2] = { 0 };

                action[0] = p_cp->action;
                lua_createtable(L, 0, 7);
                lua_pushstring(L, p_cp->path);
                lua_setfield(L, -2, "path");
                lua_pushstring(L, action);
                lua_setfield(L, -2, "action");
                lua_pushstring(L, p_cp->copyfrom_path);
                lua_setfield(L, -2, "copyfrom_path");
                lua_pushinteger(L, (lua_Integer)p_cp->copyfrom_rev);
                lua_setfield(L, -2, "copyfrom_rev");
                lua_pushstring(L, p_cp->node_kind);
                lua_setfield(L, -2, "node_kind");
                lua_pushboolean(L, (p_cp->text_mods > 0) ? 1 : 0);
                lua_setfield(L, -2, "text_mods");
                lua_pushboolean(L, (p_cp->prop_mods > 0) ? 1 : 0);
                lua_setfield(L, -2, "prop_mods");
                lua_rawseti(L, -2, j + 1);
            }
            lua_setfield(L, -2, "changed_paths");
        }

        lua_rawseti(L, -2, i + 1);
    }

    svn_ra_log_entries_free(p_ents, count);
    return 1;
}


static void klua_ksvnra_commit_ops_free(svn_ra_commit_op_t* p_ops, int op_count)
{
    int i = 0;

    if (NULL == p_ops)
    {
        return;
    }

    for (i = 0; i < op_count; ++i)
    {
        void* p_path = (void*)p_ops[i].p_path;
        void* p_data = (void*)p_ops[i].p_data;
        void* p_copyfrom = (void*)p_ops[i].p_copyfrom_path;
        void* p_prop_name = (void*)p_ops[i].p_prop_name;
        void* p_prop_value = (void*)p_ops[i].p_prop_value;

        p_ops[i].p_path = NULL;
        p_ops[i].p_data = NULL;
        p_ops[i].p_copyfrom_path = NULL;
        p_ops[i].p_prop_name = NULL;
        p_ops[i].p_prop_value = NULL;
        KLB_FREE(p_path);
        KLB_FREE(p_data);
        KLB_FREE(p_copyfrom);
        KLB_FREE(p_prop_name);
        KLB_FREE(p_prop_value);
    }

    KLB_FREE(p_ops);
}


static char* klua_ksvnra_strdup(const char* p_s)
{
    size_t n = 0;
    char* p_out = NULL;

    if (NULL == p_s)
    {
        p_s = "";
    }

    n = strlen(p_s);
    p_out = (char*)KLB_MALLOC(char, n + 1, 0);
    memcpy(p_out, p_s, n + 1);
    return p_out;
}


static uint8_t* klua_ksvnra_memdup(const void* p_data, size_t n)
{
    uint8_t* p_out = NULL;

    if (0 == n)
    {
        return (uint8_t*)KLB_MALLOC(uint8_t, 1, 0);
    }

    p_out = (uint8_t*)KLB_MALLOC(uint8_t, n, 0);
    memcpy(p_out, p_data, n);
    return p_out;
}


/// @brief ra:commit({ log=, ops={ {op=,path=,...}, ... } }) -> new_rev | nil, err
static int klua_ksvnra_commit(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    char* p_log = NULL;
    svn_ra_commit_op_t* p_ops = NULL;
    int op_count = 0;
    int i = 0;
    int64_t new_rev = 0;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    luaL_checktype(L, 2, LUA_TTABLE);

    lua_getfield(L, 2, "log");
    if (LUA_TSTRING == lua_type(L, -1))
    {
        p_log = klua_ksvnra_strdup(lua_tostring(L, -1));
    }
    else
    {
        p_log = klua_ksvnra_strdup("");
    }
    lua_pop(L, 1);

    lua_getfield(L, 2, "ops");
    luaL_checktype(L, -1, LUA_TTABLE);
    op_count = (int)lua_rawlen(L, -1);
    if (op_count <= 0)
    {
        lua_pop(L, 1);
        KLB_FREE(p_log);
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra commit: empty ops");
        return 2;
    }

    p_ops = (svn_ra_commit_op_t*)KLB_MALLOCZ(svn_ra_commit_op_t, (size_t)op_count, 0);

    for (i = 0; i < op_count; ++i)
    {
        const char* p_opname = NULL;
        svn_ra_commit_op_t* p_op = &p_ops[i];
        size_t data_len = 0;

        lua_rawgeti(L, -1, i + 1);
        luaL_checktype(L, -1, LUA_TTABLE);

        lua_getfield(L, -1, "op");
        p_opname = luaL_checkstring(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "path");
        p_op->p_path = klua_ksvnra_strdup(luaL_checkstring(L, -1));
        lua_pop(L, 1);

        p_op->base_rev = -1;
        lua_getfield(L, -1, "rev");
        if (LUA_TNUMBER == lua_type(L, -1))
        {
            p_op->base_rev = (int64_t)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);

        if (0 == strcmp(p_opname, "mkdir"))
        {
            p_op->type = SVN_RA_OP_MKDIR;
        }
        else if (0 == strcmp(p_opname, "delete"))
        {
            p_op->type = SVN_RA_OP_DELETE;
        }
        else if (0 == strcmp(p_opname, "put"))
        {
            const char* p_blob = NULL;

            p_op->type = SVN_RA_OP_PUT;
            p_op->is_add = 1;

            lua_getfield(L, -1, "add");
            if (LUA_TBOOLEAN == lua_type(L, -1))
            {
                p_op->is_add = lua_toboolean(L, -1) ? 1 : 0;
            }
            lua_pop(L, 1);

            lua_getfield(L, -1, "svndiff");
            if (LUA_TSTRING == lua_type(L, -1))
            {
                p_blob = lua_tolstring(L, -1, &data_len);
                p_op->data_is_svndiff = 1;
            }
            else
            {
                lua_pop(L, 1);
                lua_getfield(L, -1, "data");
                if (LUA_TSTRING != lua_type(L, -1))
                {
                    lua_pop(L, 2);
                    klua_ksvnra_commit_ops_free(p_ops, op_count);
                    KLB_FREE(p_log);
                    lua_pushnil(L);
                    lua_pushstring(L, "ksvnra commit put: need data or svndiff");
                    return 2;
                }
                p_blob = lua_tolstring(L, -1, &data_len);
                p_op->data_is_svndiff = 0;
            }

            p_op->p_data = klua_ksvnra_memdup(p_blob, data_len);
            p_op->data_len = (int)data_len;
            lua_pop(L, 1);
        }
        else if (0 == strcmp(p_opname, "copy"))
        {
            const char* p_kind = NULL;

            p_op->type = SVN_RA_OP_COPY;
            p_op->copyfrom_rev = -1;

            lua_getfield(L, -1, "copyfrom_path");
            p_op->p_copyfrom_path = klua_ksvnra_strdup(luaL_checkstring(L, -1));
            lua_pop(L, 1);

            lua_getfield(L, -1, "copyfrom_rev");
            if (LUA_TNUMBER == lua_type(L, -1))
            {
                p_op->copyfrom_rev = (int64_t)lua_tointeger(L, -1);
            }
            lua_pop(L, 1);

            lua_getfield(L, -1, "kind");
            p_kind = luaL_optstring(L, -1, "file");
            p_op->copy_is_dir = (0 == strcmp(p_kind, "dir")) ? 1 : 0;
            lua_pop(L, 1);
        }
        else if (0 == strcmp(p_opname, "prop"))
        {
            const char* p_kind = NULL;

            p_op->type = SVN_RA_OP_PROP;
            p_op->prop_is_dir = 0;

            lua_getfield(L, -1, "name");
            p_op->p_prop_name = klua_ksvnra_strdup(luaL_checkstring(L, -1));
            lua_pop(L, 1);

            lua_getfield(L, -1, "value");
            if (LUA_TSTRING == lua_type(L, -1))
            {
                p_op->p_prop_value = klua_ksvnra_strdup(lua_tostring(L, -1));
            }
            else
            {
                p_op->p_prop_value = NULL;
            }
            lua_pop(L, 1);

            lua_getfield(L, -1, "kind");
            p_kind = luaL_optstring(L, -1, "file");
            p_op->prop_is_dir = (0 == strcmp(p_kind, "dir")) ? 1 : 0;
            lua_pop(L, 1);
        }
        else
        {
            lua_pop(L, 2);
            klua_ksvnra_commit_ops_free(p_ops, op_count);
            KLB_FREE(p_log);
            lua_pushnil(L);
            lua_pushfstring(L, "ksvnra commit: bad op %s", p_opname);
            return 2;
        }

        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (0 != svn_ra_commit(p_obj->p_ra, p_log, p_ops, op_count, &new_rev))
    {
        klua_ksvnra_commit_ops_free(p_ops, op_count);
        KLB_FREE(p_log);
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra commit failed");
        return 2;
    }

    klua_ksvnra_commit_ops_free(p_ops, op_count);
    KLB_FREE(p_log);
    lua_pushinteger(L, (lua_Integer)new_rev);
    return 1;
}


/// @brief 失败返回累加: nil, code, msg
static int push_ksvnra_fail(lua_State* L, svn_ra_t* p_ra)
{
    int64_t code = 0;
    char msg[512];

    msg[0] = '\0';
    if (NULL != p_ra)
    {
        svn_ra_last_error(p_ra, &code, msg, (int)sizeof(msg));
    }

    lua_pushnil(L);
    lua_pushinteger(L, (lua_Integer)code);
    lua_pushstring(L, msg);
    return 3;
}


/// @brief ra:last_error() -> code, msg
static int klua_ksvnra_last_error(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    int64_t code = 0;
    char msg[512];

    msg[0] = '\0';
    if (NULL != p_obj->p_ra)
    {
        svn_ra_last_error(p_obj->p_ra, &code, msg, (int)sizeof(msg));
    }

    lua_pushinteger(L, (lua_Integer)code);
    lua_pushstring(L, msg);
    return 2;
}


/// @brief ra:dated_rev(date) -> rev | nil, code, msg
static int klua_ksvnra_dated_rev(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    const char* p_date = luaL_checkstring(L, 2);
    int64_t rev = -1;

    if (NULL == p_obj->p_ra)
    {
        return push_ksvnra_fail(L, NULL);
    }

    if (0 != svn_ra_get_dated_rev(p_obj->p_ra, p_date, &rev))
    {
        return push_ksvnra_fail(L, p_obj->p_ra);
    }

    lua_pushinteger(L, (lua_Integer)rev);
    return 1;
}


/// @brief ra:deleted_rev(path, peg_rev[, end_rev]) -> rev | nil, code, msg
static int klua_ksvnra_deleted_rev(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    const char* p_path = luaL_checkstring(L, 2);
    int64_t peg_rev = (int64_t)luaL_optinteger(L, 3, -1);
    int64_t end_rev = (int64_t)luaL_optinteger(L, 4, -1);
    int64_t del_rev = -1;

    if (NULL == p_obj->p_ra)
    {
        return push_ksvnra_fail(L, NULL);
    }

    if (0 != svn_ra_get_deleted_rev(p_obj->p_ra, p_path, peg_rev, end_rev, &del_rev))
    {
        return push_ksvnra_fail(L, p_obj->p_ra);
    }

    lua_pushinteger(L, (lua_Integer)del_rev);
    return 1;
}


static void klua_ksvnra_createmeta(lua_State* L)
{
    static luaL_Reg meta[] =
    {
        { "youngest",   klua_ksvnra_youngest },
        { "uuid",       klua_ksvnra_uuid },
        { "list",       klua_ksvnra_list },
        { "list_r",     klua_ksvnra_list_r },
        { "dir_props",  klua_ksvnra_dir_props },
        { "mergeinfo",  klua_ksvnra_mergeinfo },
        { "location_segments", klua_ksvnra_location_segments },
        { "replay_begin", klua_ksvnra_replay_begin },
        { "replay_next", klua_ksvnra_replay_next },
        { "cat",        klua_ksvnra_cat },
        { "txdelta_file", klua_ksvnra_txdelta_file },
        { "cat_open",   klua_ksvnra_cat_open },
        { "cat_next",   klua_ksvnra_cat_next },
        { "cat_close",  klua_ksvnra_cat_close },
        { "log",        klua_ksvnra_log },
        { "commit",     klua_ksvnra_commit },
        { "last_error", klua_ksvnra_last_error },
        { "dated_rev",  klua_ksvnra_dated_rev },
        { "deleted_rev", klua_ksvnra_deleted_rev },
        { "close",      klua_ksvnra_close },
        { "__gc",       klua_ksvnra_gc },
        { NULL,         NULL }
    };

    luaL_newmetatable(L, KLUA_KSVNRA_HANDLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta, 0);
    lua_pop(L, 1);
}


static int klua_ksvnra_svndiff_apply(lua_State* L)
{
    size_t old_len = 0;
    size_t diff_len = 0;
    const char* p_old = luaL_optlstring(L, 1, "", &old_len);
    const char* p_diff = luaL_checklstring(L, 2, &diff_len);
    uint8_t* p_plain = NULL;
    int plain_len = 0;

    if (!svn_ra_svndiff_apply((const uint8_t*)p_old, (int)old_len,
            (const uint8_t*)p_diff, (int)diff_len, &p_plain, &plain_len))
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra svndiff_apply failed");
        return 2;
    }

    if (plain_len > 0 && NULL != p_plain)
    {
        lua_pushlstring(L, (const char*)p_plain, (size_t)plain_len);
        KLB_FREE(p_plain);
        return 1;
    }

    KLB_FREE(p_plain);
    lua_pushliteral(L, "");
    return 1;
}


/// @brief ksvnra.sha1_hex(data) -> hex | nil, err
static int klua_ksvnra_sha1_hex(lua_State* L)
{
    size_t data_len = 0;
    const char* p_data = luaL_optlstring(L, 1, "", &data_len);
    char out_hex[41] = { 0 };

    svn_ra_sha1_hex((const uint8_t*)p_data, (int)data_len, out_hex);
    lua_pushstring(L, out_hex);
    return 1;
}


/// @brief ksvnra.md5_hex(data) -> hex | nil, err
static int klua_ksvnra_md5_hex(lua_State* L)
{
    size_t data_len = 0;
    const char* p_data = luaL_optlstring(L, 1, "", &data_len);
    char out_hex[33] = { 0 };

    svn_ra_md5_hex((const uint8_t*)p_data, (int)data_len, out_hex);
    lua_pushstring(L, out_hex);
    return 1;
}


/// @brief ksvnra.svndiff_new(data) -> svndiff | nil, err
static int klua_ksvnra_svndiff_new(lua_State* L)
{
    size_t data_len = 0;
    const char* p_data = luaL_optlstring(L, 1, "", &data_len);
    svn_buf_t diff = { 0 };

    if (!svn_ra_make_svndiff0_new((const uint8_t*)p_data, (int)data_len, &diff))
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra svndiff_new failed");
        return 2;
    }

    if (diff.len > 0 && NULL != diff.p_data)
    {
        lua_pushlstring(L, diff.p_data, (size_t)diff.len);
    }
    else
    {
        lua_pushliteral(L, "");
    }
    svn_buf_free(&diff);
    return 1;
}


/// @brief ksvnra.svndiff_replace(old, new) -> svndiff | nil (内容未变化)
static int klua_ksvnra_svndiff_replace(lua_State* L)
{
    size_t old_len = 0;
    size_t new_len = 0;
    const char* p_old = luaL_optlstring(L, 1, "", &old_len);
    const char* p_new = luaL_optlstring(L, 2, "", &new_len);
    svn_buf_t diff = { 0 };

    if (!svn_ra_make_svndiff0_replace((const uint8_t*)p_old, (int)old_len,
            (const uint8_t*)p_new, (int)new_len, &diff))
    {
        lua_pushnil(L);
        return 1;
    }

    if (diff.len > 0 && NULL != diff.p_data)
    {
        lua_pushlstring(L, diff.p_data, (size_t)diff.len);
    }
    else
    {
        lua_pushliteral(L, "");
    }
    svn_buf_free(&diff);
    return 1;
}


/// @brief base64 编码并入栈 (空串压 ""; 不区分失败返回)
/// @param [in] p_data  原始字节; 可为 NULL
/// @param [in] data_len 字节数
/// @return true 成功; false 内存不足
static bool push_b64_lstring(lua_State* L, const char* p_data, size_t data_len)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t out_len = 0;
    char* p_out = NULL;
    size_t i = 0;
    size_t o = 0;

    if (0 == data_len || NULL == p_data)
    {
        lua_pushliteral(L, "");
        return true;
    }

    out_len = ((data_len + 2) / 3) * 4;
    p_out = KLB_MALLOC(char, out_len + 1, 0);
    if (NULL == p_out)
    {
        return false;
    }

    while (i < data_len)
    {
        size_t remain = data_len - i;
        uint8_t a = (uint8_t)p_data[i];
        uint8_t b = (remain > 1) ? (uint8_t)p_data[i + 1] : 0;
        uint8_t c = (remain > 2) ? (uint8_t)p_data[i + 2] : 0;

        p_out[o++] = b64[a >> 2];
        p_out[o++] = b64[((a & 0x03) << 4) | (b >> 4)];
        if (remain > 1)
        {
            p_out[o++] = b64[((b & 0x0f) << 2) | (c >> 6)];
        }
        else
        {
            p_out[o++] = '=';
        }
        if (remain > 2)
        {
            p_out[o++] = b64[c & 0x3f];
        }
        else
        {
            p_out[o++] = '=';
        }

        i += 3;
    }

    p_out[o] = '\0';
    lua_pushlstring(L, p_out, o);
    KLB_FREE(p_out);
    return true;
}


/// @brief ksvnra.b64enc(data) -> base64 (replay 属性值/文本增量用)
static int klua_ksvnra_b64enc(lua_State* L)
{
    size_t data_len = 0;
    const char* p_data = luaL_optlstring(L, 1, "", &data_len);

    if (!push_b64_lstring(L, p_data, data_len))
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra b64enc: out of memory");
        return 2;
    }

    return 1;
}


/// @brief ra:txdelta_file(path, rev[, old_rev[, max_bytes]]) -> b64, sha1, md5 | nil, err
/// @note 取文/建 svndiff/算校验和/编码 base64 全在 C 侧完成, 大树 update 不再逐次跨界传大串
///   \n old_rev < 0 为 add-file (全新 svndiff); >= 0 为 open-file (与 old_rev 正文比对)
///   \n max_bytes > 0 时超限返回 nil, "too_large"; 旧文超限按空源处理
static int klua_ksvnra_txdelta_file(lua_State* L)
{
    klua_ksvnra_t* p_obj = to_klua_ksvnra(L, 1);
    const char* p_path = luaL_checkstring(L, 2);
    int64_t rev = (int64_t)luaL_checkinteger(L, 3);
    int64_t old_rev = (int64_t)luaL_optinteger(L, 4, -1);
    lua_Integer max_bytes = luaL_optinteger(L, 5, 0);
    uint8_t* p_new = NULL;
    int new_len = 0;
    uint8_t* p_old = NULL;
    int old_len = 0;
    svn_buf_t diff = { 0 };
    char sha1_hex[41] = { 0 };
    char md5_hex[33] = { 0 };
    bool made = false;

    if (NULL == p_obj->p_ra)
    {
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra closed");
        return 2;
    }

    if (0 != svn_ra_cat(p_obj->p_ra, p_path, rev, &p_new, &new_len))
    {
        return push_ksvnra_fail(L, p_obj->p_ra);
    }

    if (max_bytes > 0 && new_len > (int)max_bytes)
    {
        KLB_FREE(p_new);
        lua_pushnil(L);
        lua_pushstring(L, "too_large");
        return 2;
    }

    if (old_rev >= 0)
    {
        if (0 == svn_ra_cat(p_obj->p_ra, p_path, old_rev, &p_old, &old_len))
        {
            if (max_bytes > 0 && old_len > (int)max_bytes)
            {
                // 旧文超限: 按空源重建全量窗, 语义同 _safe_cat 的 too_large
                KLB_FREE(p_old);
                p_old = NULL;
                old_len = 0;
            }
        }
    }

    if (old_rev >= 0)
    {
        made = svn_ra_make_svndiff0_replace(p_old, old_len, p_new, new_len, &diff);
    }
    else
    {
        made = svn_ra_make_svndiff0_new(p_new, new_len, &diff);
    }

    svn_ra_sha1_hex(p_new, new_len, sha1_hex);
    svn_ra_md5_hex(p_new, new_len, md5_hex);
    KLB_FREE(p_old);
    KLB_FREE(p_new);

    if (!made)
    {
        svn_buf_free(&diff);
        if (old_rev >= 0)
        {
            // 内容未变化: 空 delta, 校验和照常下发
            lua_pushliteral(L, "");
            lua_pushstring(L, sha1_hex);
            lua_pushstring(L, md5_hex);
            return 3;
        }

        lua_pushnil(L);
        lua_pushstring(L, "ksvnra txdelta_file failed");
        return 2;
    }

    if (!push_b64_lstring(L, diff.p_data, (size_t)diff.len))
    {
        svn_buf_free(&diff);
        lua_pushnil(L);
        lua_pushstring(L, "ksvnra txdelta_file: out of memory");
        return 2;
    }
    svn_buf_free(&diff);

    lua_pushstring(L, sha1_hex);
    lua_pushstring(L, md5_hex);
    return 3;
}


static int klua_ksvnra_open(lua_State* L)
{
    const char* p_hostport = NULL;
    const char* p_user = NULL;
    const char* p_pass = NULL;
    const char* p_repo = NULL;
    svn_ra_t* p_ra = NULL;
    klua_ksvnra_t* p_obj = NULL;

    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "hostport");
    p_hostport = luaL_optstring(L, -1, "127.0.0.1:3690");
    lua_pop(L, 1);

    lua_getfield(L, 1, "user");
    p_user = luaL_optstring(L, -1, "");
    lua_pop(L, 1);

    lua_getfield(L, 1, "pass");
    p_pass = luaL_optstring(L, -1, "");
    lua_pop(L, 1);

    lua_getfield(L, 1, "repo");
    p_repo = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    p_ra = svn_ra_open(p_hostport, p_user, p_pass, p_repo);
    if (NULL == p_ra)
    {
        lua_pushnil(L);
        lua_pushfstring(L, "ksvnra open failed: hostport=[%s] repo=[%s]", p_hostport, p_repo);
        return 2;
    }

    p_obj = new_klua_ksvnra(L);
    p_obj->p_ra = p_ra;
    return 1;
}


//////////////////////////////////////////////////////////////////////////


int klua_open_ksvnra(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "open",           klua_ksvnra_open },
        { "svndiff_apply",  klua_ksvnra_svndiff_apply },
        { "svndiff_new",    klua_ksvnra_svndiff_new },
        { "svndiff_replace", klua_ksvnra_svndiff_replace },
        { "sha1_hex",       klua_ksvnra_sha1_hex },
        { "md5_hex",        klua_ksvnra_md5_hex },
        { "b64enc",         klua_ksvnra_b64enc },
        { NULL,             NULL }
    };

    klua_ksvnra_createmeta(L);
    luaL_newlib(L, lib);
    return 1;
}

// end
