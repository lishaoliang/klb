#include "klbutil/klb_rand.h"
#include <stdlib.h>
#include <assert.h>


/// @brief 伪随机字符串
/// @param [in,out] *p_dest
/// @param [in]     str_len
/// @return char* 返回p_dest
/// @note 缓存长度需要 str_len + 1
char* klb_rand_string(char* p_dest, int str_len, bool set_end)
{
    static const char s_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static const int s_chars_len = sizeof(s_chars) - 1;

    char* ptr = p_dest;
    for (int i = 0; i < str_len; i++)
    {
        *ptr = s_chars[rand() % s_chars_len];
        ptr++;
    }

    if (set_end)
    {
        *ptr = '\0';
    }

    return p_dest;
}

/// @brief 伪随机首位不为0的正整数(10进制)
/// @param [in,out] *p_dest
/// @param [in]     str_len
/// @return char* 返回p_dest
/// @note 缓存长度需要 str_len + 1
char* klb_rand_integer(char* p_dest, int str_len, bool set_end)
{
    static const char s_num10[] = "0123456789";
    static const int  s_num10_len = sizeof(s_num10) - 1;

    char* ptr = p_dest;

    // 第一位, 不为0
    while (0 < str_len)
    {
        int idx = rand() % s_num10_len;
        if (0 != idx)
        {
            *ptr = s_num10[idx];
            ptr++;
            break;
        }
    }

    for (int i = 1; i < str_len; i++)
    {
        *ptr = s_num10[rand() % s_num10_len];
        ptr++;
    }

    if (set_end)
    {
        *ptr = '\0';
    }

    return p_dest;
}
