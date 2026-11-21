// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbtar/klbtar.h"


struct klbtar_t_
{
    int unused;     ///< 桩占位; 补实现时替换
};


//////////////////////////////////////////////////////////////////////////


int klbtar_probe(const char* p_path, int* p_fmt)
{
    if (NULL != p_fmt)
    {
        *p_fmt = KLBTAR_FMT_UNKNOWN;
    }

    if (NULL == p_path || '\0' == p_path[0])
    {
        return KLBTAR_ERR_ARG;
    }

    return KLBTAR_ERR_UNSUP;
}


klbtar_t* klbtar_open(const char* p_path)
{
    return klbtar_open_ex(p_path, NULL);
}


klbtar_t* klbtar_open_ex(const char* p_path, int* p_err)
{
    if (NULL != p_err)
    {
        *p_err = KLBTAR_ERR_UNSUP;
    }

    if (NULL == p_path || '\0' == p_path[0])
    {
        if (NULL != p_err)
        {
            *p_err = KLBTAR_ERR_ARG;
        }

        return NULL;
    }

    return NULL;
}


void klbtar_close(klbtar_t* p_tar)
{
    (void)p_tar;
}


int klbtar_extract(const char* p_path, const char* p_dest)
{
    if (NULL == p_path || '\0' == p_path[0] || NULL == p_dest || '\0' == p_dest[0])
    {
        return KLBTAR_ERR_ARG;
    }

    return KLBTAR_ERR_UNSUP;
}


// end
