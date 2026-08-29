// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/klbwui.h"
#include "klbwui/core/klbwui_widgets.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static int klbwui_register_embed_widgets(klb_gui_t* p_gui)
{
    int ret = 0;

    ret |= klbui_register_kbutton(p_gui);
    ret |= klbui_register_kdemo(p_gui);
    ret |= klbui_register_kpicture(p_gui);
    ret |= klbui_register_kstatic(p_gui);
    ret |= klbui_register_kview(p_gui);

    return ret;
}

static int klbwui_register_embed_shwnd(klb_gui_t* p_gui)
{
    (void)p_gui;

    // todo. 迁入 embed_shwnd/ 后在此注册共享窗
    return 0;
}

static int klbwui_register_sim_widgets(klb_gui_t* p_gui)
{
    (void)p_gui;

    // todo. 迁入 sim_widgets/ 后在此注册 PC 增补 type
    return 0;
}

static int klbwui_register_sim_shwnd(klb_gui_t* p_gui)
{
    (void)p_gui;

    // todo. 迁入 sim_shwnd/ 后在此注册 PC 增补共享窗
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// klbwui.h

int klbwui_register_embed(klb_gui_t* p_gui)
{
    int ret = 0;

    if (NULL == p_gui)
    {
        return -1;
    }

    ret |= klbwui_register_embed_widgets(p_gui);
    ret |= klbwui_register_embed_shwnd(p_gui);

    return ret;
}

int klbwui_register_sim(klb_gui_t* p_gui)
{
    int ret = 0;

    if (NULL == p_gui)
    {
        return -1;
    }

    ret |= klbwui_register_embed(p_gui);
    ret |= klbwui_register_sim_widgets(p_gui);
    ret |= klbwui_register_sim_shwnd(p_gui);

    return ret;
}

// end
