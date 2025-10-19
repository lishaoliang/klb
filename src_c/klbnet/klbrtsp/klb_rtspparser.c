// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klbrtsp/klb_rtspparser.h"
#include "klbbase/klb_string.h"
#include "klbbase/klb_mnp.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>


static const uint8_t KLB_RTSPPARSER_mark = 0x24; // "$"


static char s_rn_klb_rtsp[] = "\r\n";
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
    char* p_tail = klb_strnstr_memcmp(p_txt, txt_len, s_rnrn_klb_rtsp, sizeof(s_rnrn_klb_rtsp) - 1);

    if (NULL != p_tail)
    {
        int head_len = p_tail - p_txt + sizeof(s_rnrn_klb_rtsp) - 1;

        // 找到头结束后, 查看是否有长度定义
        char* p_length = klb_strnstr_memcmp(p_txt, head_len, s_con_len_klb_rtsp, sizeof(s_con_len_klb_rtsp) - 1);

        if (NULL == p_length)
        {
            p_length = klb_strnstr_memcmp(p_txt, head_len, s_con_len_1_klb_rtsp, sizeof(s_con_len_1_klb_rtsp) - 1);
        }

        if (NULL == p_length)
        {
            p_length = klb_strnstr_memcmp(p_txt, head_len, s_con_len_2_klb_rtsp, sizeof(s_con_len_2_klb_rtsp) - 1);
        }

        int body_len = 0;

        if (NULL != p_length)
        {
            int line_len = head_len - (int)(p_length - p_txt);
            char* p_length_tail = klb_strnstr_memcmp(p_length, line_len, s_rn_klb_rtsp, sizeof(s_rn_klb_rtsp) - 1);
            if (NULL != p_length_tail)
            {
                char tmp[256];
                int len = p_length_tail - p_length;
                int cp_len = MIN(len, sizeof(tmp) - 1);
                if (0 < cp_len)
                {
                    memcpy(tmp, p_length, cp_len);
                    tmp[cp_len] = '\0';

                    sscanf(tmp, "%*[^:]:%d", &body_len);
                }
            }
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


//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//+ -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//| V | P | X | CC | M | PT | sequence number                  |
//| +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+|
//| timestamp                                                      |
//| +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+|
//| synchronization source(SSRC) identifier                        |
//| +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+|
//| contributing source(CSRC) identifiers                          |
//| ....                                                           |
//| +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+|
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
        // http://www.ietf.org/rfc/rfc3550.txt

        uint8_t ch = ptr[1];            // 0,2RTP包; 1,3RTCP包
        int length = KLB_RB16(ptr + 2); // 包长度, 不含4字节头部
        length += 4;                    // 完整包长度, 包含4字节头部

        if (data_len < length)
        {
            return 1; // 数据长度不足
        }

        if (0 == (ch % 2))
        {
            // RTP 包
            if (length < (sizeof(klb_rtp_head_t) + 4))
            {
                return -1; // 数据错误
            }

            // RTP固定头
            klb_rtp_head_t* p_rtp = &p_parser->rtp_head;
            p_rtp->version = (ptr[4] >> 6) & 0x3;
            p_rtp->padding = (ptr[4] >> 5) & 0x1;
            p_rtp->extension = (ptr[4] >> 4) & 0x1;
            p_rtp->csrc_count = (ptr[4]) & 0xF;

            p_rtp->marker = (ptr[5] >> 7) & 0x1;
            p_rtp->payload_type = (ptr[5]) & 0x7F;

            p_rtp->sequence = KLB_RB16(ptr + 6);
            p_rtp->timestamp = KLB_RB32(ptr + 8);
            p_rtp->ssrc = KLB_RB32(ptr + 12);

            // 接下来的数据
            int pos = 16;
            int padding_len = (0 == p_rtp->padding) ? 0 : (ptr[length - 1]);

            // SSRC 列表
            if (0 < p_rtp->csrc_count)
            {
                pos += (p_rtp->csrc_count * 4);
            }

            // 扩展
            if (0 != p_rtp->extension)
            {
                // RFC 3550 和后续的 RFC 8285(标准化扩展机制) 定义
                klb_rtp_headex_t ex = { 0 };
                ex.profile_id = KLB_RB16(ptr + pos);
                ex.length = KLB_RB16(ptr + pos + 2);

                pos += sizeof(klb_rtp_headex_t);
                pos += (ex.length * 4);
            }

            if (length < pos + padding_len)
            {
                return -2; // 数据错误
            }

            // 解析结果
            p_parser->rtsptype = KLB_RTSPTYPE_rtp;
            p_parser->pack_len = length;

            p_parser->nalu_pos = pos;
            p_parser->nalu_len = length - pos - padding_len;

            assert(0 <= p_parser->nalu_len);
            assert(p_parser->nalu_len <= 65536);
            return 0;
        }
        else
        {
            // RTCP 包
            p_parser->rtsptype = KLB_RTSPTYPE_rtcp;
            p_parser->pack_len = length;

            return 0;
        }
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

int klb_rtspparser_parse_rtp_h264(klb_rtspparser_t* p_parser, char* p_nalu, int nalu_len)
{
    // 初始化解析结果值
    {
        p_parser->opt = KLB_MNP_FULL;
        p_parser->nalu_value = 0;
        p_parser->p_data = NULL;
        p_parser->data_len = 0;
    }

    // H264格式解包
    // http://www.ietf.org/rfc/rfc3984.txt
    uint8_t* ptr = (uint8_t*)p_nalu;
    int pos = 0;

    // nalu head
    klb_rtp_nalu_h264_t nalu = { 0 };
    nalu.nalu_type = ptr[pos] & 0x1F;

    uint8_t fnri_type = ptr[pos] & 0xE0;
    pos += 1;

    if (KLB_RTP_NALU_NULL == nalu.nalu_type || KLB_RTP_NALU_UNDEFINED_30 <= nalu.nalu_type)
    {
        return 1; // 忽略 这些包
    }
    else if (KLB_RTP_NALU_FU_A == nalu.nalu_type)
    {
        // 当进行 FU 分片时, 原始 NALU 头部的 F 和 NRI 会被提取出来, 作为所有分片的公共标识(复用在每个分片的 RTP 载荷前缀中)
        // FU 前缀(前一个字节) = (F << 7) | (NRI << 5) | 0x1C
        klb_rtp_nalu_fu_t fu = { 0 };
        fu.start = (ptr[pos] >> 7) & 0x1;
        fu.end = (ptr[pos] >> 6) & 0x1;
        fu.nalu_type = (ptr[pos]) & 0x1F;

        pos += 1;

        // 还原 h264 nal 类型
        // 0x67  SPS
        // 0x68  PPS
        // 0x65  I帧
        // 0x61, 0x41 P帧
        uint8_t nalu_value = fnri_type | fu.nalu_type;
        int opt = KLB_MNP_FULL;

        if (0 != fu.end)
        {
            opt = KLB_MNP_END;  // 结束
        }
        else if(0 != fu.start)
        {
            opt = KLB_MNP_BEGIN; // 起始
        }
        else
        {
            opt = KLB_MNP_CONTINUE; // 中间包
        }

        // 解析得到的值
        {
            p_parser->opt = opt;
            p_parser->nalu_value = nalu_value;
            p_parser->p_data = ptr + pos;
            p_parser->data_len = nalu_len - pos;
        }
    }
    else if(KLB_RTP_NALU_FU_B == nalu.nalu_type)
    {
        // 当进行 FU 分片时, 原始 NALU 头部的 F 和 NRI 会被提取出来, 作为所有分片的公共标识(复用在每个分片的 RTP 载荷前缀中)
        // FU 前缀 = (F << 7) | (NRI << 5) | 0x1C
        klb_rtp_nalu_fu_t fu = { 0 };
        fu.start = (ptr[pos] >> 7) & 0x1;
        fu.end = (ptr[pos] >> 6) & 0x1;
        fu.nalu_type = (ptr[pos]) & 0x1F;

        uint16_t done = KLB_RB16(ptr + 1);

        pos += 3;

        // 还原 h264 nal 类型
        // 0x67  SPS
        // 0x68  PPS
        // 0x65  I帧
        // 0x61, 0x41 P帧
        uint8_t nalu_value = fnri_type | fu.nalu_type;
        int opt = KLB_MNP_FULL;

        if (0 != fu.end)
        {
            opt = KLB_MNP_END;  // 结束
        }
        else if (0 != fu.start)
        {
            opt = KLB_MNP_BEGIN; // 起始
        }
        else
        {
            opt = KLB_MNP_CONTINUE; // 中间包
        }

        // 解析得到的值
        {
            p_parser->opt = opt;
            p_parser->nalu_value = nalu_value;
            p_parser->p_data = ptr + pos;
            p_parser->data_len = nalu_len - pos;
        }
    }
    else if(KLB_RTP_NALU_VCL_1 <= nalu.nalu_type && nalu.nalu_type <= KLB_RTP_NALU_VCL_23)
    {
        // 单分包
        uint8_t nalu_value = ptr[pos - 1];

        // 解析得到的值
        {
            p_parser->opt = KLB_MNP_FULL;
            p_parser->nalu_value = nalu_value;
            p_parser->p_data = ptr + pos;
            p_parser->data_len = nalu_len - pos;
        }
    }
    else
    {
        // 其他封包, 不支持
        return -1;
    }

    return 0;
}

//end
