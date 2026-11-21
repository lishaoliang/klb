// Doc Encode : UTF-8 BOM, Unix(LF)
#include "alua/svn_ra/core/svn_ra_priv.h"

// MD5 + HMAC-MD5 (CRAM-MD5)

typedef struct svn_md5_ctx_t_
{
    uint32_t    state[4];
    uint32_t    count[2];
    uint8_t     buffer[64];
} svn_md5_ctx_t;


/// @brief 将 uint32 小端写入字节; words=4 得 16 字节 digest, words=2 得 8 字节 bitcount
void svn_md5_encode(uint8_t* p_out, const uint32_t* p_in, int words)
{
    int i = 0;
    int n = words * 4;

    for (i = 0; i < n; ++i)
    {
        p_out[i] = (uint8_t)((p_in[i >> 2] >> ((i % 4) * 8)) & 0xff);
    }
}


void svn_md5_transform(uint32_t state[4], const uint8_t block[64])
{
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t x[16] = { 0 };
    int i = 0;

    for (i = 0; i < 16; ++i)
    {
        x[i] = (uint32_t)block[i * 4]
            | ((uint32_t)block[i * 4 + 1] << 8)
            | ((uint32_t)block[i * 4 + 2] << 16)
            | ((uint32_t)block[i * 4 + 3] << 24);
    }

#define SVN_MD5_F(x_, y_, z_) (((x_) & (y_)) | ((~x_) & (z_)))
#define SVN_MD5_G(x_, y_, z_) (((x_) & (z_)) | ((y_) & (~z_)))
#define SVN_MD5_H(x_, y_, z_) ((x_) ^ (y_) ^ (z_))
#define SVN_MD5_I(x_, y_, z_) ((y_) ^ ((x_) | (~z_)))
#define SVN_MD5_ROTATE(x_, n_) (((x_) << (n_)) | ((x_) >> (32 - (n_))))

#define SVN_MD5_FF(a_, b_, c_, d_, x_, s_, ac_) \
    { \
        (a_) += SVN_MD5_F((b_), (c_), (d_)) + (x_) + (uint32_t)(ac_); \
        (a_) = SVN_MD5_ROTATE((a_), (s_)); \
        (a_) += (b_); \
    }
#define SVN_MD5_GG(a_, b_, c_, d_, x_, s_, ac_) \
    { \
        (a_) += SVN_MD5_G((b_), (c_), (d_)) + (x_) + (uint32_t)(ac_); \
        (a_) = SVN_MD5_ROTATE((a_), (s_)); \
        (a_) += (b_); \
    }
#define SVN_MD5_HH(a_, b_, c_, d_, x_, s_, ac_) \
    { \
        (a_) += SVN_MD5_H((b_), (c_), (d_)) + (x_) + (uint32_t)(ac_); \
        (a_) = SVN_MD5_ROTATE((a_), (s_)); \
        (a_) += (b_); \
    }
#define SVN_MD5_II(a_, b_, c_, d_, x_, s_, ac_) \
    { \
        (a_) += SVN_MD5_I((b_), (c_), (d_)) + (x_) + (uint32_t)(ac_); \
        (a_) = SVN_MD5_ROTATE((a_), (s_)); \
        (a_) += (b_); \
    }

    SVN_MD5_FF(a, b, c, d, x[0], 7, 0xd76aa478);
    SVN_MD5_FF(d, a, b, c, x[1], 12, 0xe8c7b756);
    SVN_MD5_FF(c, d, a, b, x[2], 17, 0x242070db);
    SVN_MD5_FF(b, c, d, a, x[3], 22, 0xc1bdceee);
    SVN_MD5_FF(a, b, c, d, x[4], 7, 0xf57c0faf);
    SVN_MD5_FF(d, a, b, c, x[5], 12, 0x4787c62a);
    SVN_MD5_FF(c, d, a, b, x[6], 17, 0xa8304613);
    SVN_MD5_FF(b, c, d, a, x[7], 22, 0xfd469501);
    SVN_MD5_FF(a, b, c, d, x[8], 7, 0x698098d8);
    SVN_MD5_FF(d, a, b, c, x[9], 12, 0x8b44f7af);
    SVN_MD5_FF(c, d, a, b, x[10], 17, 0xffff5bb1);
    SVN_MD5_FF(b, c, d, a, x[11], 22, 0x895cd7be);
    SVN_MD5_FF(a, b, c, d, x[12], 7, 0x6b901122);
    SVN_MD5_FF(d, a, b, c, x[13], 12, 0xfd987193);
    SVN_MD5_FF(c, d, a, b, x[14], 17, 0xa679438e);
    SVN_MD5_FF(b, c, d, a, x[15], 22, 0x49b40821);

    SVN_MD5_GG(a, b, c, d, x[1], 5, 0xf61e2562);
    SVN_MD5_GG(d, a, b, c, x[6], 9, 0xc040b340);
    SVN_MD5_GG(c, d, a, b, x[11], 14, 0x265e5a51);
    SVN_MD5_GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);
    SVN_MD5_GG(a, b, c, d, x[5], 5, 0xd62f105d);
    SVN_MD5_GG(d, a, b, c, x[10], 9, 0x02441453);
    SVN_MD5_GG(c, d, a, b, x[15], 14, 0xd8a1e681);
    SVN_MD5_GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);
    SVN_MD5_GG(a, b, c, d, x[9], 5, 0x21e1cde6);
    SVN_MD5_GG(d, a, b, c, x[14], 9, 0xc33707d6);
    SVN_MD5_GG(c, d, a, b, x[3], 14, 0xf4d50d87);
    SVN_MD5_GG(b, c, d, a, x[8], 20, 0x455a14ed);
    SVN_MD5_GG(a, b, c, d, x[13], 5, 0xa9e3e905);
    SVN_MD5_GG(d, a, b, c, x[2], 9, 0xfcefa3f8);
    SVN_MD5_GG(c, d, a, b, x[7], 14, 0x676f02d9);
    SVN_MD5_GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    SVN_MD5_HH(a, b, c, d, x[5], 4, 0xfffa3942);
    SVN_MD5_HH(d, a, b, c, x[8], 11, 0x8771f681);
    SVN_MD5_HH(c, d, a, b, x[11], 16, 0x6d9d6122);
    SVN_MD5_HH(b, c, d, a, x[14], 23, 0xfde5380c);
    SVN_MD5_HH(a, b, c, d, x[1], 4, 0xa4beea44);
    SVN_MD5_HH(d, a, b, c, x[4], 11, 0x4bdecfa9);
    SVN_MD5_HH(c, d, a, b, x[7], 16, 0xf6bb4b60);
    SVN_MD5_HH(b, c, d, a, x[10], 23, 0xbebfbc70);
    SVN_MD5_HH(a, b, c, d, x[13], 4, 0x289b7ec6);
    SVN_MD5_HH(d, a, b, c, x[0], 11, 0xeaa127fa);
    SVN_MD5_HH(c, d, a, b, x[3], 16, 0xd4ef3085);
    SVN_MD5_HH(b, c, d, a, x[6], 23, 0x04881d05);
    SVN_MD5_HH(a, b, c, d, x[9], 4, 0xd9d4d039);
    SVN_MD5_HH(d, a, b, c, x[12], 11, 0xe6db99e5);
    SVN_MD5_HH(c, d, a, b, x[15], 16, 0x1fa27cf8);
    SVN_MD5_HH(b, c, d, a, x[2], 23, 0xc4ac5665);

    SVN_MD5_II(a, b, c, d, x[0], 6, 0xf4292244);
    SVN_MD5_II(d, a, b, c, x[7], 10, 0x432aff97);
    SVN_MD5_II(c, d, a, b, x[14], 15, 0xab9423a7);
    SVN_MD5_II(b, c, d, a, x[5], 21, 0xfc93a039);
    SVN_MD5_II(a, b, c, d, x[12], 6, 0x655b59c3);
    SVN_MD5_II(d, a, b, c, x[3], 10, 0x8f0ccc92);
    SVN_MD5_II(c, d, a, b, x[10], 15, 0xffeff47d);
    SVN_MD5_II(b, c, d, a, x[1], 21, 0x85845dd1);
    SVN_MD5_II(a, b, c, d, x[8], 6, 0x6fa87e4f);
    SVN_MD5_II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    SVN_MD5_II(c, d, a, b, x[6], 15, 0xa3014314);
    SVN_MD5_II(b, c, d, a, x[13], 21, 0x4e0811a1);
    SVN_MD5_II(a, b, c, d, x[4], 6, 0xf7537e82);
    SVN_MD5_II(d, a, b, c, x[11], 10, 0xbd3af235);
    SVN_MD5_II(c, d, a, b, x[2], 15, 0x2ad7d2bb);
    SVN_MD5_II(b, c, d, a, x[9], 21, 0xeb86d391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}


void svn_md5_init(svn_md5_ctx_t* p_ctx)
{
    p_ctx->count[0] = 0;
    p_ctx->count[1] = 0;
    p_ctx->state[0] = 0x67452301;
    p_ctx->state[1] = 0xefcdab89;
    p_ctx->state[2] = 0x98badcfe;
    p_ctx->state[3] = 0x10325476;
}


void svn_md5_update(svn_md5_ctx_t* p_ctx, const uint8_t* p_in, size_t in_len)
{
    size_t i = 0;
    size_t index = 0;
    size_t part_len = 0;

    index = (size_t)((p_ctx->count[0] >> 3) & 0x3f);

    if ((p_ctx->count[0] += ((uint32_t)in_len << 3)) < ((uint32_t)in_len << 3))
    {
        ++p_ctx->count[1];
    }
    p_ctx->count[1] += ((uint32_t)in_len >> 29);

    part_len = 64 - index;

    if (in_len >= part_len)
    {
        memcpy(&p_ctx->buffer[index], p_in, part_len);
        svn_md5_transform(p_ctx->state, p_ctx->buffer);

        for (i = part_len; i + 63 < in_len; i += 64)
        {
            svn_md5_transform(p_ctx->state, &p_in[i]);
        }

        index = 0;
    }
    else
    {
        i = 0;
    }

    memcpy(&p_ctx->buffer[index], &p_in[i], in_len - i);
}


void svn_md5_final(uint8_t digest[16], svn_md5_ctx_t* p_ctx)
{
    uint8_t bits[8] = { 0 };
    uint8_t padding[64] = { 0 };
    size_t index = 0;
    size_t pad_len = 0;

    svn_md5_encode(bits, p_ctx->count, 2);

    index = (size_t)((p_ctx->count[0] >> 3) & 0x3f);
    pad_len = (index < 56) ? (56 - index) : (120 - index);
    padding[0] = 0x80;

    svn_md5_update(p_ctx, padding, pad_len);
    svn_md5_update(p_ctx, bits, 8);
    svn_md5_encode(digest, p_ctx->state, 4);

    memset(p_ctx, 0, sizeof(*p_ctx));
}


void svn_hmac_md5(const char* p_key, const char* p_data, uint8_t digest[16])
{
    uint8_t k_pad[64] = { 0 };
    uint8_t tk[16] = { 0 };
    svn_md5_ctx_t ctx = { 0 };
    size_t key_len = 0;
    int i = 0;

    if (NULL == p_key)
    {
        p_key = "";
    }
    if (NULL == p_data)
    {
        p_data = "";
    }

    key_len = strlen(p_key);
    if (key_len > 64)
    {
        svn_md5_init(&ctx);
        svn_md5_update(&ctx, (const uint8_t*)p_key, key_len);
        svn_md5_final(tk, &ctx);
        p_key = (const char*)tk;
        key_len = 16;
    }

    memcpy(k_pad, p_key, key_len);
    for (i = 0; i < 64; ++i)
    {
        k_pad[i] ^= 0x36;
    }

    svn_md5_init(&ctx);
    svn_md5_update(&ctx, k_pad, 64);
    svn_md5_update(&ctx, (const uint8_t*)p_data, strlen(p_data));
    svn_md5_final(digest, &ctx);

    for (i = 0; i < 64; ++i)
    {
        k_pad[i] = (uint8_t)((k_pad[i] ^ 0x36) ^ 0x5c);
    }

    svn_md5_init(&ctx);
    svn_md5_update(&ctx, k_pad, 64);
    svn_md5_update(&ctx, digest, 16);
    svn_md5_final(digest, &ctx);
}


void svn_hex_lower(const uint8_t* p_in, int in_len, char* p_out, int out_len)
{
    static const char hex[] = "0123456789abcdef";
    int i = 0;

    if (out_len < in_len * 2 + 1)
    {
        if (out_len > 0)
        {
            p_out[0] = '\0';
        }
        return;
    }

    for (i = 0; i < in_len; ++i)
    {
        p_out[i * 2] = hex[(p_in[i] >> 4) & 0x0f];
        p_out[i * 2 + 1] = hex[p_in[i] & 0x0f];
    }
    p_out[in_len * 2] = '\0';
}


void svn_ra_md5_hex(const uint8_t* p_data, int len, char out_hex[33])
{
    svn_md5_ctx_t ctx = { 0 };
    uint8_t digest[16] = { 0 };
    int i = 0;

    svn_md5_init(&ctx);
    if (NULL != p_data && len > 0)
    {
        svn_md5_update(&ctx, p_data, (size_t)len);
    }
    svn_md5_final(digest, &ctx);

    for (i = 0; i < 16; ++i)
    {
        snprintf(out_hex + i * 2, 3, "%02x", digest[i]);
    }
    out_hex[32] = '\0';
}


// SHA1 (update-report 的 sha1-checksum; 原 dav.lua 纯 Lua 实现过慢)

typedef struct svn_sha1_ctx_t_
{
    uint32_t    state[5];
    uint64_t    total;
    uint8_t     buffer[64];
    int         used;
} svn_sha1_ctx_t;


static uint32_t svn_sha1_rol(uint32_t value, int bits)
{
    return (value << bits) | (value >> (32 - bits));
}


static void svn_sha1_transform(uint32_t state[5], const uint8_t* p_block)
{
    uint32_t w[80] = { 0 };
    uint32_t a = 0;
    uint32_t b = 0;
    uint32_t c = 0;
    uint32_t d = 0;
    uint32_t e = 0;
    int i = 0;

    for (i = 0; i < 16; ++i)
    {
        w[i] = ((uint32_t)p_block[i * 4] << 24)
             | ((uint32_t)p_block[i * 4 + 1] << 16)
             | ((uint32_t)p_block[i * 4 + 2] << 8)
             | ((uint32_t)p_block[i * 4 + 3]);
    }

    for (i = 16; i < 80; ++i)
    {
        w[i] = svn_sha1_rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    for (i = 0; i < 80; ++i)
    {
        uint32_t f = 0;
        uint32_t k = 0;
        uint32_t t = 0;

        if (19 >= i)
        {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        }
        else if (39 >= i)
        {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        }
        else if (59 >= i)
        {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        }
        else
        {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        t = svn_sha1_rol(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = svn_sha1_rol(b, 30);
        b = a;
        a = t;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}


static void svn_sha1_init(svn_sha1_ctx_t* p_ctx)
{
    p_ctx->state[0] = 0x67452301;
    p_ctx->state[1] = 0xEFCDAB89;
    p_ctx->state[2] = 0x98BADCFE;
    p_ctx->state[3] = 0x10325476;
    p_ctx->state[4] = 0xC3D2E1F0;
    p_ctx->total = 0;
    p_ctx->used = 0;
}


static void svn_sha1_update(svn_sha1_ctx_t* p_ctx, const uint8_t* p_data, size_t len)
{
    size_t i = 0;

    p_ctx->total += (uint64_t)len;

    while (i < len)
    {
        size_t n = (size_t)(64 - p_ctx->used);

        if (n > (len - i))
        {
            n = len - i;
        }

        memcpy(p_ctx->buffer + p_ctx->used, p_data + i, n);
        p_ctx->used += (int)n;
        i += n;

        if (64 == p_ctx->used)
        {
            svn_sha1_transform(p_ctx->state, p_ctx->buffer);
            p_ctx->used = 0;
        }
    }
}


static void svn_sha1_final(svn_sha1_ctx_t* p_ctx, uint8_t digest[20])
{
    uint64_t bits = p_ctx->total * 8;
    int used = p_ctx->used;
    int i = 0;

    p_ctx->buffer[used] = 0x80;
    ++used;

    if (used > 56)
    {
        memset(p_ctx->buffer + used, 0, (size_t)(64 - used));
        svn_sha1_transform(p_ctx->state, p_ctx->buffer);
        used = 0;
    }

    memset(p_ctx->buffer + used, 0, (size_t)(56 - used));
    p_ctx->buffer[56] = (uint8_t)(bits >> 56);
    p_ctx->buffer[57] = (uint8_t)(bits >> 48);
    p_ctx->buffer[58] = (uint8_t)(bits >> 40);
    p_ctx->buffer[59] = (uint8_t)(bits >> 32);
    p_ctx->buffer[60] = (uint8_t)(bits >> 24);
    p_ctx->buffer[61] = (uint8_t)(bits >> 16);
    p_ctx->buffer[62] = (uint8_t)(bits >> 8);
    p_ctx->buffer[63] = (uint8_t)(bits);
    svn_sha1_transform(p_ctx->state, p_ctx->buffer);

    for (i = 0; i < 5; ++i)
    {
        uint32_t v = p_ctx->state[i];

        digest[i * 4] = (uint8_t)(v >> 24);
        digest[i * 4 + 1] = (uint8_t)(v >> 16);
        digest[i * 4 + 2] = (uint8_t)(v >> 8);
        digest[i * 4 + 3] = (uint8_t)v;
    }
}


void svn_ra_sha1_hex(const uint8_t* p_data, int len, char out_hex[41])
{
    svn_sha1_ctx_t ctx = { 0 };
    uint8_t digest[20] = { 0 };
    int i = 0;

    svn_sha1_init(&ctx);
    if (NULL != p_data && len > 0)
    {
        svn_sha1_update(&ctx, p_data, (size_t)len);
    }
    svn_sha1_final(&ctx, digest);

    for (i = 0; i < 20; ++i)
    {
        snprintf(out_hex + i * 2, 3, "%02x", digest[i]);
    }
    out_hex[40] = '\0';
}

// end
