#include "klbapp/klb_app.h"
#include "klbapp/klb_app_in.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
static klb_app_t* g_klb_app = NULL;


int klb_app_main(int argc, char** argv)
{
    klb_app_t* p_app = klb_app_instance();
    assert(NULL != p_app);






    return 0;
}

klb_app_t* klb_app_instance()
{
    return g_klb_app;
}

int klb_app_init()
{
    assert(NULL == g_klb_app);

    g_klb_app = klb_app_create();

    return 0;
}

void klb_app_quit()
{
    KLB_FREE_BY(g_klb_app, klb_app_destroy);
}

//////////////////////////////////////////////////////////////////////////

/// @brief 创建
klb_app_t* klb_app_create()
{
    klb_app_t* p_app = KLB_MALLOCZ(klb_app_t, 1, 0);

    return p_app;
}

/// @brief 销毁
/// @return 无
void klb_app_destroy(klb_app_t* p_app)
{
    KLB_FREE(p_app);
}


/////////////////////////////////////////

/// @brief 注册app扩展
int klb_app_register_extension(klb_app_t* p_app, const char* p_name, const klb_app_extension_t* p_extension)
{
    return 0;
}

/// @brief 获取app扩展
void* klb_app_get_extension(klb_app_t* p_app, const char* p_name)
{
    return NULL;
}
