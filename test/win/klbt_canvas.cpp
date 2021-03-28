#include "klbt_canvas.h"
#include "klbmem/klb_mem.h"
#include "platform/klb_mutex.h"
#include <assert.h>


//CreateCompatibleDC(NULL);
//CreateCompatibleBitmap();
//SelectObject();
//BitBlt();
//SetBitmapBits();
//DeleteObject();
//DeleteDC();


typedef struct klbt_canvas_ex_t_
{
    void(*old_free)(klb_canvas_t* p_canvas);

    klb_mutex_t*    p_mutex;

    HDC             hdc;
    HBITMAP         hbitmap;
    HBITMAP         hbitmap_old;
}klbt_canvas_ex_t;


static void klbt_canvas_free(klb_canvas_t* p_canvas)
{
    assert(NULL != p_canvas);
    klbt_canvas_ex_t* p_ex = (klbt_canvas_ex_t*)p_canvas->p_obj;

    SelectObject(p_ex->hdc, p_ex->hbitmap_old);
    DeleteObject(p_ex->hbitmap);
    DeleteDC(p_ex->hdc);

    p_ex->old_free(p_canvas);
    KLB_FREE_BY(p_ex->p_mutex, klb_mutex_destroy);

    KLB_FREE(p_ex);
}

static int klbt_canvas_lock(klb_canvas_t* p_canvas)
{
    assert(NULL != p_canvas);
    klbt_canvas_ex_t* p_ex = (klbt_canvas_ex_t*)p_canvas->p_obj;

    klb_mutex_lock(p_ex->p_mutex);

    return 0;
}

static int klbt_canvas_unlock(klb_canvas_t* p_canvas)
{
    assert(NULL != p_canvas);
    klbt_canvas_ex_t* p_ex = (klbt_canvas_ex_t*)p_canvas->p_obj;

    klb_mutex_unlock(p_ex->p_mutex);

    return 0;
}

//static int klbt_canvas_refresh(klb_canvas_t* p_canvas, int x, int y, int w, int h)
//{
//    return 0;
//}

klb_canvas_t* klbt_canvas_create(HDC hdc, int w, int h, int(*refresh)(klb_canvas_t* p_canvas, int x, int y, int w, int h))
{
    klb_canvas_t* p_canvas = klb_canvas_create(w, h, KLB_COLOR_FMT_ARGB8888);

    klbt_canvas_ex_t* p_ex = KLB_MALLOC(klbt_canvas_ex_t, 1, 0);
    KLB_MEMSET(p_ex, 0, sizeof(klbt_canvas_ex_t));

    p_ex->old_free = p_canvas->vtable.free;

    p_ex->p_mutex = klb_mutex_create();

    BITMAP bitmap = { 0 };
    bitmap.bmWidth = w;
    bitmap.bmHeight = h;
    bitmap.bmWidthBytes = w * 4;
    bitmap.bmBitsPixel = 32;
    bitmap.bmPlanes = 1;

    p_ex->hdc = CreateCompatibleDC(hdc);
    //p_ex->hbitmap = CreateCompatibleBitmap(p_ex->hdc, w, h);
    p_ex->hbitmap = CreateBitmapIndirect(&bitmap);
    p_ex->hbitmap_old = (HBITMAP)SelectObject(p_ex->hdc, p_ex->hbitmap);

    //BITMAP bm;
    //GetObject(p_ex->hbitmap, sizeof(bm), &bm);

    p_canvas->p_obj = p_ex;
    p_canvas->vtable.free = klbt_canvas_free;
    p_canvas->vtable.lock = klbt_canvas_lock;
    p_canvas->vtable.unlock = klbt_canvas_unlock;
    p_canvas->vtable.refresh = refresh;

    return p_canvas;
}

int klbt_canvas_refresh(klb_canvas_t* p_canvas, HDC dst, int x, int y, int w, int h)
{
    assert(NULL != p_canvas);
    klbt_canvas_ex_t* p_ex = (klbt_canvas_ex_t*)p_canvas->p_obj;

#if 1
    klb_mutex_lock(p_ex->p_mutex);
    LONG len = SetBitmapBits(p_ex->hbitmap, p_canvas->mem_len, p_canvas->p_addr);
    assert(len == p_canvas->mem_len);
    klb_mutex_unlock(p_ex->p_mutex);
#else
    for (int j = 0; j < p_canvas->rect.h; j++)
    {
        for (int i = 0; i < p_canvas->rect.w; i++)
        {
            SetPixel(dst, i, j, RGB(0, 255, 0));
        }
    }
#endif

    BitBlt(dst, x, y, w, h, p_ex->hdc, x, y, SRCCOPY);

    return 0;
}
