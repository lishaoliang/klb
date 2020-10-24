#include "klbt_gl_wnd.h"
#include "mem/klb_mem.h"
#include "thread/klb_thread.h"
#include "log/klb_log.h"
#include "time/klb_time.h"
#include <windows.h>

#include "glad/glad.h"

#include <assert.h>

#define GL_WND_USED         1

#if GL_WND_USED


#include <windows.h>
#pragma comment(lib, "opengl32.lib")


#define KLBT_CTX_DOUBLEBUFFER        0

const char KLBT_V_WNDCLASSEX_NAME[] = "klbt_gl_video";



typedef struct klbt_gl_wnd_t_
{
    HINSTANCE       instance;
    HWND            hwnd;
    HDC             hdc;            // 窗口绘图句柄
    HGLRC           glrc;           // gl环境

    int             w;              // 实际宽
    int             h;              // 实际高

    struct
    {
        uint32_t          interval_tc;
        uint32_t          last_tc;
        uint32_t          tc;

        klbt_gl_wnd_msg_cb cb;
        void*           p_obj;
    };

    struct
    {
        klb_thread_t*   p_thread;       ///< thread线程
        bool            run;
    };

    struct
    {
        int             w;              ///< 参数宽
        int             h;              ///< 参数高
        char            name[132];      ///< 名称
    }param;
}klbt_gl_wnd_t;

static int klbt_v_wnd_thread(void* p_param, int* p_run);


klbt_gl_wnd_t* klbt_gl_wnd_create(int w, int h, char* p_name)
{
    klbt_gl_wnd_t* p_wnd = KLB_MALLOC(klbt_gl_wnd_t, 1, 0);
    KLB_MEMSET(p_wnd, 0, sizeof(klbt_gl_wnd_t));

    p_wnd->instance = GetModuleHandle(NULL);

    p_wnd->param.w = w;
    p_wnd->param.h = h;

    if (NULL != p_name)
    {
        strncpy(p_wnd->param.name, p_name, 128);
    }
    else
    {
        strcpy(p_wnd->param.name, "hello");
    }

    p_wnd->run = false;

    return p_wnd;
}

void klbt_gl_wnd_destroy(klbt_gl_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);

    KLB_FREE(p_wnd);
}

int klbt_gl_wnd_start(klbt_gl_wnd_t* p_wnd, uint32_t interval, klbt_gl_wnd_msg_cb cb, void* p_obj)
{
    assert(NULL != p_wnd);
    assert(NULL == p_wnd->p_thread);

    uint32_t tc = klb_tick_count();
    p_wnd->interval_tc = interval;
    p_wnd->tc = tc;
    p_wnd->last_tc = tc;

    p_wnd->cb = cb;
    p_wnd->p_obj = p_obj;

    p_wnd->run = true;

    p_wnd->p_thread = klb_thread_create(klbt_v_wnd_thread, p_wnd, -1, "l_gl_wnd");
    assert(NULL != p_wnd->p_thread);

    return (NULL != p_wnd->p_thread) ? 0 : 1;
}

void klbt_gl_wnd_stop(klbt_gl_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);
    assert(NULL != p_wnd->p_thread);

    if (NULL != p_wnd->p_thread)
    {
        klb_thread_destroy(p_wnd->p_thread);
        p_wnd->p_thread = NULL;
    }
}

bool klbt_gl_wnd_is_run(klbt_gl_wnd_t* p_wnd)
{
    return p_wnd->run;
}

void klbt_gl_wnd_begin(klbt_gl_wnd_t* p_wnd)
{
    wglMakeCurrent(p_wnd->hdc, p_wnd->glrc);
}

void klbt_gl_wnd_end(klbt_gl_wnd_t* p_wnd)
{
#if KLBT_CTX_DOUBLEBUFFER
    SwapBuffers(p_wnd->hdc); // 双缓存
#else
    glFlush(); // 单缓存
#endif
}

void klbt_gl_wnd_flush(klbt_gl_wnd_t* p_wnd)
{
#if KLBT_CTX_DOUBLEBUFFER
    SwapBuffers(p_wnd->hdc); // 双缓存
#else
    glFlush(); // 单缓存
#endif
}

void klbt_gl_wnd_viewport(klbt_gl_wnd_t* p_wnd, int x, int y, int w, int h)
{
    assert(NULL != p_wnd);

    glViewport(x, y, w, h);
}

void klbt_gl_wnd_clear(klbt_gl_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

int klbt_gl_wnd_get_client_rect(klbt_gl_wnd_t* p_wnd, int* p_w, int* p_h)
{
    assert(NULL != p_wnd);

    if (0 != p_wnd->w || 0 != p_wnd->h)
    {
        *p_w = p_wnd->w;
        *p_h = p_wnd->h;

        return 0;
    }

    return 1;
}

static int msg_callback(klbt_gl_wnd_t* p_wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int ret = 0;
    if (NULL != p_wnd->cb)
    {
        ret = p_wnd->cb(p_wnd, p_wnd->p_obj, message, wParam, lParam);
    }

    return ret;
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    klbt_gl_wnd_t* p_wnd = NULL;
    if (WM_CREATE == message)
    {
        //创建窗口的时, 会将窗口绑定的对象传递过来
        //将其保存 在窗口的 GWL_USERDATA 字段中
        p_wnd = (klbt_gl_wnd_t*)(((LPCREATESTRUCT)lParam)->lpCreateParams);

#ifdef _WIN64
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)p_wnd);
#else
        SetWindowLong(hwnd, GWL_USERDATA, (LONG)p_wnd);
#endif
    }
    else
    {
        //不是创建消息时, 取出 GWL_USERDATA字段对应的数据
#ifdef _WIN64
        p_wnd = (klbt_gl_wnd_t*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
        p_wnd = (klbt_gl_wnd_t*)GetWindowLong(hwnd, GWL_USERDATA);
#endif
    }

    int ret = 0;

    switch (message)
    {
    case WM_CHAR:
        if (27 == wParam)
        {
            // "Esc"键 退出
            PostMessage(hwnd, WM_DESTROY, 0, 0);
        }
        break;
    case WM_CREATE:
        {
            if (NULL != p_wnd) msg_callback(p_wnd, message, wParam, lParam);
        }
        break;
    case WM_DESTROY:
        {
            if (NULL != p_wnd) msg_callback(p_wnd, message, wParam, lParam);
            PostQuitMessage(0);
        }
        break;
    case WM_PAINT:
        {
            // 删除这3句话, 会导致cpu使用率很高
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);

            //glClear(GL_COLOR_BUFFER_BIT);
            //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            //glFlush(); // 单缓存

            if (NULL != p_wnd) msg_callback(p_wnd, message, wParam, lParam);
        }
        break;
    case WM_ERASEBKGND:
        {
            // 不更新背景
            ret = 1;
        }
        break;
    case WM_SIZE:
        {
            RECT rect = { 0 };
            GetClientRect(hwnd, &rect);

            if (NULL != p_wnd)
            {
                p_wnd->w = rect.right - rect.left;
                p_wnd->h = rect.bottom - rect.top;
                
                //glViewport(0, 0, rect.right - rect.left, rect.bottom - rect.top);
                msg_callback(p_wnd, message, wParam, lParam);
            }
        }
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return ret;
}

//  函数: MyRegisterClass()
//  目的: 注册窗口类。
static ATOM my_register_class(HINSTANCE h_instance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = wnd_proc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = h_instance;
    wcex.hIcon = NULL;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_SDKS_EX));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;//(HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL; // MAKEINTRESOURCEW(IDC_CLIENT_GL);
    wcex.lpszClassName = KLBT_V_WNDCLASSEX_NAME;
    wcex.hIconSm = NULL;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_SDKS_EX));

    return RegisterClassEx(&wcex);
}

static void adjust_wnd(HWND hwnd, bool b_full, int w, int h)
{
    //调整屏幕
    if (!b_full)
    {
        RECT wnd = { 0 }, client = { 0 };
        GetWindowRect(hwnd, &wnd);
        GetClientRect(hwnd, &client);

        int sc_cx = w + (wnd.right - wnd.left) - (client.right - client.left);
        int sc_cy = h + (wnd.bottom - wnd.top) - (client.bottom - client.top);

        MoveWindow(hwnd, (GetSystemMetrics(SM_CXSCREEN) - sc_cx) / 2, (GetSystemMetrics(SM_CYSCREEN) - sc_cy) / 2, sc_cx, sc_cy, false);
    }
    else
    {
        int sc_cx = GetSystemMetrics(SM_CXSCREEN);
        int sc_cy = GetSystemMetrics(SM_CYSCREEN);

        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style &= ~(WS_DLGFRAME | WS_THICKFRAME | WS_BORDER);
        SetWindowLong(hwnd, GWL_STYLE, style);

        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, sc_cx, sc_cy, SWP_FRAMECHANGED);
    }
}

static int klbt_v_wnd_thread(void* p_param, int* p_run)
{
    KLB_LOG("start thread,klbt_v_wnd_thread.\n");
    klbt_gl_wnd_t* p_wnd = (klbt_gl_wnd_t*)p_param;

    my_register_class(p_wnd->instance);

    p_wnd->hwnd = CreateWindow(KLBT_V_WNDCLASSEX_NAME, p_wnd->param.name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, p_wnd->instance, p_wnd);
    assert(NULL != p_wnd->hwnd);

    p_wnd->hdc = GetDC(p_wnd->hwnd);

    static PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        0,                              // dwFlags
        PFD_TYPE_RGBA,                  // RGBA type
        32,                             // 32-bit color depth
        0, 0, 0, 0, 0, 0,               // color bits ignored
        0,                              // no alpha buffer
        0,                              // shift bit ignored
        0,                              // no accumulation buffer
        0, 0, 0, 0,                     // accum bits ignored
        24,                             // 24-bit z-buffer
        0,                              // no stencil buffer
        0,                              // no auxiliary buffer
        PFD_MAIN_PLANE,                 // main layer
        0,                              // reserved
        0, 0, 0                         // layer masks ignored
    };

#if KLBT_CTX_DOUBLEBUFFER
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |  // support window
        PFD_DOUBLEBUFFER |      // double buffered
        PFD_SUPPORT_OPENGL;     // support OpenGL
#else
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |  // support window
        PFD_SUPPORT_OPENGL;     // support OpenGL
#endif

    int format = ChoosePixelFormat(p_wnd->hdc, &pfd); // 选择一个像素格式
    SetPixelFormat(p_wnd->hdc, format, &pfd);   // 设置到DC中

    p_wnd->glrc = wglCreateContext(p_wnd->hdc); // 创建绘图描述表
    wglMakeCurrent(p_wnd->hdc, p_wnd->glrc);    // 使之成为当前绘图描述表

    msg_callback(p_wnd, KLBT_GL_WND_CREATE, 0, 0);

    // 调整窗口大小
    adjust_wnd(p_wnd->hwnd, false, p_wnd->param.w, p_wnd->param.h);
    klbt_gl_wnd_viewport(p_wnd, 0, 0, p_wnd->param.w, p_wnd->param.h);

    ShowWindow(p_wnd->hwnd, SW_SHOW/*SW_MINIMIZE*/);
    UpdateWindow(p_wnd->hwnd);

    while (0 != (*p_run))
    {
        MSG msg = { 0 };

        int peek = PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE); // 先检索有无消息, 防止被 GetMessage函数阻塞
        if (0 < peek)
        {
            int ret = GetMessage(&msg, NULL, 0, 0); // 没有消息时, 会被阻塞在此

            if (0 < ret)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else if (0 == ret)
            {
                // 退出
                break;
            }
            else
            {
                // 出现异常
                break;
            }
        }
        else
        {
            p_wnd->tc = klb_tick_count();

            if (p_wnd->interval_tc <= ABS_SUB(p_wnd->tc, p_wnd->last_tc))
            {
                msg_callback(p_wnd, KLBT_GL_WND_TIMER, p_wnd->tc, p_wnd->last_tc);

                p_wnd->last_tc = p_wnd->tc;
            }

            klb_sleep(5);
        }
    }

    p_wnd->run = false;

    msg_callback(p_wnd, KLBT_GL_WND_DESTROY, 0, 0);

    KLB_LOG("stop thread,klbt_v_wnd_thread.\n");
    return 0;
}

#endif
