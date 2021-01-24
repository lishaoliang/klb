// klb_test.cpp : 定义应用程序的入口点。
//

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件: 
#include <windows.h>

// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>

#include "klb_type.h"
#include "platform/klb_thread.h"
#include "mem/klb_mem.h"
#include "gui/klb_gui.h"
#include "image/klb_canvas.h"
#include "klbt_canvas.h"

#include "socket/klb_socket.h"
#include "socket/klb_socket_base.h"
#include "string/sds.h"
#include "mem/klb_mem.h"

#include "string/klb_str_map.h"
#include "platform/klb_time.h"
#include "log/klb_log.h"

#include <WS2tcpip.h>

#if !defined(NDEBUG)
//#include "vld.h"
#endif

#define MAX_LOADSTRING  100
#define DLG_W           960
#define DLG_H           540


// 全局变量: 
HINSTANCE hInst;                                // 当前实例
char szTitle[MAX_LOADSTRING];                  // 标题栏文本
char szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


static HWND             g_hwnd = 0;
static klb_gui_t*       g_gui = NULL;
static klb_canvas_t*    g_canvas = NULL;

void my_log_printf(const char* p_fmt, ...)
{
    char buf[1024] = { 0 }, *p = buf;

    va_list args;
    va_start(args, p_fmt);
    p += _vsnprintf_s(p, sizeof(buf) - 1, sizeof(buf) - 1, p_fmt, args);
    va_end(args);

    OutputDebugString(buf);
}

#define LOG(fmt, ...) my_log_printf(fmt, ## __VA_ARGS__)

static void init_my_dlgs();
static int cb_refresh(klb_canvas_t* p_canvas, int x, int y, int w, int h);
static void my_pre_dispatch_message(const MSG* p_msg);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。

    // 初始化全局字符串
    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    //LoadStringW(hInstance, IDC_KLB_TEST, szWindowClass, MAX_LOADSTRING);
    strcpy_s(szTitle, "klb test");
    strcpy_s(szWindowClass, "klb.test.windows");
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

#if 0
    HACCEL hAccelTable = NULL;// LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KLB_TEST));

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
#else
    MSG msg = { 0 };
    while (true)
    {
        int peek = PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE); // 先检索有无消息, 防止被 GetMessage函数阻塞
        if (0 < peek)
        {
            int ret = GetMessage(&msg, NULL, 0, 0); // 没有消息时, 会被阻塞在此

            if (0 < ret)
            {
                TranslateMessage(&msg);
                my_pre_dispatch_message(&msg);
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
            klb_sleep(5);
        }
    }
#endif

    klb_gui_stop(g_gui);
    klb_gui_destroy(g_gui);

    KLB_FREE_BY(g_canvas, klb_canvas_destroy);


    klb_socket_quit();
    return 0;
}


//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;// LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KLB_TEST));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;// MAKEINTRESOURCEW(IDC_KLB_TEST);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = NULL;// LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // 将实例句柄存储在全局变量中


#define MY_OVERLAPPEDWINDOW (WS_OVERLAPPED     | \
                             WS_SYSMENU        | \
                             WS_MINIMIZEBOX)

    HWND hWnd = CreateWindow(szWindowClass, szTitle, MY_OVERLAPPEDWINDOW/*WS_OVERLAPPEDWINDOW*/,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    g_hwnd = hWnd;

    adjust_wnd(hWnd, false, DLG_W, DLG_H);

    // 创建
    g_canvas = klbt_canvas_create(NULL, DLG_W, DLG_H, cb_refresh);
    klb_canvas_draw_fill(g_canvas, 0, 0, DLG_W, DLG_H, KLB_ARGB8888(0, 0, 0, 0));

    g_gui = klb_gui_create(g_canvas);
    init_my_dlgs();
    
    klb_gui_start(g_gui);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            
            //klb_canvas_draw_fill(g_canvas, 0, 0, DLG_W, DLG_H, KLB_ARGB8888(255, 31, 21, 31));
            klbt_canvas_refresh(g_canvas, hdc, 0, 0, DLG_W, DLG_H);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_ERASEBKGND:
        {
            return 1;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////


static int test_my_dlgs_btn_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_gui_t* p_gui = g_gui;

    switch (msg)
    {
    case KLB_WM_LBUTTONDOWN:
        LOG("klb_button_on_command,[%d],[%d,%d]\n", msg, p_pt1->x, p_pt1->y);
        klb_gui_do_model(p_gui, "/aaaa");
        break;
    default:
        break;
    }

    return 0;
}

static int test_my_dlgs_btn_222_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_gui_t* p_gui = g_gui;

    switch (msg)
    {
    case KLB_WM_LBUTTONDOWN:
        LOG("klb_button_on_command,[%d],[%d,%d]\n", msg, p_pt1->x, p_pt1->y);
        klb_gui_end_model(p_gui, "");
        break;
    default:
        break;
    }

    return 0;
}

static void init_my_dlgs()
{
    klb_gui_t* p_gui = g_gui;

    klb_gui_append(p_gui, "kdialog", "/home", 0, 0, DLG_W / 2, DLG_H - 20, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn1", 10, 10, 100, 24, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn2", 10, 40, 100, 24, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn3", 10, 70, 100, 24, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn4", 10, 100, 100, 24, NULL);

    klb_gui_append(p_gui, "kdialog", "/aaaa", DLG_W / 2, 0, DLG_W / 2, DLG_H - 10, NULL);
    klb_gui_append(p_gui, "kbutton", "/aaaa/btn1", 10, 10, 100, 24, NULL);
    klb_gui_append(p_gui, "kbutton", "/aaaa/btn2", 10, 40, 100, 24, NULL);

    klb_gui_bind_command(p_gui, "/home/btn1", test_my_dlgs_btn_on_command, NULL);
    klb_gui_bind_command(p_gui, "/aaaa/btn1", test_my_dlgs_btn_222_on_command, NULL);

    klb_gui_do_model(p_gui, "/home");
}


static int cb_refresh(klb_canvas_t* p_canvas, int x, int y, int w, int h)
{
    //PostMessage(g_hwnd, WM_PAINT, 0, 0);
    RECT rect = {x, y, x + w, y + h};

    InvalidateRect(g_hwnd, &rect, TRUE);

    return 0;
}


static void message_lparam_to_pt(const MSG* p_msg, klb_point_t* p_pt)
{
    p_pt->x = p_msg->lParam & 0xFFFF;
    p_pt->y = (p_msg->lParam >> 16) & 0xFFFF;
}

static void my_pre_dispatch_message(const MSG* p_msg)
{

#if 0
    if (WM_LBUTTONDOWN == p_msg->message)
    {
        klb_canvas_draw_fill(g_canvas, 0, 0, DLG_W, DLG_H, KLB_ARGB8888(255, 31, 21, 31));
        klb_canvas_draw_fill(g_canvas, 100, 100, 100, 100, KLB_ARGB8888(255, 61, 61, 61));
        klb_canvas_draw_rect(g_canvas, 100, 100, 100, 100, KLB_ARGB8888(255, 120, 120, 120), 2);

        klb_canvas_draw_line(g_canvas, 100, 250, 400, 250, KLB_ARGB8888(255, 120, 120, 120), 2);
        klb_canvas_draw_line(g_canvas, 100, 250, 100, 500, KLB_ARGB8888(255, 120, 120, 120), 2);

        klb_canvas_draw_canvas(g_canvas, 300, 100, g_canvas, 100, 100, 100, 100);

        cb_refresh(g_canvas, 0, 0, DLG_W, DLG_H);
    }
    else if (WM_RBUTTONDOWN == p_msg->message)
    {
        klb_canvas_draw_fill(g_canvas, 0, 0, DLG_W, DLG_H, KLB_ARGB8888(255, 81, 81, 81));
        cb_refresh(g_canvas, 0, 0, DLG_W, DLG_H);
    }
#endif

#if 0
    if (WM_MOUSEMOVE == p_msg->message)
    {
        //LOG("pt xy:[%d,%d]\n", p_msg->lParam & 0xFFFF, (p_msg->lParam >> 16) & 0xFFFF);
    }

    if (WM_MOUSEMOVE == p_msg->message ||
        WM_LBUTTONDOWN == p_msg->message ||
        WM_LBUTTONUP == p_msg->message ||
        WM_LBUTTONDBLCLK == p_msg->message ||
        WM_RBUTTONDOWN == p_msg->message ||
        WM_RBUTTONUP == p_msg->message ||
        WM_RBUTTONDBLCLK == p_msg->message)
    {

    }
#endif

    int msg = 0;
    klb_point_t pt1 = { 0 };

    switch (p_msg->message)
    {
    case WM_MOUSEMOVE:
        {
            msg = KLB_WM_MOUSEMOVE;
            message_lparam_to_pt(p_msg, &pt1);
        }
        break;
    case WM_LBUTTONDOWN:
        {
            msg = KLB_WM_LBUTTONDOWN;
            message_lparam_to_pt(p_msg, &pt1);
        }
        break;
    default:
        break;
    }

    if (0 != msg)
    {
        klb_gui_push(g_gui, msg, pt1.x, pt1.y, 0, 0, 0, 0);
    }
}
