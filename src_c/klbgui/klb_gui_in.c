// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_gui.h"
#include "klbgui/klb_gui_in.h"
#include <assert.h>


int klb_gui_register_ktype(klb_gui_t* p_gui)
{
    return 0;
}

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
