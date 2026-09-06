// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_wnd_flex.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   窗口自动布局声明; 详见 klbuiex_flex_t 扩展
///          自动布局设计:
///          1. L1 -> L2 -> L3 多层级布局; 
///          2. L1 层级: klb_wnd_pos_t.layout_algo = klb_wnd_flex_algo_e -> 布局引擎
///          3. L2 层级: tag( PX/FLOW )
///          4. L3 层级: payload( px/flow )
///
///          关联信息:
///          1. klb_wnd_state_t.style : KLB_WND_STYLE_FLEX -> 标记窗口启用 flex, 核心标记; 否则使用原始坐标布局
///          2. klb_wnd_pos_t.[rect_in_flex, layout_algo, z_index] -> flex 布局信息, 布局算法依据此信息进行计算
///          3. klb_wnd_state_t.status : KLB_WND_STATUS_FLEX_DIRTY -> 标记窗口需要重新计算flex布局
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WND_FLEX_H__
#define __KLB_WND_FLEX_H__


#include "klb_type.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @brief klb_wnd_pos_t.layout_algo 属性值类型; 决定 flex 的主算法
/// @note flex 算法依据:
///      1. klb_wnd_pos_t.layout_algo = klb_wnd_flex_algo_e -> 主算法
///      2. klb_wnd_pos_t.rect_in_flex.[x, y, w, h] = [px, flow] -> 子算法
///      3. klb_wnd_pos_t.z_index = int16 -> 堆叠顺序; 默认0;
typedef enum klb_wnd_flex_algo_e_
{
    KLB_WND_FLEX_ALGO_NONE          = 0,    ///< 无 flex 算法
    KLB_WND_FLEX_ALGO_FLOW_COLUMN   = 1,    ///< BoxFlow 列主轴 (当前 klbuiex_flex 默认)
    KLB_WND_FLEX_ALGO_FLOW_ROW      = 2,    ///< BoxFlow 行主轴

    // P2 预留
    // KLB_WND_FLEX_ALGO_FORM       = 3,
    // KLB_WND_FLEX_ALGO_GRID       = 4,    // 若永不实现可不占号
}klb_wnd_flex_algo_e;


/// @def  klb wnd flex
/// @brief 32位 int 中: 高8位为klb_wnd_flex_tag_e, 低24位为坐标值等
#define KLB_WND_FLEX_TAG_SHIFT           24
#define KLB_WND_FLEX_PAYLOAD_MASK        0x00FFFFFF ///< 低24位为坐标值等 (无符号)    

/// @enum  klb_wnd_flex_tag_e
/// @brief flex 自动布局声明: 详见 klbuiex_flex_t 扩展
/// @note 若窗口样式包含 KLB_WND_STYLE_FLEX, 则 klb_wnd_pos_t.rect_in_flex 有效
///       klb_wnd_pos_t.rect_in_flex.[x, y, w, h] 分别表示需要自动布局信息
typedef enum klb_wnd_flex_tag_e_
{
    KLB_WND_FLEX_TAG_NONE               = 0,        ///< 无 flex 声明
    KLB_WND_FLEX_TAG_PX                 = 1,        ///< 使用低24位作为坐标值(带符号)
    KLB_WND_FLEX_TAG_FLOW               = 2,        ///< 参与流式/弹性策略
}klb_wnd_flex_tag_e;

/// @enum  klb_wnd_flex_flow_e
/// @brief flex 自动布局声明: 详见 klbuiex_flex_t 扩展; 低24位为坐标值等
/// @note 若窗口样式包含 KLB_WND_STYLE_FLEX, 则 klb_wnd_pos_t.rect_in_flex 有效
///       klb_wnd_pos_t.rect_in_flex.[x, y, w, h].tag 为 KLB_WND_FLEX_TAG_FLOW
///       klb_wnd_pos_t.rect_in_flex.[x, y, w, h] 分别表示需要自动布局信息
typedef enum klb_wnd_flex_flow_e_
{
    KLB_WND_FLEX_FLOW_NONE              = 0,        ///< 无 flex 声明
    KLB_WND_FLEX_FLOW_AUTO              = 1,        ///< 自动布局; eg. [x,y]固定PX，[w,h]主轴均分剩余空间
    KLB_WND_FLEX_FLOW_GROW              = 2,        ///< 主轴均分剩余空间
    KLB_WND_FLEX_FLOW_FILL              = 3,        ///< 填充剩余空间
    KLB_WND_FLEX_FLOW_STRETCH           = 4,        ///< 交叉轴拉满 content
    KLB_WND_FLEX_FLOW_INHERIT           = 5,        ///< 沿用父默认
}klb_wnd_flex_flow_e;


#ifdef __cplusplus
}
#endif

#endif // __KLB_WND_FLEX_H__

// end
