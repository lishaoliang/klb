// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rect.h
/// @brief   区域等定义
/// @version 0.1
/// @history 修改历史
///  \n [2020] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_RECT_H__
#define __KLB_RECT_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_point_t
/// @brief  点
typedef struct klb_point_t_
{
    int     x;
    int     y;
}klb_point_t;


/// @struct klb_size_t
/// @brief  size尺寸
typedef struct klb_size_t_
{
    int     w;
    int     h;
}klb_size_t;


/// @struct klb_rect_t
/// @brief  区域
typedef struct klb_rect_t_
{
    int     x;
    int     y;
    int     w;
    int     h;
}klb_rect_t;


/// @brief 点(x,y)是否在区域内部
/// @param [in] *p_rect     区域
/// @param [in] x           x坐标
/// @param [in] y           y坐标
/// @return bool true.在区域内; false.不在区域内部
/// @note 半开区间 [x, x+w) x [y, y+h); 右/下边界不含
KLB_API bool klb_pt_in_rect(const klb_rect_t* p_rect, int x, int y);


/// @brief 初始化
/// @param [out] *p_dst     目标区域
/// @param [in] x           x坐标
/// @param [in] y           y坐标
/// @param [in] w           宽
/// @param [in] h           高
/// @return 无
KLB_API void klb_rect_init(klb_rect_t* p_dst, int x, int y, int w, int h);


/// @brief 求取2个矩形的并集
/// @param [out] *p_dst     输出并集区域
/// @param [in] *p_rect1    区域1
/// @param [in] *p_rect2    区域2
/// @return 无
/// @note 允许 *p_dst 与输入矩形重叠
KLB_API void klb_rect_union(klb_rect_t* p_dst, const klb_rect_t* p_rect1, const klb_rect_t* p_rect2);


/// @brief 求取2个矩形的交集
/// @param [out] *p_dst     输出交集区域
/// @param [in] *p_rect1    区域1
/// @param [in] *p_rect2    区域2
/// @return bool true.有交集; false.无交集
/// @note *p_dst 可为 NULL, 仅判定是否相交; 允许与输入矩形重叠
KLB_API bool klb_rect_intersect(klb_rect_t* p_dst, const klb_rect_t* p_rect1, const klb_rect_t* p_rect2);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_RECT_H__

// end
