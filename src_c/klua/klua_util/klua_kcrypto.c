// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "libavutil/md5.h"
#include "libavutil/sha.h"
#include "libavutil/hmac.h"
#include "libavutil/crc.h"
#include "libavutil/base64.h"
#include "libavutil/mem.h"
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>


/// @def   KLUA_KCRYPTO_MD5_LEN
/// @brief MD5 digest length
#define KLUA_KCRYPTO_MD5_LEN            16

/// @def   KLUA_KCRYPTO_SHA1_LEN
/// @brief SHA-1 digest length
#define KLUA_KCRYPTO_SHA1_LEN           20

/// @def   KLUA_KCRYPTO_SHA256_LEN
/// @brief SHA-256 digest length
#define KLUA_KCRYPTO_SHA256_LEN         32

/// @def   KLUA_KCRYPTO_HEX_STACK
/// @brief stack hex buffer (covers sha256 * 2 + NUL)
#define KLUA_KCRYPTO_HEX_STACK          80


/// @brief check data length fits libavutil int APIs
static int check_len_klua_kcrypto(lua_State* L, size_t len)
{
    if ((size_t)INT_MAX < len)
    {
        return luaL_error(L, "kcrypto: data too large");
    }

    return (int)len;
}


/// @brief push lowercase hex of binary digest
static int push_hex_klua_kcrypto(lua_State* L, const uint8_t* p_dig, int dig_len)
{
    char hex[KLUA_KCRYPTO_HEX_STACK];
    char* p_hex = hex;
    char* p_heap = NULL;
    int need = dig_len * 2 + 1;
    int i = 0;

    if ((int)sizeof(hex) < need)
    {
        p_heap = KLB_MALLOC(char, (size_t)need, 0);
        p_hex = p_heap;
    }

    for (i = 0; i < dig_len; ++i)
    {
        sprintf(p_hex + (i * 2), "%02x", (unsigned int)p_dig[i]);
    }

    lua_pushlstring(L, p_hex, (size_t)(dig_len * 2));
    KLB_FREE(p_heap);
    return 1;
}


/// @brief read required binary/string arg
static const uint8_t* check_lstring_klua_kcrypto(lua_State* L, int idx, size_t* p_len)
{
    size_t len = 0;
    const char* p = luaL_checklstring(L, idx, &len);

    if (NULL != p_len)
    {
        *p_len = len;
    }

    return (const uint8_t*)p;
}


//////////////////////////////////////////////////////////////////////////
// MD5


static int klua_kcrypto_md5(lua_State* L)
{
    size_t len = 0;
    const uint8_t* p_data = check_lstring_klua_kcrypto(L, 1, &len);
    int n = check_len_klua_kcrypto(L, len);
    uint8_t dig[KLUA_KCRYPTO_MD5_LEN];

    av_md5_sum(dig, p_data, n);
    lua_pushlstring(L, (const char*)dig, KLUA_KCRYPTO_MD5_LEN);
    return 1;
}


static int klua_kcrypto_md5_hex(lua_State* L)
{
    size_t len = 0;
    const uint8_t* p_data = check_lstring_klua_kcrypto(L, 1, &len);
    int n = check_len_klua_kcrypto(L, len);
    uint8_t dig[KLUA_KCRYPTO_MD5_LEN];

    av_md5_sum(dig, p_data, n);
    return push_hex_klua_kcrypto(L, dig, KLUA_KCRYPTO_MD5_LEN);
}


//////////////////////////////////////////////////////////////////////////
// SHA


static int digest_sha_klua_kcrypto(lua_State* L, int bits, int dig_len, bool as_hex)
{
    size_t len = 0;
    const uint8_t* p_data = check_lstring_klua_kcrypto(L, 1, &len);
    int n = check_len_klua_kcrypto(L, len);
    struct AVSHA* p_ctx = NULL;
    uint8_t dig[KLUA_KCRYPTO_SHA256_LEN];
    int rc = 0;

    p_ctx = av_sha_alloc();
    if (NULL == p_ctx)
    {
        return luaL_error(L, "kcrypto: out of memory");
    }

    rc = av_sha_init(p_ctx, bits);
    if (0 != rc)
    {
        av_free(p_ctx);
        return luaL_error(L, "kcrypto: sha init failed");
    }

    av_sha_update(p_ctx, p_data, (unsigned int)n);
    av_sha_final(p_ctx, dig);
    av_free(p_ctx);

    if (as_hex)
    {
        return push_hex_klua_kcrypto(L, dig, dig_len);
    }

    lua_pushlstring(L, (const char*)dig, (size_t)dig_len);
    return 1;
}


static int klua_kcrypto_sha1(lua_State* L)
{
    return digest_sha_klua_kcrypto(L, 160, KLUA_KCRYPTO_SHA1_LEN, false);
}


static int klua_kcrypto_sha1_hex(lua_State* L)
{
    return digest_sha_klua_kcrypto(L, 160, KLUA_KCRYPTO_SHA1_LEN, true);
}


static int klua_kcrypto_sha256(lua_State* L)
{
    return digest_sha_klua_kcrypto(L, 256, KLUA_KCRYPTO_SHA256_LEN, false);
}


static int klua_kcrypto_sha256_hex(lua_State* L)
{
    return digest_sha_klua_kcrypto(L, 256, KLUA_KCRYPTO_SHA256_LEN, true);
}


//////////////////////////////////////////////////////////////////////////
// HMAC


static int digest_hmac_klua_kcrypto(lua_State* L, enum AVHMACType type, int dig_len, bool as_hex)
{
    size_t key_len = 0;
    size_t data_len = 0;
    const uint8_t* p_key = check_lstring_klua_kcrypto(L, 1, &key_len);
    const uint8_t* p_data = check_lstring_klua_kcrypto(L, 2, &data_len);
    int key_n = check_len_klua_kcrypto(L, key_len);
    int data_n = check_len_klua_kcrypto(L, data_len);
    AVHMAC* p_hmac = NULL;
    uint8_t dig[KLUA_KCRYPTO_SHA256_LEN];
    int nout = 0;

    p_hmac = av_hmac_alloc(type);
    if (NULL == p_hmac)
    {
        return luaL_error(L, "kcrypto: out of memory");
    }

    nout = av_hmac_calc(p_hmac, p_data, (unsigned int)data_n,
        p_key, (unsigned int)key_n, dig, (unsigned int)dig_len);
    av_hmac_free(p_hmac);

    if (0 > nout)
    {
        return luaL_error(L, "kcrypto: hmac failed");
    }

    if (as_hex)
    {
        return push_hex_klua_kcrypto(L, dig, dig_len);
    }

    lua_pushlstring(L, (const char*)dig, (size_t)dig_len);
    return 1;
}


static int klua_kcrypto_hmac_md5(lua_State* L)
{
    return digest_hmac_klua_kcrypto(L, AV_HMAC_MD5, KLUA_KCRYPTO_MD5_LEN, false);
}


static int klua_kcrypto_hmac_md5_hex(lua_State* L)
{
    return digest_hmac_klua_kcrypto(L, AV_HMAC_MD5, KLUA_KCRYPTO_MD5_LEN, true);
}


static int klua_kcrypto_hmac_sha1(lua_State* L)
{
    return digest_hmac_klua_kcrypto(L, AV_HMAC_SHA1, KLUA_KCRYPTO_SHA1_LEN, false);
}


static int klua_kcrypto_hmac_sha1_hex(lua_State* L)
{
    return digest_hmac_klua_kcrypto(L, AV_HMAC_SHA1, KLUA_KCRYPTO_SHA1_LEN, true);
}


static int klua_kcrypto_hmac_sha256(lua_State* L)
{
    return digest_hmac_klua_kcrypto(L, AV_HMAC_SHA256, KLUA_KCRYPTO_SHA256_LEN, false);
}


static int klua_kcrypto_hmac_sha256_hex(lua_State* L)
{
    return digest_hmac_klua_kcrypto(L, AV_HMAC_SHA256, KLUA_KCRYPTO_SHA256_LEN, true);
}


//////////////////////////////////////////////////////////////////////////
// CRC
// crc16 default: modbus (AV_CRC_16_ANSI_LE, init 0xFFFF)
// crc32 default: ieee/zlib (AV_CRC_32_IEEE_LE, init/xor 0xFFFFFFFF)


static int parse_crc16_id_klua_kcrypto(lua_State* L, int idx, AVCRCId* p_id, uint32_t* p_init, bool* p_xor_out)
{
    const char* p_name = "modbus";

    *p_id = AV_CRC_16_ANSI_LE;
    *p_init = 0xFFFFu;
    *p_xor_out = false;

    if (!lua_isnoneornil(L, idx))
    {
        p_name = luaL_checkstring(L, idx);
    }

    if (0 == strcmp(p_name, "modbus") || 0 == strcmp(p_name, "ansi_le"))
    {
        *p_id = AV_CRC_16_ANSI_LE;
        *p_init = 0xFFFFu;
        *p_xor_out = false;
    }
    else if (0 == strcmp(p_name, "ansi"))
    {
        *p_id = AV_CRC_16_ANSI;
        *p_init = 0u;
        *p_xor_out = false;
    }
    else if (0 == strcmp(p_name, "ccitt"))
    {
        *p_id = AV_CRC_16_CCITT;
        *p_init = 0xFFFFu;
        *p_xor_out = false;
    }
    else
    {
        return luaL_error(L, "kcrypto: bad crc16 variant (modbus|ansi|ansi_le|ccitt)");
    }

    return 0;
}


static int parse_crc32_id_klua_kcrypto(lua_State* L, int idx, AVCRCId* p_id, uint32_t* p_init, bool* p_xor_out)
{
    const char* p_name = "ieee";

    *p_id = AV_CRC_32_IEEE_LE;
    *p_init = 0xFFFFFFFFu;
    *p_xor_out = true;

    if (!lua_isnoneornil(L, idx))
    {
        p_name = luaL_checkstring(L, idx);
    }

    if (0 == strcmp(p_name, "ieee") || 0 == strcmp(p_name, "zlib"))
    {
        *p_id = AV_CRC_32_IEEE_LE;
        *p_init = 0xFFFFFFFFu;
        *p_xor_out = true;
    }
    else if (0 == strcmp(p_name, "ieee_be"))
    {
        *p_id = AV_CRC_32_IEEE;
        *p_init = 0xFFFFFFFFu;
        *p_xor_out = true;
    }
    else
    {
        return luaL_error(L, "kcrypto: bad crc32 variant (ieee|zlib|ieee_be)");
    }

    return 0;
}


static int klua_kcrypto_crc16(lua_State* L)
{
    size_t len = 0;
    const uint8_t* p_data = check_lstring_klua_kcrypto(L, 1, &len);
    int n = check_len_klua_kcrypto(L, len);
    AVCRCId id = AV_CRC_16_ANSI_LE;
    uint32_t init = 0xFFFFu;
    bool xor_out = false;
    const AVCRC* p_tab = NULL;
    uint32_t crc = 0;

    parse_crc16_id_klua_kcrypto(L, 2, &id, &init, &xor_out);

    p_tab = av_crc_get_table(id);
    if (NULL == p_tab)
    {
        return luaL_error(L, "kcrypto: crc16 table unavailable");
    }

    crc = av_crc(p_tab, init, p_data, (size_t)n);
    if (xor_out)
    {
        crc ^= 0xFFFFu;
    }

    lua_pushinteger(L, (lua_Integer)(crc & 0xFFFFu));
    return 1;
}


static int klua_kcrypto_crc32(lua_State* L)
{
    size_t len = 0;
    const uint8_t* p_data = check_lstring_klua_kcrypto(L, 1, &len);
    int n = check_len_klua_kcrypto(L, len);
    AVCRCId id = AV_CRC_32_IEEE_LE;
    uint32_t init = 0xFFFFFFFFu;
    bool xor_out = true;
    const AVCRC* p_tab = NULL;
    uint32_t crc = 0;

    parse_crc32_id_klua_kcrypto(L, 2, &id, &init, &xor_out);

    p_tab = av_crc_get_table(id);
    if (NULL == p_tab)
    {
        return luaL_error(L, "kcrypto: crc32 table unavailable");
    }

    crc = av_crc(p_tab, init, p_data, (size_t)n);
    if (xor_out)
    {
        crc ^= 0xFFFFFFFFu;
    }

    lua_pushinteger(L, (lua_Integer)(uint32_t)crc);
    return 1;
}


//////////////////////////////////////////////////////////////////////////
// Base64


static int klua_kcrypto_base64_encode(lua_State* L)
{
    size_t len = 0;
    const uint8_t* p_data = check_lstring_klua_kcrypto(L, 1, &len);
    int n = check_len_klua_kcrypto(L, len);
    int out_size = 0;
    char* p_out = NULL;
    char* p_ret = NULL;

    out_size = (int)AV_BASE64_SIZE(n);
    p_out = KLB_MALLOC(char, (size_t)out_size, 0);

    p_ret = av_base64_encode(p_out, out_size, p_data, n);
    if (NULL == p_ret)
    {
        KLB_FREE(p_out);
        return luaL_error(L, "kcrypto: base64_encode failed");
    }

    lua_pushstring(L, p_out);
    KLB_FREE(p_out);
    return 1;
}


static int klua_kcrypto_base64_decode(lua_State* L)
{
    size_t len = 0;
    const char* p_in = luaL_checklstring(L, 1, &len);
    int out_max = 0;
    uint8_t* p_out = NULL;
    int nout = 0;

    if ((size_t)INT_MAX < len)
    {
        return luaL_error(L, "kcrypto: data too large");
    }

    out_max = (int)AV_BASE64_DECODE_SIZE(len);
    if (0 >= out_max)
    {
        lua_pushlstring(L, "", 0);
        return 1;
    }

    p_out = KLB_MALLOC(uint8_t, (size_t)out_max, 0);
    nout = av_base64_decode(p_out, p_in, out_max);
    if (0 > nout)
    {
        KLB_FREE(p_out);
        lua_pushnil(L);
        lua_pushstring(L, "invalid base64");
        return 2;
    }

    lua_pushlstring(L, (const char*)p_out, (size_t)nout);
    KLB_FREE(p_out);
    return 1;
}


//////////////////////////////////////////////////////////////////////////
// Stream: kcrypto.new(type, ...)


/// @def   KLUA_KCRYPTO_HANDLE
/// @brief Lua meta mark (align khttp: KHTTP_HANDLE*)
#define KLUA_KCRYPTO_HANDLE             "KCRYPTO_HANDLE*"


/// @enum  klua_kcrypto_kind_e
/// @brief stream algorithm kind
typedef enum klua_kcrypto_kind_e_
{
    KLUA_KCRYPTO_KIND_MD5 = 0,
    KLUA_KCRYPTO_KIND_SHA1,
    KLUA_KCRYPTO_KIND_SHA256,
    KLUA_KCRYPTO_KIND_HMAC_MD5,
    KLUA_KCRYPTO_KIND_HMAC_SHA1,
    KLUA_KCRYPTO_KIND_HMAC_SHA256,
    KLUA_KCRYPTO_KIND_CRC16,
    KLUA_KCRYPTO_KIND_CRC32,
}klua_kcrypto_kind_e;


/// @struct klua_kcrypto_ctx_t
/// @brief  stream context userdata
typedef struct klua_kcrypto_ctx_t_
{
    klua_kcrypto_kind_e kind;       ///< algorithm
    bool                finished;   ///< final already called
    int                 dig_len;    ///< digest bytes (hash/hmac)
    bool                crc_xor;    ///< apply xor on crc final
    uint32_t            crc_xormask;///< xor mask for crc final
    uint32_t            crc_outmask;///< 0xFFFF or 0xFFFFFFFF

    union
    {
        struct AVMD5*       p_md5;
        struct AVSHA*       p_sha;
        AVHMAC*             p_hmac;
        struct
        {
            const AVCRC*    p_tab;
            uint32_t        crc;
        }crc;
    }u;
}klua_kcrypto_ctx_t;


static void free_ctx_klua_kcrypto(klua_kcrypto_ctx_t* p_ctx)
{
    if (NULL == p_ctx)
    {
        return;
    }

    if (KLUA_KCRYPTO_KIND_MD5 == p_ctx->kind)
    {
        if (NULL != p_ctx->u.p_md5)
        {
            av_free(p_ctx->u.p_md5);
            p_ctx->u.p_md5 = NULL;
        }
    }
    else if (KLUA_KCRYPTO_KIND_SHA1 == p_ctx->kind || KLUA_KCRYPTO_KIND_SHA256 == p_ctx->kind)
    {
        if (NULL != p_ctx->u.p_sha)
        {
            av_free(p_ctx->u.p_sha);
            p_ctx->u.p_sha = NULL;
        }
    }
    else if (KLUA_KCRYPTO_KIND_HMAC_MD5 == p_ctx->kind
        || KLUA_KCRYPTO_KIND_HMAC_SHA1 == p_ctx->kind
        || KLUA_KCRYPTO_KIND_HMAC_SHA256 == p_ctx->kind)
    {
        if (NULL != p_ctx->u.p_hmac)
        {
            av_hmac_free(p_ctx->u.p_hmac);
            p_ctx->u.p_hmac = NULL;
        }
    }
    else
    {
        p_ctx->u.crc.p_tab = NULL;
    }
}


static klua_kcrypto_ctx_t* new_klua_kcrypto(lua_State* L)
{
    klua_kcrypto_ctx_t* p_ctx = (klua_kcrypto_ctx_t*)lua_newuserdata(L, sizeof(klua_kcrypto_ctx_t));
    KLB_MEMSET(p_ctx, 0, sizeof(klua_kcrypto_ctx_t));
    luaL_setmetatable(L, KLUA_KCRYPTO_HANDLE);
    return p_ctx;
}


static klua_kcrypto_ctx_t* to_klua_kcrypto(lua_State* L, int index)
{
    klua_kcrypto_ctx_t* p_ctx = (klua_kcrypto_ctx_t*)luaL_checkudata(L, index, KLUA_KCRYPTO_HANDLE);
    luaL_argcheck(L, NULL != p_ctx, index, "'kcrypto' expected");
    return p_ctx;
}


static int klua_kcrypto_tostring(lua_State* L)
{
    klua_kcrypto_ctx_t* p_ctx = to_klua_kcrypto(L, 1);
    lua_pushfstring(L, "kcrypto:%p", p_ctx);
    return 1;
}


static int klua_kcrypto_close(lua_State* L)
{
    klua_kcrypto_ctx_t* p_ctx = to_klua_kcrypto(L, 1);

    free_ctx_klua_kcrypto(p_ctx);
    p_ctx->finished = true;
    return 0;
}


static void require_open_klua_kcrypto(lua_State* L, klua_kcrypto_ctx_t* p_ctx)
{
    if (p_ctx->finished)
    {
        luaL_error(L, "kcrypto: already finished");
    }
}


static bool is_hash_kind_klua_kcrypto(klua_kcrypto_kind_e kind)
{
    if (KLUA_KCRYPTO_KIND_MD5 == kind
        || KLUA_KCRYPTO_KIND_SHA1 == kind
        || KLUA_KCRYPTO_KIND_SHA256 == kind
        || KLUA_KCRYPTO_KIND_HMAC_MD5 == kind
        || KLUA_KCRYPTO_KIND_HMAC_SHA1 == kind
        || KLUA_KCRYPTO_KIND_HMAC_SHA256 == kind)
    {
        return true;
    }

    return false;
}


static bool is_crc_kind_klua_kcrypto(klua_kcrypto_kind_e kind)
{
    if (KLUA_KCRYPTO_KIND_CRC16 == kind || KLUA_KCRYPTO_KIND_CRC32 == kind)
    {
        return true;
    }

    return false;
}


static int klua_kcrypto_update(lua_State* L)
{
    klua_kcrypto_ctx_t* p_ctx = to_klua_kcrypto(L, 1);
    size_t len = 0;
    const uint8_t* p_data = check_lstring_klua_kcrypto(L, 2, &len);
    int n = check_len_klua_kcrypto(L, len);

    require_open_klua_kcrypto(L, p_ctx);

    if (KLUA_KCRYPTO_KIND_MD5 == p_ctx->kind)
    {
        av_md5_update(p_ctx->u.p_md5, p_data, n);
    }
    else if (KLUA_KCRYPTO_KIND_SHA1 == p_ctx->kind || KLUA_KCRYPTO_KIND_SHA256 == p_ctx->kind)
    {
        av_sha_update(p_ctx->u.p_sha, p_data, (unsigned int)n);
    }
    else if (KLUA_KCRYPTO_KIND_HMAC_MD5 == p_ctx->kind
        || KLUA_KCRYPTO_KIND_HMAC_SHA1 == p_ctx->kind
        || KLUA_KCRYPTO_KIND_HMAC_SHA256 == p_ctx->kind)
    {
        av_hmac_update(p_ctx->u.p_hmac, p_data, (unsigned int)n);
    }
    else if (is_crc_kind_klua_kcrypto(p_ctx->kind))
    {
        p_ctx->u.crc.crc = av_crc(p_ctx->u.crc.p_tab, p_ctx->u.crc.crc, p_data, (size_t)n);
    }
    else
    {
        return luaL_error(L, "kcrypto: bad kind");
    }

    lua_settop(L, 1);
    return 1;
}


static int final_ctx_klua_kcrypto(lua_State* L, bool as_hex)
{
    klua_kcrypto_ctx_t* p_ctx = to_klua_kcrypto(L, 1);
    uint8_t dig[KLUA_KCRYPTO_SHA256_LEN];
    int nout = 0;
    uint32_t crc = 0;

    require_open_klua_kcrypto(L, p_ctx);

    if (is_crc_kind_klua_kcrypto(p_ctx->kind))
    {
        if (as_hex)
        {
            return luaL_error(L, "kcrypto: crc has no final_hex");
        }

        crc = p_ctx->u.crc.crc;
        if (p_ctx->crc_xor)
        {
            crc ^= p_ctx->crc_xormask;
        }

        p_ctx->finished = true;
        free_ctx_klua_kcrypto(p_ctx);
        lua_pushinteger(L, (lua_Integer)(crc & p_ctx->crc_outmask));
        return 1;
    }

    if (!is_hash_kind_klua_kcrypto(p_ctx->kind))
    {
        return luaL_error(L, "kcrypto: bad kind");
    }

    if (KLUA_KCRYPTO_KIND_MD5 == p_ctx->kind)
    {
        av_md5_final(p_ctx->u.p_md5, dig);
    }
    else if (KLUA_KCRYPTO_KIND_SHA1 == p_ctx->kind || KLUA_KCRYPTO_KIND_SHA256 == p_ctx->kind)
    {
        av_sha_final(p_ctx->u.p_sha, dig);
    }
    else
    {
        nout = av_hmac_final(p_ctx->u.p_hmac, dig, (unsigned int)p_ctx->dig_len);
        if (0 > nout)
        {
            return luaL_error(L, "kcrypto: hmac final failed");
        }
    }

    p_ctx->finished = true;
    free_ctx_klua_kcrypto(p_ctx);

    if (as_hex)
    {
        return push_hex_klua_kcrypto(L, dig, p_ctx->dig_len);
    }

    lua_pushlstring(L, (const char*)dig, (size_t)p_ctx->dig_len);
    return 1;
}


static int klua_kcrypto_final(lua_State* L)
{
    return final_ctx_klua_kcrypto(L, false);
}


static int klua_kcrypto_final_hex(lua_State* L)
{
    return final_ctx_klua_kcrypto(L, true);
}


static void klua_kcrypto_createmeta(lua_State* L)
{
    static luaL_Reg meth[] =
    {
        { "close",          klua_kcrypto_close },

        { "update",         klua_kcrypto_update },
        { "final",          klua_kcrypto_final },
        { "final_hex",      klua_kcrypto_final_hex },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] =
    {
        { "__index",        NULL },
        { "__gc",           klua_kcrypto_close },
        { "__close",        klua_kcrypto_close },
        { "__tostring",     klua_kcrypto_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KCRYPTO_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


static int init_md5_stream_klua_kcrypto(lua_State* L, klua_kcrypto_ctx_t* p_ctx)
{
    p_ctx->kind = KLUA_KCRYPTO_KIND_MD5;
    p_ctx->dig_len = KLUA_KCRYPTO_MD5_LEN;
    p_ctx->u.p_md5 = av_md5_alloc();
    if (NULL == p_ctx->u.p_md5)
    {
        return luaL_error(L, "kcrypto: out of memory");
    }

    av_md5_init(p_ctx->u.p_md5);
    return 1;
}


static int init_sha_stream_klua_kcrypto(lua_State* L, klua_kcrypto_ctx_t* p_ctx, int bits, int dig_len, klua_kcrypto_kind_e kind)
{
    int rc = 0;

    p_ctx->kind = kind;
    p_ctx->dig_len = dig_len;
    p_ctx->u.p_sha = av_sha_alloc();
    if (NULL == p_ctx->u.p_sha)
    {
        return luaL_error(L, "kcrypto: out of memory");
    }

    rc = av_sha_init(p_ctx->u.p_sha, bits);
    if (0 != rc)
    {
        av_free(p_ctx->u.p_sha);
        p_ctx->u.p_sha = NULL;
        return luaL_error(L, "kcrypto: sha init failed");
    }

    return 1;
}


static int init_hmac_stream_klua_kcrypto(lua_State* L, klua_kcrypto_ctx_t* p_ctx,
    enum AVHMACType type, int dig_len, klua_kcrypto_kind_e kind,
    const uint8_t* p_key, int key_n)
{
    p_ctx->kind = kind;
    p_ctx->dig_len = dig_len;
    p_ctx->u.p_hmac = av_hmac_alloc(type);
    if (NULL == p_ctx->u.p_hmac)
    {
        return luaL_error(L, "kcrypto: out of memory");
    }

    av_hmac_init(p_ctx->u.p_hmac, p_key, (unsigned int)key_n);
    return 1;
}


static int init_crc_stream_klua_kcrypto(lua_State* L, klua_kcrypto_ctx_t* p_ctx,
    bool is_crc16, AVCRCId id, uint32_t init, bool xor_out)
{
    const AVCRC* p_tab = NULL;

    if (is_crc16)
    {
        p_ctx->kind = KLUA_KCRYPTO_KIND_CRC16;
        p_ctx->crc_outmask = 0xFFFFu;
        p_ctx->crc_xormask = 0xFFFFu;
    }
    else
    {
        p_ctx->kind = KLUA_KCRYPTO_KIND_CRC32;
        p_ctx->crc_outmask = 0xFFFFFFFFu;
        p_ctx->crc_xormask = 0xFFFFFFFFu;
    }

    p_tab = av_crc_get_table(id);
    if (NULL == p_tab)
    {
        return luaL_error(L, "kcrypto: crc table unavailable");
    }

    p_ctx->crc_xor = xor_out;
    p_ctx->u.crc.p_tab = p_tab;
    p_ctx->u.crc.crc = init;
    return 1;
}


static int klua_kcrypto_new(lua_State* L)
{
    const char* p_type = luaL_checkstring(L, 1);
    klua_kcrypto_ctx_t* p_ctx = NULL;
    size_t key_len = 0;
    const uint8_t* p_key = NULL;
    int key_n = 0;
    AVCRCId crc_id = AV_CRC_16_ANSI_LE;
    uint32_t crc_init = 0xFFFFu;
    bool crc_xor = false;
    bool is_crc16 = false;

    // step1. read optional args BEFORE pushing userdata (avoid index clash)
    if (0 == strcmp(p_type, "hmac_md5")
        || 0 == strcmp(p_type, "hmac_sha1")
        || 0 == strcmp(p_type, "hmac_sha256"))
    {
        p_key = check_lstring_klua_kcrypto(L, 2, &key_len);
        key_n = check_len_klua_kcrypto(L, key_len);
    }
    else if (0 == strcmp(p_type, "crc16"))
    {
        is_crc16 = true;
        parse_crc16_id_klua_kcrypto(L, 2, &crc_id, &crc_init, &crc_xor);
    }
    else if (0 == strcmp(p_type, "crc32"))
    {
        is_crc16 = false;
        parse_crc32_id_klua_kcrypto(L, 2, &crc_id, &crc_init, &crc_xor);
    }

    // step2. create userdata
    p_ctx = new_klua_kcrypto(L);

    if (0 == strcmp(p_type, "md5"))
    {
        return init_md5_stream_klua_kcrypto(L, p_ctx);
    }
    else if (0 == strcmp(p_type, "sha1"))
    {
        return init_sha_stream_klua_kcrypto(L, p_ctx, 160, KLUA_KCRYPTO_SHA1_LEN, KLUA_KCRYPTO_KIND_SHA1);
    }
    else if (0 == strcmp(p_type, "sha256"))
    {
        return init_sha_stream_klua_kcrypto(L, p_ctx, 256, KLUA_KCRYPTO_SHA256_LEN, KLUA_KCRYPTO_KIND_SHA256);
    }
    else if (0 == strcmp(p_type, "hmac_md5"))
    {
        return init_hmac_stream_klua_kcrypto(L, p_ctx, AV_HMAC_MD5, KLUA_KCRYPTO_MD5_LEN,
            KLUA_KCRYPTO_KIND_HMAC_MD5, p_key, key_n);
    }
    else if (0 == strcmp(p_type, "hmac_sha1"))
    {
        return init_hmac_stream_klua_kcrypto(L, p_ctx, AV_HMAC_SHA1, KLUA_KCRYPTO_SHA1_LEN,
            KLUA_KCRYPTO_KIND_HMAC_SHA1, p_key, key_n);
    }
    else if (0 == strcmp(p_type, "hmac_sha256"))
    {
        return init_hmac_stream_klua_kcrypto(L, p_ctx, AV_HMAC_SHA256, KLUA_KCRYPTO_SHA256_LEN,
            KLUA_KCRYPTO_KIND_HMAC_SHA256, p_key, key_n);
    }
    else if (0 == strcmp(p_type, "crc16") || 0 == strcmp(p_type, "crc32"))
    {
        return init_crc_stream_klua_kcrypto(L, p_ctx, is_crc16, crc_id, crc_init, crc_xor);
    }

    return luaL_error(L, "kcrypto: bad type (md5|sha1|sha256|hmac_md5|hmac_sha1|hmac_sha256|crc16|crc32)");
}


//////////////////////////////////////////////////////////////////////////


int klua_open_kcrypto(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "new",                klua_kcrypto_new },

        { "md5",                klua_kcrypto_md5 },
        { "md5_hex",            klua_kcrypto_md5_hex },

        { "sha1",               klua_kcrypto_sha1 },
        { "sha1_hex",           klua_kcrypto_sha1_hex },
        { "sha256",             klua_kcrypto_sha256 },
        { "sha256_hex",         klua_kcrypto_sha256_hex },

        { "hmac_md5",           klua_kcrypto_hmac_md5 },
        { "hmac_md5_hex",       klua_kcrypto_hmac_md5_hex },
        { "hmac_sha1",          klua_kcrypto_hmac_sha1 },
        { "hmac_sha1_hex",      klua_kcrypto_hmac_sha1_hex },
        { "hmac_sha256",        klua_kcrypto_hmac_sha256 },
        { "hmac_sha256_hex",    klua_kcrypto_hmac_sha256_hex },

        { "crc16",              klua_kcrypto_crc16 },
        { "crc32",              klua_kcrypto_crc32 },

        { "base64_encode",      klua_kcrypto_base64_encode },
        { "base64_decode",      klua_kcrypto_base64_decode },

        { NULL,                 NULL }
    };

    // step1. stream metatable
    klua_kcrypto_createmeta(L);

    // step2. export library
    luaL_newlib(L, lib);

    return 1;
}

// end
