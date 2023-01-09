// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "wsdl_ui_video.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klb_gui.h"
#include "klbutil/klb_map.h"
#include "klbthird/sds.h"
#include "wsdl_extension.h"
#include "klbutil/CMap.hpp"



typedef struct wsdl_ui_video_t_
{
    wsdl_extension_t*   p_ex;           ///< ex

    int                 idx;            ///< 窗口序号

    bool                play;

    sds                 pic;            ///< 默认图片
}wsdl_ui_video_t;


static void wsdl_ui_video_destroy(klb_wnd_t* p_wnd)
{
    wsdl_ui_video_t* p_video = (wsdl_ui_video_t*)p_wnd->ctrl;

    KLB_FREE_BY(p_video->pic, sdsfree);
    KLB_FREE(p_wnd);
}

static int wsdl_ui_video_on_paint(klb_wnd_t* p_wnd)
{
    wsdl_ui_video_t* p_video = (wsdl_ui_video_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    if (p_video->play)
    {
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(0, 0, 0, 0));
    }
    else
    {
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 30, 30, 30));

        if (0 < sdslen(p_video->pic))
        {
            klb_wnd_draw_image(p_wnd, p_rect, p_video->pic, NULL);
        }
    }

    klb_wnd_draw_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 120, 120, 120));

    return 0;
}

static int wsdl_ui_video_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    wsdl_ui_video_t* p_video = (wsdl_ui_video_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_PAINT:
        return wsdl_ui_video_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int wsdl_ui_video_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    wsdl_ui_video_t* p_video = (wsdl_ui_video_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLB_WM_LBUTTONDOWN:
        break;
    default:
        break;
    }

    return 0;
}

static int wsdl_ui_video_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    wsdl_ui_video_t* p_video = (wsdl_ui_video_t*)p_wnd->ctrl;

    klua_env_t* p_env = klb_gui_get_klua_env(klb_wnd_get_gui(p_wnd));
    wsdl_extension_t* p_ex = kluaex_get_wsdl(p_env);

    klb::CMap r((klb_map_t*)p_map, true);

    if (r[0] == "picture")
    {
        p_video->pic = sdscpy(p_video->pic, r[1].ToString().c_str());
        klb_wnd_update(p_wnd);
    }
    else if(r[0] == "index")
    {
        p_video->idx = (int)r[1].ToInt64();
        klb_wnd_update(p_wnd);
    }
    else if(r[0] == "dst_rect")
    {
        klb_rect_t rect = { (int)r[1].ToInt64(), (int)r[2].ToInt64(), (int)r[3].ToInt64(), (int)r[4].ToInt64() };

        kluaex_wsdl_set_video_pos(p_ex, p_video->idx, &rect);

        klb_wnd_update(p_wnd);
    }

    return 0;
}

static klb_map_t* wsdl_ui_video_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    wsdl_ui_video_t* p_video = (wsdl_ui_video_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string((klb_map_t*)p_map, 0);

    klb_map_t* p_out = klb_map_create();
    if (0 == strcmp(p_key, "picture"))
    {
        klb_map_set_idx_string(p_out, 0, p_video->pic);
    }

    return p_out;
}

klb_wnd_t* wsdl_ui_video_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(wsdl_ui_video_t));
    wsdl_ui_video_t* p_video = (wsdl_ui_video_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = wsdl_ui_video_destroy;
    p_wnd->vtable.on_control = wsdl_ui_video_on_control;
    p_wnd->vtable.on_command = wsdl_ui_video_on_command;
    p_wnd->vtable.on_set = wsdl_ui_video_on_set;
    p_wnd->vtable.on_get = wsdl_ui_video_on_get;

    // init
    p_video->pic = sdsempty();

    p_video->play = true;

    return p_wnd;
}
