#include "wsdl_wnd.h"
#include "klbmem/klb_mem.h"


typedef struct wsdl_wnd_t_
{
    int a;
}wsdl_wnd_t;


wsdl_wnd_t* wsdl_wnd_create()
{
    wsdl_wnd_t* p_wnd = KLB_MALLOCZ(wsdl_wnd_t, 1, 0);



    return p_wnd;
}

void wsdl_wnd_destroy(wsdl_wnd_t* p_wnd)
{
    KLB_FREE(p_wnd);
}
