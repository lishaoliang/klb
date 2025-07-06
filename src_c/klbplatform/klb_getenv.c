#include "klbplatform/klb_getenv.h"
#include "klbmem/klb_mem.h"


#if defined(_WIN32)

#include <windows.h>

sds klb_getenv(const char* p_name)
{
    DWORD size = GetEnvironmentVariableA(p_name, NULL, 0);
    if (0 == size)
    {
        return NULL;
    }

    char* p_str = KLB_MALLOCZ(char, size + 4, 0);
    GetEnvironmentVariableA(p_name, p_str, size + 4);

    sds env = sdsnew(p_str);

    KLB_FREE(p_str);
    return env;
}

int klb_setenv(const char* p_name, const char* p_value, bool overwrite)
{
    if (NULL == p_name || NULL == p_value)
    {
        return -1;
    }

    if (!overwrite) 
    {
        if (GetEnvironmentVariableA(p_name, NULL, 0) > 0)
        {
            return 0;
        }
    }

    if (!SetEnvironmentVariableA(p_name, (NULL != p_value) ? p_value : NULL))
    {
        return -1;
    }

    return 0;
}

#else

sds klb_getenv(const char* p_name)
{
    if (NULL == p_name)
    {
        return NULL;
    }

    char* p_str = getenv(p_name);
    return sdsnew(p_str);
}

int klb_setenv(const char* p_name, const char* p_value, bool overwrite)
{
    if (NULL == p_name || NULL == p_value)
    {
        return -1;
    }

    return setenv(p_name, p_value, overwrite ? 1 : 0);
}

#endif
