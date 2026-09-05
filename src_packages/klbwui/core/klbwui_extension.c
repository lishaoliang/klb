// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_extension.h"
#include "klua/klua.h"
#include "klua/klua_gui.h"
#include "klbmem/klb_mem.h"


typedef struct klbwui_extension_t_
{
    klua_env_t*     p_env;
    klb_gui_t*      p_gui;
} klbwui_extension_t;


static void* klbwui_extension_create(klua_env_t* p_env)
{
    klbwui_extension_t* p_ex = KLB_MALLOCZ(klbwui_extension_t, 1, 0);

    p_ex->p_env = p_env;
    p_ex->p_gui = klua_gui_get(p_env);

    return p_ex;
}

static void klbwui_extension_destroy(void* ptr)
{
    klbwui_extension_t* p_ex = (klbwui_extension_t*)ptr;

    KLB_FREE(p_ex);
}

static int klbwui_extension_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{

    return 0;
}


klb_gui_t* kluaex_klbwui_get_gui(klbwui_extension_t* p_ex)
{
    if (NULL == p_ex)
    {
        return NULL;
    }

    return p_ex->p_gui;
}


static int kluaex_register_klbwui(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klbwui_extension_create;
    ex.cb_destroy = klbwui_extension_destroy;
    ex.cb_loop_once = klbwui_extension_loop_once;

    return klua_env_register_extension(p_env, KLUAEX_wui_extension, &ex);
}

klbwui_extension_t* kluaex_get_klbwui(klua_env_t* p_env)
{
    klbwui_extension_t* p_ex = (klbwui_extension_t*)klua_env_get_extension(p_env, KLUAEX_wui_extension);

    if (NULL == p_ex)
    {
        kluaex_register_klbwui(p_env);
        p_ex = (klbwui_extension_t*)klua_env_get_extension(p_env, KLUAEX_wui_extension);
    }

    return p_ex;
}

klbwui_extension_t* kluaex_get_klbwui_by_L(lua_State* L)
{
    return kluaex_get_klbwui(klua_env_get_by_L(L));
}

// end
