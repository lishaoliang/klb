#include "klbutil/klb_color.h"


int klb_color_bpp(int color_fmt)
{
    switch (color_fmt)
    {
    case KLB_COLOR_FMT_ARGB8888:
        return 4;
        break;
    default:
        break;
    }

    return 4;
}

uint32_t klb_color_argb8888_to(uint32_t argb8888, int color_fmt)
{
    return argb8888;
}


uint32_t klb_color_rgba8888_to(uint32_t rgba8888, int color_fmt)
{
    return rgba8888;
}
