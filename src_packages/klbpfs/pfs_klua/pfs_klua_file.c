// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua_path.h"
#include "pfs_klua_util.h"
#include "klbmem/klb_mem.h"
#include "klua/klua.h"

#include <string.h>

/// @brief file userdata metatable 名
#define PFS_KLUA_FILE_MT            "kpfs.file*"

/// @brief read 默认/上限字节数
#define PFS_KLUA_FILE_READ_DEF      4096
#define PFS_KLUA_FILE_READ_MAX      (1 << 20)

typedef struct pfs_klua_file_ud_t {
    pfs_file_t      *p_file;
} pfs_klua_file_ud_t;

static int pfs_klua_file_parse_open_mode(lua_State *L, int arg_idx, int *p_mode, int *p_seek_end)
{
    if (NULL == p_mode || NULL == p_seek_end)
    {
        return PFS_EINVAL;
    }

    *p_mode = PFS_O_RD;
    *p_seek_end = 0;

    if (0 != lua_isnoneornil(L, arg_idx))
    {
        return PFS_OK;
    }

    if (LUA_TNUMBER == lua_type(L, arg_idx))
    {
        *p_mode = (int)lua_tointeger(L, arg_idx);
        return PFS_OK;
    }

    if (LUA_TSTRING != lua_type(L, arg_idx))
    {
        return PFS_EINVAL;
    }

    const char *s = lua_tostring(L, arg_idx);
    if (0 == strcmp(s, "r") || 0 == strcmp(s, "rb"))
    {
        *p_mode = PFS_O_RD;
    }
    else if (0 == strcmp(s, "r+") || 0 == strcmp(s, "rb+"))
    {
        *p_mode = PFS_O_RDWR;
    }
    else if (0 == strcmp(s, "w") || 0 == strcmp(s, "wb"))
    {
        *p_mode = PFS_O_RDWR | PFS_O_CREAT;
    }
    else if (0 == strcmp(s, "w+") || 0 == strcmp(s, "wb+"))
    {
        *p_mode = PFS_O_RDWR | PFS_O_CREAT;
    }
    else if (0 == strcmp(s, "a") || 0 == strcmp(s, "ab"))
    {
        *p_mode = PFS_O_RDWR | PFS_O_CREAT;
        *p_seek_end = 1;
    }
    else if (0 == strcmp(s, "a+") || 0 == strcmp(s, "ab+"))
    {
        *p_mode = PFS_O_RDWR | PFS_O_CREAT;
        *p_seek_end = 1;
    }
    else
    {
        return PFS_EINVAL;
    }

    return PFS_OK;
}

static int pfs_klua_file_parse_whence(lua_State *L, int arg_idx, int *p_whence)
{
    if (NULL == p_whence)
    {
        return PFS_EINVAL;
    }

    if (0 != lua_isnoneornil(L, arg_idx))
    {
        *p_whence = PFS_SEEK_SET;
        return PFS_OK;
    }

    if (LUA_TNUMBER == lua_type(L, arg_idx))
    {
        *p_whence = (int)lua_tointeger(L, arg_idx);
        return PFS_OK;
    }

    if (LUA_TSTRING != lua_type(L, arg_idx))
    {
        return PFS_EINVAL;
    }

    const char *s = lua_tostring(L, arg_idx);
    if (0 == strcmp(s, "set"))
    {
        *p_whence = PFS_SEEK_SET;
    }
    else if (0 == strcmp(s, "cur"))
    {
        *p_whence = PFS_SEEK_CUR;
    }
    else if (0 == strcmp(s, "end"))
    {
        *p_whence = PFS_SEEK_END;
    }
    else
    {
        return PFS_EINVAL;
    }

    return PFS_OK;
}

static void pfs_klua_file_close_ud(pfs_klua_file_ud_t *p_ud)
{
    if (NULL != p_ud && NULL != p_ud->p_file)
    {
        pfs_fclose(p_ud->p_file);
        p_ud->p_file = NULL;
    }
}

static int pfs_klua_file_gc(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);

    pfs_klua_file_close_ud(p_ud);
    return 0;
}

static int pfs_klua_file_close(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);

    pfs_klua_file_close_ud(p_ud);
    return 0;
}

static int pfs_klua_file_error(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);

    if (NULL == p_ud->p_file)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int err = pfs_ferror(p_ud->p_file);

    return pfs_klua_push_rc(L, err);
}

static int pfs_klua_file_eof(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);

    if (NULL == p_ud->p_file)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    lua_pushboolean(L, 0 != pfs_feof(p_ud->p_file));
    return 1;
}

static int pfs_klua_file_read(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);

    if (NULL == p_ud->p_file)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    lua_Integer want_i = luaL_optinteger(L, 2, PFS_KLUA_FILE_READ_DEF);
    if (want_i < 0)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    size_t want = (size_t)want_i;
    if (want > PFS_KLUA_FILE_READ_MAX)
    {
        want = PFS_KLUA_FILE_READ_MAX;
    }
    if (0 == want)
    {
        lua_pushlstring(L, "", 0);
        return 1;
    }

    char *buf = KLB_MALLOC(char, want, 0);
    int n = pfs_fread(buf, 1, (int)want, p_ud->p_file);
    if (n < 0)
    {
        int err = pfs_ferror(p_ud->p_file);
        KLB_FREE(buf);
        return pfs_klua_push_rc(L, err);
    }

    lua_pushlstring(L, buf, (size_t)n);
    KLB_FREE(buf);
    return 1;
}

static int pfs_klua_file_write(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);

    if (NULL == p_ud->p_file)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    int n = pfs_fwrite(data, 1, (int)len, p_ud->p_file);
    if (n < 0)
    {
        int err = pfs_ferror(p_ud->p_file);
        return pfs_klua_push_rc(L, err);
    }

    lua_pushinteger(L, n);
    return 1;
}

static int pfs_klua_file_seek(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);
    int64_t offset = (int64_t)luaL_checkinteger(L, 2);

    if (NULL == p_ud->p_file)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int whence;
    int rc = pfs_klua_file_parse_whence(L, 3, &whence);
    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    rc = pfs_fseek(p_ud->p_file, offset, whence);
    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    lua_pushinteger(L, (lua_Integer)pfs_ftell(p_ud->p_file));
    return 1;
}

static int pfs_klua_file_tell(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);

    if (NULL == p_ud->p_file)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int64_t pos = pfs_ftell(p_ud->p_file);
    if (pos < 0)
    {
        return pfs_klua_push_rc(L, pfs_ferror(p_ud->p_file));
    }

    lua_pushinteger(L, (lua_Integer)pos);
    return 1;
}

static int pfs_klua_file_flush(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);

    if (NULL == p_ud->p_file)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int rc = pfs_fflush(p_ud->p_file);
    return pfs_klua_push_rc(L, rc);
}

static int pfs_klua_file_sync(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);

    if (NULL == p_ud->p_file)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int rc = pfs_fsync(p_ud->p_file);
    return pfs_klua_push_rc(L, rc);
}

static int pfs_klua_file_truncate(lua_State *L)
{
    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_FILE_MT);
    int64_t size = (int64_t)luaL_checkinteger(L, 2);

    if (NULL == p_ud->p_file)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int rc = pfs_ftruncate(p_ud->p_file, size);
    return pfs_klua_push_rc(L, rc);
}

int pfs_klua_file_open(lua_State *L)
{
    const char *drive_path = luaL_checkstring(L, 1);
    int open_mode;
    int seek_end;
    int rc = pfs_klua_file_parse_open_mode(L, 2, &open_mode, &seek_end);

    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    pfs_klua_drive_path_t parsed;
    pfs_ctx_t *p_ctx = NULL;

    rc = pfs_klua_path_resolve_drive(L, drive_path, &parsed, &p_ctx);
    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    pfs_file_t *p_file = pfs_fopen(p_ctx, parsed.vfs_path, open_mode);
    if (NULL == p_file)
    {
        return pfs_klua_push_rc(L, PFS_EIO);
    }

    if (0 != seek_end)
    {
        rc = pfs_fseek(p_file, 0, PFS_SEEK_END);
        if (PFS_OK != rc)
        {
            pfs_fclose(p_file);
            return pfs_klua_push_rc(L, rc);
        }
    }

    pfs_klua_file_ud_t *p_ud = (pfs_klua_file_ud_t *)lua_newuserdatauv(L, sizeof(*p_ud), 0);
    p_ud->p_file = p_file;
    luaL_setmetatable(L, PFS_KLUA_FILE_MT);
    return 1;
}

void pfs_klua_file_setup(lua_State *L)
{
    static const luaL_Reg s_mt[] = {
        { "read",           pfs_klua_file_read },
        { "write",          pfs_klua_file_write },
        { "seek",           pfs_klua_file_seek },
        { "tell",           pfs_klua_file_tell },
        { "close",          pfs_klua_file_close },
        { "flush",          pfs_klua_file_flush },
        { "fsync",          pfs_klua_file_sync },
        { "truncate",       pfs_klua_file_truncate },
        { "error",          pfs_klua_file_error },
        { "eof",            pfs_klua_file_eof },
        { "__gc",           pfs_klua_file_gc },
        { NULL,             NULL }
    };

    if (0 == luaL_newmetatable(L, PFS_KLUA_FILE_MT))
    {
        return;
    }

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, s_mt, 0);
    lua_pop(L, 1);
}

// end
