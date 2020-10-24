///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rect.h
/// @brief   区域等定义
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
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
KLB_API bool klb_pt_in_rect(const klb_rect_t* p_rect, int x, int y);


/// @brief 求取2个矩形的并集
/// @param [out] *p_dst     输出并集区域
/// @param [in] *p_rect1    区域1
/// @param [in] *p_rect2    区域2
/// @return 无
KLB_API void klb_rect_union(klb_rect_t* p_dst, const klb_rect_t* p_rect1, const klb_rect_t* p_rect2);


/// @brief 求取2个矩形的交集
/// @param [out] *p_dst     输出交集区域
/// @param [in] *p_rect1    区域1
/// @param [in] *p_rect2    区域2
/// @return bool true.有交集; false.无交集
KLB_API bool klb_rect_intersect(klb_rect_t* p_dst, const klb_rect_t* p_rect1, const klb_rect_t* p_rect2);


#ifdef __cplusplus
}
#endif

#endif // __KLB_RECT_H__
//end
