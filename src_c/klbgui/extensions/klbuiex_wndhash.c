#include "klbgui/extensions/klbuiex_wndhash.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbgui/klb_wnd_in.h"


#define KLBUIEX_WNDHASH   "_KLBUIEX_WNDHASH_"


/// @struct klbuiex_wndhash_t
/// @brief  存储wnd,及hash查找表
typedef struct klbuiex_wndhash_t_
{
    klb_gui_t*      p_gui;              ///< gui对象

    klb_hlist_t*    p_create_map;       ///< 类型对应的创建函数表
    klb_hlist_t*    p_wnd_map;          ///< 窗体查找表
}klbuiex_wndhash_t;


//////////////////////////////////////////////////////////////////////////
// 

int klbuiex_wndhash_register(klbuiex_wndhash_t* p_wndhash, const char* p_type, klb_wnd_create_cb cb_create)
{
    int len = strlen(p_type);

    if (NULL == klb_hlist_find_iter(p_wndhash->p_create_map, p_type, len))
    {
        // 若不存在, 则添加
        klb_hlist_push_tail(p_wndhash->p_create_map, p_type, len, cb_create);
    }
    else
    {
        // 若已存在类型, 则覆盖
        klb_hlist_update(p_wndhash->p_create_map, p_type, len, cb_create);
    }

    return 0;
}

klb_wnd_create_cb klbuiex_wndhash_get_creater(klbuiex_wndhash_t* p_wndhash, const char* p_type)
{
    int len = strlen(p_type);

    return (klb_wnd_create_cb)klb_hlist_find(p_wndhash->p_create_map, p_type, len);
}

static void klbuiex_wndhash_split_path_name(const char* p_path_name, char** p_dir, int* p_dir_len)
{
    const char* p = p_path_name ? strrchr(p_path_name, '/') : NULL;

    if (NULL != p)
    {
        *p_dir = (char*)p_path_name;
        *p_dir_len = p - p_path_name;
    }
    else
    {
        *p_dir = NULL;
        *p_dir_len = 0;
    }
}

int klbuiex_wndhash_append(klbuiex_wndhash_t* p_wndhash, const char* p_type, const char* p_path_name, int x, int y, int w, int h, uint32_t style)
{
    klb_wnd_create_cb cb_create = (klb_wnd_create_cb)klb_hlist_find(p_wndhash->p_create_map, p_type, strlen(p_type));
    if (NULL == cb_create)
    {
        return 1; // 未知类型
    }

    char* p_dir = NULL;
    int dir_len = 0;

    klbuiex_wndhash_split_path_name(p_path_name, &p_dir, &dir_len);
    if (NULL == p_dir)
    {
        return 2; // 路径格式错误
    }

    int path_len = strlen(p_path_name);
    klb_wnd_t* p_wnd = NULL;

    if (0 < dir_len)
    {
        // 路径中有父窗口, 则为子窗口, 添加到对应的父窗口路径下
        klb_wnd_t* p_parent = (klb_wnd_t*)klb_hlist_find(p_wndhash->p_wnd_map, p_dir, dir_len);
        if (NULL == p_parent)
        {
            return 3;
        }

        if (NULL != klb_hlist_find(p_wndhash->p_wnd_map, p_path_name, path_len))
        {
            return 4; // 已经存在
        }

        klb_wnd_t* p_wnd = cb_create(p_wndhash->p_gui, x, y, w, h);
        p_wnd->p_gui = p_wndhash->p_gui;
        klb_wnd_set_style(p_wnd, style | klb_wnd_get_style(p_wnd));
        klb_wnd_push_child(p_parent, p_wnd);

        klb_hlist_push_tail(p_wndhash->p_wnd_map, p_path_name, path_len, p_wnd);
    }
    else
    {
        // 路径中无父窗口, 则为根目录, 顶层窗口
        if (NULL != klb_hlist_find(p_wndhash->p_wnd_map, p_path_name, path_len))
        {
            return 4; // 已经存在
        }

        klb_wnd_t* p_wnd = cb_create(p_wndhash->p_gui, x, y, w, h);
        p_wnd->p_gui = p_wndhash->p_gui;
        klb_wnd_set_top(p_wnd, p_wndhash->p_gui);
        klb_wnd_set_style(p_wnd, style | klb_wnd_get_style(p_wnd));

        klb_hlist_push_tail(p_wndhash->p_wnd_map, p_path_name, path_len, p_wnd);
    }

    return 0;
}

int klbuiex_wndhash_remove(klbuiex_wndhash_t* p_wndhash, const char* p_path_name)
{
    return 0;
}

klb_wnd_t* klbuiex_wndhash_find(klbuiex_wndhash_t* p_wndhash, const char* p_path_name)
{
    return (klb_wnd_t*)klb_hlist_find(p_wndhash->p_wnd_map, p_path_name, strlen(p_path_name));
}

//////////////////////////////////////////////////////////////////////////
// 

static void* klbuiex_wndhash_create(klb_gui_t* p_gui)
{
    klbuiex_wndhash_t* p_wndhash = KLB_MALLOCZ(klbuiex_wndhash_t, 1, 0);

    p_wndhash->p_gui = p_gui;

    p_wndhash->p_create_map = klb_hlist_create(0);
    p_wndhash->p_wnd_map = klb_hlist_create(0);

    return p_wndhash;
}

static void klbuiex_wndhash_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_wndhash_t* p_wndhash = (klbuiex_wndhash_t*)ptr;

    // 释放窗口, todo.
    klb_hlist_clear(p_wndhash->p_wnd_map, NULL, NULL);


    // 释放类型
    klb_hlist_clear(p_wndhash->p_create_map, NULL, NULL);

    KLB_FREE_BY(p_wndhash->p_wnd_map, klb_hlist_destroy);
    KLB_FREE_BY(p_wndhash->p_create_map, klb_hlist_destroy);

    KLB_FREE(p_wndhash)
}

//////////////////////////////////////////////////////////////////////////

int klbuiex_register_wndhash(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_wndhash_create;
    ex.cb_destroy = klbuiex_wndhash_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLBUIEX_WNDHASH, &ex);

    return 0;
}

klbuiex_wndhash_t* klbuiex_get_wndhash(klb_gui_t* p_gui)
{
    return (klbuiex_wndhash_t*)klb_gui_get_extension(p_gui, KLBUIEX_WNDHASH);
}
