// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/klb_canvas_in.h"
#include <string.h>


// Fixed Bug.2026-9-klb: software clear for ARGB8888 canvas
static int draw_clear_klb_canvas_argb8888(klb_canvas_t* p_canvas)
{
    if (NULL == p_canvas || NULL == p_canvas->p_addr || p_canvas->mem_len <= 0)
    {
        return 1;
    }

    memset(p_canvas->p_addr, 0, (size_t)p_canvas->mem_len);
    return 0;
}

void klb_canvas_vtable_init_argb8888(klb_canvas_t* p_canvas)
{
    p_canvas->vtable.draw_clear = draw_clear_klb_canvas_argb8888;
}

// end
