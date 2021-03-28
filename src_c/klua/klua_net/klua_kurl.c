#include "klua/klua.h"
#include "klbthird/http_parser.h"
#include "klbutil/klb_log.h"
#include "klbthird/sds.h"
#include <assert.h>


static char s_klua_kurl_key[UF_MAX][12] = {
    "schema",
    "host",
    "port",
    "path",
    "query",
    "fragment",
    "userinfo"
};

static int klua_kurl_parse(lua_State* L)
{
    size_t url_len = 0;
    const char* p_url = luaL_checklstring(L, 1, &url_len);

    struct http_parser_url url = { 0 };

    // eg. http://username:password@127.0.0.1:8080/test/test.aspx?name=sviergn&x=true#stuff
    if (0 != http_parser_parse_url(p_url, url_len, 0, &url))
    {
        lua_newtable(L);
        return 1;
    }

    // table
    lua_newtable(L);

    for (int i = 0; i < UF_MAX; i++)
    {
        if (url.field_set & (1 << i))
        {
            klua_setfield_lstring(L, s_klua_kurl_key[i], p_url + url.field_data[i].off, url.field_data[i].len);
        }
    }

    return 1;
}

int klua_open_kurl(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "parse",      klua_kurl_parse },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
