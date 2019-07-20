///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/07/06
//
/// @file    klb_mnp_media.h
/// @brief   media net protocol, media定义
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/07/06 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNP_MEDIA_H__
#define __KLB_MNP_MEDIA_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#pragma pack(4)

/// @struct klb_mnp_mh_t
/// @brief  media net protocol, media head
///  \n {[klb_mnp_mh_t + N][data...]}
typedef struct klb_mnp_mh_t_
{
    uint32_t    size;       ///< 媒体数据大小(包含本结构体)
    uint16_t    chnn;       ///< 通道(channel)
    uint16_t    sidx;       ///< 流序号(stream index): klb_mnp_sidx_e
    int64_t     time;       ///< 时间戳(毫秒)
    // - 4 + 2 + 2 + 8 = 16 Byte

    uint16_t    dtype;      ///< 数据类型(data type): klb_mnp_dtype_e
    union
    {
        uint8_t vtype;      ///< 视频(video type): klb_mnp_vtype_e
    };
    uint8_t     st_size;    ///< 本结构体大小: [sizeof(klb_mnp_mh_t), 256]
    // - 16 + 4 = 20 Byte

    uint32_t    uid;        ///< 用户自定义ID(user defined id)
    // - 20 + 4 = 24 Byte
    // extra: 24 + N Byte
}klb_mnp_mh_t;

#pragma pack()


/// @enum   klb_mnp_dtype_e
/// @brief  视频帧类型
typedef enum klb_mnp_vtype_e_
{
    KLB_NMP_VTYPE_I    = 0x00,      ///< I帧
    KLB_NMP_VTYPE_P    = 0x01,      ///< P帧
    KLB_NMP_VTYPE_B    = 0x02,      ///< B帧
    KLB_NMP_VTYPE_MAX  = 0xFF       ///< MAX
}klb_mnp_vtype_e;


/// @enum   klb_mnp_dtype_e
/// @brief  媒体数据类型
typedef enum klb_mnp_dtype_e_
{
    KLB_NMP_DTYPE_NULL = 0x0000,    ///< NULL

    KLB_NMP_VIDEO_B    = 0x0000,    ///< 视频 Begin
    KLB_NMP_MJPEG      = 0x0001,    ///< MJPEG
    KLB_NMP_H264       = 0x0002,    ///< H264
    KLB_NMP_H265       = 0x0003,    ///< H265
    KLB_NMP_VIDEO_E    = 0x03FF,    ///< 视频 End

    KLB_NMP_AUDIO_B    = 0x0400,    ///< 音频 Begin
    KLB_NMP_G711A      = 0x0401,    ///< G711A
    KLB_NMP_G711U      = 0x0402,    ///< G711U
    KLB_NMP_AUDIO_E    = 0x07FF,    ///< 音频 End

    KLB_NMP_PIC_B      = 0x0800,    ///< 图片 Begin
    KLB_NMP_JPEG       = 0x0801,    ///< JPEG
    KLB_NMP_PIC_E      = 0x0BFF,    ///< 图片 End

    KLB_NMP_IMG_B      = 0x0E00,    ///< 图像 Begin
    KLB_NMP_ARGB8888   = 0x0E01,    ///< [ARGB][ARGB]...[ARGB]
    KLB_NMP_IMG_E      = 0x0FFF,    ///< 图像 End

    KLB_NMP_DTYPE_MAX  = 0xFFFF     ///< MAX
}klb_mnp_dtype_e;


/// @enum   klb_mnp_sidx_e
/// @brief  媒体流序号
typedef enum klb_mnp_sidx_e_
{
    KLB_NMP_SIDX_NULL  = 0x0000,    ///< NULL
    KLB_NMP_SIDX_V1    = 0x0001,    ///< Video 1
    KLB_NMP_SIDX_V2    = 0x0002,    ///< Video 2
    KLB_NMP_SIDX_V3    = 0x0003,    ///< Video 3

    KLB_NMP_SIDX_A1    = 0x0021,    ///< Audio 1
    KLB_NMP_SIDX_A2    = 0x0022,    ///< Audio 2
    KLB_NMP_SIDX_A3    = 0x0023,    ///< Audio 3

    KLB_NMP_SIDX_P1    = 0x0041,    ///< Picture 1
    KLB_NMP_SIDX_P2    = 0x0042,    ///< Picture 2
    KLB_NMP_SIDX_P3    = 0x0043,    ///< Picture 3

    KLB_NMP_SIDX_I1    = 0x0061,    ///< Image 1
    KLB_NMP_SIDX_I2    = 0x0062,    ///< Image 2
    KLB_NMP_SIDX_I3    = 0x0063,    ///< Image 3

    KLB_NMP_SIDX_MAX   = 0xFFFF,    ///< MAX
}klb_mnp_sidx_e;

#ifdef __cplusplus
}
#endif

#endif // __KLB_MNP_MEDIA_H__
//end
