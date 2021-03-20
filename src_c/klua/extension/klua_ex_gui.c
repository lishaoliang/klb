#include "klua/extension/klua_ex_gui.h"
#include "gui/klb_gui.h"
#include "mem/klb_mem.h"
#include <assert.h>


typedef struct klua_ex_gui_t_
{
    klua_env_t* p_env;              ///< Lua环境

    klb_gui_t*  p_gui;              ///< gui实例
}klua_ex_gui_t;


//////////////////////////////////////////////////////////////////////////

static void* klua_ex_gui_create(klua_env_t* p_env)
{
    klua_ex_gui_t* p_ex = KLB_MALLOC(klua_ex_gui_t, 1, 0);
    KLB_MEMSET(p_ex, 0, sizeof(klua_ex_gui_t));

    p_ex->p_env = p_env;

    p_ex->p_gui = klb_gui_create(NULL);

    return p_ex;
}


static void klua_ex_gui_destroy(void* ptr)
{
    klua_ex_gui_t* p_ex = (klua_ex_gui_t*)ptr;
    klua_env_t* p_env = p_ex->p_env;

    KLB_FREE_BY(p_ex->p_gui, klb_gui_destroy);
    KLB_FREE(p_ex);
}

static int klua_ex_gui_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_gui_t* p_ex = (klua_ex_gui_t*)ptr;

    klb_gui_loop_once(p_ex->p_gui, now);

    return 0;
}

//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////

int klua_ex_register_gui(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_gui_create;
    ex.cb_destroy = klua_ex_gui_destroy;
    ex.cb_loop_once = klua_ex_gui_loop_once;

    klua_env_register_extension(p_env, KLUA_EX_GUI_NAME, &ex);

    return 0;
}

klua_ex_gui_t* klua_ex_get_gui(klua_env_t* p_env)
{
    klua_ex_gui_t* p_ex = (klua_ex_gui_t*)klua_env_get_extension(p_env, KLUA_EX_GUI_NAME);
    return p_ex;
}
