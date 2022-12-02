#include "wsdl_extension.h"
#include "SDL.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_canvas.h"
#include "klua/klua_gui.h"
#include "klbutil/klb_log.h"
#include "ft_raster.h"
#include "wsdl_images.h"
#include "wsdl_wnd.h"
#include "wsdl_ui_video.h"
#include "klbgui/klb_gui.h"
#include "klua/klua_gui.h"
#include <assert.h>


#define WSDL_EXTENSION      "WSDL_EXTENSION"
#define WSDL_VIDEO          "wsdl_video"


typedef struct wsdl_extension_t_
{
    klua_env_t*         p_env;              ///< 附加环境: env
    klb_gui_t*          p_gui;              ///< gui

    ft_raster_t*        p_ft;               ///< 字体
    wsdl_images_t*      p_imgs;             ///< 图片

    wsdl_wnd_t*         p_wnd;              ///< 系统窗口(对话框)
    klb_canvas_t        canvas;             ///< gui 画布

    sds                 base_path;          ///< 执行文件路径

    bool                refresh;
}wsdl_extension_t;


//////////////////////////////////////////////////////////////////////////

static void wsdl_extension_init(wsdl_extension_t* p_ex, const char* p_font_path)
{
    // 基础路径
    char* p_base_path = SDL_GetBasePath();
    p_ex->base_path = sdsnew(p_base_path);
    KLB_FREE_BY(p_base_path, SDL_free);

    // 图片
    p_ex->p_imgs = wsdl_images_create();

    // 字体
    sds font_path = sdsnew(p_base_path);
    font_path = sdscat(font_path, p_font_path);
    p_ex->p_ft = ft_raster_create(font_path);
    KLB_FREE_BY(font_path, sdsfree);
}

static void wsdl_extension_quit(wsdl_extension_t* p_ex)
{
    KLB_FREE_BY(p_ex->p_ft, ft_raster_destroy);
    KLB_FREE_BY(p_ex->p_imgs, wsdl_images_destroy);
    KLB_FREE_BY(p_ex->base_path, sdsfree);
}

void* wsdl_extension_create(klua_env_t* p_env)
{
    wsdl_extension_t* p_ex = KLB_MALLOCZ(wsdl_extension_t, 1, 0);

    p_ex->p_env = p_env;
    p_ex->p_wnd = wsdl_wnd_create();

    p_ex->refresh = false;

    return p_ex;
}

void wsdl_extension_destroy(void* ptr)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)ptr;

    KLB_FREE_BY(p_ex->p_wnd, wsdl_wnd_destroy);
    KLB_FREE(p_ex);
}

int wsdl_extension_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)ptr;

    if (!wsdl_wnd_is_open(p_ex->p_wnd))
    {
        return 0;
    }

    int done = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event) > 0) 
    {
        if (event.type == SDL_QUIT) 
        {
            done = 1;
        }

        if (event.type == SDL_KEYDOWN) 
        {
            if (event.key.keysym.sym == SDLK_ESCAPE) 
            {
                //done = 1;
            }
        }

        if (SDL_WINDOWEVENT == event.type)
        {
            if (SDL_WINDOWEVENT_EXPOSED == event.window.event)
            {
                wsdl_wnd_render_present(p_ex->p_wnd);
            }
        }
        
        if (event.type == SDL_MOUSEMOTION && NULL != p_ex->p_gui)
        {
            klb_gui_push_msg(p_ex->p_gui, KLB_WM_MOUSEMOVE, event.motion.x, event.motion.y, 0, 0, 0, 0);
        }

        if (event.type == SDL_MOUSEBUTTONDOWN && NULL != p_ex->p_gui)
        {
            if (1 == event.button.button)
            {
                klb_gui_push_msg(p_ex->p_gui, KLB_WM_LBUTTONDOWN, event.motion.x, event.motion.y, 0, 0, 0, 0);
            }
            else if(3 == event.button.button)
            {
                klb_gui_push_msg(p_ex->p_gui, KLB_WM_LBUTTONDBLCLK, event.motion.x, event.motion.y, 0, 0, 0, 0);
            }
        }
    }

    if (p_ex->refresh)
    {
        wsdl_wnd_refresh(p_ex->p_wnd);

        p_ex->refresh = false;
    }

    if (0 != done)
    {
        klua_env_exit(p_ex->p_env);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int kluaex_register_wsdl(klua_env_t* p_env)
{
    // 注册 lua env 扩展
    klua_env_extension_t ex = { 0 };

    ex.cb_create = wsdl_extension_create;
    ex.cb_destroy = wsdl_extension_destroy;
    ex.cb_loop_once = wsdl_extension_loop_once;

    klua_env_register_extension(p_env, WSDL_EXTENSION, &ex);

    // 注册 gui 组件
    klb_gui_t* p_gui = klua_gui_get(p_env);
    klb_gui_register(p_gui, WSDL_VIDEO, wsdl_ui_video_create); // gui 视频播放组件

    return 0;
}

wsdl_extension_t* kluaex_get_wsdl(klua_env_t* p_env)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)klua_env_get_extension(p_env, WSDL_EXTENSION);
    if (NULL == p_ex)
    {
        kluaex_register_wsdl(p_env);
        p_ex = (wsdl_extension_t*)klua_env_get_extension(p_env, WSDL_EXTENSION);
    }

    return p_ex;
}

wsdl_extension_t* kluaex_get_wsdl_by_L(lua_State* L)
{
    return kluaex_get_wsdl(klua_env_get_by_L(L));
}

//////////////////////////////////////////////////////////////////////////


/// @brief 设置绘制颜色
int kluaex_sdl_canvas_set_draw_color(klb_canvas_t* p_canvas, uint32_t color)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_set_draw_color(p_ex->p_wnd, color);
}

/// @brief 获取绘制颜色
/// @return uint32_t ARGB8888颜色值
static uint32_t kluaex_sdl_canvas_get_draw_color(klb_canvas_t* p_canvas)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_get_draw_color(p_ex->p_wnd);
}

/// @brief 设置字体高度
static int kluaex_sdl_canvas_set_font_height(klb_canvas_t* p_canvas, int h)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_set_font_height(p_ex->p_wnd, h);
}

/// @brief 获取字体高度
static int kluaex_sdl_canvas_get_font_height(klb_canvas_t* p_canvas)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_get_font_height(p_ex->p_wnd);
}

/// @brief 加载图片
int kluaex_sdl_canvas_load_image(klb_canvas_t* p_canvas, const char* p_path, int* p_w, int* p_h)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    sds path = sdsnew(p_ex->base_path);
    path = sdscat(path, p_path);

    int ret = wsdl_images_load(p_ex->p_imgs, wsdl_wnd_get_render(p_ex->p_wnd), p_path, path);

    KLB_FREE_BY(path, sdsfree);
    return ret;
}

/// @brief 使用单色清空屏幕
static int kluaex_sdl_canvas_draw_clear(klb_canvas_t* p_canvas)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_clear(p_ex->p_wnd);
}

/// @brief 绘制点
static int kluaex_sdl_canvas_draw_point(klb_canvas_t* p_canvas, int x, int y)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_point(p_ex->p_wnd, x, y);
}

/// @brief 绘制多个点
static int kluaex_sdl_canvas_draw_points(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_points(p_ex->p_wnd, p_points, count);
}

/// @brief 绘制线段
static int kluaex_sdl_canvas_draw_line(klb_canvas_t* p_canvas, int x1, int y1, int x2, int y2)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_line(p_ex->p_wnd, x1, y1, x2, y2);
}

/// @brief 绘制多个线段
static int kluaex_sdl_canvas_draw_lines(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_lines(p_ex->p_wnd, p_points, count);
}

/// @brief 绘制空心矩形
static int kluaex_sdl_canvas_draw_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_rect(p_ex->p_wnd, p_rect);
}

/// @brief 绘制多个空心矩形
static int kluaex_sdl_canvas_draw_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_rects(p_ex->p_wnd, p_rects, count);
}

/// @brief 使用单色填充绘制单个区域
static int kluaex_sdl_canvas_draw_fill_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_fill_rect(p_ex->p_wnd, p_rect);
}

/// @brief 使用单色填充多个区域
static int kluaex_sdl_canvas_draw_fill_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_fill_rects(p_ex->p_wnd, p_rects, count);
}

static int kluaex_sdl_canvas_draw_text(klb_canvas_t* p_canvas, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_text(p_ex->p_wnd, p_ex->p_ft, p_rect, p_utf8, utf8_len);
}

int kluaex_sdl_canvas_draw_image(klb_canvas_t* p_canvas, const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    return wsdl_wnd_draw_image(p_ex->p_wnd, p_ex->p_imgs, p_dst_rect, p_path, p_src_rect);
}

static int kluaex_sdl_canvas_refresh_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    p_ex->refresh = true;
    return 0;
}

int kluaex_sdl_canvas_refresh_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    p_ex->refresh = true;
    return 0;
}

static void kluaex_sdl_init_canvas(wsdl_extension_t* p_ex)
{
    p_ex->canvas.p_obj = p_ex;
    p_ex->canvas.color_fmt = KLB_COLOR_FMT_ARGB8888;

    p_ex->canvas.vtable.set_draw_color = kluaex_sdl_canvas_set_draw_color;
    p_ex->canvas.vtable.get_draw_color = kluaex_sdl_canvas_get_draw_color;
    p_ex->canvas.vtable.set_font_height = kluaex_sdl_canvas_set_font_height;
    p_ex->canvas.vtable.get_font_height = kluaex_sdl_canvas_get_font_height;
    p_ex->canvas.vtable.load_image = kluaex_sdl_canvas_load_image;
    p_ex->canvas.vtable.draw_clear = kluaex_sdl_canvas_draw_clear;
    p_ex->canvas.vtable.draw_point = kluaex_sdl_canvas_draw_point;
    p_ex->canvas.vtable.draw_points = kluaex_sdl_canvas_draw_points;
    p_ex->canvas.vtable.draw_line = kluaex_sdl_canvas_draw_line;
    p_ex->canvas.vtable.draw_lines = kluaex_sdl_canvas_draw_lines;
    p_ex->canvas.vtable.draw_rect = kluaex_sdl_canvas_draw_rect;
    p_ex->canvas.vtable.draw_rects = kluaex_sdl_canvas_draw_rects;
    p_ex->canvas.vtable.draw_fill_rect = kluaex_sdl_canvas_draw_fill_rect;
    p_ex->canvas.vtable.draw_fill_rects = kluaex_sdl_canvas_refresh_rects;
    p_ex->canvas.vtable.draw_text = kluaex_sdl_canvas_draw_text;
    p_ex->canvas.vtable.draw_image = kluaex_sdl_canvas_draw_image;
    p_ex->canvas.vtable.refresh_rect = kluaex_sdl_canvas_refresh_rect;
    p_ex->canvas.vtable.refresh_rects = kluaex_sdl_canvas_refresh_rects;
}

int kluaex_wsdl_open_wnd(wsdl_extension_t* p_ex, const char* p_font_path, int w, int h, const char* p_title)
{
    // 初始化

    // gui
    p_ex->p_gui = klua_gui_get(p_ex->p_env);

    // font/image
    wsdl_extension_init(p_ex, p_font_path);

    // 初始化画布信息
    kluaex_sdl_init_canvas(p_ex);

    // 附加画布
    klb_gui_attach_canvas(p_ex->p_gui, &p_ex->canvas);

    // 打开窗口
    wsdl_wnd_open(p_ex->p_wnd, p_ex->p_gui, w, h, p_title);

    // 刷新
    p_ex->refresh = true;

    return 0;
}

int kluaex_wsdl_close_wnd(wsdl_extension_t* p_ex)
{
    // 退出 font/
    wsdl_extension_quit(p_ex);

    // 退出窗口
    wsdl_wnd_close(p_ex->p_wnd);

    return 0;
}


#include "ffmpeg_dec.h"
#include "klbbase/klb_mnp.h"

static ffmpeg_dec_t* g_dec = NULL;

static klb_buf_t* kluaex_wsdl_join(klb_buf_t* p_media)
{
    int size = 0;

    klb_buf_t* p_next = p_media;
    while (NULL != p_next)
    {
        size =  size + p_next->end - p_next->start;
        p_next = p_next->p_next;
    }

    klb_buf_t* ptr = klb_buf_malloc(size, false);

    p_next = p_media;
    while (NULL != p_next && p_next->start < p_next->end)
    {
        memcpy(ptr->p_buf + ptr->end, p_next->p_buf + p_next->start + sizeof(klb_mnp_t), p_next->end - p_next->start - sizeof(klb_mnp_t));
        ptr->end = ptr->end + p_next->end - p_next->start - sizeof(klb_mnp_t);

        p_next = p_next->p_next;
    }

    return ptr;
}

int kluaex_wsdl_push_media(wsdl_extension_t* p_ex, int chnn, int sidx, klb_buf_t* p_media)
{
    if (0 == chnn && 0 == sidx && NULL != p_media)
    {
        // test
        if (NULL == g_dec)
        {
            g_dec = ffmpeg_dec_create(AV_CODEC_ID_H264);
        }

        klb_buf_t* ptr = kluaex_wsdl_join(p_media);

        uint8_t* p_data = (uint8_t*)ptr->p_buf + sizeof(klb_mnp_media_t);
        int data_len = ptr->end - sizeof(klb_mnp_media_t);

        if (NULL != p_data && 0 < data_len)
        {
            AVFrame* p_frame = ffmpeg_dec_decode(g_dec, p_data, data_len, 0, 0);
            if (NULL != p_frame)
            {
                if (NULL != p_ex->p_wnd)
                {
                    wsdl_wnd_video_update(p_ex->p_wnd, 0, p_frame);

                    p_ex->refresh = true;
                }

                ffmpeg_dec_avframe_free(p_frame);
            }
        }

        KLB_FREE(ptr);
    }

    return 0;
}

int kluaex_wsdl_set_video_pos(wsdl_extension_t* p_ex, int idx, const klb_rect_t* p_dst_rect)
{
    wsdl_wnd_video_set_pos(p_ex->p_wnd, idx, p_dst_rect, NULL);

    return 0;
}
