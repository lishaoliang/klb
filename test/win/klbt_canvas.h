#ifndef __KLBT_CANVAS_H__
#define __KLBT_CANVAS_H__

#include "klb_type.h"
#include "klbutil/klb_canvas.h"
#include <windows.h>


klb_canvas_t* klbt_canvas_create(HDC hdc, int w, int h, int(*refresh)(klb_canvas_t* p_canvas, int x, int y, int w, int h));


int klbt_canvas_refresh(klb_canvas_t* p_canvas, HDC dst, int x, int y, int w, int h);


#endif // __KLBT_CANVAS_H__
//end
