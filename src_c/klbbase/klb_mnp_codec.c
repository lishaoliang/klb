// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbbase/klb_mnp_codec.h"
#include "klbmem/klb_mem.h"
#include <string.h>
#include <limits.h>
#include <assert.h>

#ifndef __KLB_NO_ZLIB__
#include "klbthird/zlib.h"
#endif


/// 明文段最大长度(协议 size 上限 2^27, 留头空间)
#define KLB_MNP_CODEC_SRC_MAX   ((int)((1u << 27) - 64u))


KLB_API bool klb_mnp_codec_compress_supported(void)
{
#ifndef __KLB_NO_ZLIB__
    return true;
#else
    return false;
#endif
}


KLB_API int klb_mnp_codec_tb_encode(const klb_buf_t* p_in)
{
    if (NULL == p_in)
    {
        return KLB_MNP_ENCODE_NULL;
    }

    int data_len = klb_buf_data_len((klb_buf_t*)p_in);
    if (data_len < (int)sizeof(klb_mnp_text_t))
    {
        return KLB_MNP_ENCODE_NULL;
    }

    const klb_mnp_text_t* p_txt = (const klb_mnp_text_t*)(p_in->p_buf + p_in->start);
    return (int)p_txt->encode;
}


KLB_API uint32_t klb_mnp_codec_tb_format(const klb_buf_t* p_in)
{
    if (NULL == p_in)
    {
        return 0;
    }

    int data_len = klb_buf_data_len((klb_buf_t*)p_in);
    if (data_len < (int)(sizeof(klb_mnp_text_t) + sizeof(klb_mnp_compress_t)))
    {
        return 0;
    }

    const klb_mnp_text_t* p_txt = (const klb_mnp_text_t*)(p_in->p_buf + p_in->start);
    if (KLB_MNP_ENCODE_COMPRESS != p_txt->encode)
    {
        return 0;
    }

    const klb_mnp_compress_t* p_c = (const klb_mnp_compress_t*)(p_txt + 1);
    return p_c->format;
}


#ifndef __KLB_NO_ZLIB__

static int zlib_compress_seg_klb_mnp_codec(const uint8_t* p_src, int src_len, uint8_t** pp_out, int* p_out_len)
{
    uLong bound = 0;

    if (0 > src_len || KLB_MNP_CODEC_SRC_MAX < src_len)
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    if (NULL == p_src && 0 != src_len)
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    bound = compressBound((uLong)src_len);
    if (0 == bound || (uLong)INT_MAX < bound)
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    {
        int dest_cap = (int)bound;
        uint8_t* p_dest = KLB_MALLOC(uint8_t, dest_cap, 0);
        uLongf out_len = (uLongf)dest_cap;
        const Bytef* p_in = (0 < src_len) ? (const Bytef*)p_src : (const Bytef*)"";
        int zret = compress2((Bytef*)p_dest, &out_len, p_in, (uLong)src_len, Z_DEFAULT_COMPRESSION);

        if (Z_MEM_ERROR == zret)
        {
            assert(false);
        }

        if (Z_OK != zret || (uLong)INT_MAX < out_len)
        {
            KLB_FREE(p_dest);
            return KLB_MNP_CODEC_EPROTO;
        }

        *pp_out = p_dest;
        *p_out_len = (int)out_len;
    }

    return KLB_MNP_CODEC_OK;
}


static int zlib_uncompress_seg_klb_mnp_codec(const uint8_t* p_src, int src_len, int expect_size, uint8_t** pp_out)
{
    if (0 > expect_size || KLB_MNP_CODEC_SRC_MAX < expect_size)
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    if (0 > src_len)
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    if (0 == expect_size)
    {
        *pp_out = KLB_MALLOC(uint8_t, 1, 0);
        return KLB_MNP_CODEC_OK;
    }

    if (NULL == p_src || 0 >= src_len)
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    {
        uint8_t* p_dest = KLB_MALLOC(uint8_t, expect_size, 0);
        uLongf dest_len = (uLongf)expect_size;
        int zret = uncompress((Bytef*)p_dest, &dest_len, (const Bytef*)p_src, (uLong)src_len);

        if (Z_MEM_ERROR == zret)
        {
            assert(false);
        }

        if (Z_OK != zret || (int)dest_len != expect_size)
        {
            KLB_FREE(p_dest);
            return KLB_MNP_CODEC_EPROTO;
        }

        *pp_out = p_dest;
    }

    return KLB_MNP_CODEC_OK;
}

#endif // !__KLB_NO_ZLIB__


static klb_buf_t* pack_null_klb_mnp_codec(int sequence,
    const uint8_t* p_head, int head_len,
    const uint8_t* p_body, int body_len)
{
    int payload_len = (int)sizeof(klb_mnp_text_t) + head_len + body_len;
    klb_buf_t* p_payload = klb_buf_malloc(payload_len, false);
    klb_mnp_text_t txt = { 0 };

    txt.size = (uint32_t)payload_len;
    txt.encode = KLB_MNP_ENCODE_NULL;
    txt.sequence = (uint32_t)sequence;
    txt.head_size = (uint32_t)head_len;

    klb_buf_write(p_payload, (const char*)&txt, (int)sizeof(txt));

    if (0 < head_len && NULL != p_head)
    {
        klb_buf_write(p_payload, (const char*)p_head, head_len);
    }

    if (0 < body_len && NULL != p_body)
    {
        klb_buf_write(p_payload, (const char*)p_body, body_len);
    }

    return p_payload;
}


#ifndef __KLB_NO_ZLIB__

static klb_buf_t* pack_compress_klb_mnp_codec(int sequence,
    const uint8_t* p_head, int head_len,
    const uint8_t* p_body, int body_len)
{
    uint8_t* p_zhead = NULL;
    uint8_t* p_zbody = NULL;
    int zhead_len = 0;
    int zbody_len = 0;
    int rc = 0;
    int head_wire = 0;
    int body_wire = 0;
    int payload_len = 0;
    klb_buf_t* p_payload = NULL;
    klb_mnp_text_t txt = { 0 };
    klb_mnp_compress_t c_head = { 0 };
    klb_mnp_compress_t c_body = { 0 };

    if (0 > head_len || 0 > body_len ||
        KLB_MNP_CODEC_SRC_MAX < head_len || KLB_MNP_CODEC_SRC_MAX < body_len)
    {
        return NULL;
    }

    rc = zlib_compress_seg_klb_mnp_codec(p_head, head_len, &p_zhead, &zhead_len);
    if (KLB_MNP_CODEC_OK != rc)
    {
        return NULL;
    }

    rc = zlib_compress_seg_klb_mnp_codec(p_body, body_len, &p_zbody, &zbody_len);
    if (KLB_MNP_CODEC_OK != rc)
    {
        KLB_FREE(p_zhead);
        return NULL;
    }

    head_wire = (int)sizeof(klb_mnp_compress_t) + zhead_len;
    body_wire = (int)sizeof(klb_mnp_compress_t) + zbody_len;
    payload_len = (int)sizeof(klb_mnp_text_t) + head_wire + body_wire;

    if (payload_len <= 0 || (1 << 27) <= payload_len)
    {
        KLB_FREE(p_zhead);
        KLB_FREE(p_zbody);
        return NULL;
    }

    p_payload = klb_buf_malloc(payload_len, false);

    txt.size = (uint32_t)payload_len;
    txt.encode = KLB_MNP_ENCODE_COMPRESS;
    txt.sequence = (uint32_t)sequence;
    txt.head_size = (uint32_t)head_wire;

    c_head.src_size = (uint32_t)head_len;
    c_head.format = KLB_MNP_COMPRESS_ZIP;
    c_body.src_size = (uint32_t)body_len;
    c_body.format = KLB_MNP_COMPRESS_ZIP;

    klb_buf_write(p_payload, (const char*)&txt, (int)sizeof(txt));
    klb_buf_write(p_payload, (const char*)&c_head, (int)sizeof(c_head));
    if (0 < zhead_len)
    {
        klb_buf_write(p_payload, (const char*)p_zhead, zhead_len);
    }

    klb_buf_write(p_payload, (const char*)&c_body, (int)sizeof(c_body));
    if (0 < zbody_len)
    {
        klb_buf_write(p_payload, (const char*)p_zbody, zbody_len);
    }

    KLB_FREE(p_zhead);
    KLB_FREE(p_zbody);
    return p_payload;
}

#endif // !__KLB_NO_ZLIB__


KLB_API klb_buf_t* klb_mnp_codec_pack_tb(int sequence, int encode,
    const uint8_t* p_head, int head_len,
    const uint8_t* p_body, int body_len)
{
    if (0 > head_len)
    {
        head_len = 0;
    }

    if (0 > body_len)
    {
        body_len = 0;
    }

    if (KLB_MNP_ENCODE_COMPRESS == encode)
    {
#ifndef __KLB_NO_ZLIB__
        return pack_compress_klb_mnp_codec(sequence, p_head, head_len, p_body, body_len);
#else
        return NULL;
#endif
    }

    return pack_null_klb_mnp_codec(sequence, p_head, head_len, p_body, body_len);
}


KLB_API int klb_mnp_codec_unpack_tb(klb_buf_t* p_in, klb_buf_t** pp_out)
{
    int data_len = 0;
    const klb_mnp_text_t* p_txt = NULL;
    int head_wire = 0;
    int body_wire = 0;

    if (NULL == p_in || NULL == pp_out)
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    *pp_out = NULL;
    data_len = klb_buf_data_len(p_in);
    if (data_len < (int)sizeof(klb_mnp_text_t))
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    p_txt = (const klb_mnp_text_t*)(p_in->p_buf + p_in->start);
    if ((int)p_txt->size != data_len)
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    if (KLB_MNP_ENCODE_NULL == p_txt->encode)
    {
        *pp_out = p_in;
        return KLB_MNP_CODEC_OK;
    }

    if (KLB_MNP_ENCODE_COMPRESS != p_txt->encode)
    {
        return KLB_MNP_CODEC_ENOSUP;
    }

#ifndef __KLB_NO_ZLIB__
    head_wire = (int)p_txt->head_size;
    body_wire = data_len - (int)sizeof(klb_mnp_text_t) - head_wire;
    if (0 > head_wire || 0 > body_wire)
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    {
        const uint8_t* p_wire = (const uint8_t*)(p_txt + 1);
        const klb_mnp_compress_t* p_ch = NULL;
        const klb_mnp_compress_t* p_cb = NULL;
        const uint8_t* p_zh = NULL;
        const uint8_t* p_zb = NULL;
        int zh_len = 0;
        int zb_len = 0;
        uint8_t* p_head = NULL;
        uint8_t* p_body = NULL;
        int rc = 0;
        klb_buf_t* p_plain = NULL;
        klb_mnp_text_t out = { 0 };
        int plain_len = 0;

        if (head_wire < (int)sizeof(klb_mnp_compress_t) ||
            body_wire < (int)sizeof(klb_mnp_compress_t))
        {
            return KLB_MNP_CODEC_EPROTO;
        }

        p_ch = (const klb_mnp_compress_t*)p_wire;
        zh_len = head_wire - (int)sizeof(klb_mnp_compress_t);
        p_zh = p_wire + sizeof(klb_mnp_compress_t);

        p_cb = (const klb_mnp_compress_t*)(p_wire + head_wire);
        zb_len = body_wire - (int)sizeof(klb_mnp_compress_t);
        p_zb = p_wire + head_wire + sizeof(klb_mnp_compress_t);

        if (KLB_MNP_COMPRESS_ZIP != p_ch->format || KLB_MNP_COMPRESS_ZIP != p_cb->format)
        {
            return KLB_MNP_CODEC_ENOSUP;
        }

        rc = zlib_uncompress_seg_klb_mnp_codec(p_zh, zh_len, (int)p_ch->src_size, &p_head);
        if (KLB_MNP_CODEC_OK != rc)
        {
            return rc;
        }

        rc = zlib_uncompress_seg_klb_mnp_codec(p_zb, zb_len, (int)p_cb->src_size, &p_body);
        if (KLB_MNP_CODEC_OK != rc)
        {
            KLB_FREE(p_head);
            return rc;
        }

        plain_len = (int)sizeof(klb_mnp_text_t) + (int)p_ch->src_size + (int)p_cb->src_size;
        p_plain = klb_buf_malloc(plain_len, false);

        out.size = (uint32_t)plain_len;
        out.encode = KLB_MNP_ENCODE_NULL;
        out.sequence = p_txt->sequence;
        out.head_size = p_ch->src_size;

        klb_buf_write(p_plain, (const char*)&out, (int)sizeof(out));
        if (0 < (int)p_ch->src_size)
        {
            klb_buf_write(p_plain, (const char*)p_head, (int)p_ch->src_size);
        }

        if (0 < (int)p_cb->src_size)
        {
            klb_buf_write(p_plain, (const char*)p_body, (int)p_cb->src_size);
        }

        KLB_FREE(p_head);
        KLB_FREE(p_body);
        *pp_out = p_plain;
    }

    return KLB_MNP_CODEC_OK;
#else
    (void)head_wire;
    (void)body_wire;
    return KLB_MNP_CODEC_ENOSUP;
#endif
}


KLB_API int klb_mnp_codec_text_send_encode(const klb_mnp_codec_text_policy_t* p_pol)
{
    if (NULL == p_pol)
    {
        return KLB_MNP_ENCODE_NULL;
    }

    if (!klb_mnp_codec_compress_supported())
    {
        return KLB_MNP_ENCODE_NULL;
    }

    if (p_pol->is_server)
    {
        if (p_pol->peer_locked && p_pol->peer_compress)
        {
            return KLB_MNP_ENCODE_COMPRESS;
        }

        return KLB_MNP_ENCODE_NULL;
    }

    if (p_pol->use_compress)
    {
        return KLB_MNP_ENCODE_COMPRESS;
    }

    return KLB_MNP_ENCODE_NULL;
}


KLB_API int klb_mnp_codec_text_on_recv(klb_mnp_codec_text_policy_t* p_pol, klb_buf_t** pp_data)
{
    klb_buf_t* p_in = NULL;
    int encode = 0;
    bool is_compress = false;
    uint32_t fmt = 0;
    klb_buf_t* p_out = NULL;
    int rc = 0;

    if (NULL == p_pol || NULL == pp_data || NULL == *pp_data)
    {
        return KLB_MNP_CODEC_EPROTO;
    }

    p_in = *pp_data;
    encode = klb_mnp_codec_tb_encode(p_in);
    is_compress = (KLB_MNP_ENCODE_COMPRESS == encode) ? true : false;

    if (is_compress)
    {
        if (!klb_mnp_codec_compress_supported())
        {
            return KLB_MNP_CODEC_ENOSUP;
        }

        // 客户端未开压却收到压缩: 服务端违规
        if (!p_pol->is_server && !p_pol->use_compress)
        {
            return KLB_MNP_CODEC_EPROTO;
        }

        fmt = klb_mnp_codec_tb_format(p_in);
        if (KLB_MNP_COMPRESS_ZIP != fmt)
        {
            return KLB_MNP_CODEC_ENOSUP;
        }
    }

    if (p_pol->is_server)
    {
        if (!p_pol->peer_locked)
        {
            p_pol->peer_locked = true;
            p_pol->peer_compress = is_compress;
            p_pol->peer_format = fmt;
        }
        else
        {
            if (p_pol->peer_compress != is_compress)
            {
                return KLB_MNP_CODEC_EPROTO;
            }

            if (is_compress && p_pol->peer_format != fmt)
            {
                return KLB_MNP_CODEC_EPROTO;
            }
        }
    }

    if (!is_compress)
    {
        return KLB_MNP_CODEC_OK;
    }

    rc = klb_mnp_codec_unpack_tb(p_in, &p_out);
    if (KLB_MNP_CODEC_OK != rc)
    {
        return rc;
    }

    if (p_out != p_in)
    {
        klb_buf_unref(p_in);
        *pp_data = p_out;
    }

    return KLB_MNP_CODEC_OK;
}


// end
