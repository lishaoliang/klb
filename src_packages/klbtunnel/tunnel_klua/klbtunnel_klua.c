// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbtunnel/klbtunnel.h"
#include "klbtunnel/core/klb_tunnel_wire.h"
#include "klbtunnel/core/klb_tunnel_crypto.h"
#include "klua/klua.h"
#include "klbmem/klb_mem.h"

#include <string.h>


static int klua_ktunnel_available(lua_State* L)
{
    lua_pushboolean(L, 1);
    return 1;
}


static int klua_ktunnel_encode(lua_State* L)
{
    uint8_t typ = 0;
    size_t payload_len = 0;
    const char* p_payload = NULL;
    int need = 0;
    uint8_t* p_buf = NULL;
    int n = 0;

    typ = (uint8_t)luaL_checkinteger(L, 1);

    if (lua_isnoneornil(L, 2))
    {
        p_payload = NULL;
        payload_len = 0;
    }
    else
    {
        p_payload = luaL_checklstring(L, 2, &payload_len);
    }

    if (payload_len > 65535)
    {
        lua_pushnil(L);
        lua_pushstring(L, "payload too large");
        return 2;
    }

    need = KLB_TUNNEL_HDR_LEN + (int)payload_len;
    p_buf = (uint8_t*)klb_malloc((size_t)need);
    n = klb_tunnel_encode(p_buf, need, typ, (const uint8_t*)p_payload, (int)payload_len);

    if (n < 0)
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushstring(L, "encode fail");
        return 2;
    }

    lua_pushlstring(L, (const char*)p_buf, (size_t)n);
    KLB_FREE(p_buf);
    return 1;
}


static int klua_ktunnel_try_decode(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    klb_tunnel_frame_t frame;
    int rc = 0;

    p = luaL_optlstring(L, 1, "", &len);
    rc = klb_tunnel_try_decode((const uint8_t*)p, (int)len, &frame);

    if (1 == rc)
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "short");
        lua_pushlstring(L, p, len);
        return 3;
    }

    if (0 != rc)
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "bad magic");
        lua_pushlstring(L, p, len);
        return 3;
    }

    lua_pushboolean(L, 1);

    lua_createtable(L, 0, 3);
    lua_pushinteger(L, (lua_Integer)frame.type);
    lua_setfield(L, -2, "type");
    lua_pushinteger(L, (lua_Integer)frame.ver);
    lua_setfield(L, -2, "ver");

    if (frame.payload_len > 0 && NULL != frame.p_payload)
    {
        lua_pushlstring(L, (const char*)frame.p_payload, (size_t)frame.payload_len);
    }
    else
    {
        lua_pushliteral(L, "");
    }

    lua_setfield(L, -2, "payload");

    if ((int)len > frame.frame_len)
    {
        lua_pushlstring(L, p + frame.frame_len, len - (size_t)frame.frame_len);
    }
    else
    {
        lua_pushliteral(L, "");
    }

    return 3;
}


static int klua_ktunnel_encode_hello(lua_State* L)
{
    size_t token_len = 0;
    const char* p_token = NULL;
    uint8_t* p_buf = NULL;
    int need = 0;
    int n = 0;

    if (lua_isnoneornil(L, 1))
    {
        p_token = "";
        token_len = 0;
    }
    else
    {
        p_token = luaL_checklstring(L, 1, &token_len);
    }

    if (token_len > 65535)
    {
        token_len = 65535;
    }

    need = KLB_TUNNEL_HDR_LEN + (int)token_len;
    p_buf = (uint8_t*)klb_malloc((size_t)need);
    n = klb_tunnel_encode(p_buf, need, KLB_TUNNEL_T_HELLO, (const uint8_t*)p_token, (int)token_len);

    if (n < 0)
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushstring(L, "encode fail");
        return 2;
    }

    lua_pushlstring(L, (const char*)p_buf, (size_t)n);
    KLB_FREE(p_buf);
    return 1;
}


static int klua_ktunnel_encode_connect(lua_State* L)
{
    size_t host_len = 0;
    const char* p_host = NULL;
    int port = 0;
    uint8_t payload[1 + KLB_TUNNEL_HOST_MAX + 2];
    uint8_t frame[KLB_TUNNEL_HDR_LEN + 1 + KLB_TUNNEL_HOST_MAX + 2];
    int plen = 0;
    int n = 0;

    p_host = luaL_checklstring(L, 1, &host_len);
    port = (int)luaL_checkinteger(L, 2);

    if (port < 0)
    {
        port = 0;
    }

    if (port > 65535)
    {
        port = 65535;
    }

    plen = klb_tunnel_encode_connect_payload(payload, (int)sizeof(payload),
        p_host, (int)host_len, (uint16_t)port);

    if (plen < 0)
    {
        lua_pushnil(L);
        lua_pushstring(L, "encode fail");
        return 2;
    }

    n = klb_tunnel_encode(frame, (int)sizeof(frame), KLB_TUNNEL_T_CONNECT, payload, plen);

    if (n < 0)
    {
        lua_pushnil(L);
        lua_pushstring(L, "encode fail");
        return 2;
    }

    lua_pushlstring(L, (const char*)frame, (size_t)n);
    return 1;
}


static int klua_ktunnel_parse_connect_payload(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    char host[KLB_TUNNEL_HOST_MAX + 1];
    uint16_t port = 0;
    int rc = 0;

    p = luaL_checklstring(L, 1, &len);
    rc = klb_tunnel_parse_connect_payload((const uint8_t*)p, (int)len,
        host, (int)sizeof(host), &port);

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushstring(L, "bad connect payload");
        return 3;
    }

    lua_pushstring(L, host);
    lua_pushinteger(L, (lua_Integer)port);
    lua_pushnil(L);
    return 3;
}


static int klua_ktunnel_sha256(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    uint8_t out[KLB_TUNNEL_SHA256_LEN];

    p = luaL_checklstring(L, 1, &len);
    klb_tunnel_sha256((const uint8_t*)p, (int)len, out);
    lua_pushlstring(L, (const char*)out, KLB_TUNNEL_SHA256_LEN);
    return 1;
}


static int klua_ktunnel_derive_session_key(lua_State* L)
{
    size_t psk_len = 0;
    size_t nc_len = 0;
    size_t ns_len = 0;
    const char* p_psk = NULL;
    const char* p_nc = NULL;
    const char* p_ns = NULL;
    uint8_t out[KLB_TUNNEL_SHA256_LEN];

    p_psk = luaL_checklstring(L, 1, &psk_len);
    p_nc = luaL_checklstring(L, 2, &nc_len);
    p_ns = luaL_checklstring(L, 3, &ns_len);
    klb_tunnel_derive_session_key((const uint8_t*)p_psk, (int)psk_len,
        (const uint8_t*)p_nc, (int)nc_len, (const uint8_t*)p_ns, (int)ns_len, out);
    lua_pushlstring(L, (const char*)out, KLB_TUNNEL_SHA256_LEN);
    return 1;
}


static int klua_ktunnel_seal(lua_State* L)
{
    size_t key_len = 0;
    size_t plain_len = 0;
    const char* p_key = NULL;
    const char* p_plain = NULL;
    uint32_t seq = 0;
    uint8_t* p_buf = NULL;
    int n = 0;

    p_key = luaL_checklstring(L, 1, &key_len);
    seq = (uint32_t)luaL_checkinteger(L, 2);

    if (lua_isnoneornil(L, 3))
    {
        p_plain = "";
        plain_len = 0;
    }
    else
    {
        p_plain = luaL_checklstring(L, 3, &plain_len);
    }

    if (key_len < KLB_TUNNEL_SHA256_LEN)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad key");
        return 2;
    }

    p_buf = (uint8_t*)klb_malloc((size_t)(4 + (int)plain_len + KLB_TUNNEL_TAG_LEN));
    n = klb_tunnel_seal(p_buf, 4 + (int)plain_len + KLB_TUNNEL_TAG_LEN, (const uint8_t*)p_key,
        seq, (const uint8_t*)p_plain, (int)plain_len);

    if (n < 0)
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushstring(L, "seal fail");
        return 2;
    }

    lua_pushlstring(L, (const char*)p_buf, (size_t)n);
    KLB_FREE(p_buf);
    return 1;
}


static int klua_ktunnel_open(lua_State* L)
{
    size_t key_len = 0;
    size_t blob_len = 0;
    const char* p_key = NULL;
    const char* p_blob = NULL;
    uint8_t* p_buf = NULL;
    uint32_t seq = 0;
    int n = 0;

    p_key = luaL_checklstring(L, 1, &key_len);
    p_blob = luaL_checklstring(L, 2, &blob_len);

    if (key_len < KLB_TUNNEL_SHA256_LEN)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad key");
        return 2;
    }

    if (blob_len < 4 + KLB_TUNNEL_TAG_LEN)
    {
        lua_pushnil(L);
        lua_pushstring(L, "short seal");
        return 2;
    }

    p_buf = (uint8_t*)klb_malloc(blob_len);
    n = klb_tunnel_open(p_buf, (int)blob_len, (const uint8_t*)p_key,
        (const uint8_t*)p_blob, (int)blob_len, &seq);

    if (n < 0)
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushstring(L, "open fail");
        return 2;
    }

    lua_pushlstring(L, (const char*)p_buf, (size_t)n);
    lua_pushinteger(L, (lua_Integer)seq);
    KLB_FREE(p_buf);
    return 2;
}


static int klua_ktunnel_random_nonce(lua_State* L)
{
    int n = KLB_TUNNEL_NONCE_LEN;
    uint8_t buf[64];
    int rc = 0;

    if (!lua_isnoneornil(L, 1))
    {
        n = (int)luaL_checkinteger(L, 1);
    }

    if (n <= 0 || n > (int)sizeof(buf))
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad nonce len");
        return 2;
    }

    rc = klb_tunnel_random_nonce(buf, n);

    if (rc < 0)
    {
        lua_pushnil(L);
        lua_pushstring(L, "nonce fail");
        return 2;
    }

    lua_pushlstring(L, (const char*)buf, (size_t)n);
    return 1;
}


static int klua_ktunnel_encode_hello_crypto(lua_State* L)
{
    size_t token_len = 0;
    size_t nonce_len = 0;
    const char* p_token = NULL;
    const char* p_nonce = NULL;
    uint8_t nonce[KLB_TUNNEL_NONCE_LEN];
    uint8_t* p_buf = NULL;
    int need = 0;
    int n = 0;

    if (lua_isnoneornil(L, 1))
    {
        p_token = "";
        token_len = 0;
    }
    else
    {
        p_token = luaL_checklstring(L, 1, &token_len);
    }

    if (lua_isnoneornil(L, 2))
    {
        if (0 > klb_tunnel_random_nonce(nonce, KLB_TUNNEL_NONCE_LEN))
        {
            lua_pushnil(L);
            lua_pushstring(L, "nonce fail");
            return 2;
        }

        p_nonce = (const char*)nonce;
        nonce_len = KLB_TUNNEL_NONCE_LEN;
    }
    else
    {
        p_nonce = luaL_checklstring(L, 2, &nonce_len);

        if (nonce_len != KLB_TUNNEL_NONCE_LEN)
        {
            lua_pushnil(L);
            lua_pushstring(L, "bad nonce");
            return 2;
        }
    }

    need = 1 + 2 + (int)token_len + KLB_TUNNEL_NONCE_LEN;
    p_buf = (uint8_t*)klb_malloc((size_t)need);
    n = klb_tunnel_encode_hello_crypto(p_buf, need,
        (const uint8_t*)p_token, (int)token_len, (const uint8_t*)p_nonce);

    if (n < 0)
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushstring(L, "encode fail");
        return 2;
    }

    lua_pushlstring(L, (const char*)p_buf, (size_t)n);
    lua_pushlstring(L, p_nonce, KLB_TUNNEL_NONCE_LEN);
    KLB_FREE(p_buf);
    return 2;
}


static int klua_ktunnel_parse_hello_payload(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    char token[4096];
    uint8_t nonce[KLB_TUNNEL_NONCE_LEN];
    int crypto = 0;
    int token_len = 0;
    int rc = 0;

    p = luaL_optlstring(L, 1, "", &len);

    if (len >= (size_t)sizeof(token))
    {
        lua_pushnil(L);
        lua_pushstring(L, "token too long");
        return 2;
    }

    rc = klb_tunnel_parse_hello_payload((const uint8_t*)p, (int)len,
        &crypto, token, (int)sizeof(token), &token_len, nonce);

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "short hello crypto");
        return 2;
    }

    lua_createtable(L, 0, 3);
    lua_pushboolean(L, crypto);
    lua_setfield(L, -2, "crypto");
    lua_pushlstring(L, token, (size_t)token_len);
    lua_setfield(L, -2, "token");

    if (0 != crypto)
    {
        lua_pushlstring(L, (const char*)nonce, KLB_TUNNEL_NONCE_LEN);
    }
    else
    {
        lua_pushnil(L);
    }

    lua_setfield(L, -2, "nonce_c");
    return 1;
}


static int klua_ktunnel_encode_hello_ok_crypto(lua_State* L)
{
    size_t nonce_len = 0;
    const char* p_nonce = NULL;
    uint8_t nonce[KLB_TUNNEL_NONCE_LEN];
    uint8_t buf[1 + KLB_TUNNEL_NONCE_LEN];
    int n = 0;

    if (lua_isnoneornil(L, 1))
    {
        if (0 > klb_tunnel_random_nonce(nonce, KLB_TUNNEL_NONCE_LEN))
        {
            lua_pushnil(L);
            lua_pushstring(L, "nonce fail");
            return 2;
        }

        p_nonce = (const char*)nonce;
        nonce_len = KLB_TUNNEL_NONCE_LEN;
    }
    else
    {
        p_nonce = luaL_checklstring(L, 1, &nonce_len);

        if (nonce_len != KLB_TUNNEL_NONCE_LEN)
        {
            lua_pushnil(L);
            lua_pushstring(L, "bad nonce");
            return 2;
        }
    }

    n = klb_tunnel_encode_hello_ok_crypto(buf, (int)sizeof(buf), (const uint8_t*)p_nonce);

    if (n < 0)
    {
        lua_pushnil(L);
        lua_pushstring(L, "encode fail");
        return 2;
    }

    lua_pushlstring(L, (const char*)buf, (size_t)n);
    lua_pushlstring(L, p_nonce, KLB_TUNNEL_NONCE_LEN);
    return 2;
}


static int klua_ktunnel_parse_hello_ok_payload(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    uint8_t nonce[KLB_TUNNEL_NONCE_LEN];
    int crypto = 0;

    p = luaL_optlstring(L, 1, "", &len);
    klb_tunnel_parse_hello_ok_payload((const uint8_t*)p, (int)len, &crypto, nonce);

    lua_createtable(L, 0, 2);
    lua_pushboolean(L, crypto);
    lua_setfield(L, -2, "crypto");

    if (0 != crypto)
    {
        lua_pushlstring(L, (const char*)nonce, KLB_TUNNEL_NONCE_LEN);
    }
    else
    {
        lua_pushnil(L);
    }

    lua_setfield(L, -2, "nonce_s");
    return 1;
}


int klua_open_ktunnel(lua_State* L)
{
    static luaL_Reg lib[] = {
        { "available",               klua_ktunnel_available },
        { "encode",                  klua_ktunnel_encode },
        { "try_decode",              klua_ktunnel_try_decode },
        { "encode_hello",            klua_ktunnel_encode_hello },
        { "encode_connect",          klua_ktunnel_encode_connect },
        { "parse_connect_payload",   klua_ktunnel_parse_connect_payload },
        { "sha256",                  klua_ktunnel_sha256 },
        { "derive_session_key",      klua_ktunnel_derive_session_key },
        { "seal",                    klua_ktunnel_seal },
        { "open",                    klua_ktunnel_open },
        { "random_nonce",            klua_ktunnel_random_nonce },
        { "encode_hello_crypto",     klua_ktunnel_encode_hello_crypto },
        { "parse_hello_payload",     klua_ktunnel_parse_hello_payload },
        { "encode_hello_ok_crypto",  klua_ktunnel_encode_hello_ok_crypto },
        { "parse_hello_ok_payload",  klua_ktunnel_parse_hello_ok_payload },
        { NULL, NULL }
    };

    luaL_newlib(L, lib);

    lua_pushinteger(L, KLB_TUNNEL_MAGIC0);
    lua_setfield(L, -2, "MAGIC0");
    lua_pushinteger(L, KLB_TUNNEL_MAGIC1);
    lua_setfield(L, -2, "MAGIC1");
    lua_pushinteger(L, KLB_TUNNEL_VER);
    lua_setfield(L, -2, "VER");
    lua_pushinteger(L, KLB_TUNNEL_T_HELLO);
    lua_setfield(L, -2, "T_HELLO");
    lua_pushinteger(L, KLB_TUNNEL_T_HELLO_OK);
    lua_setfield(L, -2, "T_HELLO_OK");
    lua_pushinteger(L, KLB_TUNNEL_T_HELLO_NAK);
    lua_setfield(L, -2, "T_HELLO_NAK");
    lua_pushinteger(L, KLB_TUNNEL_T_CONNECT);
    lua_setfield(L, -2, "T_CONNECT");
    lua_pushinteger(L, KLB_TUNNEL_T_CONNECT_OK);
    lua_setfield(L, -2, "T_CONNECT_OK");
    lua_pushinteger(L, KLB_TUNNEL_T_CONNECT_NAK);
    lua_setfield(L, -2, "T_CONNECT_NAK");
    lua_pushinteger(L, KLB_TUNNEL_T_DATA);
    lua_setfield(L, -2, "T_DATA");
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "aead");
    lua_pushinteger(L, KLB_TUNNEL_FLAG_CRYPTO);
    lua_setfield(L, -2, "FLAG_CRYPTO");
    lua_pushinteger(L, KLB_TUNNEL_NONCE_LEN);
    lua_setfield(L, -2, "NONCE_LEN");

    return 1;
}

// end
