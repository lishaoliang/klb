#include "platform/klb_dynamic_link.h"
#include "klbmem/klb_mem.h"

#if defined(_WIN32)
#include <windows.h>
typedef HMODULE     KLB_HMODULE;
#else
#include <dlfcn.h>
typedef void*       KLB_HMODULE;
#endif

typedef struct klb_dl_t_
{
    KLB_HMODULE     hmodule;
}klb_dl_t;


klb_dl_t* klb_dlopen(const char* p_path_name)
{
#if defined(_WIN32)
    KLB_HMODULE hmodule = LoadLibrary(p_path_name);
#else
    KLB_HMODULE hmodule = dlopen(p_path_name, RTLD_LAZY);
#endif

    if (NULL == hmodule)
    {
        return NULL;
    }

    klb_dl_t* p_dl = KLB_MALLOC(klb_dl_t, 1, 0);
    KLB_MEMSET(p_dl, 0, sizeof(klb_dl_t));

    p_dl->hmodule = hmodule;

    return p_dl;
}

void klb_dlclose(klb_dl_t* p_dl)
{
    assert(NULL != p_dl);

#if defined(_WIN32)
    FreeLibrary(p_dl->hmodule);
#else
    dlclose(p_dl->hmodule);
#endif

    KLB_FREE(p_dl);
}

void* klb_dlsym(klb_dl_t* p_dl, const char* p_procname)
{
#if defined(_WIN32)
    return (void*)GetProcAddress(p_dl->hmodule, p_procname);
#else
    return (void*)dlsym(p_dl->hmodule, p_procname);
#endif
}
