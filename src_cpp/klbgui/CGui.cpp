// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/CGui.hpp"
#include "klbgui/CUiWidgets.hpp"
#include <assert.h>


using namespace klb;


namespace klbui {

//////////////////////////////////////////////////////////////////////////
// 构造/析构

CGui::CGui()
{
    m_is_share_gui = false;
    m_gui = klb_gui_create(NULL);

    // 初始化 cpp 额外部分
    Init();
}

// 共享 *p_gui 指针, *p_gui 的生命周期由创建者维护
CGui::CGui(klb_gui_t* p_gui, bool share)
{
    assert(NULL != p_gui);
    assert(true == share); // share 参数 仅仅为 与 构造CGui::CGui(klb_gui_t* p_gui)函数区分

    m_is_share_gui = true;
    m_gui = p_gui;

    // 初始化 cpp 额外部分
    Init();
}

// 托管 *p_gui 指针, *p_gui 由 CGui释放
CGui::CGui(klb_gui_t* p_gui)
{
    assert(NULL != p_gui);

    m_is_share_gui = false;
    m_gui = p_gui;

    // 初始化 cpp 额外部分
    Init();
}

CGui::~CGui()
{
    if (!m_is_share_gui)
    {
        KLB_FREE_BY(m_gui, klb_gui_destroy);
    }

    m_is_share_gui = false;
    m_gui = NULL;
}

void CGui::Init()
{
    // 
    klb_gui_attach_cppgui(m_gui, this);

    // 注册标准cpp窗口类型
    {
        // 注册标准窗口类型
        KLB_GUI_REGISTER_STD_CPP(m_gui);
    }
}

//////////////////////////////////////////////////////////////////////////
// cpp 额外函数

klb_gui_t* CGui::GetGui()
{
    return m_gui;
}

//////////////////////////////////////////////////////////////////////////
// 定义于 "klbgui/klb_gui.h" 的导出函数

int CGui::AttachKluaEnv(klua_env_t* p_env)
{
    return klb_gui_attach_klua_env(m_gui, p_env);
}

int CGui::AttachKluaEnv(klb::CKluaEnv* p_env)
{
    return 0;
}

klua_env_t* CGui::GetKluaEnv()
{
    return klb_gui_get_klua_env(m_gui);
}

int CGui::LoopOnce(int64_t tc)
{
    return klb_gui_loop_once(m_gui, tc);
}

void CGui::AttachCanvas(klb_canvas_t* p_canvas)
{
    klb_gui_attach_canvas(m_gui, p_canvas);
}

klb_canvas_t* CGui::GetCanvas()
{
    return klb_gui_get_canvas(m_gui);
}

void CGui::PushMsg(int msg, int x1, int y1, int x2, int y2, int lparam, int wparam)
{
    klb_gui_push_msg(m_gui, msg, x1, y1, x2, y2, lparam, wparam);
}

int CGui::Register(const char* p_type, klb_wnd_create_cb cb_create)
{
    return klb_gui_register(m_gui, p_type, cb_create);
}

int CGui::Register(const std::string& type, klb_wnd_create_cb cb_create)
{
    return klb_gui_register(m_gui, type.c_str(), cb_create);
}

int CGui::Register(const klb::CString& type, klb_wnd_create_cb cb_create)
{
    return klb_gui_register(m_gui, type.c_str(), cb_create);
}

klb_wnd_create_cb CGui::GetCreater(const char* p_type)
{
    return klb_gui_get_creater(m_gui, p_type);
}

klb_wnd_create_cb CGui::GetCreater(const std::string& type)
{
    return klb_gui_get_creater(m_gui, type.c_str());
}

klb_wnd_create_cb CGui::GetCreater(const klb::CString& type)
{
    return klb_gui_get_creater(m_gui, type.c_str());
}

klb_wnd_t* CGui::CreateWnd(const char* p_type, int x, int y, int w, int h)
{
    return klb_gui_create_wnd(m_gui, p_type, x, y, w, h);
}

klb_wnd_t* CGui::CreateWnd(const std::string& type, int x, int y, int w, int h)
{
    return klb_gui_create_wnd(m_gui, type.c_str(), x, y, w, h);
}

klb_wnd_t* CGui::CreateWnd(const klb::CString& type, int x, int y, int w, int h)
{
    return klb_gui_create_wnd(m_gui, type.c_str(), x, y, w, h);
}

int CGui::LoadImage(const char* p_key, const char* p_img_path)
{
    return klb_gui_load_image(m_gui,p_key, p_img_path);
}

int CGui::LoadImage(const std::string& key, const std::string& img_path)
{
    return klb_gui_load_image(m_gui, key.c_str(), img_path.c_str());
}

int CGui::LoadImage(const klb::CString& key, const std::string& img_path)
{
    return klb_gui_load_image(m_gui, key.c_str(), img_path.c_str());
}

int CGui::ImageSize(const char* p_key, int* p_out_w, int* p_out_h)
{
    return klb_gui_image_size(m_gui, p_key, p_out_w, p_out_h);
}

int CGui::ImageSize(const std::string& key, int* p_out_w, int* p_out_h)
{
    return klb_gui_image_size(m_gui, key.c_str(), p_out_w, p_out_h);
}

int CGui::ImageSize(const klb::CString& key, int* p_out_w, int* p_out_h)
{
    return klb_gui_image_size(m_gui, key.c_str(), p_out_w, p_out_h);
}

int CGui::Append(const char* p_type, const char* p_path_name, int x, int y, int w, int h, uint32_t style)
{
    return klb_gui_append(m_gui, p_type, p_path_name, x, y, w, h, style);
}

int CGui::Append(const std::string& type, const std::string& path_name, int x, int y, int w, int h, uint32_t style)
{
    return klb_gui_append(m_gui, type.c_str(), path_name.c_str(), x, y, w, h, style);
}

int CGui::Append(const klb::CString& type, const klb::CString& path_name, int x, int y, int w, int h, uint32_t style)
{
    return klb_gui_append(m_gui, type.c_str(), path_name.c_str(), x, y, w, h, style);
}

int CGui::Remove(const char* p_path_name)
{
    return klb_gui_remove(m_gui, p_path_name);
}

int CGui::Remove(const std::string& path_name)
{
    return klb_gui_remove(m_gui, path_name.c_str());
}

int CGui::Remove(const klb::CString& path_name)
{
    return klb_gui_remove(m_gui, path_name.c_str());
}

int CGui::Clear()
{
    return klb_gui_clear(m_gui);
}

int CGui::ClearAsync(klb_gui_clear_result_cb cb_clear, void* ptr)
{
    return klb_gui_clear_async(m_gui, cb_clear, ptr);
}

int CGui::Modal(const char* p_path_name)
{
    return klb_gui_modal(m_gui, p_path_name);
}

int CGui::Modal(const std::string& path_name)
{
    return klb_gui_modal(m_gui, path_name.c_str());
}

int CGui::Modal(const klb::CString& path_name)
{
    return klb_gui_modal(m_gui, path_name.c_str());
}

int CGui::ModalWnd(klb_wnd_t* p_top)
{
    return klb_gui_modal_wnd(m_gui, p_top);
}

int CGui::ModalWnd(CWnd* p_top)
{
    return klb_gui_modal_wnd(m_gui, p_top->GetWnd());
}

int CGui::ModalEnd(bool all)
{
    return klb_gui_modal_end(m_gui, all, NULL);
}

int CGui::ModalEnd(bool all, const char* p_path_name)
{
    return klb_gui_modal_end(m_gui, all, p_path_name);
}

int CGui::ModalEnd(bool all, const std::string& path_name)
{
    return klb_gui_modal_end(m_gui, all, path_name.c_str());
}

int CGui::ModalEnd(bool all, const klb::CString& path_name)
{
    return klb_gui_modal_end(m_gui, all, path_name.c_str());
}

int CGui::ModalNum()
{
    return klb_gui_modal_num(m_gui);
}

int CGui::Popup(const char* p_path_name)
{
    return klb_gui_popup(m_gui, p_path_name);
}

int CGui::Popup(const std::string& path_name)
{
    return klb_gui_popup(m_gui, path_name.c_str());
}

int CGui::Popup(const klb::CString& path_name)
{
    return klb_gui_popup(m_gui, path_name.c_str());
}

int CGui::PopupWnd(klb_wnd_t* p_top)
{
    return klb_gui_popup_wnd(m_gui, p_top);
}

int CGui::PopupWnd(CWnd* p_top)
{
    return klb_gui_popup_wnd(m_gui, p_top->GetWnd());
}

int CGui::PopupEnd(bool all)
{
    return klb_gui_popup_end(m_gui, all);
}

int CGui::PopupNum()
{
    return klb_gui_popup_num(m_gui);
}

int CGui::MessageBox(const char* p_path_name)
{
    return klb_gui_messagebox(m_gui, p_path_name);
}

int CGui::MessageBox(const std::string& path_name)
{
    return klb_gui_messagebox(m_gui, path_name.c_str());
}

int CGui::MessageBox(const klb::CString& path_name)
{
    return klb_gui_messagebox(m_gui, path_name.c_str());
}

int CGui::MessageBoxWnd(klb_wnd_t* p_top)
{
    return klb_gui_messagebox_wnd(m_gui, p_top);
}

int CGui::MessageBoxWnd(CWnd* p_top)
{
    return klb_gui_messagebox_wnd(m_gui, p_top->GetWnd());
}

int CGui::MessageBoxEnd()
{
    return klb_gui_messagebox_end(m_gui);
}

int CGui::MessageBoxNum()
{
    return klb_gui_messagebox_num(m_gui);
}

int CGui::BindCommand(const char* p_path_name, klb_wnd_on_command_cb on_command, void* p_obj)
{
    return klb_gui_bind_command(m_gui, p_path_name, on_command, p_obj);
}

int CGui::BindCommand(const std::string& path_name, klb_wnd_on_command_cb on_command, void* p_obj)
{
    return klb_gui_bind_command(m_gui, path_name.c_str(), on_command, p_obj);
}

int CGui::BindCommand(const klb::CString& path_name, klb_wnd_on_command_cb on_command, void* p_obj)
{
    return klb_gui_bind_command(m_gui, path_name.c_str(), on_command, p_obj);
}

int CGui::CallControlAndCommand(const char* p_path_name, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return klb_gui_call_control_and_command(m_gui, p_path_name, msg, p_pt1, p_pt2, lparam, wparam);
}

int CGui::CallControlAndCommand(const std::string& path_name, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return klb_gui_call_control_and_command(m_gui, path_name.c_str(), msg, p_pt1, p_pt2, lparam, wparam);
}

int CGui::CallControlAndCommand(const klb::CString& path_name, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return klb_gui_call_control_and_command(m_gui, path_name.c_str(), msg, p_pt1, p_pt2, lparam, wparam);
}

int CGui::Set(const char* p_path_name, const klb_map_t* p_map)
{
    return klb_gui_set(m_gui, p_path_name, p_map);
}

int CGui::Set(const std::string& path_name, const klb_map_t* p_map)
{
    return klb_gui_set(m_gui, path_name.c_str(), p_map);
}

int CGui::Set(const klb::CString& path_name, const klb_map_t* p_map)
{
    return klb_gui_set(m_gui, path_name.c_str(), p_map);
}

klb_map_t* CGui::Get(const char* p_path_name, const klb_map_t* p_map)
{
    return klb_gui_get(m_gui, p_path_name, p_map);
}

klb_map_t* CGui::Get(const std::string& path_name, const klb_map_t* p_map)
{
    return klb_gui_get(m_gui, path_name.c_str(), p_map);
}

klb_map_t* CGui::Get(const klb::CString& path_name, const klb_map_t* p_map)
{
    return klb_gui_get(m_gui, path_name.c_str(), p_map);
}

int CGui::Show(const char* p_path_name, bool show)
{
    return klb_gui_show(m_gui, p_path_name, show);
}

int CGui::Show(const std::string& path_name, bool show)
{
    return klb_gui_show(m_gui, path_name.c_str(), show);
}

int CGui::Show(const klb::CString& path_name, bool show)
{
    return klb_gui_show(m_gui, path_name.c_str(), show);
}

int CGui::Move(const char* p_path_name, int x, int y)
{
    return klb_gui_move(m_gui, p_path_name, x, y);
}

int CGui::Move(const std::string& path_name, int x, int y)
{
    return klb_gui_move(m_gui, path_name.c_str(), x, y);
}

int CGui::Move(const klb::CString& path_name, int x, int y)
{
    return klb_gui_move(m_gui, path_name.c_str(), x, y);
}

int CGui::Resize(const char* p_path_name, int w, int h)
{
    return klb_gui_resize(m_gui, p_path_name, w, h);
}

int CGui::Resize(const std::string& path_name, int w, int h)
{
    return klb_gui_resize(m_gui, path_name.c_str(), w, h);
}

int CGui::Resize(const klb::CString& path_name, int w, int h)
{
    return klb_gui_resize(m_gui, path_name.c_str(), w, h);
}

int CGui::WndposInCanvas(const char* p_path_name, klb_rect_t* p_out_rect)
{
    return klb_gui_wndpos_in_canvas(m_gui, p_path_name, p_out_rect);
}

int CGui::WndposInCanvas(const std::string& path_name, klb_rect_t* p_out_rect)
{
    return klb_gui_wndpos_in_canvas(m_gui, path_name.c_str(), p_out_rect);
}

int CGui::WndposInCanvas(const klb::CString& path_name, klb_rect_t* p_out_rect)
{
    return klb_gui_wndpos_in_canvas(m_gui, path_name.c_str(), p_out_rect);
}

int CGui::WndposInParent(const char* p_path_name, klb_rect_t* p_out_rect)
{
    return klb_gui_wndpos_in_parent(m_gui, p_path_name, p_out_rect);
}

int CGui::WndposInParent(const std::string& path_name, klb_rect_t* p_out_rect)
{
    return klb_gui_wndpos_in_parent(m_gui, path_name.c_str(), p_out_rect);
}

int CGui::WndposInParent(const klb::CString& path_name, klb_rect_t* p_out_rect)
{
    return klb_gui_wndpos_in_parent(m_gui, path_name.c_str(), p_out_rect);
}

int CGui::SuggestW(const char* p_path_name, int* p_out_w)
{
    return klb_gui_suggestw(m_gui, p_path_name, p_out_w);
}

int CGui::SuggestW(const std::string& path_name, int* p_out_w)
{
    return klb_gui_suggestw(m_gui, path_name.c_str(), p_out_w);
}

int CGui::SuggestW(const klb::CString& path_name, int* p_out_w)
{
    return klb_gui_suggestw(m_gui, path_name.c_str(), p_out_w);
}

int CGui::SuggestH(const char* p_path_name, int* p_out_h)
{
    return klb_gui_suggesth(m_gui, p_path_name, p_out_h);
}

int CGui::SuggestH(const std::string& path_name, int* p_out_h)
{
    return klb_gui_suggesth(m_gui, path_name.c_str(), p_out_h);
}

int CGui::SuggestH(const klb::CString& path_name, int* p_out_h)
{
    return klb_gui_suggesth(m_gui, path_name.c_str(), p_out_h);
}

int CGui::GetWH(int* p_out_w, int* p_out_h)
{
    return klb_gui_get_wh(m_gui, p_out_w, p_out_h);
}

klb_wnd_t* CGui::GetFocus()
{
    return klb_gui_get_focus(m_gui);
}

klb_wnd_t* CGui::GetFocusTop()
{
    return klb_gui_get_focus_top(m_gui);
}

void CGui::SetFocusdelay(int64_t timeout)
{
    klb_gui_set_focusdelay(m_gui, timeout);
}

void CGui::Update()
{
    klb_gui_update(m_gui);
}

int64_t CGui::GetTickCount()
{
    return klb_gui_get_tick_count(m_gui);
}

int64_t CGui::GetTickerInterval()
{
    return klb_gui_get_ticker_interval(m_gui);
}

void CGui::SetTickerInterval(int64_t interval)
{
    klb_gui_set_ticker_interval(m_gui, interval);
}

//////////////////////////////////////////////////////////////////////////
// 定义于 "klbgui/klbui_css.h" 的导出函数
// CSS相关 辅助函数

bool CGui::CheckCssColor(const klb_map_t* p_map, int start, uint32_t* p_out_color)
{
    return klb_gui_check_color(m_gui, p_map, start, p_out_color);
}

bool CGui::CheckCssColor(klb::CMap* p_map, int start, uint32_t* p_out_color)
{
    klb::CMap* p_cmap = (klb::CMap*)p_map;
    return klb_gui_check_color(m_gui, p_cmap->GetMap(), start, p_out_color);
}

klb_map_t* CGui::CssMap(const char* p_type)
{
    return klb_gui_css_map(m_gui, p_type);
}

klb_map_t* CGui::CssMap(const std::string& type)
{
    return klb_gui_css_map(m_gui, type.c_str());
}

klb_map_t* CGui::CssMap(const klb::CString& type)
{
    return klb_gui_css_map(m_gui, type.c_str());
}

klb_map_t* CGui::NewCssMap(const char* p_type)
{
    return klb_gui_new_css_map(m_gui, p_type);
}

klb_map_t* CGui::NewCssMap(const std::string& type)
{
    return klb_gui_new_css_map(m_gui, type.c_str());
}

klb_map_t* CGui::NewCssMap(const klb::CString& type)
{
    return klb_gui_new_css_map(m_gui, type.c_str());
}

klb_map_t* CGui::GlobalCssMap(const char* p_type)
{
    return klb_gui_globalcss_map(m_gui, p_type);
}

klb_map_t* CGui::GlobalCssMap(const std::string& type)
{
    return klb_gui_globalcss_map(m_gui, type.c_str());
}

klb_map_t* CGui::GlobalCssMap(const klb::CString& type)
{
    return klb_gui_globalcss_map(m_gui, type.c_str());
}

klb_map_t* CGui::NewGlobalCssMap(const char* p_type)
{
    return klb_gui_new_globalcss_map(m_gui, p_type);
}

klb_map_t* CGui::NewGlobalCssMap(const std::string& type)
{
    return klb_gui_new_globalcss_map(m_gui, type.c_str());
}

klb_map_t* CGui::NewGlobalCssMap(const klb::CString& type)
{
    return klb_gui_new_globalcss_map(m_gui, type.c_str());
}

void CGui::SetGlobalCssPtr(const char* p_type, void* p_css, klb_gui_globalcss_destroy_cb cb_destroy)
{
    klb_gui_globalcss_set_ptr(m_gui, p_type, p_css, cb_destroy);
}

void CGui::SetGlobalCssPtr(const std::string& type, void* p_css, klb_gui_globalcss_destroy_cb cb_destroy)
{
    klb_gui_globalcss_set_ptr(m_gui, type.c_str(), p_css, cb_destroy);
}

void CGui::SetGlobalCssPtr(const klb::CString& type, void* p_css, klb_gui_globalcss_destroy_cb cb_destroy)
{
    klb_gui_globalcss_set_ptr(m_gui, type.c_str(), p_css, cb_destroy);
}

void* CGui::GetGlobalCssPtr(const char* p_type)
{
    return klb_gui_globalcss_get_ptr(m_gui, p_type);
}

void* CGui::GetGlobalCssPtr(const std::string& type)
{
    return klb_gui_globalcss_get_ptr(m_gui, type.c_str());
}

void* CGui::GetGlobalCssPtr(const klb::CString& type)
{
    return klb_gui_globalcss_get_ptr(m_gui, type.c_str());
}

int CGui::SetGlobalCss(const char* p_type, const klb_map_t* p_map)
{
    return klb_gui_globalcss_set(m_gui, p_type, p_map);
}

int CGui::SetGlobalCss(const std::string& type, const klb_map_t* p_map)
{
    return klb_gui_globalcss_set(m_gui, type.c_str(), p_map);
}

int CGui::SetGlobalCss(const klb::CString& type, const klb_map_t* p_map)
{
    return klb_gui_globalcss_set(m_gui, type.c_str(), p_map);
}

klb_map_t* CGui::GetGlobalCss(const char* p_type, const klb_map_t* p_map)
{
    return klb_gui_globalcss_get(m_gui, p_type, p_map);
}

klb_map_t* CGui::GetGlobalCss(const std::string& type, const klb_map_t* p_map)
{
    return klb_gui_globalcss_get(m_gui, type.c_str(), p_map);
}

klb_map_t* CGui::GetGlobalCss(const klb::CString& type, const klb_map_t* p_map)
{
    return klb_gui_globalcss_get(m_gui, type.c_str(), p_map);
}

//////////////////////////////////////////////////////////////////////////
// 定义于 "klbgui/klbui_datetime.h" 的导出函数


void CGui::SetDateFmt(int fmt)
{
    klb_gui_set_datefmt(m_gui, fmt);
}

int CGui::GetDateFmt()
{
    return klb_gui_get_datefmt(m_gui);
}

void CGui::SetTimeFmt(int fmt)
{
    klb_gui_set_timefmt(m_gui, fmt);
}

int CGui::GetTimeFmt()
{
    return klb_gui_get_timefmt(m_gui);
}


//////////////////////////////////////////////////////////////////////////
// 定义于 "klbgui/klbui_default.h" 的导出函数

const klbui_default_t* CGui::GetStdDefault()
{
    return klb_gui_get_std_default(m_gui);
}

/// @brief 设置CSS默认值(全局值)
int CGui::SetDefaultCss(const klb_map_t* p_map)
{
    return klb_gui_default_css_set(m_gui, p_map);
}

/// @brief 获取CSS默认值(全局值)
klb_map_t* CGui::GetDefaultCss(const klb_map_t* p_map)
{
    return klb_gui_default_css_get(m_gui, p_map);
}


//////////////////////////////////////////////////////////////////////////
// 定义于 "klbgui/klbui_extension.h" 的导出函数

int CGui::RegisterExtension(const char* p_name, const klb_gui_extension_t* p_extension)
{
    return klb_gui_register_extension(m_gui, p_name, p_extension);
}

int CGui::RegisterExtension(const std::string& name, const klb_gui_extension_t* p_extension)
{
    return klb_gui_register_extension(m_gui, name.c_str(), p_extension);
}

int CGui::RegisterExtension(const klb::CString& name, const klb_gui_extension_t* p_extension)
{
    return klb_gui_register_extension(m_gui, name.c_str(), p_extension);
}

void* CGui::GetExtension(const char* p_name)
{
    return klb_gui_get_extension(m_gui, p_name);
}

void* CGui::GetExtension(const std::string& name)
{
    return klb_gui_get_extension(m_gui, name.c_str());
}

void* CGui::GetExtension(const klb::CString& name)
{
    return klb_gui_get_extension(m_gui, name.c_str());
}

//////////////////////////////////////////////////////////////////////////
// 定义于 "klbgui/klbui_shwnd.h" 的导出函数


int CGui::PushShWnd(const char* p_path, klb_wnd_t* p_top_wnd)
{
    return klb_gui_push_shwnd(m_gui, p_path, p_top_wnd);
}

int CGui::PushShWnd(const std::string& path, klb_wnd_t* p_top_wnd)
{
    return klb_gui_push_shwnd(m_gui, path.c_str(), p_top_wnd);
}

int CGui::PushShWnd(const klb::CString& path, klb_wnd_t* p_top_wnd)
{
    return klb_gui_push_shwnd(m_gui, path.c_str(), p_top_wnd);
}

klb_wnd_t* CGui::GetShWnd(const char* p_path)
{
    return klb_gui_get_shwnd(m_gui, p_path);
}

klb_wnd_t* CGui::GetShWnd(const std::string& path)
{
    return klb_gui_get_shwnd(m_gui, path.c_str());
}

klb_wnd_t* CGui::GetShWnd(const klb::CString& path)
{
    return klb_gui_get_shwnd(m_gui, path.c_str());
}

int CGui::SetShWndCss(const char* p_path, const klb_map_t* p_map)
{
    return klb_gui_shwnd_css_set(m_gui, p_path, p_map);
}

int CGui::SetShWndCss(const std::string& path, const klb_map_t* p_map)
{
    return klb_gui_shwnd_css_set(m_gui, path.c_str(), p_map);
}

int CGui::SetShWndCss(const klb::CString& path, const klb_map_t* p_map)
{
    return klb_gui_shwnd_css_set(m_gui, path.c_str(), p_map);
}

klb_map_t* CGui::GetShWndCss(const char* p_path, const klb_map_t* p_map)
{
    return klb_gui_shwnd_css_get(m_gui, p_path, p_map);
}

klb_map_t* CGui::GetShWndCss(const std::string& path, const klb_map_t* p_map)
{
    return klb_gui_shwnd_css_get(m_gui, path.c_str(), p_map);
}

klb_map_t* CGui::GetShWndCss(const klb::CString& path, const klb_map_t* p_map)
{
    return klb_gui_shwnd_css_get(m_gui, path.c_str(), p_map);
}

//////////////////////////////////////////////////////////////////////////
// 定义于 "klbgui/klbui_timer.h" 的导出函数


//////////////////////////////////////////////////////////////////////////
} // namespace klbgui
//end
