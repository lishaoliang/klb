// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbbase/klb_string.h"
#include <string.h>
#include <assert.h>


/// @brief 在源字符串 中 查找 指定的字符串; 不需要'\0'结束
char* klb_strnstr_memcmp(const char* p_src, int src_len, const char* p_dst, int dst_len)
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

//end
