// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_flex.h
/// @brief   "flex"扩展: 自动布局 (BoxFlow) 引擎挂接
///         GUI flex 自动布局思路:
///         1. klb_gui_append 添加窗口时, 标记 KLB_WND_STYLE_FLEX, 表示 append 的 x,y,w,h 为 flex 声明
///         2. KLB_WND_STATUS_FLEX_DIRTY 状态 表示需要重新 flex 布局: 详见 klb_wnd_status_e
///         3. klb_wnd_pos_t.rect_in_flex 记录 flex 声明信息
///         4. klb_gui_append(KLB_WND_STYLE_FLEX) 时, 设置 KLB_WND_STATUS_FLEX_DIRTY 状态, 将父亲窗口记录在本模块
///         5. 待到合适时节(待定), 执行 flex 布局, 取消 KLB_WND_STATUS_FLEX_DIRTY, 表示已经布局完成
///
///         flex 自动布局, 处理的事务:
///         1. 在父窗口里面, 根据 layout_algo, 计算出每个子窗口的 rect_in_parent
///         2. 递归处理子窗口
///         3. 每个处理的窗口 取消 KLB_WND_STATUS_FLEX_DIRTY 状态, 表示已经布局完成
///         4. 仅处理 rect_in_flex -> rect_in_parent 流程; 从 rect_in_parent -> rect_in_canvas 由其他流程完成
/// @version 0.1
/// @history 修改历史
///  \n [2026] 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_FLEX_H__
#define __KLBUIEX_FLEX_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbuiex_flex_t
/// @brief  flex 自动布局扩展
typedef struct klbuiex_flex_t_ klbuiex_flex_t;


int klbuiex_register_flex(klb_gui_t* p_gui);
klbuiex_flex_t* klbuiex_get_flex(klb_gui_t* p_gui);


// 放入需要自动布局的窗口列表
int klbuiex_flex_push(klbuiex_flex_t* p_ex, klb_wnd_t* p_wnd);

// 从需要自动布局的窗口列表中移除
int klbuiex_flex_remove(klbuiex_flex_t* p_ex, klb_wnd_t* p_wnd);

// 清空需要自动布局的窗口列表
int klbuiex_flex_clear(klbuiex_flex_t* p_ex);

// 布局流程: 处理 rect_in_flex -> rect_in_parent 流程
int klbuiex_flex_do_parent_rect(klbuiex_flex_t* p_ex);

//////////////////////////////////////////////////////////////////////////
// 其他

/// @brief 从 rect_in_flex.[x,y,w,h] 编码值解析 flex tag
/// @param [in] flex_val    klb_wnd_pos_t.rect_in_flex 分量 (高8位 tag, 低24位 payload)
/// @return klb_wnd_flex_tag_e
/// @note 编码规则见 klb_wnd.h klb_wnd_flex_tag_e 节
klb_wnd_flex_tag_e klbuiex_flex_get_tag(int flex_val);


/// @brief 从 rect_in_flex.[x,y,w,h] 编码值解析 payload (低24位, 无符号)
/// @param [in] flex_val    klb_wnd_pos_t.rect_in_flex 分量
/// @return unsigned int 低24位原始值
unsigned int klbuiex_flex_get_payload_u(int flex_val);


/// @brief 解析 flex 编码值为带符号像素 (tag 应为 KLB_WND_FLEX_TAG_PX)
/// @param [in] flex_val    klb_wnd_pos_t.rect_in_flex 分量
/// @return int 带符号坐标/尺寸 (px)
/// @note 低24位按补码符号扩展; 与 tag 是否匹配由调用方保证
int klbuiex_flex_get_payload_px(int flex_val);


/// @brief 解析 flex 编码值为 flow 策略 (tag 应为 KLB_WND_FLEX_TAG_FLOW)
/// @param [in] flex_val    klb_wnd_pos_t.rect_in_flex 分量
/// @return klb_wnd_flex_flow_e
/// @note payload 为 klb_wnd_flex_flow_e 枚举值; 与 tag 是否匹配由调用方保证
klb_wnd_flex_flow_e klbuiex_flex_get_payload_flow(int flex_val);


/// @brief 判定 append 是否 flex 声明, 并输出创建窗口用的像素坐标
/// @param [in] x,y,w,h         append 坐标 (flex 时为编码值)
/// @param [in] style           窗口样式 (含 KLB_WND_STYLE_FLEX 时为 flex)
/// @param [out] p_out_x,y,w,h  创建窗口用的像素坐标
/// @return bool true.flex 声明; false.普通像素
bool klbuiex_flex_is_flex(int x, int y, int w, int h, uint32_t style, int* p_out_x, int* p_out_y, int* p_out_w, int* p_out_h);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUIEX_FLEX_H__

// end
