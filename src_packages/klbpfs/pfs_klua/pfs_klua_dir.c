// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua_path.h"
#include "pfs_klua_util.h"
#include "klua/klua.h"

/// @brief dir userdata metatable 名
#define PFS_KLUA_DIR_MT             "kpfs.dir*"

typedef struct pfs_klua_dir_ud_t {
    pfs_dir_t       *p_dir;
} pfs_klua_dir_ud_t;

static void pfs_klua_dir_close_ud(pfs_klua_dir_ud_t *p_ud)
{
    if (NULL != p_ud && NULL != p_ud->p_dir)
    {
        pfs_closedir(p_ud->p_dir);
        p_ud->p_dir = NULL;
    }
}

static int pfs_klua_dir_gc(lua_State *L)
{
    pfs_klua_dir_ud_t *p_ud = (pfs_klua_dir_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_DIR_MT);

    pfs_klua_dir_close_ud(p_ud);
    return 0;
}

static int pfs_klua_dir_close(lua_State *L)
{
    pfs_klua_dir_ud_t *p_ud = (pfs_klua_dir_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_DIR_MT);

    pfs_klua_dir_close_ud(p_ud);
    return 0;
}

static int pfs_klua_dir_error(lua_State *L)
{
    pfs_klua_dir_ud_t *p_ud = (pfs_klua_dir_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_DIR_MT);

    if (NULL == p_ud->p_dir)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int err = pfs_direrror(p_ud->p_dir);

    return pfs_klua_push_rc(L, err);
}

static int pfs_klua_dir_readdir(lua_State *L)
{
    pfs_klua_dir_ud_t *p_ud = (pfs_klua_dir_ud_t *)luaL_checkudata(L, 1, PFS_KLUA_DIR_MT);

    if (NULL == p_ud->p_dir)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    pfs_dirent_t *p_ent = pfs_readdir(p_ud->p_dir);
    if (NULL != p_ent)
    {
        lua_newtable(L);
        lua_pushstring(L, p_ent->d_name);
        lua_setfield(L, -2, "name");
        if (PFS_DT_DIR == p_ent->d_type)
        {
            lua_pushstring(L, "dir");
        }
        else if (PFS_DT_REG == p_ent->d_type)
        {
            lua_pushstring(L, "file");
        }
        else
        {
            lua_pushstring(L, "?");
        }
        lua_setfield(L, -2, "type");
        return 1;
    }

    int err = pfs_direrror(p_ud->p_dir);

    if (0 == err)
    {
        return 0;
    }

    return pfs_klua_push_rc(L, err);
}

int pfs_klua_dir_opendir(lua_State *L)
{
    const char *drive_path = luaL_checkstring(L, 1);

    pfs_klua_drive_path_t parsed;
    int rc = pfs_klua_path_parse(drive_path, &parsed);

    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    pfs_ctx_t *p_ctx = NULL;

    rc = pfs_klua_path_resolve_ctx(L, &parsed, &p_ctx);
    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    pfs_dir_t *p_dir = pfs_opendir(p_ctx, parsed.vfs_path);
    if (NULL == p_dir)
    {
        return pfs_klua_push_rc(L, PFS_EIO);
    }

    pfs_klua_dir_ud_t *p_ud = (pfs_klua_dir_ud_t *)lua_newuserdatauv(L, sizeof(*p_ud), 0);
    p_ud->p_dir = p_dir;
    luaL_setmetatable(L, PFS_KLUA_DIR_MT);
    return 1;
}

void pfs_klua_dir_setup(lua_State *L)
{
    static const luaL_Reg s_mt[] = {
        { "readdir",        pfs_klua_dir_readdir },
        { "close",          pfs_klua_dir_close },
        { "closedir",       pfs_klua_dir_close },
        { "direrror",       pfs_klua_dir_error },
        { "__gc",           pfs_klua_dir_gc },
        { NULL,             NULL }
    };

    if (0 == luaL_newmetatable(L, PFS_KLUA_DIR_MT))
    {
        return;
    }

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, s_mt, 0);
    lua_pop(L, 1);
}

// end
