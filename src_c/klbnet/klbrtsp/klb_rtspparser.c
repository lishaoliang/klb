// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klbrtsp/klb_rtspparser.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

static const uint8_t KLB_RTSPPARSER_mark = 0x24; // "$"


static char* klb_strnstr(const char* p_src, size_t src_len, const char* p_dst, size_t dst_len)
{
    assert(NULL != p_src);
    assert(NULL != p_dst);

    if (dst_len <= 0)
    {
        //查找目标为空, 则直接被认为找到头部
        return (char*)p_src;
    }

    while (dst_len <= src_len)
    {
        src_len--;
        if (!memcmp(p_src, p_dst, dst_len))
        {
            return (char*)p_src;
        }

        p_src++;
    }

    return NULL;
}

static char s_rnrn_klb_rtsp[] = "\r\n\r\n";
static char s_con_len_klb_rtsp[] = "Content-Length";
static char s_con_len_1_klb_rtsp[] = "Content-length";
static char s_con_len_2_klb_rtsp[] = "content-length";

static int klb_rtsp_is_full(char* p_txt, int txt_len, int* p_real_len)
{
    assert(NULL != p_txt);
    assert(0 <= txt_len);

    // 注意网络来的数据, 末尾不一定为'\0'
    if (txt_len < sizeof(s_rnrn_klb_rtsp))
    {
        return 1;
    }

    // 找到头末尾
    char* p_tail = klb_strnstr(p_txt, txt_len, s_rnrn_klb_rtsp, sizeof(s_rnrn_klb_rtsp) - 1);

    if (NULL != p_tail)
    {
        int head_len = p_tail - p_txt + sizeof(s_rnrn_klb_rtsp) - 1;

        // 找到头结束后, 查看是否有长度定义
        char* p_length = klb_strnstr(p_txt, head_len, s_con_len_klb_rtsp, sizeof(s_con_len_klb_rtsp) - 1);

        if (NULL == p_length)
        {
            p_length = klb_strnstr(p_txt, head_len, s_con_len_1_klb_rtsp, sizeof(s_con_len_1_klb_rtsp) - 1);
        }

        if (NULL == p_length)
        {
            p_length = klb_strnstr(p_txt, head_len, s_con_len_2_klb_rtsp, sizeof(s_con_len_2_klb_rtsp) - 1);
        }

        int body_len = 0;

        if (NULL != p_length)
        {
            sscanf(p_length, "%*[^:]:%d", &body_len);
        }

        if (head_len + body_len <= txt_len)
        {
            if (NULL != p_real_len)
            {
                *p_real_len = head_len + body_len;
            }

            return 0;   // 数据完整
        }
    }

    return 1; // 数据不完整
}

int klb_rtspparser_parse(klb_rtspparser_t* p_parser, char* p_data, int data_len)
{
    uint8_t* ptr = (uint8_t*)p_data;

    if (data_len <= 4)
    {
        return 1; // 数据长度不足
    }

    // 第一个字符为 "$", 则为 RTP/RTCP包
    // 否则 为 RTSP包
    if (KLB_RTSPPARSER_mark == ptr[0])
    {
        // RTP / RTCP 包

        // todo.
        p_parser->rtsptype = KLB_RTSPTYPE_rtcp;
        p_parser->pack_len = data_len;
        return 0;
    }
    else
    {
        // RTSP 包

        int length = 0;
        if (0 == klb_rtsp_is_full(p_data, data_len, &length))
        {
            p_parser->rtsptype = KLB_RTSPTYPE_rtsp;
            p_parser->pack_len = length;

            return 0;
        }

    }

    return 1; // 数据长度不足
}

//end
