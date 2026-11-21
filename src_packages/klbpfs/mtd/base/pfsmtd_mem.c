// Doc Encode : UTF-8 BOM, Unix(LF)
// 平台堆内存 (malloc 族 + 对齐分配); 同步自 portfs base/pfs_mem.c
#include "base/pfsmtd_mem.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <malloc.h>
#endif

void *pfsmtd_malloc(size_t size)
{
    if (0 == size)
    {
        size = 1;
    }
    return malloc(size);
}

void *pfsmtd_mallocz(size_t size)
{
    size_t n = (0 == size) ? 1 : size;
    void *p = pfsmtd_malloc(n);
    if (NULL != p)
    {
        memset(p, 0, n);
    }
    return p;
}

void *pfsmtd_calloc(size_t nmemb, size_t size)
{
    if (0 == nmemb || 0 == size)
    {
        nmemb = 1;
        size = 1;
    }
    return calloc(nmemb, size);
}

void *pfsmtd_realloc(void *ptr, size_t size)
{
    if (0 == size)
    {
        pfsmtd_free(ptr);
        return NULL;
    }
    return realloc(ptr, size);
}

void pfsmtd_free(void *ptr)
{
    if (NULL != ptr)
    {
        free(ptr);
    }
}

void *pfsmtd_aligned_malloc(size_t align, size_t size)
{
    if (0 == align || 0 != (align & (align - 1U)) || align < sizeof(void *))
    {
        return NULL;
    }
    if (0 == size)
    {
        size = 1;
    }

#ifdef _WIN32
    return _aligned_malloc(size, align);
#else
    void *p = NULL;
    if (0 != posix_memalign(&p, align, size))
    {
        p = NULL;
    }
    return p;
#endif
}

void *pfsmtd_aligned_mallocz(size_t align, size_t size)
{
    size_t n = (0 == size) ? 1 : size;
    void *p = pfsmtd_aligned_malloc(align, n);
    if (NULL != p)
    {
        memset(p, 0, n);
    }
    return p;
}

void pfsmtd_aligned_free(void *ptr)
{
    if (NULL == ptr)
    {
        return;
    }
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

// end
