// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/klb_h26x.h"
#include <string.h>
#include <assert.h>


int klb_h26x_scan_nalu(char* p_h26x, int h26x_len, int* p_nal_start, int* p_nal_len, int* p_nal_h_len, bool* p_is_end)
{
    assert(NULL != p_h26x);
    assert(NULL != p_nal_start);
    assert(NULL != p_nal_len);
    // 从视频流中查找H264 H265的nal单元

    static uint8_t nal_h4[4] = { 0x00, 0x00, 0x00, 0x01 };
    static uint8_t nal_h3[3] = { 0x00, 0x00, 0x01 };

    int pos = 0, flag = -1, last_h_len = 0;

    while (pos + 4 < h26x_len)
    {
        if (0 == memcmp(p_h26x + pos, nal_h3, 3))
        {
            int h_len = 3;

            if ((0 < pos) && (0 == memcmp(p_h26x + pos - 1, nal_h4, 4)))
            {
                pos--;
                h_len = 4;
            }

            if (0 <= flag)
            {
                *p_nal_start = flag;
                *p_nal_len = pos - flag;

                if (NULL != p_nal_h_len)
                {
                    *p_nal_h_len = last_h_len;
                }

                if (NULL != p_is_end)
                {
                    *p_is_end = false;
                }

                return 0; // 找到了nal单元, 处于中间
            }

            flag = pos;
            last_h_len = h_len;
            pos += h_len;
        }
        else
        {
            pos += 1;
        }
    }

    if (0 <= flag)
    {
        *p_nal_start = flag;
        *p_nal_len = h26x_len - flag;

        if (NULL != p_nal_h_len)
        {
            *p_nal_h_len = last_h_len;
        }

        if (NULL != p_is_end)
        {
            *p_is_end = true;
        }

        return 0; // 找到了nal单元单处于末尾
    }

    return 1; // 没有找到nal单元
}

uint8_t klb_h264_nalu_type(uint8_t v)
{
    return v & 0x1F;
}

uint8_t klb_h265_nalu_type(uint8_t v)
{
    return (v >> 1) & 0x3F;
}
