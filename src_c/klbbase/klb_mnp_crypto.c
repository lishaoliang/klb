// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbbase/klb_mnp_crypto.h"
#include "klbmem/klb_mem.h"
#include "libavutil/aes.h"
#include "libavutil/aes_ctr.h"
#include "libavutil/random_seed.h"
#ifndef __KLB_NO_SSL__
#include "mbedtls/aes.h"
#include "mbedtls/chacha20.h"
#endif // !__KLB_NO_SSL__
#include <string.h>
#include <assert.h>


/// CRYPTO_PSK 整帧硬顶(与 MNP 单块一致); 含帧头时由连接层再约束
#define KLB_MNP_CRYPTO_PSK_MAX      (KLB_MNP_BLOCK_SIZE_MAX)


static int ascii_tolower_klb_mnp_crypto(int c)
{
    if ('A' <= c && c <= 'Z')
    {
        return c - 'A' + 'a';
    }

    return c;
}


static bool name_eq_klb_mnp_crypto(const char* p_a, const char* p_b)
{
    int i = 0;

    if (NULL == p_a || NULL == p_b)
    {
        return false;
    }

    for (i = 0; ; i++)
    {
        int ca = ascii_tolower_klb_mnp_crypto((unsigned char)p_a[i]);
        int cb = ascii_tolower_klb_mnp_crypto((unsigned char)p_b[i]);

        if (ca != cb)
        {
            return false;
        }

        if (0 == ca)
        {
            return true;
        }
    }
}


static void side_reset_klb_mnp_crypto(klb_mnp_crypto_side_t* p_side)
{
    if (NULL == p_side)
    {
        return;
    }

    KLB_FREE(p_side->p_key);
    p_side->format = KLB_MNP_CRYPTO_NULL;
    p_side->key_size = 0;
}


KLB_API void klb_mnp_crypto_policy_init(klb_mnp_crypto_policy_t* p_pol)
{
    if (NULL == p_pol)
    {
        return;
    }

    memset(p_pol, 0, sizeof(*p_pol));
}


KLB_API void klb_mnp_crypto_policy_clear(klb_mnp_crypto_policy_t* p_pol)
{
    if (NULL == p_pol)
    {
        return;
    }

    side_reset_klb_mnp_crypto(&p_pol->tx);
    side_reset_klb_mnp_crypto(&p_pol->rx);
}


KLB_API bool klb_mnp_crypto_format_supported(uint32_t format)
{
    if (KLB_MNP_CRYPTO_NULL == format ||
        KLB_MNP_CRYPTO_XOR == format ||
        KLB_MNP_CRYPTO_AES_CTR == format ||
        KLB_MNP_CRYPTO_AES_OFB == format ||
        KLB_MNP_CRYPTO_AES_ECB == format ||
        KLB_MNP_CRYPTO_AES_CBC == format ||
        KLB_MNP_CRYPTO_CHACHA20 == format)
    {
        return true;
    }

    return false;
}


/// @brief 校验 format 对应密钥材料长度
static bool key_size_ok_klb_mnp_crypto(uint32_t format, uint32_t key_size)
{
    if (KLB_MNP_CRYPTO_XOR == format)
    {
        return (KLB_MNP_CRYPTO_XOR_KEY_MIN <= key_size) &&
            (key_size <= KLB_MNP_CRYPTO_KEY_SIZE_MAX);
    }

    if (KLB_MNP_CRYPTO_AES_CTR == format)
    {
        return (KLB_MNP_CRYPTO_AES_CTR_KEY_N == key_size);
    }

    if (KLB_MNP_CRYPTO_AES_OFB == format || KLB_MNP_CRYPTO_AES_CBC == format)
    {
        return (KLB_MNP_CRYPTO_AES_IV_KEY_N == key_size);
    }

    if (KLB_MNP_CRYPTO_AES_ECB == format)
    {
        return (KLB_MNP_CRYPTO_AES_ECB_KEY_N == key_size);
    }

    if (KLB_MNP_CRYPTO_CHACHA20 == format)
    {
        return (KLB_MNP_CRYPTO_CHACHA20_KEY_N == key_size);
    }

    return false;
}


KLB_API bool klb_mnp_crypto_packtype_exempt(int packtype)
{
    if (KLB_MNP_PING == packtype ||
        KLB_MNP_PONG == packtype ||
        KLB_MNP_OPTIONS == packtype)
    {
        return true;
    }

    return false;
}


KLB_API bool klb_mnp_crypto_packtype_controlled(int packtype)
{
    if (klb_mnp_crypto_packtype_exempt(packtype))
    {
        return false;
    }

    if (KLB_MNP_TEXT == packtype ||
        KLB_MNP_BINARY == packtype ||
        KLB_MNP_MEDIA == packtype ||
        KLB_MNP_RPC_LUA == packtype ||
        KLB_MNP_RPC_JSON == packtype ||
        KLB_MNP_RPC_EX == packtype)
    {
        return true;
    }

    if (KLB_MNP_PACKTYPE_USER_B <= packtype && packtype <= KLB_MNP_PACKTYPE_USER_E)
    {
        return true;
    }

    return false;
}


KLB_API int klb_mnp_crypto_subhdr_size(int packtype)
{
    if (KLB_MNP_TEXT == packtype || KLB_MNP_BINARY == packtype)
    {
        return (int)sizeof(klb_mnp_text_t);
    }

    if (KLB_MNP_MEDIA == packtype)
    {
        return (int)sizeof(klb_mnp_media_t);
    }

    if (KLB_MNP_RPC_LUA == packtype || KLB_MNP_RPC_JSON == packtype)
    {
        return (int)sizeof(klb_mnp_rpc_t);
    }

    if (KLB_MNP_RPC_EX == packtype)
    {
        return (int)sizeof(klb_mnp_rpcex_t);
    }

    if (KLB_MNP_PACKTYPE_USER_B <= packtype && packtype <= KLB_MNP_PACKTYPE_USER_E)
    {
        return (int)sizeof(klb_mnp_ptuser_t);
    }

    return -1;
}


KLB_API int klb_mnp_crypto_apply_psk(klb_mnp_crypto_side_t* p_side,
    uint32_t format, const uint8_t* p_key, uint32_t key_size)
{
    uint8_t* p_new = NULL;

    if (NULL == p_side)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    // step1. 禁止加密态切回 NULL
    if (KLB_MNP_CRYPTO_NULL == format)
    {
        if (KLB_MNP_CRYPTO_NULL != p_side->format)
        {
            return KLB_MNP_CRYPTO_EPROTO;
        }

        return KLB_MNP_CRYPTO_OK;
    }

    // step2. 本端能力
    if (!klb_mnp_crypto_format_supported(format))
    {
        return KLB_MNP_CRYPTO_ENOSUP;
    }

    // step3. 密钥材料长度与指针
    if (NULL == p_key || !key_size_ok_klb_mnp_crypto(format, key_size))
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    // step4. 拷贝新钥后再替换(失败保持旧态)
    if (0 < key_size)
    {
        p_new = KLB_MALLOC(uint8_t, (int)key_size, 0);
        memcpy(p_new, p_key, (size_t)key_size);
    }

    KLB_FREE(p_side->p_key);
    p_side->p_key = p_new;
    p_side->key_size = key_size;
    p_side->format = format;

    return KLB_MNP_CRYPTO_OK;
}


KLB_API int klb_mnp_crypto_set_tx(klb_mnp_crypto_policy_t* p_pol,
    uint32_t format, const uint8_t* p_key, uint32_t key_size)
{
    if (NULL == p_pol)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    return klb_mnp_crypto_apply_psk(&p_pol->tx, format, p_key, key_size);
}


KLB_API int klb_mnp_crypto_on_options_recv(klb_mnp_crypto_policy_t* p_pol,
    const uint8_t* p_payload, int payload_len)
{
    const klb_mnp_options_t* p_opt = NULL;
    const klb_mnp_crypto_t* p_c = NULL;
    const uint8_t* p_key = NULL;
    uint32_t body_len = 0;
    int need = 0;

    if (NULL == p_pol || NULL == p_payload || 0 > payload_len)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    if (payload_len < (int)sizeof(klb_mnp_options_t))
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    p_opt = (const klb_mnp_options_t*)p_payload;

    // 其它 OPTIONS 类型: 本模块不处理, 不改状态
    if (KLB_MNP_OPTIONS_CRYPTO_PSK != p_opt->options_type)
    {
        return KLB_MNP_CRYPTO_OK;
    }

    if (KLB_MNP_ENCODE_NULL != p_opt->encode)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    if ((uint32_t)payload_len != p_opt->size)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    if (sizeof(klb_mnp_crypto_t) != p_opt->head_size)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    need = (int)sizeof(klb_mnp_options_t) + (int)sizeof(klb_mnp_crypto_t);
    if (payload_len < need)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    p_c = (const klb_mnp_crypto_t*)(p_payload + sizeof(klb_mnp_options_t));
    body_len = p_opt->size - (uint32_t)sizeof(klb_mnp_options_t) - p_opt->head_size;

    if (body_len != p_c->key_size)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    if (payload_len != need + (int)p_c->key_size)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    if (0 < p_c->key_size)
    {
        p_key = p_payload + need;
    }

    // 对端宣告其发送方案 → 本端 RX
    return klb_mnp_crypto_apply_psk(&p_pol->rx, p_c->format, p_key, p_c->key_size);
}


KLB_API klb_buf_t* klb_mnp_crypto_pack_psk(int sequence,
    uint32_t format, const uint8_t* p_key, uint32_t key_size)
{
    klb_mnp_options_t opt;
    klb_mnp_crypto_t cry;
    int total = 0;
    klb_buf_t* p_buf = NULL;

    if (KLB_MNP_CRYPTO_NULL == format)
    {
        return NULL;
    }

    if (!klb_mnp_crypto_format_supported(format))
    {
        return NULL;
    }

    if (NULL == p_key || !key_size_ok_klb_mnp_crypto(format, key_size))
    {
        return NULL;
    }

    total = (int)sizeof(opt) + (int)sizeof(cry) + (int)key_size;
    if (KLB_MNP_CRYPTO_PSK_MAX < total + 8)
    {
        // 预留帧头 8B, 超过单包硬顶
        return NULL;
    }

    memset(&opt, 0, sizeof(opt));
    opt.size = (uint32_t)total;
    opt.options_type = KLB_MNP_OPTIONS_CRYPTO_PSK;
    opt.sequence = (uint32_t)sequence & 0xFFFFFu;
    opt.encode = KLB_MNP_ENCODE_NULL;
    opt.head_size = (uint32_t)sizeof(cry);

    memset(&cry, 0, sizeof(cry));
    cry.format = format;
    cry.key_size = key_size;

    p_buf = klb_buf_malloc(total, false);
    klb_buf_write(p_buf, (const char*)&opt, (int)sizeof(opt));
    klb_buf_write(p_buf, (const char*)&cry, (int)sizeof(cry));
    if (0 < (int)key_size)
    {
        klb_buf_write(p_buf, (const char*)p_key, (int)key_size);
    }

    return p_buf;
}


/// @brief 循环 XOR; 密钥按 key_size 循环
static int xform_xor_klb_mnp_crypto(const uint8_t* p_key, uint32_t key_size,
    uint8_t* p_body, int body_len)
{
    int i = 0;

    if (0 >= body_len)
    {
        return KLB_MNP_CRYPTO_OK;
    }

    if (NULL == p_key || KLB_MNP_CRYPTO_XOR_KEY_MIN > key_size)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    for (i = 0; i < body_len; i++)
    {
        p_body[i] ^= p_key[(uint32_t)i % key_size];
    }

    return KLB_MNP_CRYPTO_OK;
}


#ifndef __KLB_NO_SSL__

/// @brief AES-ECB(mbedtls); 仅整块; 尾字节保持原样
static int xform_aes_ecb_klb_mnp_crypto(const uint8_t* p_keymat,
    uint8_t* p_body, int body_len, bool encrypt)
{
    mbedtls_aes_context ctx;
    int blocks = 0;
    int mode = encrypt ? MBEDTLS_AES_ENCRYPT : MBEDTLS_AES_DECRYPT;
    int i = 0;
    int rc = 0;

    if (0 >= body_len)
    {
        return KLB_MNP_CRYPTO_OK;
    }

    blocks = body_len / 16;
    if (0 == blocks)
    {
        return KLB_MNP_CRYPTO_OK;
    }

    mbedtls_aes_init(&ctx);

    if (encrypt)
    {
        rc = mbedtls_aes_setkey_enc(&ctx, p_keymat, 128);
    }
    else
    {
        rc = mbedtls_aes_setkey_dec(&ctx, p_keymat, 128);
    }

    if (0 != rc)
    {
        mbedtls_aes_free(&ctx);
        return KLB_MNP_CRYPTO_EPROTO;
    }

    for (i = 0; i < blocks; i++)
    {
        rc = mbedtls_aes_crypt_ecb(&ctx, mode, p_body + (i * 16), p_body + (i * 16));
        if (0 != rc)
        {
            mbedtls_aes_free(&ctx);
            return KLB_MNP_CRYPTO_EPROTO;
        }
    }

    mbedtls_aes_free(&ctx);
    return KLB_MNP_CRYPTO_OK;
}

#endif // !__KLB_NO_SSL__


/// @brief AES-CTR(libavutil); iv 取自 PSK 尾部
static int xform_aes_ctr_klb_mnp_crypto(const uint8_t* p_keymat, const uint8_t* p_iv,
    uint8_t* p_body, int body_len)
{
    struct AVAESCTR* p_ctr = NULL;
    int rc = 0;

    if (0 >= body_len)
    {
        return KLB_MNP_CRYPTO_OK;
    }

    p_ctr = av_aes_ctr_alloc();
    if (NULL == p_ctr)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    rc = av_aes_ctr_init(p_ctr, p_keymat);
    if (0 != rc)
    {
        av_aes_ctr_free(p_ctr);
        return KLB_MNP_CRYPTO_EPROTO;
    }

    av_aes_ctr_set_iv(p_ctr, p_iv);
    av_aes_ctr_crypt(p_ctr, p_body, p_body, body_len);
    av_aes_ctr_free(p_ctr);
    return KLB_MNP_CRYPTO_OK;
}


#ifndef __KLB_NO_SSL__

/// @brief AES-OFB(mbedtls); iv 取自 PSK 尾部; 对称
static int xform_aes_ofb_klb_mnp_crypto(const uint8_t* p_keymat, const uint8_t* p_iv,
    uint8_t* p_body, int body_len)
{
    mbedtls_aes_context ctx;
    unsigned char iv[16];
    size_t iv_off = 0;
    int rc = 0;

    if (0 >= body_len)
    {
        return KLB_MNP_CRYPTO_OK;
    }

    mbedtls_aes_init(&ctx);
    rc = mbedtls_aes_setkey_enc(&ctx, p_keymat, 128);
    if (0 != rc)
    {
        mbedtls_aes_free(&ctx);
        return KLB_MNP_CRYPTO_EPROTO;
    }

    memcpy(iv, p_iv, sizeof(iv));
    rc = mbedtls_aes_crypt_ofb(&ctx, (size_t)body_len, &iv_off, iv, p_body, p_body);
    mbedtls_aes_free(&ctx);

    if (0 != rc)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    return KLB_MNP_CRYPTO_OK;
}


/// @brief AES-CBC(mbedtls); 仅整块; 尾字节保持原样; iv 取自 PSK 尾部
static int xform_aes_cbc_klb_mnp_crypto(const uint8_t* p_keymat, const uint8_t* p_iv,
    uint8_t* p_body, int body_len, bool encrypt)
{
    mbedtls_aes_context ctx;
    unsigned char iv[16];
    int blocks = 0;
    int mode = encrypt ? MBEDTLS_AES_ENCRYPT : MBEDTLS_AES_DECRYPT;
    int rc = 0;

    if (0 >= body_len)
    {
        return KLB_MNP_CRYPTO_OK;
    }

    blocks = body_len / 16;
    if (0 == blocks)
    {
        return KLB_MNP_CRYPTO_OK;
    }

    mbedtls_aes_init(&ctx);

    if (encrypt)
    {
        rc = mbedtls_aes_setkey_enc(&ctx, p_keymat, 128);
    }
    else
    {
        rc = mbedtls_aes_setkey_dec(&ctx, p_keymat, 128);
    }

    if (0 != rc)
    {
        mbedtls_aes_free(&ctx);
        return KLB_MNP_CRYPTO_EPROTO;
    }

    memcpy(iv, p_iv, sizeof(iv));
    rc = mbedtls_aes_crypt_cbc(&ctx, mode, (size_t)(blocks * 16), iv, p_body, p_body);
    mbedtls_aes_free(&ctx);

    if (0 != rc)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    return KLB_MNP_CRYPTO_OK;
}


/// @brief ChaCha20; counter=0; nonce 取自 PSK 尾部
static int xform_chacha20_klb_mnp_crypto(const uint8_t* p_keymat, const uint8_t* p_nonce,
    uint8_t* p_body, int body_len)
{
    int rc = 0;

    if (0 >= body_len)
    {
        return KLB_MNP_CRYPTO_OK;
    }

    rc = mbedtls_chacha20_crypt(p_keymat, p_nonce, 0,
        (size_t)body_len, p_body, p_body);
    if (0 != rc)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    return KLB_MNP_CRYPTO_OK;
}

#endif // !__KLB_NO_SSL__


KLB_API int klb_mnp_crypto_xform_payload(const klb_mnp_crypto_side_t* p_side,
    int packtype, uint8_t* p_data, int data_len, bool encrypt)
{
    int sub = 0;
    uint8_t* p_body = NULL;
    int body_len = 0;

    if (NULL == p_side)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    if (KLB_MNP_CRYPTO_NULL == p_side->format)
    {
        return KLB_MNP_CRYPTO_OK;
    }

    if (!klb_mnp_crypto_packtype_controlled(packtype))
    {
        return KLB_MNP_CRYPTO_OK;
    }

    if (NULL == p_data || 0 > data_len)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    sub = klb_mnp_crypto_subhdr_size(packtype);
    if (0 > sub)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    if (data_len < sub)
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    if (NULL == p_side->p_key || !key_size_ok_klb_mnp_crypto(p_side->format, p_side->key_size))
    {
        return KLB_MNP_CRYPTO_EPROTO;
    }

    p_body = p_data + sub;
    body_len = data_len - sub;

    // IV/nonce 随 PSK 尾部固定下发; 约束 keystream 复用的手段是换钥(rekey_after)
    if (KLB_MNP_CRYPTO_XOR == p_side->format)
    {
        return xform_xor_klb_mnp_crypto(p_side->p_key, p_side->key_size, p_body, body_len);
    }
    else if (KLB_MNP_CRYPTO_AES_CTR == p_side->format)
    {
        return xform_aes_ctr_klb_mnp_crypto(p_side->p_key, p_side->p_key + 16, p_body, body_len);
    }
#ifndef __KLB_NO_SSL__
    else if (KLB_MNP_CRYPTO_AES_OFB == p_side->format)
    {
        return xform_aes_ofb_klb_mnp_crypto(p_side->p_key, p_side->p_key + 16, p_body, body_len);
    }
    else if (KLB_MNP_CRYPTO_AES_ECB == p_side->format)
    {
        return xform_aes_ecb_klb_mnp_crypto(p_side->p_key, p_body, body_len, encrypt);
    }
    else if (KLB_MNP_CRYPTO_AES_CBC == p_side->format)
    {
        return xform_aes_cbc_klb_mnp_crypto(p_side->p_key, p_side->p_key + 16, p_body, body_len, encrypt);
    }
    else if (KLB_MNP_CRYPTO_CHACHA20 == p_side->format)
    {
        return xform_chacha20_klb_mnp_crypto(p_side->p_key, p_side->p_key + 32, p_body, body_len);
    }
#endif // !__KLB_NO_SSL__

    return KLB_MNP_CRYPTO_ENOSUP;
}


KLB_API void klb_mnp_crypto_fill_key(uint8_t* p_key, uint32_t key_size)
{
    uint32_t i = 0;

    if (NULL == p_key || 0 == key_size)
    {
        return;
    }

    // CSPRNG: 每次向系统熵源(/dev/urandom | BCrypt | arc4random)取 4B
    while (i < key_size)
    {
        uint32_t r = av_get_random_seed();
        uint32_t n = key_size - i;
        uint32_t k = 0;

        if (4 < n)
        {
            n = 4;
        }

        for (k = 0; k < n; k++)
        {
            p_key[i + k] = (uint8_t)(r >> (8 * k));
        }

        i += n;
    }
}


KLB_API int klb_mnp_crypto_format_from_name(const char* p_name, uint32_t* p_format)
{
    if (NULL == p_name || NULL == p_format)
    {
        return 1;
    }

    if (name_eq_klb_mnp_crypto(p_name, "AES_CTR"))
    {
        *p_format = KLB_MNP_CRYPTO_AES_CTR;
        return 0;
    }

    if (name_eq_klb_mnp_crypto(p_name, "AES_OFB"))
    {
        *p_format = KLB_MNP_CRYPTO_AES_OFB;
        return 0;
    }

    if (name_eq_klb_mnp_crypto(p_name, "CHACHA20"))
    {
        *p_format = KLB_MNP_CRYPTO_CHACHA20;
        return 0;
    }

    if (name_eq_klb_mnp_crypto(p_name, "AES_CBC"))
    {
        *p_format = KLB_MNP_CRYPTO_AES_CBC;
        return 0;
    }

    if (name_eq_klb_mnp_crypto(p_name, "AES_ECB"))
    {
        *p_format = KLB_MNP_CRYPTO_AES_ECB;
        return 0;
    }

    if (name_eq_klb_mnp_crypto(p_name, "XOR"))
    {
        *p_format = KLB_MNP_CRYPTO_XOR;
        return 0;
    }

    return 1;
}


// end
