///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_h26x.h
/// @brief   颜色相关
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_H26X_H__
#define __KLB_H26X_H__


#include "klb_type.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 扫描 H264/H265数据, 得到第一个nal单元
/// @param [in]  *p_h26x      媒体数据地址
/// @param [in]  h26x_len     数据长度
/// @param [out] *p_nal_start nal单元的起始位置, 算上起始标记[0x0 0x0 0x0 0x1]
/// @param [out] *p_nal_len   nal单元的长度
/// @param [out] *p_nal_h_len nal开头长度(可为NULL): 3.[0x0 0x0 0x1]; 4.[0x0 0x0 0x0 0x1]
/// @param [out] *p_is_end    是否为末尾: XT_TRUE.是末尾; XT_FALSE.不是末尾
/// @return int 返回 0.成功; 非0 失败,没有nal单元
/// @note 注意如果为数据块最后一个nal单元, 返回的长度为: *p_nal_len = h26x_len - *p_nal_start
KLB_API int klb_h26x_scan_nalu(char* p_h26x, int h26x_len, int* p_nal_start, int* p_nal_len, int* p_nal_h_len, int8_t* p_is_end);


/// @enum  klb_h264_nalu_type_e
/// @brief h264的帧类型
typedef enum klb_h264_nalu_type_e_
{
    KLB_H264_BSLICE   = 0,   ///< B SLICE
    KLB_H264_PSLICE   = 1,   ///< P SLICE
    KLB_H264_ISLICE   = 2,   ///< I SLICE
    KLB_H264_IDRSLICE = 5,   ///< 关键帧
    KLB_H264_SEI      = 6,   ///< SEI
    KLB_H264_SPS      = 7,   ///< SPS
    KLB_H264_PPS      = 8,   ///< PPS
}klb_h264_nalu_type_e;


/// @brief 获取h264的nal类型
/// @param [in] v nal值
/// @return uint8_t klb_h264_nalu_type_e
KLB_API uint8_t klb_h264_nalu_type(uint8_t v);


typedef enum klb_h265_nalu_type_e_
{
    KLB_H265_BSLICE   = 0,    ///< B SLICE
    KLB_H265_PSLICE   = 1,    ///< P SLICE
    KLB_H265_ISLICE   = 2,    ///< I SLICE
    KLB_H265_IDRSLICE = 19,   ///< 关键帧
    KLB_H265_VPS      = 32,   ///< VPS
    KLB_H265_SPS      = 33,   ///< SPS
    KLB_H265_PPS      = 34,   ///< PPS
    KLB_H265_SEI      = 39,   ///< SEI
}klb_h265_nalu_type_e;


/// @brief 获取h265的nalu类型
/// @param [in] v nal值
/// @return uint8_t klb_h265_nalu_type_e
KLB_API uint8_t klb_h265_nalu_type(uint8_t v);


#ifdef __cplusplus
}
#endif


#endif // __KLB_H26X_H__
//end
