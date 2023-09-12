#include "klbgui/klbui_css.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klb_gui_in.h"


//////////////////////////////////////////////////////////////////////////
// CSS辅助函数

bool klb_gui_check_color(klb_gui_t* p_gui, const klb_map_t* p_map, int start, uint32_t* p_out_color)
{
    klb_map_t* p_in = (klb_map_t*)p_map;
    int type = klb_map_array_type(p_in, start);
    switch (type)
    {
    case KLB_ADT_map:
        {
            klb_map_t* ptr = (klb_map_t*)klb_map_idx_to_map(p_in, start);
            if (0 < klb_map_array_size(ptr))
            {
                uint8_t a = (uint8_t)klb_map_idx_to_int64(ptr, 0);
                uint8_t r = (uint8_t)klb_map_idx_to_int64(ptr, 1);
                uint8_t g = (uint8_t)klb_map_idx_to_int64(ptr, 2);
                uint8_t b = (uint8_t)klb_map_idx_to_int64(ptr, 3);

                if (NULL != p_out_color)
                {
                    *p_out_color = KLB_ARGB8888(a, r, g, b);
                }

                return true;
            }
        }
        break;
    case KLB_ADT_int64:
        {
            uint32_t argb = (uint32_t)klb_map_idx_to_int64(p_in, start);
            if (NULL != p_out_color)
            {
                *p_out_color = argb;
            }
            return true;
        }
        break;
    case KLB_ADT_uint64:
        {
            uint32_t argb = (uint32_t)klb_map_idx_to_uint64(p_in, start);
            if (NULL != p_out_color)
            {
                *p_out_color = argb;
            }
            return true;
        }
        break;
    case KLB_ADT_string:
        {
            char* p_str = (char*)klb_map_idx_to_string(p_in, start);
            if (NULL != p_out_color)
            {
                *p_out_color = (uint32_t)strtoll(p_str, NULL, 16);
            }
            return true;
        }
        break;
    default:
        break;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
// 同类型控件, 私有CSS属性 函数处理表


/// @brief 通过组件类型获取 css 属性函数处理 map
klb_map_t* klb_gui_css_map(klb_gui_t* p_gui, const char* p_type)
{
    return klb_map_to_map(&p_gui->p_util->css_map, p_type);
}

/// @brief 新建 css 属性函数处理 map
klb_map_t* klb_gui_new_css_map(klb_gui_t* p_gui, const char* p_type)
{
    klb_map_set_map(&p_gui->p_util->css_map, p_type, NULL); // 重设置一个 map
    return klb_map_to_map(&p_gui->p_util->css_map, p_type);
}


//////////////////////////////////////////////////////////////////////////
// 同类型控件, 全局公共CSS属性 函数处理表

klb_map_t* klb_gui_globalcss_map(klb_gui_t* p_gui, const char* p_type)
{
    return klb_map_to_map(&p_gui->p_util->globalcss_map, p_type);
}

klb_map_t* klb_gui_new_globalcss_map(klb_gui_t* p_gui, const char* p_type)
{
    klb_map_set_map(&p_gui->p_util->globalcss_map, p_type, NULL); // 重设置一个 map
    return klb_map_to_map(&p_gui->p_util->globalcss_map, p_type);
}

void klb_gui_globalcss_set_ptr(klb_gui_t* p_gui, const char* p_type, void* p_css, klb_gui_globalcss_destroy_cb cb_destroy)
{
    // 若存在, 则销毁
    {
        void* ptr = NULL;
        void* p_old_css = (void*)klb_map_to_ptr(&p_gui->p_util->globalcss_attr_map, p_type, &ptr);

        klb_gui_globalcss_destroy_cb destroy = (klb_gui_globalcss_destroy_cb)ptr;

        if (NULL != p_old_css && NULL != destroy)
        {
            destroy(p_old_css);
        }
    }

    // 更新节点
    klb_map_set_ptr(&p_gui->p_util->globalcss_attr_map, p_type, p_css, cb_destroy);
}

void* klb_gui_globalcss_get_ptr(klb_gui_t* p_gui, const char* p_type)
{
    return (void*)klb_map_to_ptr(&p_gui->p_util->globalcss_attr_map, p_type, NULL);
}

int klb_gui_globalcss_set(klb_gui_t* p_gui, const char* p_type, const klb_map_t* p_map)
{
    klb_map_t* p_func_map = klb_gui_globalcss_map(p_gui, p_type);
    void* p_css = klb_gui_globalcss_get_ptr(p_gui, p_type);

    if (NULL != p_func_map && NULL != p_css)
    {
        const char* p_key = klb_map_idx_to_string(p_map, 0);
        klb_gui_globalcss_cb func = (klb_gui_globalcss_cb)klb_map_to_ptr(p_func_map, p_key, NULL);

        if (NULL != func)
        {
            func(p_css, KLBUI_CSSEX_set, p_map, NULL);
        }
    }

    return 0;
}

klb_map_t* klb_gui_globalcss_get(klb_gui_t* p_gui, const char* p_type, const klb_map_t* p_map)
{
    klb_map_t* p_func_map = klb_gui_globalcss_map(p_gui, p_type);
    void* p_css = klb_gui_globalcss_get_ptr(p_gui, p_type);

    klb_map_t* p_out = klb_map_create();

    if (NULL != p_func_map && NULL != p_css)
    {
        const char* p_key = klb_map_idx_to_string(p_map, 0);
        klb_gui_globalcss_cb func = (klb_gui_globalcss_cb)klb_map_to_ptr(p_func_map, p_key, NULL);

        if (NULL != func)
        {
            func(p_css, KLBUI_CSSEX_get, p_map, p_out);
        }
    }

    return p_out;
}
