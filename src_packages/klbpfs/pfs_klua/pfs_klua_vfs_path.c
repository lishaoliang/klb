// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua_path.h"
#include "pfs_klua_util.h"
#include "klbmem/klb_mem.h"
#include "klua/klua.h"

#include <stdio.h>
#include <string.h>

#define PFS_KLUA_VFS_COPY_BUF     (64 * 1024)

typedef struct pfs_klua_vfs_copy_end_t {
    int                     is_host;
    const char             *path;
    pfs_klua_drive_path_t   parsed;
    pfs_ctx_t              *p_ctx;
} pfs_klua_vfs_copy_end_t;

static int pfs_klua_vfs_copy_resolve_end(lua_State *L, const char *arg,
        pfs_klua_vfs_copy_end_t *p_end)
{
    if (NULL == arg || NULL == p_end)
    {
        return PFS_EINVAL;
    }

    memset(p_end, 0, sizeof(*p_end));
    p_end->path = arg;

    if ('@' == arg[0])
    {
        p_end->is_host = 1;
        p_end->path = arg + 1;
        if ('\0' == p_end->path[0])
        {
            return PFS_EINVAL;
        }
        return PFS_OK;
    }

    return pfs_klua_path_resolve_drive(L, arg, &p_end->parsed, &p_end->p_ctx);
}

static int pfs_klua_vfs_copy_vfs_to_vfs(pfs_ctx_t *p_src_ctx, const char *src_vfs,
        pfs_ctx_t *p_dst_ctx, const char *dst_vfs)
{
    pfs_file_t *p_src = pfs_fopen(p_src_ctx, src_vfs, PFS_O_RD);

    if (NULL == p_src)
    {
        return PFS_ENOENT;
    }

    pfs_file_t *p_dst = pfs_fopen(p_dst_ctx, dst_vfs, PFS_O_RDWR | PFS_O_CREAT);

    if (NULL == p_dst)
    {
        pfs_fclose(p_src);
        return PFS_EIO;
    }

    uint8_t *buf = (uint8_t *)KLB_MALLOC(uint8_t, PFS_KLUA_VFS_COPY_BUF, 0);

    if (NULL == buf)
    {
        pfs_fclose(p_src);
        pfs_fclose(p_dst);
        return PFS_ENOMEM;
    }

    int rc = PFS_OK;

    for (;;)
    {
        int nread = pfs_fread(buf, 1, PFS_KLUA_VFS_COPY_BUF, p_src);

        if (nread < 0)
        {
            rc = pfs_ferror(p_src);
            break;
        }
        if (0 == nread)
        {
            if (0 != pfs_ferror(p_src))
            {
                rc = pfs_ferror(p_src);
            }
            break;
        }

        int nwrite = pfs_fwrite(buf, 1, nread, p_dst);

        if (nwrite < 0)
        {
            rc = pfs_ferror(p_dst);
            break;
        }
        if (nwrite != nread)
        {
            rc = PFS_EIO;
            break;
        }
    }

    KLB_FREE(buf);
    pfs_fclose(p_src);
    pfs_fclose(p_dst);
    return rc;
}

static int pfs_klua_vfs_copy_host_to_vfs(pfs_ctx_t *p_ctx, const char *host_src,
        const char *vfs_dst)
{
    FILE *fp = fopen(host_src, "rb");

    if (NULL == fp)
    {
        return PFS_EIO;
    }

    pfs_file_t *p_dst = pfs_fopen(p_ctx, vfs_dst, PFS_O_RDWR | PFS_O_CREAT);

    if (NULL == p_dst)
    {
        fclose(fp);
        return PFS_EIO;
    }

    uint8_t *buf = (uint8_t *)KLB_MALLOC(uint8_t, PFS_KLUA_VFS_COPY_BUF, 0);

    if (NULL == buf)
    {
        fclose(fp);
        pfs_fclose(p_dst);
        return PFS_ENOMEM;
    }

    int rc = PFS_OK;

    for (;;)
    {
        size_t nread = fread(buf, 1, PFS_KLUA_VFS_COPY_BUF, fp);

        if (0 != ferror(fp))
        {
            rc = PFS_EIO;
            break;
        }
        if (0 == nread)
        {
            break;
        }

        int nwrite = pfs_fwrite(buf, 1, (int)nread, p_dst);

        if (nwrite < 0)
        {
            rc = pfs_ferror(p_dst);
            break;
        }
        if (nwrite != (int)nread)
        {
            rc = PFS_EIO;
            break;
        }
    }

    KLB_FREE(buf);
    fclose(fp);
    pfs_fclose(p_dst);
    return rc;
}

static int pfs_klua_vfs_copy_vfs_to_host(pfs_ctx_t *p_ctx, const char *vfs_src,
        const char *host_dst)
{
    pfs_file_t *p_src = pfs_fopen(p_ctx, vfs_src, PFS_O_RD);

    if (NULL == p_src)
    {
        return PFS_ENOENT;
    }

    FILE *fp = fopen(host_dst, "wb");

    if (NULL == fp)
    {
        pfs_fclose(p_src);
        return PFS_EIO;
    }

    uint8_t *buf = (uint8_t *)KLB_MALLOC(uint8_t, PFS_KLUA_VFS_COPY_BUF, 0);

    if (NULL == buf)
    {
        fclose(fp);
        pfs_fclose(p_src);
        return PFS_ENOMEM;
    }

    int rc = PFS_OK;

    for (;;)
    {
        int nread = pfs_fread(buf, 1, PFS_KLUA_VFS_COPY_BUF, p_src);

        if (nread < 0)
        {
            rc = pfs_ferror(p_src);
            break;
        }
        if (0 == nread)
        {
            if (0 != pfs_ferror(p_src))
            {
                rc = pfs_ferror(p_src);
            }
            break;
        }

        size_t nwrite = fwrite(buf, 1, (size_t)nread, fp);

        if (nwrite != (size_t)nread)
        {
            rc = PFS_EIO;
            break;
        }
        if (0 != ferror(fp))
        {
            rc = PFS_EIO;
            break;
        }
    }

    KLB_FREE(buf);
    pfs_fclose(p_src);
    fclose(fp);
    return rc;
}

static int pfs_klua_vfs_parse_access_mode(lua_State *L, int arg_idx, int *p_mode)
{
    if (NULL == p_mode)
    {
        return PFS_EINVAL;
    }

    if (0 != lua_isnoneornil(L, arg_idx))
    {
        *p_mode = PFS_F_OK;
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
    int m = 0;

    while ('\0' != *s)
    {
        if ('r' == *s)
        {
            m |= PFS_R_OK;
        }
        else if ('w' == *s)
        {
            m |= PFS_W_OK;
        }
        else if ('x' == *s)
        {
            m |= PFS_X_OK;
        }
        else if ('f' == *s)
        {
            m |= PFS_F_OK;
        }
        else
        {
            return PFS_EINVAL;
        }
        s++;
    }

    if (0 == m)
    {
        m = PFS_F_OK;
    }
    *p_mode = m;
    return PFS_OK;
}

static int pfs_klua_vfs_access(lua_State *L)
{
    const char *drive_path = luaL_checkstring(L, 1);
    int acc_mode;
    int rc = pfs_klua_vfs_parse_access_mode(L, 2, &acc_mode);

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

    rc = pfs_access(p_ctx, parsed.vfs_path, acc_mode);
    return pfs_klua_push_rc(L, rc);
}

static int pfs_klua_vfs_mkdir(lua_State *L)
{
    const char *drive_path = luaL_checkstring(L, 1);
    pfs_klua_drive_path_t parsed;
    pfs_ctx_t *p_ctx = NULL;
    int rc = pfs_klua_path_resolve_drive(L, drive_path, &parsed, &p_ctx);

    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    rc = pfs_mkdir(p_ctx, parsed.vfs_path, 0);
    return pfs_klua_push_rc(L, rc);
}

static int pfs_klua_vfs_rmdir(lua_State *L)
{
    const char *drive_path = luaL_checkstring(L, 1);
    pfs_klua_drive_path_t parsed;
    pfs_ctx_t *p_ctx = NULL;
    int rc = pfs_klua_path_resolve_drive(L, drive_path, &parsed, &p_ctx);

    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    rc = pfs_rmdir(p_ctx, parsed.vfs_path);
    return pfs_klua_push_rc(L, rc);
}

static int pfs_klua_vfs_unlink(lua_State *L)
{
    const char *drive_path = luaL_checkstring(L, 1);
    pfs_klua_drive_path_t parsed;
    pfs_ctx_t *p_ctx = NULL;
    int rc = pfs_klua_path_resolve_drive(L, drive_path, &parsed, &p_ctx);

    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    rc = pfs_unlink(p_ctx, parsed.vfs_path);
    return pfs_klua_push_rc(L, rc);
}

static int pfs_klua_vfs_remove(lua_State *L)
{
    const char *drive_path = luaL_checkstring(L, 1);
    pfs_klua_drive_path_t parsed;
    pfs_ctx_t *p_ctx = NULL;
    int rc = pfs_klua_path_resolve_drive(L, drive_path, &parsed, &p_ctx);

    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    rc = pfs_remove(p_ctx, parsed.vfs_path);
    return pfs_klua_push_rc(L, rc);
}

static int pfs_klua_vfs_rename(lua_State *L)
{
    const char *old_drive = luaL_checkstring(L, 1);
    const char *new_drive = luaL_checkstring(L, 2);
    pfs_klua_drive_path_t old_parsed;
    pfs_klua_drive_path_t new_parsed;
    pfs_ctx_t *p_old_ctx = NULL;
    pfs_ctx_t *p_new_ctx = NULL;
    int rc = pfs_klua_path_resolve_drive(L, old_drive, &old_parsed, &p_old_ctx);

    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    rc = pfs_klua_path_resolve_drive(L, new_drive, &new_parsed, &p_new_ctx);
    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    rc = pfs_rename_cross(p_old_ctx, old_parsed.vfs_path, p_new_ctx, new_parsed.vfs_path);
    return pfs_klua_push_rc(L, rc);
}

static int pfs_klua_vfs_copy(lua_State *L)
{
    const char *src_arg = luaL_checkstring(L, 1);
    const char *dst_arg = luaL_checkstring(L, 2);
    pfs_klua_vfs_copy_end_t src;
    pfs_klua_vfs_copy_end_t dst;

    int rc = pfs_klua_vfs_copy_resolve_end(L, src_arg, &src);

    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    rc = pfs_klua_vfs_copy_resolve_end(L, dst_arg, &dst);
    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    if (0 != src.is_host && 0 != dst.is_host)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    if (0 != src.is_host)
    {
        rc = pfs_klua_vfs_copy_host_to_vfs(dst.p_ctx, src.path, dst.parsed.vfs_path);
    }
    else if (0 != dst.is_host)
    {
        rc = pfs_klua_vfs_copy_vfs_to_host(src.p_ctx, src.parsed.vfs_path, dst.path);
    }
    else
    {
        rc = pfs_klua_vfs_copy_vfs_to_vfs(src.p_ctx, src.parsed.vfs_path,
                dst.p_ctx, dst.parsed.vfs_path);
    }

    return pfs_klua_push_rc(L, rc);
}

void pfs_klua_vfs_path_register(lua_State *L)
{
    static const luaL_Reg s_path[] = {
        { "access",         pfs_klua_vfs_access },
        { "mkdir",          pfs_klua_vfs_mkdir },
        { "rmdir",          pfs_klua_vfs_rmdir },
        { "unlink",         pfs_klua_vfs_unlink },
        { "remove",         pfs_klua_vfs_remove },
        { "rename",         pfs_klua_vfs_rename },
        { "copy",           pfs_klua_vfs_copy },
        { NULL,             NULL }
    };

    luaL_setfuncs(L, s_path, 0);
}

// end
