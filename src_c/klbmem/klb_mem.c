// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_mem.h"
#include <stdlib.h>
#include <assert.h>


#if defined(_WIN32)

#include <malloc.h>

void* klb_aligned_malloc(size_t size, size_t align)
{
    void* ptr = _aligned_malloc(size, align);
    assert(NULL != ptr);
    return ptr;
}

#else

void* klb_aligned_malloc(size_t size, size_t align)
{
    void* ptr = NULL;
    int ret = posix_memalign(&ptr, align, size);

    assert(0 == ret && NULL != ptr);
    return ptr;
}

#endif

void* klb_malloc(size_t size)
{
    if (size <= 0)
    {
        size = 1; // 最小1字节(避免 size==0 的歧义)
    }

    void* ptr = malloc(size);
    assert(NULL != ptr); // fatal-on-OOM: 分配失败立即崩溃
    return ptr;
}

void* klb_mallocz(size_t size)
{
    void* p = KLB_MALLOC(char, size, 0); // fail-fast: klb_malloc 内部 assert
    KLB_MEMSET(p, 0, size);

    return p;
}

void* klb_realloc(void* p, size_t size)
{
    void* ptr = NULL;
    if (0 == size) { if (NULL != p) free(p); return NULL; } // size==0: 释放旧块, 避免 realloc(p,0) 语义歧义
    ptr = realloc(p, size);
    // OOM (size>0): 返回 NULL, 且 p 非空时原块保持不变
    return ptr;
}

void* klb_reallocz(void* p, size_t size)
{
    void* ptr = klb_realloc(p, size);
    if (NULL == ptr) return NULL; // 非 fail-fast: 调用方须处理
    if (0 < size) memset(ptr, 0, size); // 成功时整块清零
    return ptr;
}

// end
