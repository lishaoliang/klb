// Doc Encode : UTF-8 BOM, Unix(LF)
// 平台堆内存 (malloc 族 + 对齐分配)
#include "base/pfs_mem.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <malloc.h>
#endif

void *pfs_malloc(size_t size)
{
    if (0 == size)
    {
        size = 1;
    }
    return malloc(size);
}

void *pfs_mallocz(size_t size)
{
    size_t n = (0 == size) ? 1 : size;
    void *p = pfs_malloc(n);

    if (NULL != p)
    {
        memset(p, 0, n);
    }
    return p;
}

void *pfs_calloc(size_t nmemb, size_t size)
{
    if (0 == nmemb || 0 == size)
    {
        nmemb = 1;
        size = 1;
    }
    return calloc(nmemb, size);
}

void *pfs_realloc(void *ptr, size_t size)
{
    if (0 == size)
    {
        pfs_free(ptr);
        return NULL;
    }
    return realloc(ptr, size);
}

void pfs_free(void *ptr)
{
    if (NULL != ptr)
    {
        free(ptr);
    }
}

void *pfs_aligned_malloc(size_t align, size_t size)
{
    if (0 == align || 0 != (align & (align - 1U)) || align < sizeof(void *))
    {
        return NULL;
    }
    if (0 == size)
    {
        size = 1;
    }

    void *p = NULL;

#ifdef _WIN32
    p = _aligned_malloc(size, align);
#else
    if (0 != posix_memalign(&p, align, size))
    {
        p = NULL;
    }
#endif
    return p;
}

void *pfs_aligned_mallocz(size_t align, size_t size)
{
    size_t n = (0 == size) ? 1 : size;
    void *p = pfs_aligned_malloc(align, n);

    if (NULL != p)
    {
        memset(p, 0, n);
    }
    return p;
}

void pfs_aligned_free(void *ptr)
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
