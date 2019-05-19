#include "mem/klb_mem.h"
#include <assert.h>


void* klb_malloc(size_t size)
{
    void* p = malloc(size);
    assert(NULL != p);

    return p;
}

void klb_free(void* p)
{
    assert(NULL != p);

    free(p);
}
