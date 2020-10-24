#include "klbt_glad.h"
#include "log/klb_log.h"
#include <assert.h>

#ifdef _WIN32
#include "glad/glad.h"

#include <windows.h>
#pragma comment(lib, "opengl32.lib")

static int g_lgl_glad = 0;


int klbt_glad_init()
{
    // ����glad
    if (0 == g_lgl_glad)
    {

#ifdef _WIN32
        HINSTANCE h_instance = GetModuleHandle(NULL);

        HWND hwnd = CreateWindow("STATIC", "hello", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, h_instance, NULL);
        assert(NULL != hwnd);

        HDC hdc = GetDC(hwnd);
        assert(NULL != hdc);

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

        int format = ChoosePixelFormat(hdc, &pfd); // ѡ��һ�����ظ�ʽ
        SetPixelFormat(hdc, format, &pfd);   // ���õ�DC��

        HGLRC glrc = wglCreateContext(hdc); // ������ͼ������
        wglMakeCurrent(hdc, glrc);    // ʹ֮��Ϊ��ǰ��ͼ������
#endif

        // ������һ��
        int ver = gladLoadGL();

        if (0 != ver)
        {
            g_lgl_glad = 1;
            KLB_LOG("glad load GL ok,GLVersion:[%d.%d]\n", GLVersion.major, GLVersion.minor);
        }
        else
        {
            KLB_LOG("glad load GL error!\n");
        }

#ifdef _WIN32
        wglDeleteContext(glrc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
#endif
    }

    return 0;
}

#endif
