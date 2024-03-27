///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2024, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CGui.hpp
/// @brief   klb_gui_t 的 cpp 封装
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CGUI_HPP__
#define __KLB_CGUI_HPP__

#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klua/CKluaEnv.hpp"
#include "klbgui/CWnd.hpp"
#include "klbgui/CCss.hpp"
#include <string>

namespace klbui {


#define KLB_CGUI_PTR(PTR_)       ((CGui*)(klb_gui_get_cppgui((PTR_))))


KLB_EXTERN class KLB_API_CPP CGui
{
public:
    //////////////////////////////////////////////////////////////////////////
    // 构造/析构
    CGui();
    CGui(klb_gui_t* p_gui, bool share); // 共享 *p_gui 指针, *p_gui 的生命周期由创建者维护
    CGui(klb_gui_t* p_gui);             // 托管 *p_gui 指针, *p_gui 由 CGui释放
    ~CGui();


public:
    //////////////////////////////////////////////////////////////////////////
    // cpp 额外函数

    /// @brief 获取gui指针
    klb_gui_t* GetGui();


public:
    //////////////////////////////////////////////////////////////////////////
    // 定义于 "klbgui/klb_gui.h" 的导出函数

    int AttachKluaEnv(klua_env_t* p_env);
    int AttachKluaEnv(klb::CKluaEnv* p_env);

    klua_env_t* GetKluaEnv();

    int LoopOnce(int64_t tc);

    void AttachCanvas(klb_canvas_t* p_canvas);
    klb_canvas_t* GetCanvas();

    /// @brief 向GUI中放入消息
    /// @param [in] msg             消息/事件: eg. KLBUI_click
    /// @return 无
    void PushMsg(int msg, int x1, int y1, int x2, int y2, int lparam, int wparam);

    /// @brief 注册窗口类型
    /// @param [in] *p_type         窗口类型名
    /// @param [in] cb_create       创建窗口
    /// @return int 0.成功; 非0.失败(错误码)
    ///  \n 标准控件类型名命名规则为 "k*", eg."kbutton", "kdialog"
    int Register(const char* p_type, klb_wnd_create_cb cb_create);
    int Register(const std::string& type, klb_wnd_create_cb cb_create);

    /// @brief 获取注册的窗口类型的 创建函数
    /// @param [in] *p_type         窗口类型名
    /// @return klb_wnd_create_cb 创建函数; 或 NULL
    klb_wnd_create_cb GetCreater(const char* p_type);
    klb_wnd_create_cb GetCreater(const std::string& type);

    /// @brief 按类型 创建一个窗口; 注意.仅创建, 并未加入到窗口树中
    /// @param [in] *p_type         窗口类型名
    /// @return klb_wnd_t* 窗口指针; 或 NULL
    klb_wnd_t* CreateWnd(const char* p_type, int x, int y, int w, int h);
    klb_wnd_t* CreateWnd(const std::string& type, int x, int y, int w, int h);

    /// @brief 加载图片资源
    int LoadImage(const char* p_key, const char* p_img_path);
    int LoadImage(const std::string& key, const std::string& img_path);

    /// @brief 获取图片资源大小
    int ImageSize(const char* p_key, int* p_out_w, int* p_out_h);
    int ImageSize(const std::string& key, int* p_out_w, int* p_out_h);


    /// @brief 添加窗口
    /// @param [in] *p_type         窗口类型名
    /// @param [in] *p_path_name    窗口路径(类unix文件路径): eg."/home/btn1"
    /// @param [in] x               相对父窗口X坐标
    /// @param [in] y               相对父窗口Y坐标
    /// @param [in] w               宽
    /// @param [in] h               高
    /// @return int 0.成功; 非0.失败(错误码)
    /// @note 前父窗口必须存在; eg. "/home/btn1" 则需要 "/home" 必须存在, 才能添加
    int Append(const char* p_type, const char* p_path_name, int x, int y, int w, int h, uint32_t style);
    int Append(const std::string& type, const std::string& path_name, int x, int y, int w, int h, uint32_t style);


    /// @brief 移除窗口
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
    /// @return int 0.成功; 非0.失败(错误码)
    int Remove(const char* p_path_name);
    int Remove(const std::string& path_name);

    /// @brief 清理所有窗口
    /// @param [in] *p_gui          GUI对象
    /// @return int 0.成功; 非0.失败(错误码)
    /// @note 当修改分辨率等需要将所有UI清理掉时
    ///     清理内容:
    ///       1. klb_gui_append 添加的所有窗口
    ///       2. klb_gui_load_image 加载的所有图片资源
    ///       3. klb_gui_push_shwnd 添加的所有共享窗口
    ///     不清理内容
    ///       a. klb_gui_register 注册的控件类型
    ///       b. klb_gui_register_extension 注册的扩展
    ///     !!!注意: 不能在GUI的流程中, 调用此函数, 否则内部数据异常
    int Clear();

    /// @brief 异步等待合适时机, 清理所有窗口
    /// @param [in] cb_clear        清理完成的回调函数
    /// @param [in] *ptr            清理完成的回调函数
    /// @return int 0.成功; 非0.失败(错误码)
    /// @note 当修改分辨率等需要将所有UI清理掉时
    ///     清理内容:
    ///       1. klb_gui_append 添加的所有窗口
    ///       2. klb_gui_load_image 加载的所有图片资源
    ///       3. klb_gui_push_shwnd 添加的所有共享窗口
    ///     不清理内容
    ///       a. klb_gui_register 注册的控件类型
    ///       b. klb_gui_register_extension 注册的扩展
    ///     注意: 清理窗口会在GUI特定的合适时机清理， 清理完成之后再使用回调函数通知调用者
    int ClearAsync(klb_gui_clear_result_cb cb_clear, void* ptr);


    /// @brief 模态显示窗口
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
    /// @return int 0.成功; 非0.失败(错误码)
    int Model(const char* p_path_name);
    int Model(const std::string& path_name);

    /// @brief 模态显示窗口
    /// @param [in] *p_top          外部自定义窗口: 由外部管理生命周期
    /// @return int 0.成功; 非0.失败(错误码)
    int ModelWnd(klb_wnd_t* p_top);
    int ModelWnd(CWnd* p_top);


    /// @brief 关闭显示窗口
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
    /// @return int 0.成功; 非0.失败(错误码)
    int ModelEnd(bool all, const char* p_path_name);
    int ModelEnd(bool all, const std::string& path_name);


    /// @brief 弹出菜单/对话框等页面
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/menu1"
    /// @return int 0.成功; 非0.失败(错误码)
    int Popup(const char* p_path_name);
    int Popup(const std::string& path_name);

    /// @brief 弹出菜单/对话框等页面
    /// @param [in] *p_top          外部自定义窗口: 由外部管理生命周期
    /// @return int 0.成功; 非0.失败(错误码)
    int PopupWnd(klb_wnd_t* p_top);
    int PopupWnd(CWnd* p_top);

    /// @brief 关闭弹出的页面
    int PopupEnd(bool all);

    /// @brief 消息框: 弹出消息框
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/messagebox1"
    /// @return int 0.成功; 非0.失败(错误码)
    int Messagebox(const char* p_path_name);
    int Messagebox(const std::string& path_name);


    /// @brief 消息框: 弹出消息框
    /// @param [in] *p_top          外部自定义窗口: 由外部管理生命周期
    /// @return int 0.成功; 非0.失败(错误码)
    int MessageboxWnd(klb_wnd_t* p_top);
    int MessageboxWnd(CWnd* p_top);


    /// @brief 关闭消息框
    /// @return int 0.成功; 非0.失败(错误码)
    int MessageboxEnd();


    /// @brief 向控件(窗口)绑定事件响应函数
    /// @param [in] *p_path_name    窗口路径(类unix文件路径): eg."/home/btn1"
    /// @param [in] on_command      事件响应函数(单击,双击等)
    /// @param [in] *p_obj          用户附加数据
    /// @return int 0.成功; 非0.失败(错误码)
    int BindCommand(const char* p_path_name, klb_wnd_on_command_cb on_command, void* p_obj);
    int BindCommand(const std::string& path_name, klb_wnd_on_command_cb on_command, void* p_obj);


    /// @brief 外部触发控件(窗口)某个事件
    /// @param [in] *p_path_name    窗口路径(类unix文件路径): eg."/home/btn1"
    /// @param [in] msg             事件
    /// @return int 0.成功; -1.终止
    int CallControlAndCommand(const char* p_path_name, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);
    int CallControlAndCommand(const std::string& path_name, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


    /// @brief 向控件(窗口)设置数据: 样式\显示\状态等等
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
    /// @param [in] *p_map          map数据集合
    /// @return int 0.成功; 非0.失败(错误码)
    /// @note map 具体数据格式由控件定义
    int Set(const char* p_path_name, const klb_map_t* p_map);
    int Set(const std::string& path_name, const klb_map_t* p_map);


    /// @brief 向控件(窗口)获取数据: 样式\显示\状态等等
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
    /// @param [in] *p_map          map数据集合
    /// @return klb_map_t* map数据集合
    /// @note map 具体数据格式由控件定义
    klb_map_t* Get(const char* p_path_name, const klb_map_t* p_map);
    klb_map_t* Get(const std::string& path_name, const klb_map_t* p_map);


    /// @brief 设置控件(窗口)的显示状态
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
    /// @param [in] *p_map          map数据集合
    /// @return int 0.成功; 非0.失败(错误码)
    int Show(const char* p_path_name, bool show);
    int Show(const std::string& path_name, bool show);


    /// @brief 基于父窗口移动(相对坐标)
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
    /// @return int 0.成功; 非0.失败(错误码)
    int Move(const char* p_path_name, int x, int y);
    int Move(const std::string& path_name, int x, int y);


    /// @brief 重新设置控件大小
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
    /// @return int 0.成功; 非0.失败(错误码)
    int Resize(const char* p_path_name, int w, int h);
    int Resize(const std::string& path_name, int w, int h);


    /// @brief 获取基于画布的窗口区域
    int WndposInCanvas(const char* p_path_name, klb_rect_t* p_out_rect);
    int WndposInCanvas(const std::string& path_name, klb_rect_t* p_out_rect);


    /// @brief 获取基于父窗口的区域
    int WndposInParent(const char* p_path_name, klb_rect_t* p_out_rect);
    int WndposInParent(const std::string& path_name, klb_rect_t* p_out_rect);


    /// @brief 控件建议宽
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
    /// @return int 0.成功; 非0.失败(错误码)
    int SuggestW(const char* p_path_name, int* p_out_w);
    int SuggestW(const std::string& path_name, int* p_out_w);


    /// @brief 控件建议高
    /// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
    /// @return int 0.成功; 非0.失败(错误码)
    int SuggestH(const char* p_path_name, int* p_out_h);
    int SuggestH(const std::string& path_name, int* p_out_h);


    /// @brief 获取主画布(主屏幕)的分辨率
    /// @param [out] *p_out_w       输出窗口宽
    /// @param [out] *p_out_h       输出窗口高
    /// @return int 0.成功; 非0.失败(错误码)
    int GetWH(int* p_out_w, int* p_out_h);


    /// @brief 获取 当前聚焦窗口
    /// @return klb_wnd_t* 聚焦窗口指针
    /// @note 可能为 NULL
    klb_wnd_t* GetFocus();


    /// @brief 获取 当前聚焦窗口 的顶层窗口
    /// @return klb_wnd_t* 聚焦窗口的 顶层窗口指针
    /// @note 可能为 NULL
    klb_wnd_t* GetFocusTop();



    //////////////////////////////////////////////////////////////////////////
    // 定义于 "klbgui/klbui_css.h" 的导出函数
    // CSS相关 辅助函数


    /// @brief 检查参数是否为颜色
    bool CheckColor(const klb_map_t* p_map, int start, uint32_t* p_out_color);


    ////////////////////////////////////
    // 同类型控件, 私有CSS属性 函数处理表

    /// @brief 通过组件类型获取 css 属性函数处理 map
    /// @param [in] *p_type         组件类型名
    /// @return klb_map_t* 函数处理表
    ///  \n 标准控件类型名命名规则为 "k*", eg."kbutton"
    ///  \n 相同组件, 使用同一套函数处理表, 目的是为了 精简组件体积
    klb_map_t* CssMap(const char* p_type);
    klb_map_t* CssMap(const std::string& type);


    /// @brief 新建 css 属性函数处理 map
    /// @param [in] *p_type         组件类型名
    /// @return klb_map_t* 函数处理表
    klb_map_t* NewCssMap(const char* p_type);
    klb_map_t* NewCssMap(const std::string& type);


    ////////////////////////////////////
    // 同类型控件, 全局公共CSS属性 函数处理表
    // 原因: 1. 同一套UI中设计中, 80%以上同类型控件会采用一致的CSS属性设置
    //       2. CSS属性数据一般比窗口属性数据 大 很多倍, 若每个控件都单独持有CSS属性, 窗口数据量大(例如十万级以上), 会消耗非常多内存
    //       3. 本GUI框架主要针对 嵌入式/资源受限 环境, 在内存与CPU消耗上, 需要谨慎对待!
    // 方案: 在基于 节省内存, 不降低使用体验的情况下, 采用如下思路:
    //       A. 默认情况下的控件, 直接使用全局公共CSS属性
    //       B. 若控件的"set"(设置CSS)方法被调用, 则新建立私有CSS属性, 转为使用私有CSS属性
    //       C. 提供操作全局公共CSS属性的一些接口
    // 其他: 在节省内存方面, 还有待考量思路:
    //       1).每个控件尽可能精简CSS属性. 缺点: 没有标准处理方法, 需要逐个去对待
    //       2).使用CSS-MINI, 精简每个属性占用的字节数: eg. 将 klbuicss_padding_t.top 属性 由"int" 修改为 "int8_t"


    /// @brief 通过组件类型获取 全局公共CSS处理表
    /// @param [in] *p_type         组件类型名
    /// @return klb_map_t* 函数处理表
    klb_map_t* GlobalCssMap(const char* p_type);
    klb_map_t* GlobalCssMap(const std::string& type);


    /// @brief 新建 全局公共CSS处理表
    /// @param [in] *p_type         组件类型名
    /// @return klb_map_t* 函数处理表
    klb_map_t* NewGlobalcssMap(const char* p_type);
    klb_map_t* NewGlobalcssMap(const std::string& type);


    /// @brief 设置 全局公共CSS属性 指针
    /// @param [in] *p_type         组件类型名
    /// @param [in] *p_css          全局公共CSS属性; 设置后, 由GUI框架托管
    /// @param [in] cb_destroy      *p_css的销毁函数
    /// @return 无
    /// @note 若重复设置, 以最后设置的为准;
    ///       注意: 不要轻易覆盖, 可能造成 缓存的指针失效!
    void GlobalcssSetPtr(const char* p_type, void* p_css, klb_gui_globalcss_destroy_cb cb_destroy);
    void GlobalcssSetPtr(const std::string& type, void* p_css, klb_gui_globalcss_destroy_cb cb_destroy);


    /// @brief 获取 全局公共CSS属性 指针
    /// @param [in] *p_type         组件类型名
    /// @return void* 全局公共CSS属性 指针
    /// @note 控件自定义其CSS属性 结构体
    void* GlobalcssGetPtr(const char* p_type);
    void* GlobalcssGetPtr(const std::string& type);


    /// @brief 设置 全局公共CSS属性
    /// @note 参考 klb_wnd_set
    int GlobalcssSet(const char* p_type, const klb_map_t* p_map);
    int GlobalcssSet(const std::string& type, const klb_map_t* p_map);


    /// @brief 获取 全局公共CSS属性
    /// @note 参考 klb_wnd_get
    klb_map_t* GlobalcssGet(const char* p_type, const klb_map_t* p_map);
    klb_map_t* GlobalcssGet(const std::string& type, const klb_map_t* p_map);


    //////////////////////////////////////////////////////////////////////////
    // 定义于 "klbgui/klbui_datetime.h" 的导出函数

    /// @brief 设置/获取日期格式: KLBUI_DATE_YMD1
    void SetDateFmt(int fmt);
    int GetDateFmt();

    /// @brief 设置/获取时间格式: KLBUI_TIME_24H
    void SetTimeFmt(int fmt);
    int GetTimeFmt();


    //////////////////////////////////////////////////////////////////////////
    // 定义于 "klbgui/klbui_default.h" 的导出函数

    /// @brief 获取标准控件的默认值指针
    const klbui_default_t* GetStdDefault();

    /// @brief 设置CSS默认值(全局值)
    int DefaultCssSet(const klb_map_t* p_map);

    /// @brief 获取CSS默认值(全局值)
    klb_map_t* DefaultCssGet(const klb_map_t* p_map);


    //////////////////////////////////////////////////////////////////////////
    // 定义于 "klbgui/klbui_extension.h" 的导出函数


    /// @brief 注册gui扩展
    /// @param [in] *p_name             名称
    /// @param [in] *p_extension        扩展的接口函数
    /// @return int 0
    int RegisterExtension(const char* p_name, const klb_gui_extension_t* p_extension);
    int RegisterExtension(const std::string& name, const klb_gui_extension_t* p_extension);


    /// @brief 获取gui扩展
    /// @param [in] *p_name             名称
    /// @return void* 扩展的指针
    void* GetExtension(const char* p_name);
    void* GetExtension(const std::string& name);


    //////////////////////////////////////////////////////////////////////////
    // 定义于 "klbgui/klbui_shwnd.h" 的导出函数


    /// @brief 放入共享使用界面
    /// @param [in] *p_path         路径(关键字)
    /// @return int 0.成功; 非0.失败(错误码)
    /// @note 注意:
    ///       1. push之后， 由ui框架管理生命周期
    ///       2. 共享界面是唯一的
    ///       3. path冲突时, push失败
    ///       4. "/klbui"路径, 由ui框架保留使用
    int PushShWnd(const char* p_path, klb_wnd_t* p_top_wnd);
    int PushShWnd(const std::string& path, klb_wnd_t* p_top_wnd);


    /// @brief 获取共享使用界面
    /// @param [in] *p_path         路径(关键字)
    /// @return int 0.成功; 非0.失败(错误码)
    klb_wnd_t* GetShWnd(const char* p_path);
    klb_wnd_t* GetShWnd(const std::string& path);


    /// @brief 设置共享窗口的CSS值
    int ShWndCssSet(const char* p_path, const klb_map_t* p_map);
    int ShWndCssSet(const std::string& path, const klb_map_t* p_map);


    /// @brief 获取共享窗口的CSS值
    klb_map_t* ShWndCssGet(const char* p_path, const klb_map_t* p_map);
    klb_map_t* ShWndCssGet(const std::string& path, const klb_map_t* p_map);


    //////////////////////////////////////////////////////////////////////////
    // 定义于 "klbgui/klbui_timer.h" 的导出函数


private:
    bool            m_is_share_gui;     ///< 是否共享控制
    klb_gui_t*      m_gui;              ///< gui指针
};

} // namespace klbui 

#endif // __KLB_CGUI_HPP__
