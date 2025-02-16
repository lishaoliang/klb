///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2024, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CWnd.hpp
/// @brief   klb_wnd_t 的 cpp 封装
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_CWND_HPP__
#define __KLBUI_CWND_HPP__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klb_gui.h"
#include "klbgui/CCss.hpp"
#include "klbutil/CString.hpp"
#include "klbutil/CMap.hpp"

namespace klbui {


class CGui;
class CWnd;


/// @brief 属性函数格式
/// @return void
typedef void(*klb_cwnd_css_cb)(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

/// @brief 消息响应函数
typedef int(*klb_cwnd_on_command_cb)(CWnd* p_cwnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


/// @class CWnd
/// @brief 窗口基类
KLB_EXTERN class KLB_API_CPP CWnd
{
public:
    //////////////////////////////////////////////////////////////////////////
    // 构造/析构
    CWnd(CGui* p_gui, int x, int y, int w, int h);
    virtual ~CWnd();

public:
    //////////////////////////////////////////////////////////////////////////
    // 

    /// @brief 获取原始 C 指针
    klb_wnd_t* GetWnd();

    /// @brief 设置/获取 注册类型(字符串)
    void SetType(const std::string& type);
    const klb::CString& GetType();

    /// @brief 设置/获取 类型(整型); type id
    void SetTID(int tid);
    int GetTID();

    /// @brief CSS
    void SetCss(CCss* p_css);
    CCss* GetCss();

public:
    // to cwnd
    static CWnd* ToCWnd(klb_wnd_t* p_wnd);

    // udata
    static void* GetUserData(klb_wnd_t* p_wnd);
    static CWnd* GetUserDataCWnd(klb_wnd_t* p_wnd);

public:
    //////////////////////////////////////////////////////////////////////////
    // 定义于 "klbgui/klb_wnd.h" 的导出函数


    ////////////////////////////////////
    // 通用

    /// @brief 获取gui/CGui
    klb_gui_t* GetGui();
    void GetGui(CGui** p_gui);
    CGui* GetCGui();

    /// @brief 获取画布
    /// @return klb_canvas_t* 画布指针
    /// @note 不要缓存窗口画布指针; 需要使用时, 临时获取
    ///       框架可能会依据当前状态情况, 变更窗口画布
    klb_canvas_t* GetCanvas();

    ////////////////////////////////////
    // 样式

    /// @brief 获取样式
    /// @return uint32_t 样式
    uint32_t GetStyle();


    /// @brief 设置样式
    /// @param [in] style       样式
    /// @return 无
    void SetStyle(uint32_t style);

    ////////////////////////////////////
    // 状态

    /// @brief 获取当前状态值
    uint32_t GetStatus();

    /// @brief 获取是否聚焦
    bool IsFocus();

    /// @brief 设置是否显示: 附带标记更新 窗口
    void Show(bool show);

    /// @brief 获取是否显示
    bool IsShow();

    /// @brief 设置是否隐藏: 只设置状态
    /// @note  多数时候, 可能只是需要设置显隐标记, 并不需要触发更新标记
    ///        这里使用 hide / show 函数来区分
    void Hide(bool hide);

    /// @brief 获取是否隐藏
    bool IsHide();

    /// @brief 设置输入状态
    void Input(bool input);

    /// @brief 获取是否是输入状态
    bool IsInput();

    /// @brief 设置选中状态
    void Check(bool check);

    /// @brief 获取是否是选中状态
    bool IsCheck();

    /// @brief 设置不使能状态
    void Disable(bool disable);

    /// @brief 获取是否是 不使能
    bool IsDisable();

    /// @brief 设置使能状态
    void Enable(bool enable);

    /// @brief 获取是否是 使能
    bool IsEnable();

    /// @brief 获取是否是 所有激活窗口中的最顶层
    /// @note  此状态的设置函数, 只能由框架内部决定
    bool IsTopMost();

    /// @brief 设置动态 是否需要更新
    ///  dynamic tip 
    void DynTip(bool update);

    /// @brief 获取动态tip 是否需要更新
    ///  dynamic tip 
    bool IsDynTip();


    ////////////////////////////////////
    // tip

    /// @brief 设置,获取 静态tip
    void SetTip(const char* p_tip);
    void SetTip(const std::string& tip);
    void SetTip(const klb::CString& tip);

    void GetTip(std::string& tip);
    void GetTip(klb::CString& tip);

    /// @brief 设置,获取 动态tip
    void SetTipDynamic(const char* p_tip);
    void SetTipDynamic(const std::string& tip);
    void SetTipDynamic(const klb::CString& tip);

    void GetTipDynamic(std::string& tip);
    void GetTipDynamic(klb::CString& tip);

    /// @brief 标记刷新 tip
    /// @note 仅标记, 由框架决定合适的刷新时机
    ///       仅 设置了tip 且处于显示的情况下 生效
    void TipUpdate();


    ////////////////////////////////////
    // 窗口位置/大小

    /// @brief 基于父窗口移动到指定的相对坐标
    void Move(int x, int y);


    /// @brief 重新设置窗口大小
    void Resize(int w, int h);

    ////////////////////////////////////
    // 刷新

    /// @brief 标记窗口需要刷新
    /// @note 仅标记, 由框架决定合适的刷新时机
    void Update();


    /// @brief 标记窗口需要更新 基于画布的坐标(屏幕/顶层窗口)
    /// @note 仅标记, 由框架决定合适的更新时机
    void UpdateCanvasRect();


    ////////////////////////////////////
    // 绑定外部回调函数


    /// @brief 绑定响应函数
    int BindCommand(klb_wnd_on_command_cb on_command, void* p_obj);
    int BindCommand(klb_cwnd_on_command_cb on_command, void* p_obj);

    ////////////////////////////////////
    // 调用函数


    /// @brief 调用on_control函数
    int CallControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


    /// @brief 调用on_command函数
    int CallCommand(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


    /// @brief 1.调用on_control函数; 2.调用on_command函数
    int CallControlAndCommand(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


    ////////////////////////////////////
    // 控件建议宽/高

    /// @brief 获取建议宽
    int SuggestW();


    /// @brief 获取建议高
    int SuggestH();


    ////////////////////////////////////
    // 窗口关系 

    int PushChild(CWnd* p_child);


    ////////////////////////////////////
    // 设置 / 获取

    /// @brief 参数设置 / 获取
    int Set(const klb_map_t* p_map);
    klb_map_t* Get(const klb_map_t* p_map);

    ////////////////////////////////////
    // 时间

    /// @brief 获取当前UI滴答数
    int64_t GetTickCount();

    ////////////////////////////////////
    // 绘图

    // 获取画布区域
    klb_rect_t* GetCanvasRect();

    // 获取父窗口区域
    klb_rect_t* GetParentRect();

    /// @brief 基础绘图接口
    int SetDrawColor(uint32_t color);
    uint32_t GetDrawColor();
    int SetFontHeight(int h);
    int GetFontHeight();

    int DrawClear(uint32_t* p_color);
    int DrawPoint(int x, int y, uint32_t* p_color);
    int DrawPoints(const klb_point_t* p_points, int count, uint32_t* p_color);
    int DrawLine(int x1, int y1, int x2, int y2, uint32_t* p_color);
    int DrawLines(const klb_point_t* p_points, int count, uint32_t* p_color);
    int DrawRect(const klb_rect_t* p_rect, uint32_t* p_color);
    int DrawRects(const klb_rect_t* p_rects, int count, uint32_t* p_color);
    int DrawFillRect(const klb_rect_t* p_rect, uint32_t* p_color);
    int DrawFillRects(const klb_rect_t* p_rects, int count, uint32_t* p_color);
    int DrawText(const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t* p_color, int* p_font_h);
    int DrawText(const klb_rect_t* p_rect, const std::string& utf8, uint32_t* p_color, int* p_font_h);
    int DrawText(const klb_rect_t* p_rect, const klb::CString& str, uint32_t* p_color, int* p_font_h);
    int TextSize(const char* p_utf8, int utf8_len, int* p_font_h, int* p_out_w, int* p_out_h);
    int TextSize(const std::string& utf8, int* p_font_h, int* p_out_w, int* p_out_h);
    int TextSize(const klb::CString& str, int* p_font_h, int* p_out_w, int* p_out_h);
    int DrawImage(const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect);
    int DrawImage(const klb_rect_t* p_dst_rect, const std::string& path, const klb_rect_t* p_src_rect);
    int DrawImage(const klb_rect_t* p_dst_rect, const klb::CString& path, const klb_rect_t* p_src_rect);
    int ImageSize(const char* p_path, int* p_out_w, int* p_out_h);
    int ImageSize(const std::string& path, int* p_out_w, int* p_out_h);
    int ImageSize(const klb::CString& path, int* p_out_w, int* p_out_h);

    int DrawClear2(uint32_t color);
    int DrawPoint2(int x, int y, uint32_t color);
    int DrawPoints2(const klb_point_t* p_points, int count, uint32_t color);
    int DrawLine2(int x1, int y1, int x2, int y2, uint32_t color);
    int DrawLines2(const klb_point_t* p_points, int count, uint32_t color);
    int DrawRect2(const klb_rect_t* p_rect, uint32_t color);
    int DrawRects2(const klb_rect_t* p_rects, int count, uint32_t color);
    int DrawFillRect2(const klb_rect_t* p_rect, uint32_t color);
    int DrawFillRects2(const klb_rect_t* p_rects, int count, uint32_t color);
    int TextSize2(const char* p_utf8, int utf8_len, int font_h, int* p_out_w, int* p_out_h);
    int TextSize2(const std::string& utf8, int font_h, int* p_out_w, int* p_out_h);
    int TextSize2(const klb::CString& str, int font_h, int* p_out_w, int* p_out_h);
    int DrawText2(const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h);
    int DrawText2(const klb_rect_t* p_rect, const std::string& utf8, uint32_t color, int font_h);
    int DrawText2(const klb_rect_t* p_rect, const klb::CString& str, uint32_t color, int font_h);

    /// @brief 可扩展绘图接口
    int DrawOpt1(int opt, const void* ptr1);
    int DrawOpt2(int opt, const void* ptr1, const void* ptr2);
    int DrawOpt3(int opt, const void* ptr1, const void* ptr2, const void* ptr3);
    int DrawOpt4(int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4);
    int DrawOpt5(int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5);
    int DrawOpt6(int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6);
    int DrawOpt7(int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7);
    int DrawOpt8(int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7, const void* ptr8);


public:
    //////////////////////////////////////////////////////////////////////////
    // 

    /// @brief (继承重写)销毁内存
    virtual void OnDelete();


    /// @brief (继承重写)窗口绘制函数
    /// @return int
    virtual int OnPaint();


    /// @brief (继承重写)消息控制函数
    /// @param [in] msg         消息命令
    /// @param [in] *p_p1       点1
    /// @param [in] *p_p2       点2
    /// @return int 0
    virtual int OnControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


    /// @brief (继承重写)消息响应函数
    /// @param [in] msg         消息命令
    /// @param [in] *p_p1       点1
    /// @param [in] *p_p2       点2
    /// @return int
    virtual int OnCommand(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


public:
    
    /// @brief 向控件设置数据: 样式\显示\状态等等
    /// @param [in] *p_map      map数据集合
    /// @return int 0.成功; 非0.失败(错误码)
    /// @note map 具体数据格式由控件定义
    int OnSet(const klb_map_t* p_map);


    /// @brief 向控件获取数据: 样式\显示\状态等等
    /// @param [in] *p_map      map数据集合
    /// @return klb_map_t* map数据集合
    /// @note map 具体数据格式由控件定义
    klb_map_t* OnGet(const klb_map_t* p_map);

private:
    // 绑定响应函数(中转函数)
    static int on_command_klb_cwnd(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


public:
    //////////////////////////////////////////////////////////////////////////
    // CSS 相关
    bool CheckCssBool(klb::CMap* p_map, int start, bool* p_out_bool);
    bool CheckCssInteger(klb::CMap* p_map, int start, int* p_out_int);
    bool CheckCssInteger(klb::CMap* p_map, int start, int64_t* p_out_int);
    bool CheckCssString(klb::CMap* p_map, int start, klb::CString& out_str);
    bool CheckCssColor(klb::CMap* p_map, int start, uint32_t* p_out_color);

private:
    //////////////////////////////////////////////////////////////////////////
    // CSS 方法
    //   CSS方法 使用的是扩展含义; 即除了标准边距/颜色/字体等, 还包含其他调用者需要和控件交换接口
    //   使用标准C, 一直没有想到更好的办法处理标准的CSS方法, 需要为每类控件定制编写
    //   CPP这里 可以通过"继承"特性, 处理通用的CSS方法

    // 外边距 margin
    static void OnCssMargin(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssMarginTop(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssMarginRight(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssMarginBottom(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssMarginLeft(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

    // 内边距 padding
    static void OnCssPadding(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssPaddingTop(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssPaddingRight(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssPaddingBottom(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssPaddingLeft(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

    // 文本颜色 color
    static void OnCssColor(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssColorFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssColorDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

    // 文本对齐 text-align
    static void OnCssTextAlign(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssTextAlignFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssTextAlignDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

    // 字体大小 font-size
    static void OnCssFontSize(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssFontSizeFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssFontSizeDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

    // 背景色 background-color
    static void OnCssBackgroundColor(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssBackgroundColorFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssBackgroundColorDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

    // 背景图片 background-image
    static void OnCssBackgroundImage(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssBackgroundImageFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssBackgroundImageDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

    // 边框的宽度 border-width
    static void OnCssBorderWidth(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssBorderWidthFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssBorderWidthDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

    // 边框的颜色 border-color
    static void OnCssBorderColor(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssBorderColorFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssBorderColorDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);


private:
    /// @brief 获取CSS处理函数表
    klb_map_t* GetCssFunctionMap();

public:
    /// @brief 绑定响应函数
    /// @return void
    void BindCssFunction(const std::string& str, klb_cwnd_css_cb cb_func);

    /// @brief (继承重写)初始化CSS方法
    virtual bool InitCssFunctionMap(const std::string& type);


private:
    klb::CString            m_type;             ///< 当前控件类型 名称
    int                     m_tid;              ///< 类型ID
    CCss*                   m_css;              ///< CSS值

    klb_cwnd_on_command_cb  m_cb_on_command;    ///< 绑定响应函数(中转函数)

private:
    CGui*                   m_gui;              ///< gui 指针
    klb_wnd_t*              m_wnd;              ///< wnd 窗口指针

    klb_map_t*              m_css_func_map;     ///< CSS 属性函数表
}; // class CWnd

} // namespace klbui 

#endif // __KLBUI_CWND_HPP__
//end
