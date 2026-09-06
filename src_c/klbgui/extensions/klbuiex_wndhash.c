#include "klbgui/extensions/klbuiex_wndhash.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbgui/klb_wnd_in.h"


#define KLB_GUIEX_wndhash   "KLB-GUIEX-wndhash"


/// @struct klbuiex_wndhash_top_t
/// @brief  单个顶层窗口
typedef struct klbuiex_wndhash_top_t_
{
    klb_wnd_t*      p_top_wnd;          ///< 顶层窗口 top wnd
    klb_hlist_t*    p_hlist;            ///< 窗口对应的hlist索引
}klbuiex_wndhash_top_t;


/// @struct klbuiex_wndhash_t
/// @brief  存储wnd,及hash查找表
typedef struct klbuiex_wndhash_t_
{
    klb_gui_t*      p_gui;              ///< gui对象
    klbuiex_flex_t* p_flex;             ///< 自动布局扩展

    klb_hlist_t*    p_creater_hlist;    ///< 类型对应的创建函数表
    klb_hlist_t*    p_wnd_hlist;        ///< 顶层窗口: klbuiex_wndhash_top_t*
}klbuiex_wndhash_t;

//////////////////////////////////////////////////////////////////////////
static void klbuiex_wndhash_quit(klbuiex_wndhash_t* p_wndhash);


//////////////////////////////////////////////////////////////////////////
// 

int klbuiex_wndhash_set_flex(klbuiex_wndhash_t* p_wndhash, klbuiex_flex_t* p_flex)
{
    p_wndhash->p_flex = p_flex;
    return 0;
}

int klbuiex_wndhash_register(klbuiex_wndhash_t* p_wndhash, const char* p_type, klb_wnd_create_cb cb_create)
{
    int len = strlen(p_type);

    if (NULL == klb_hlist_find_iter(p_wndhash->p_creater_hlist, p_type, len))
    {
        // 若不存在, 则添加
        klb_hlist_push_tail(p_wndhash->p_creater_hlist, p_type, len, cb_create);
    }
    else
    {
        // 若已存在类型, 则覆盖
        klb_hlist_update(p_wndhash->p_creater_hlist, p_type, len, cb_create);
    }

    return 0;
}

klb_wnd_create_cb klbuiex_wndhash_get_creater(klbuiex_wndhash_t* p_wndhash, const char* p_type)
{
    int len = strlen(p_type);

    return (klb_wnd_create_cb)klb_hlist_find(p_wndhash->p_creater_hlist, p_type, len);
}

static klbuiex_wndhash_top_t* find_top_klbuiex_wndhash(klbuiex_wndhash_t* p_wndhash, const char* p_path_name, int path_len)
{
    const char* p1 = strchr(p_path_name, '/');
    if (NULL == p1)
    {
        return NULL; // 错误格式
    }

    const char* p2 = strchr(p1 + 1, '/');
    if (NULL != p2)
    {
        // eg. "/home/btn1"
        int baselen = p2 - p_path_name;

        return (klbuiex_wndhash_top_t*)klb_hlist_find(p_wndhash->p_wnd_hlist, p_path_name, baselen);
    }
    else
    {
        // eg. "/home"
        int baselen = path_len;

        return (klbuiex_wndhash_top_t*)klb_hlist_find(p_wndhash->p_wnd_hlist, p_path_name, baselen);
    }
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

int klbuiex_wndhash_append(klbuiex_wndhash_t* p_wndhash, const char* p_type, const char* p_path_name, int x, int y, int w, int h, uint32_t style, klb_wnd_t** p_out_wnd)
{
    // step 1. 获取控件类型创建函数
    klb_wnd_create_cb cb_create = klbuiex_wndhash_get_creater(p_wndhash, p_type);
    if (NULL == cb_create)
    {
        return 1; // 未知类型
    }

    // step2. 分割路径名
    char* p_dir = NULL;
    int dir_len = 0;

    klbuiex_wndhash_split_path_name(p_path_name, &p_dir, &dir_len);
    if (NULL == p_dir)
    {
        return 2; // 路径格式错误, eg "/home"
    }

    int path_len = strlen(p_path_name);

    if (0 < dir_len)
    {
        // 非根目录: eg. "/home/btn1"
        // 添加到对应的父窗口路径下

        // step A. 查找顶层窗口
        klbuiex_wndhash_top_t* p_top = find_top_klbuiex_wndhash(p_wndhash, p_path_name, path_len);
        if (NULL == p_top)
        {
            return 3; // 没有找到根路径窗口
        }

        // step B. 查找对应的父窗口
        klb_wnd_t* p_parent = (klb_wnd_t*)klb_hlist_find(p_top->p_hlist, p_dir, dir_len);
        if (NULL == p_parent)
        {
            return 5; // 没有找到父窗口
        }

        // step C. 检查是否存在路径冲突
        if (NULL != klb_hlist_find(p_top->p_hlist, p_path_name, path_len))
        {
            return 4; // 已经存在
        }

        // step D. 检查是否需要自动布局
        int x1 = 0, y1 = 0, w1 = 0, h1 = 0;
        bool is_flex = klbuiex_flex_is_flex(x, y, w, h, style, &x1, &y1, &w1, &h1);

        // step E. 创建窗口
        klb_wnd_t* p_wnd = cb_create(p_wndhash->p_gui, x1, y1, w1, h1);
        klb_wnd_set_style(p_wnd, style | klb_wnd_get_style(p_wnd));

        // step F. 将窗口添加到父窗口
        klb_wnd_push_child(p_parent, p_wnd);

        // step G. 存储
        klb_hlist_push_tail(p_top->p_hlist, p_path_name, path_len, p_wnd);

        // step H. 自身检查是否需要自动布局
        if( is_flex ) 
        {
            p_wnd->pos.rect_in_flex.x = x;
            p_wnd->pos.rect_in_flex.y = y;
            p_wnd->pos.rect_in_flex.w = w;
            p_wnd->pos.rect_in_flex.h = h;

            // KLB_WND_STATUS_FLEX_DIRTY 
            p_wnd->state.status |= KLB_WND_STATUS_FLEX_DIRTY;

            // 父窗口放入 klbuiex_flex_push
            klbuiex_flex_push(p_wndhash->p_flex, p_parent);
        }

        if( NULL != p_out_wnd )
        {
            *p_out_wnd = p_wnd;
        }
    }
    else
    {
        // 根目录(顶层窗口): eg. "/home"

        // step A. 检查是否有重名根路径
        if (NULL != klb_hlist_find(p_wndhash->p_wnd_hlist, p_path_name, path_len))
        {
            return 4; // 已经存在
        }

        // step B. 检查是否需要自动布局
        int x1 = 0, y1 = 0, w1 = 0, h1 = 0;
        bool is_flex = klbuiex_flex_is_flex(x, y, w, h, style, &x1, &y1, &w1, &h1);

        // step C. 创建顶层窗口
        klb_wnd_t* p_wnd = cb_create(p_wndhash->p_gui, x1, y1, w1, h1);
        klb_wnd_set_top(p_wnd);
        klb_wnd_set_style(p_wnd, style | klb_wnd_get_style(p_wnd));

        // step E. 将顶层窗口及对应的索引 存入 p_wnd_hlist
        klbuiex_wndhash_top_t* p_top = KLB_MALLOCZ(klbuiex_wndhash_top_t, 1, 0);
        p_top->p_top_wnd = p_wnd;
        p_top->p_hlist = klb_hlist_create(0);
        klb_hlist_push_tail(p_wndhash->p_wnd_hlist, p_path_name, path_len, p_top);

        // step F. 顶层窗口本身也存一份
        klb_hlist_push_tail(p_top->p_hlist, p_path_name, path_len, p_wnd);

        // step G. 检查是否需要自动布局
        if( is_flex ) 
        {
            p_wnd->pos.rect_in_flex.x = x;
            p_wnd->pos.rect_in_flex.y = y;
            p_wnd->pos.rect_in_flex.w = w;
            p_wnd->pos.rect_in_flex.h = h;

            // KLB_WND_STATUS_FLEX_DIRTY 
            p_wnd->state.status |= KLB_WND_STATUS_FLEX_DIRTY;

            // 自身放入 klbuiex_flex_push, 本身是顶层窗口,  布局则依赖画布 
            klbuiex_flex_push(p_wndhash->p_flex, p_wnd);
        }
        
        if( NULL != p_out_wnd )
        {
            *p_out_wnd = p_wnd;
        }
    }

    return 0;
}

int klbuiex_wndhash_remove(klbuiex_wndhash_t* p_wndhash, const char* p_path_name)
{
    const char* p1 = strchr(p_path_name, '/');
    if (NULL == p1)
    {
        return 1; // 错误格式
    }

    const char* p2 = strchr(p1 + 1, '/');
    if (NULL != p2)
    {
        // eg. "/home/btn1"
        return 2; // 格式错误
    }

    // 只能为根目录, eg: "/home"
    int path_len = strlen(p_path_name);

    klbuiex_wndhash_top_t* p_top = klb_hlist_remove_bykey(p_wndhash->p_wnd_hlist, p_path_name, path_len);

    if (NULL != p_top)
    {
        // 销毁顶层窗口 及 其子窗口
        klb_wnd_destroy_tree(p_top->p_top_wnd);

        // 清空节点数据即可
        klb_hlist_clear(p_top->p_hlist, NULL, NULL);

        // 销毁索引表
        klb_hlist_destroy(p_top->p_hlist);

        KLB_FREE(p_top);

        return 0;
    }

    return 1;
}

klb_wnd_t* klbuiex_wndhash_find(klbuiex_wndhash_t* p_wndhash, const char* p_path_name)
{
    int path_len = strlen(p_path_name);
    klbuiex_wndhash_top_t* p_top = find_top_klbuiex_wndhash(p_wndhash, p_path_name, path_len);

    if (NULL == p_top)
    {
        return NULL;
    }

    return (klb_wnd_t*)klb_hlist_find(p_top->p_hlist, p_path_name, path_len);
}

//////////////////////////////////////////////////////////////////////////
// 

static void* klbuiex_wndhash_create(klb_gui_t* p_gui)
{
    klbuiex_wndhash_t* p_wndhash = KLB_MALLOCZ(klbuiex_wndhash_t, 1, 0);

    p_wndhash->p_gui = p_gui;

    p_wndhash->p_creater_hlist = klb_hlist_create(0);
    p_wndhash->p_wnd_hlist = klb_hlist_create(0);

    return p_wndhash;
}

static void klbuiex_wndhash_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_wndhash_t* p_wndhash = (klbuiex_wndhash_t*)ptr;

    // 退出
    klbuiex_wndhash_quit(p_wndhash);

    // 销毁
    KLB_FREE_BY(p_wndhash->p_wnd_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_wndhash->p_creater_hlist, klb_hlist_destroy);
    KLB_FREE(p_wndhash)
}

static void klbuiex_wndhash_clear(klbuiex_wndhash_t* p_wndhash)
{
    // 清空 窗口列表
    while (0 < klb_hlist_size(p_wndhash->p_wnd_hlist))
    {
        klbuiex_wndhash_top_t* p_top = klb_hlist_pop_head(p_wndhash->p_wnd_hlist);

        // 销毁顶层窗口 及 其子窗口
        klb_wnd_destroy_tree(p_top->p_top_wnd);

        // 清空节点数据即可
        klb_hlist_clear(p_top->p_hlist, NULL, NULL);

        // 销毁索引表
        klb_hlist_destroy(p_top->p_hlist);

        KLB_FREE(p_top);
    }
}

static void klbuiex_wndhash_quit(klbuiex_wndhash_t* p_wndhash)
{
    // 清空 窗口列表
    klbuiex_wndhash_clear(p_wndhash);

    // 清空 创建函数, 清空节点数据即可
    klb_hlist_clear(p_wndhash->p_creater_hlist, NULL, NULL);
}

/// @brief 控制操作消息
static int klbuiex_wndhash_control(void* ptr, klb_gui_t* p_gui, int msg, uint8_t* p_param_in_out, int param_size)
{
    klbuiex_wndhash_t* p_wndhash = (klbuiex_wndhash_t*)ptr;

    switch (msg)
    {
    case KLBUI_EX_MSG_quit:
        klbuiex_wndhash_quit(p_wndhash);
        break;

    case KLBUI_EX_MSG_clear:
        klbuiex_wndhash_clear(p_wndhash);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// register / get

klbuiex_wndhash_t* klbuiex_get_wndhash(klb_gui_t* p_gui)
{
    return (klbuiex_wndhash_t*)klb_gui_get_extension(p_gui, KLB_GUIEX_wndhash);
}

int klbuiex_register_wndhash(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_wndhash_create;
    ex.cb_destroy = klbuiex_wndhash_destroy;
    ex.cb_control = klbuiex_wndhash_control;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLB_GUIEX_wndhash, &ex);

    return 0;
}
