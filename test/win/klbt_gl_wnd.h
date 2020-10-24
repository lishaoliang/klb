#ifndef __KLBT_GL_WND_H__
#define __KLBT_GL_WND_H__

#include "klb_type.h"

#define KLBT_GL_WND_CREATE     (0x0400 + 1000)
#define KLBT_GL_WND_DESTROY    (0x0400 + 1001)
#define KLBT_GL_WND_TIMER      (0x0400 + 1002)

typedef struct klbt_gl_wnd_t_ klbt_gl_wnd_t;
typedef int(*klbt_gl_wnd_msg_cb)(klbt_gl_wnd_t* p_wnd, void* p_obj, int msg, uint32_t wparam, uint32_t lparam);


klbt_gl_wnd_t* klbt_gl_wnd_create(int w, int h, char* p_name);
void klbt_gl_wnd_destroy(klbt_gl_wnd_t* p_wnd);

int klbt_gl_wnd_start(klbt_gl_wnd_t* p_wnd, uint32_t interval, klbt_gl_wnd_msg_cb cb, void* p_obj);
void klbt_gl_wnd_stop(klbt_gl_wnd_t* p_wnd);

bool klbt_gl_wnd_is_run(klbt_gl_wnd_t* p_wnd);

void klbt_gl_wnd_begin(klbt_gl_wnd_t* p_wnd);
void klbt_gl_wnd_end(klbt_gl_wnd_t* p_wnd);
void klbt_gl_wnd_flush(klbt_gl_wnd_t* p_wnd);
void klbt_gl_wnd_viewport(klbt_gl_wnd_t* p_wnd, int x, int y, int w, int h);
void klbt_gl_wnd_clear(klbt_gl_wnd_t* p_wnd);

int klbt_gl_wnd_get_client_rect(klbt_gl_wnd_t* p_wnd, int* p_w, int* p_h);

#endif // __KLBT_GL_WND_H__
//end
