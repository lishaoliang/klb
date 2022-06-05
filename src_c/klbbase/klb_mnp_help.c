// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbbase/klb_mnp_help.h"
#include <string.h>
#include <assert.h>


#define KLB_MNP_BLOCK_DATA_MAX  (KLB_MNP_BLOCK_SIZE_MAX - sizeof(klb_mnp_t) - sizeof(klb_mnp_common_t))


klb_buf_t* klb_mnp_pack_heart()
{
    klb_mnp_t mnp = { 0 };
    mnp.magic = KLB_MNP_MAGIC;
    mnp.size = sizeof(klb_mnp_t);
    mnp.opt = KLB_MNP_FULL;
    mnp.packtype = KLB_MNP_HEART;

    klb_buf_t* p_buf = klb_buf_malloc(sizeof(klb_mnp_t), false);
    memcpy(p_buf->p_buf, &mnp, sizeof(klb_mnp_t));

    p_buf->end = sizeof(klb_mnp_t);

    return p_buf;
}

static int check_data_klb_mnp(klb_buf_t* p_buf)
{
    char* ptr = p_buf->p_buf + p_buf->start;
    int len = p_buf->end - p_buf->start;
    while (0 < len)
    {
        klb_mnp_t mnp = *((klb_mnp_t*)ptr);

        assert(KLB_MNP_MAGIC == mnp.magic);
        assert(mnp.packtype <= KLB_MNP_MEDIA);
        assert(mnp.size <= KLB_MNP_BLOCK_SIZE_MAX);

        ptr += mnp.size;
        len -= mnp.size;
    }
    assert(0 == len);

    return 0;
}

static klb_buf_t* pack_data_klb_mnp(klb_mnp_packtype_e packtype, uint32_t sequence, uint32_t uid, uint8_t* p_head, int head_len, uint8_t* p_body, int body_len)
{
    int num_head = (head_len + KLB_MNP_BLOCK_DATA_MAX - 1) / KLB_MNP_BLOCK_DATA_MAX;
    int num_body = (body_len + KLB_MNP_BLOCK_DATA_MAX - 1) / KLB_MNP_BLOCK_DATA_MAX;

    int full_len = head_len + body_len + (num_head + num_body) * sizeof(klb_mnp_t) + sizeof(klb_mnp_common_t);
    klb_buf_t* p_buf = klb_buf_malloc(full_len, false);

    klb_mnp_common_t com = { 0 };
    com.size = head_len + body_len + sizeof(klb_mnp_common_t);
    com.extra = head_len;
    com.sequence = sequence;
    com.uid = uid;

    if (num_head + num_body <= 1)
    {
        assert(1 == num_head + num_body);

        klb_mnp_t mnp = { 0 };
        mnp.magic = KLB_MNP_MAGIC;
        mnp.size = com.size + sizeof(klb_mnp_t);
        mnp.opt = KLB_MNP_FULL;
        mnp.packtype = packtype;

        int offset = 0;
        memcpy(p_buf->p_buf + offset, &mnp, sizeof(klb_mnp_t));             offset += sizeof(klb_mnp_t);
        memcpy(p_buf->p_buf + offset, &com, sizeof(klb_mnp_common_t));      offset += sizeof(klb_mnp_common_t);

        if (0 < head_len)
        {
            memcpy(p_buf->p_buf + offset, p_head, head_len);                offset += head_len;
        }

        if (0 < body_len)
        {
            memcpy(p_buf->p_buf + offset, p_body, body_len);                offset += body_len;
        }

        p_buf->end = offset;
    }
    else
    {
        bool first = true;
        bool end = false;
        int offset = 0;
        int total_len = full_len;

        char* ptr = (char*)p_head;
        int data_len = head_len;
        while (0 < data_len)
        {
            int cp_len = MIN(data_len, KLB_MNP_BLOCK_DATA_MAX);

            klb_mnp_t mnp = { 0 };
            mnp.magic = KLB_MNP_MAGIC;
            mnp.size = first ? (cp_len + sizeof(klb_mnp_t) + sizeof(klb_mnp_common_t)) : (cp_len + sizeof(klb_mnp_t));
            mnp.packtype = packtype;

            total_len -= mnp.size;
            if (total_len <= 0)
            {
                end = true;
            }

            if (first)
            {
                mnp.opt = KLB_MNP_BEGIN;
            }
            else
            {
                mnp.opt = end ? KLB_MNP_END : KLB_MNP_CONTINUE;
            }

            memcpy(p_buf->p_buf + offset, &mnp, sizeof(klb_mnp_t));             offset += sizeof(klb_mnp_t);

            if (first)
            {
                memcpy(p_buf->p_buf + offset, &com, sizeof(klb_mnp_common_t));  offset += sizeof(klb_mnp_common_t);
                first = false;
            }

            memcpy(p_buf->p_buf + offset, ptr, cp_len);                         offset += cp_len;

            data_len -= cp_len;
            ptr += cp_len;
        }

        ptr = (char*)p_body;
        data_len = body_len;
        while (0 < data_len)
        {
            int cp_len = MIN(data_len, KLB_MNP_BLOCK_DATA_MAX);

            klb_mnp_t mnp = { 0 };
            mnp.magic = KLB_MNP_MAGIC;
            mnp.size = first ? (cp_len + sizeof(klb_mnp_t) + sizeof(klb_mnp_common_t)) : (cp_len + sizeof(klb_mnp_t));
            mnp.packtype = packtype;

            total_len -= mnp.size;
            if (total_len <= 0)
            {
                end = true;
            }

            if (first)
            {
                mnp.opt = KLB_MNP_BEGIN;
            }
            else
            {
                mnp.opt = end ? KLB_MNP_END : KLB_MNP_CONTINUE;
            }

            memcpy(p_buf->p_buf + offset, &mnp, sizeof(klb_mnp_t));             offset += sizeof(klb_mnp_t);

            if (first)
            {
                memcpy(p_buf->p_buf + offset, &com, sizeof(klb_mnp_common_t));  offset += sizeof(klb_mnp_common_t);
                first = false;
            }

            memcpy(p_buf->p_buf + offset, ptr, cp_len);                         offset += cp_len;

            data_len -= cp_len;
            ptr += cp_len;
        }

        assert(0 == total_len);
        assert(offset == full_len);

        p_buf->end = offset;
    }

    assert(0 == check_data_klb_mnp(p_buf));
    return p_buf;
}

klb_buf_t* klb_mnp_pack_text(uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    if (head_len <= 0 && body_len <= 0)
    {
        return NULL;
    }

    return pack_data_klb_mnp(KLB_MNP_TXT, sequence, uid, (uint8_t*)p_head, head_len, (uint8_t*)p_body, body_len);
}

klb_buf_t* klb_mnp_pack_binary(uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    if (head_len <= 0 && body_len <= 0)
    {
        return NULL;
    }

    return pack_data_klb_mnp(KLB_MNP_BIN, sequence, uid, (uint8_t*)p_head, head_len, (uint8_t*)p_body, body_len);
}

klb_buf_t* klb_mnp_pack_media()
{
    return NULL;
}
