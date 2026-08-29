// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/klb_sds.h"
#include "klbmem/klb_mem.h"
#include <stdio.h>


sds klb_sdscpy(sds s, const char* p_str)
{
    if (NULL == p_str)
    {
        return s; // !注意 若 s = NULL, p_str = NULL, 依然 返回 NULL
    }

    sds dst = (NULL != s) ? s : sdsempty();
    return sdscpy(dst, p_str);
}

sds klb_sdscpy_adt(sds s, const klb_adt_t* p_adt)
{
    if (NULL == p_adt)
    {
        return s; // !注意 若 s = NULL, p_adt = NULL, 依然 返回 NULL
    }

    sds dst = (NULL != s) ? s : sdsempty();
    klb_adt_t* p_src = (klb_adt_t*)p_adt;

    switch (klb_adt_type(p_src))
    {
    case KLB_ADT_bool:
        {
            if (klb_adt_to_bool(p_src))
            {
                dst = sdscpy(dst, "true");
            }
            else
            {
                dst = sdscpy(dst, "false");
            }
        }
        break;
    case KLB_ADT_string:
        {
            int len = 0;
            const char* p_str = klb_adt_to_lstring(p_src, &len);
            dst = sdscpylen(dst, p_str, len);
        }
        break;
    case KLB_ADT_double:
        {
            char str[128] = {0};
            snprintf(str, sizeof(str), "%.6f", klb_adt_to_double(p_src));
            str[sizeof(str) - 1] = '\0'; // 末尾封口, 嵌入式/老 libc 在 snprintf 截断时可能不补 '\0'

            dst = sdscpy(dst, str);
        }
        break;
    case KLB_ADT_uint64:
        {
            sdsclear(dst);
            dst = sdscatfmt(dst, "%U", klb_adt_to_uint64(p_src));
        }
        break;
    case KLB_ADT_int64:
        {
            sdsclear(dst);
            dst = sdscatfmt(dst, "%I", klb_adt_to_int64(p_src));
        }
        break;
    default:
        {
            dst = sdscpy(dst, "");
        }
        break;
    }

    return dst;
}

sds klb_sds_assign(sds dst, const char* p_str)
{
    if (NULL == p_str || '\0' == p_str[0])
    {
        KLB_FREE_BY(dst, sdsfree);
        return NULL;
    }

    return klb_sdscpy(dst, p_str);
}

//end
