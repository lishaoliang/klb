// Doc Encode : UTF-8 BOM, Unix(LF)
// ktls: TLS 证书工具 Lua 绑定 (自签)
#include "klua/klua.h"
#include "klbnet/klb_tls_cert.h"
#include "klbmem/klb_mem.h"
#include <string.h>


static int klua_ktls_available(lua_State* L)
{
    lua_pushboolean(L, klb_tls_cert_available() ? 1 : 0);
    return 1;
}


static void fill_opt_klua_ktls(lua_State* L, int idx, klb_tls_cert_selfsign_opt_t* p_opt,
    char* p_subject_buf, int subject_cap, char* p_san_buf, int san_cap)
{
    KLB_MEMSET(p_opt, 0, sizeof(*p_opt));

    if (LUA_TTABLE != lua_type(L, idx))
    {
        return;
    }

    lua_getfield(L, idx, "bits");
    if (LUA_TNUMBER == lua_type(L, -1))
    {
        p_opt->bits = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "days");
    if (LUA_TNUMBER == lua_type(L, -1))
    {
        p_opt->days = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "subject");
    if (LUA_TSTRING == lua_type(L, -1) && NULL != p_subject_buf && 0 < subject_cap)
    {
        const char* p_sub = lua_tostring(L, -1);
        if (NULL != p_sub)
        {
            strncpy(p_subject_buf, p_sub, (size_t)subject_cap - 1);
            p_subject_buf[subject_cap - 1] = '\0';
            p_opt->subject = p_subject_buf;
        }
    }
    lua_pop(L, 1);

    // san: string CSV, 或 table of "DNS:.." / "IP:.."
    lua_getfield(L, idx, "san");
    if (LUA_TSTRING == lua_type(L, -1) && NULL != p_san_buf && 0 < san_cap)
    {
        const char* p_san = lua_tostring(L, -1);
        if (NULL != p_san)
        {
            strncpy(p_san_buf, p_san, (size_t)san_cap - 1);
            p_san_buf[san_cap - 1] = '\0';
            p_opt->san_csv = p_san_buf;
        }
    }
    else if (LUA_TTABLE == lua_type(L, -1) && NULL != p_san_buf && 0 < san_cap)
    {
        size_t used = 0;
        int i = 0;
        int n = (int)lua_rawlen(L, -1);

        p_san_buf[0] = '\0';
        for (i = 1; i <= n; i++)
        {
            const char* p_item = NULL;
            size_t item_len = 0;

            lua_rawgeti(L, -1, i);
            if (LUA_TSTRING == lua_type(L, -1))
            {
                p_item = lua_tolstring(L, -1, &item_len);
            }
            lua_pop(L, 1);

            if (NULL == p_item || 0 == item_len)
            {
                continue;
            }

            if (0 < used)
            {
                if (used + 1 >= (size_t)san_cap)
                {
                    break;
                }

                p_san_buf[used++] = ',';
                p_san_buf[used] = '\0';
            }

            if (used + item_len >= (size_t)san_cap)
            {
                break;
            }

            memcpy(p_san_buf + used, p_item, item_len);
            used += item_len;
            p_san_buf[used] = '\0';
        }

        if (0 < used)
        {
            p_opt->san_csv = p_san_buf;
        }
    }
    lua_pop(L, 1);
}


static int klua_ktls_selfsign(lua_State* L)
{
    klb_tls_cert_selfsign_opt_t opt;
    char subject_buf[256];
    char san_buf[1024];
    char* p_cert = NULL;
    char* p_key = NULL;
    int cert_len = 0;
    int key_len = 0;
    int rc = 0;
    int cap = KLB_TLS_CERT_PEM_MIN * 2;

    if (!klb_tls_cert_available())
    {
        lua_pushnil(L);
        lua_pushstring(L, "ktls unavailable (no-ssl)");
        return 2;
    }

    KLB_MEMSET(subject_buf, 0, sizeof(subject_buf));
    KLB_MEMSET(san_buf, 0, sizeof(san_buf));
    fill_opt_klua_ktls(L, 1, &opt, subject_buf, (int)sizeof(subject_buf),
        san_buf, (int)sizeof(san_buf));

    p_cert = (char*)KLB_MALLOCZ(char, (size_t)cap, 0);
    p_key = (char*)KLB_MALLOCZ(char, (size_t)cap, 0);

    rc = klb_tls_cert_selfsign_rsa(&opt, p_cert, cap, &cert_len, p_key, cap, &key_len);
    if (0 != rc)
    {
        KLB_FREE(p_cert);
        KLB_FREE(p_key);
        lua_pushnil(L);
        lua_pushfstring(L, "selfsign fail rc=%d", rc);
        return 2;
    }

    lua_pushstring(L, p_cert);
    lua_pushstring(L, p_key);
    KLB_FREE(p_cert);
    KLB_FREE(p_key);
    return 2;
}


static int klua_ktls_selfsign_to_files(lua_State* L)
{
    klb_tls_cert_selfsign_opt_t opt;
    char subject_buf[256];
    char san_buf[1024];
    const char* p_cert_path = NULL;
    const char* p_key_path = NULL;
    int rc = 0;

    if (!klb_tls_cert_available())
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "ktls unavailable (no-ssl)");
        return 2;
    }

    if (LUA_TTABLE != lua_type(L, 1))
    {
        return luaL_error(L, "ktls.selfsign_to_files need table");
    }

    KLB_MEMSET(subject_buf, 0, sizeof(subject_buf));
    KLB_MEMSET(san_buf, 0, sizeof(san_buf));
    fill_opt_klua_ktls(L, 1, &opt, subject_buf, (int)sizeof(subject_buf),
        san_buf, (int)sizeof(san_buf));

    lua_getfield(L, 1, "path_cert");
    if (LUA_TSTRING == lua_type(L, -1))
    {
        p_cert_path = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "path_key");
    if (LUA_TSTRING == lua_type(L, -1))
    {
        p_key_path = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    if (NULL == p_cert_path || NULL == p_key_path)
    {
        return luaL_error(L, "need path_cert and path_key");
    }

    rc = klb_tls_cert_selfsign_rsa_to_files(&opt, p_cert_path, p_key_path);
    if (0 != rc)
    {
        lua_pushboolean(L, 0);
        lua_pushfstring(L, "selfsign_to_files fail rc=%d", rc);
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}


static int klua_ktls_not_after(lua_State* L)
{
    const char* p_pem = NULL;
    size_t pem_len = 0;
    int64_t not_after = 0;
    int64_t remaining = 0;
    int rc = 0;

    if (!klb_tls_cert_available())
    {
        lua_pushnil(L);
        lua_pushstring(L, "ktls unavailable (no-ssl)");
        return 2;
    }

    p_pem = luaL_checklstring(L, 1, &pem_len);
    rc = klb_tls_cert_pem_not_after(p_pem, (int)pem_len, &not_after, &remaining);
    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "parse cert fail");
        return 2;
    }

    lua_createtable(L, 0, 3);
    lua_pushinteger(L, (lua_Integer)not_after);
    lua_setfield(L, -2, "not_after");
    lua_pushinteger(L, (lua_Integer)remaining);
    lua_setfield(L, -2, "remaining_sec");
    lua_pushboolean(L, (remaining <= 0) ? 1 : 0);
    lua_setfield(L, -2, "expired");
    return 1;
}


static int klua_ktls_not_after_file(lua_State* L)
{
    const char* p_path = NULL;
    int64_t not_after = 0;
    int64_t remaining = 0;
    int rc = 0;

    if (!klb_tls_cert_available())
    {
        lua_pushnil(L);
        lua_pushstring(L, "ktls unavailable (no-ssl)");
        return 2;
    }

    p_path = luaL_checkstring(L, 1);
    rc = klb_tls_cert_file_not_after(p_path, &not_after, &remaining);
    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "read/parse cert fail");
        return 2;
    }

    lua_createtable(L, 0, 3);
    lua_pushinteger(L, (lua_Integer)not_after);
    lua_setfield(L, -2, "not_after");
    lua_pushinteger(L, (lua_Integer)remaining);
    lua_setfield(L, -2, "remaining_sec");
    lua_pushboolean(L, (remaining <= 0) ? 1 : 0);
    lua_setfield(L, -2, "expired");
    return 1;
}


int klua_open_ktls(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "available",          klua_ktls_available },
        { "selfsign",           klua_ktls_selfsign },
        { "selfsign_to_files",  klua_ktls_selfsign_to_files },
        { "not_after",          klua_ktls_not_after },
        { "not_after_file",     klua_ktls_not_after_file },
        { NULL,                 NULL }
    };

    luaL_newlib(L, lib);
    return 1;
}

// end
