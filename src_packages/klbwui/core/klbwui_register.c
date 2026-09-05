// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/klbwui.h"
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_shwnd/klbshw_edit_menu.h"
#include "klbwui/embed_shwnd/klbshw_decimal_menu.h"
#include "klbwui/embed_shwnd/klbshw_combo_menu.h"
#include "klbwui/embed_shwnd/klbshw_decimal_menu_ip.h"
#include "klbwui/embed_shwnd/klbshw_calendar_menu.h"
#include "klbwui/embed_shwnd/klbshw_messagebox.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static int klbwui_register_embed_widgets(klb_gui_t* p_gui)
{
    int ret = 0;

    ret |= klbui_register_kanimation(p_gui);
    ret |= klbui_register_kbtnex(p_gui);
    ret |= klbui_register_kbutton(p_gui);
    ret |= klbui_register_kcalendar(p_gui);
    ret |= klbui_register_kcheck(p_gui);
    ret |= klbui_register_kcombo(p_gui);
    ret |= klbui_register_kdate(p_gui);
    ret |= klbui_register_kdemo(p_gui);
    ret |= klbui_register_kdialog(p_gui);
    ret |= klbui_register_kdiv(p_gui);
    ret |= klbui_register_kedit(p_gui);
    ret |= klbui_register_kgroup(p_gui);
    ret |= klbui_register_khscrollbar(p_gui);
    ret |= klbui_register_kip(p_gui);
    ret |= klbui_register_kline(p_gui);
    ret |= klbui_register_klist(p_gui);
    ret |= klbui_register_klistex(p_gui);
    ret |= klbui_register_kmenu(p_gui);
    ret |= klbui_register_knum(p_gui);
    ret |= klbui_register_kpassword(p_gui);
    ret |= klbui_register_kpicture(p_gui);
    ret |= klbui_register_kprogress(p_gui);
    ret |= klbui_register_kqrcode(p_gui);
    ret |= klbui_register_kradio(p_gui);
    ret |= klbui_register_krichtext(p_gui);
    ret |= klbui_register_kslider(p_gui);
    ret |= klbui_register_kspin(p_gui);
    ret |= klbui_register_kstatic(p_gui);
    ret |= klbui_register_ktab(p_gui);
    ret |= klbui_register_kticker(p_gui);
    ret |= klbui_register_ktime(p_gui);
    ret |= klbui_register_kview(p_gui);
    ret |= klbui_register_kvscrollbar(p_gui);
    ret |= klbui_register_kvslider(p_gui);

    return ret;
}

static int klbwui_register_embed_shwnd(klb_gui_t* p_gui)
{
    klbui_shwnd_get_edit_menu(p_gui);
    klbui_shwnd_get_decimal_menu(p_gui);
    klbui_shwnd_get_combo_menu(p_gui);
    klbui_shwnd_get_decimal_menu_ip(p_gui);
    klbui_shwnd_get_calendar_menu(p_gui);
    klbui_shwnd_get_messagebox(p_gui);
    return 0;
}

#if !defined(__KLB_NO_WUI_SIM__)
static int klbwui_register_sim_widgets(klb_gui_t* p_gui)
{

    // todo. 迁入 sim_widgets/ 后在此注册 PC 增补 type
    return 0;
}

static int klbwui_register_sim_shwnd(klb_gui_t* p_gui)
{

    // todo. 迁入 sim_shwnd/ 后在此注册 PC 增补共享窗
    return 0;
}
#endif


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
#if defined(__KLB_NO_WUI_SIM__)
    return klbwui_register_embed(p_gui);
#else
    int ret = 0;

    if (NULL == p_gui)
    {
        return -1;
    }

    ret |= klbwui_register_embed(p_gui);
    ret |= klbwui_register_sim_widgets(p_gui);
    ret |= klbwui_register_sim_shwnd(p_gui);

    return ret;
#endif
}

// end
