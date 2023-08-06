// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/klb_sds.h"
#include <string.h>
#include <stdio.h>


sds klb_sdscpy(sds s, const char* p_str)
{
    sds dst = (NULL != s) ? s : sdsempty();
    return sdscpy(dst, p_str);
}

sds klb_sdscpy_adt(sds s, const klb_adt_t* p_adt)
{
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
            char str[128];
            snprintf(str, sizeof(str) - 1, "%.6f", klb_adt_to_double(p_src));
            
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
