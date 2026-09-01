// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_css.h"
#include "klbgui/klb_wnd_ex.h"
#include "klbgui/klbui_css_ex.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// CSS 辅助函数

void klbwuicss_background_image_mode(klbuicss_background_t* p_bg, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    assert(NULL != p_bg);

    if (KLBUI_CSSEX_get == method)
    {
        if (KLBUICSS_bgimg_mode_resize == p_bg->image_mode)
        {
            klb_map_set_idx_string(p_out, 0, "resize");
        }
        else if (KLBUICSS_bgimg_mode_scale9 == p_bg->image_mode)
        {
            klb_map_set_idx_string(p_out, 0, "scale9");
        }
        else
        {
            klb_map_set_idx_string(p_out, 0, "default");
        }
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);
        bool update = false;

        if (NULL == p_value)
        {
            return;
        }

        if (0 == strcmp(p_value, "resize"))
        {
            p_bg->image_mode = KLBUICSS_bgimg_mode_resize;
            update = true;
        }
        else if (0 == strcmp(p_value, "scale9"))
        {
            p_bg->image_mode = KLBUICSS_bgimg_mode_scale9;
            update = true;
        }
        else if (0 == strcmp(p_value, "default"))
        {
            p_bg->image_mode = KLBUICSS_bgimg_mode_default;
            update = true;
        }

        if (NULL != p_wnd && update)
        {
            klb_wnd_update(p_wnd);
        }
    }
}

void klbwuicss_background_image_color_key(klbuicss_background_t* p_bg, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    assert(NULL != p_bg);

    bool color_key = (0 != (p_bg->image_flags & KLBUICSS_bgimg_flag_color_key));
    klbuicssex_attribute_bool(&color_key, p_wnd, method, p_in, p_out);

    if (KLBUI_CSSEX_set == method)
    {
        if (color_key)
        {
            p_bg->image_flags |= KLBUICSS_bgimg_flag_color_key;
        }
        else
        {
            p_bg->image_flags &= (uint8_t)(~KLBUICSS_bgimg_flag_color_key);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CSS 扩展绘图函数

void klbwuicss_draw_background(klb_wnd_t* p_wnd, klb_rect_t* p_rect, klbuicss_background_t* p_bg_css)
{
    assert(NULL != p_wnd);
    assert(NULL != p_rect);

    if (NULL == p_bg_css)
    {
        return;
    }

    // step1. 背景图优先; 按 image_mode / image_flags 选绘制路径
    if (NULL != p_bg_css->image && 0 < sdslen(p_bg_css->image))
    {
        bool color_key = (0 != (p_bg_css->image_flags & KLBUICSS_bgimg_flag_color_key));

        if (KLBUICSS_bgimg_mode_resize == p_bg_css->image_mode)
        {
            if (color_key)
            {
                klb_wndex_draw_image_color_key(p_wnd, p_rect, p_bg_css->image);
            }
            else
            {
                klb_wndex_draw_image_resize(p_wnd, p_rect, p_bg_css->image);
            }
        }
        else if (KLBUICSS_bgimg_mode_scale9 == p_bg_css->image_mode)
        {
            if (color_key)
            {
                klb_wndex_draw_image_scale9_color_key(p_wnd, p_rect, p_bg_css->image);
            }
            else
            {
                klb_wndex_draw_image_scale9(p_wnd, p_rect, p_bg_css->image);
            }
        }
        else
        {
            if (color_key)
            {
                klb_wndex_draw_image_color_key(p_wnd, p_rect, p_bg_css->image);
            }
            else
            {
                klb_wnd_draw_image(p_wnd, p_rect, p_bg_css->image, NULL);
            }
        }
    }
    else
    {
        // step2. 纯色背景
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_bg_css->color);
    }
}

// end
