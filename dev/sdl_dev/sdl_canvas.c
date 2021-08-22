#include "sdl_canvas.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////

/// @brief 锁定
/// @param [in] *p_canvas   画布对象
/// @return 0
///  \n 表示开始操作画布内存
static int sdl_canvas_lock(klb_canvas_t* p_canvas)
{
    sdl_canvas_t* p_sdl = (sdl_canvas_t*)p_canvas->p_obj;

    if (0 != SDL_LockTextureToSurface(p_sdl->p_texture, NULL, &p_sdl->p_surface))
    {
        return 1;
    }

    return 0;
}

/// @brief 取消锁定
/// @param [in] *p_canvas   画布对象
/// @return 0
///  \n 表示结束操作画布内存
static int sdl_canvas_unlock(klb_canvas_t* p_canvas)
{
    sdl_canvas_t* p_sdl = (sdl_canvas_t*)p_canvas->p_obj;

    SDL_UnlockTexture(p_sdl->p_texture);

    return 0;
}

/// @brief 开始绘制
/// @param [in] *p_canvas   画布对象
/// @return int 0.成功; 非0.失败
///  \n 表示开始使用draw_*系列函数
static int sdl_canvas_draw_begin(klb_canvas_t* p_canvas)
{
    sdl_canvas_t* p_sdl = (sdl_canvas_t*)p_canvas->p_obj;

    return 0;
}

/// @brief 结束绘制
/// @param [in] *p_canvas   画布对象
/// @return int 0.成功; 非0.失败
///  \n 表示结束使用draw_*系列函数
static int sdl_canvas_draw_end(klb_canvas_t* p_canvas)
{
    sdl_canvas_t* p_sdl = (sdl_canvas_t*)p_canvas->p_obj;

    return 0;
}

/// @brief 使用单色填充绘制
/// @param [in] *p_canvas   画布对象
/// @param [in] x           起始X坐标
/// @param [in] y           起始Y坐标
/// @param [in] w           宽
/// @param [in] h           高
/// @param [in] color       颜色
/// @return int 0.成功; 非0.失败
static int sdl_canvas_draw_fill(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color)
{
    sdl_canvas_t* p_sdl = (sdl_canvas_t*)p_canvas->p_obj;

    SDL_Rect rect = { x, y, w, h };
    int ret = SDL_FillRect(p_sdl->p_surface, &rect, color);

    return ret;
}

/// @brief 拷贝绘制画布
/// @param [in] *p_canvas       画布对象
/// @param [in] x               起始X坐标
/// @param [in] y               起始Y坐标
/// @param [in] *p_src_canvas   被拷贝的原画布
/// @param [in] src_x           原画布起始X坐标
/// @param [in] src_y           原画布起始Y坐标
/// @param [in] src_w           原画布宽
/// @param [in] src_h           原画布高
/// @param [in,out] 输入输出参数
/// @return int 0.成功; 非0.失败
///  \n 原画布/目标画布像素格式必须一致
///  \n 原区域大于目标区域时,裁剪
static int sdl_canvas_draw_canvas(klb_canvas_t* p_canvas, int x, int y, const klb_canvas_t* p_src_canvas, int src_x, int src_y, int src_w, int src_h)
{
    sdl_canvas_t* p_sdl = (sdl_canvas_t*)p_canvas->p_obj;

    return 0;
}

/// @brief 刷新画布到显存
/// @param [in] *p_canvas   画布对象
/// @param [in] x           起始X坐标
/// @param [in] y           起始Y坐标
/// @param [in] w           宽
/// @param [in] h           高
/// @return int 0.成功; 非0.失败
static int sdl_canvas_refresh(klb_canvas_t* p_canvas, int x, int y, int w, int h)
{
    sdl_canvas_t* p_sdl = (sdl_canvas_t*)p_canvas->p_obj;

    p_sdl->refresh = true;

    return 0;
}

//////////////////////////////////////////////////////////////////////////

sdl_canvas_t* sdl_canvas_create(SDL_Texture* p_texture)
{
    assert(NULL != p_texture);

    sdl_canvas_t* p_sdl = KLB_MALLOC(sdl_canvas_t, 1, 0);
    KLB_MEMSET(p_sdl, 0, sizeof(sdl_canvas_t));

    p_sdl->p_texture = p_texture;

    p_sdl->p_canvas = KLB_MALLOC(klb_canvas_t, 1, 0);
    KLB_MEMSET(p_sdl->p_canvas, 0, sizeof(klb_canvas_t));

    p_sdl->p_canvas->p_obj = p_sdl;
    p_sdl->p_canvas->color_fmt = KLB_COLOR_FMT_ARGB8888;

    if (0 == SDL_LockTextureToSurface(p_sdl->p_texture, NULL, &p_sdl->p_surface))
    {
        //KLB_LOG("SDL_LockTextureToSurface ok!,[%p], pixels:[%p],wh[%d,%d],pitch:[%d]\n", p_surface, p_surface->pixels, p_surface->w, p_surface->h, p_surface->pitch);
        //SDL_Rect rect = { 0, 0, 100, 100 };
        //SDL_FillRect(p_surface, &rect, KLB_ARGB8888(255, 20, 20, 200));

        p_sdl->p_canvas->p_addr = (uint8_t*)p_sdl->p_surface->pixels;
        p_sdl->p_canvas->rect.w = p_sdl->p_surface->w;
        p_sdl->p_canvas->rect.h = p_sdl->p_surface->h;
        p_sdl->p_canvas->stride = p_sdl->p_surface->pitch;
        p_sdl->p_canvas->mem_len = (int64_t)p_sdl->p_surface->pitch * p_sdl->p_surface->h;

        SDL_Rect rect = { 0, 0, p_sdl->p_surface->w, p_sdl->p_surface->h };
        SDL_FillRect(p_sdl->p_surface, &rect, KLB_ARGB8888(255, 0, 0, 0));
        SDL_UnlockTexture(p_sdl->p_texture);
    }
    else
    {
        KLB_LOG("SDL_LockTextureToSurface error!\n");
    }

    p_sdl->p_canvas->vtable.lock = sdl_canvas_lock;
    p_sdl->p_canvas->vtable.unlock = sdl_canvas_unlock;
    //p_sdl->p_canvas->vtable.draw_begin = sdl_canvas_draw_begin;
    //p_sdl->p_canvas->vtable.draw_end = sdl_canvas_draw_end;
    p_sdl->p_canvas->vtable.draw_fill = sdl_canvas_draw_fill;
    //p_sdl->p_canvas->vtable.draw_canvas = sdl_canvas_draw_canvas;
    p_sdl->p_canvas->vtable.refresh = sdl_canvas_refresh;

    p_sdl->refresh = false;

    return p_sdl;
}

void sdl_canvas_destroy(sdl_canvas_t* p_sdl)
{
    assert(NULL != p_sdl);

    KLB_FREE(p_sdl->p_canvas);
    KLB_FREE(p_sdl);
}

bool sdl_canvas_need_refresh(sdl_canvas_t* p_sdl)
{
    assert(NULL != p_sdl);
    return p_sdl->refresh;
}

void sdl_canvas_set_refresh_ok(sdl_canvas_t* p_sdl)
{
    p_sdl->refresh = false;
}
