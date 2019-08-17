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
///  \n F包: [klb_mnp_t][klb_mnp_mh_t][extra][data...]
///  \n B包: [klb_mnp_t][klb_mnp_mh_t][extra][data...]
///  \n C包: [klb_mnp_t][data...]
///  \n E包: [klb_mnp_t][data...]
///  \n 数据包可以被存储, 数据包尽可能只描述与数据有关部分?
typedef struct klb_mnp_mh_t_
{
    uint16_t     extra : 12; ///< 附加数据长度 <= 4K
    union
    {
        uint16_t vtype : 4;  ///< 视频(video type): klb_mnp_vtype_e
    };

    uint16_t     chnn;       ///< 通道(channel)
    //- 4 Byte

    uint16_t     sidx;       ///< 流序号(stream index): klb_mnp_sidx_e
    uint16_t     dtype;      ///< 数据类型(data type): klb_mnp_dtype_e
    //- 4 + 4 = 8 Byte

    int64_t      time;       ///< 时间戳(毫秒)
    //- 8 + 8 = 16 Byte
}klb_mnp_mh_t;

#pragma pack()


/// @enum   klb_mnp_dtype_e
/// @brief  视频帧类型
typedef enum klb_mnp_vtype_e_
{
    KLB_NMP_VTYPE_I    = 0x0,       ///< I帧
    KLB_NMP_VTYPE_P    = 0x1,       ///< P帧
    KLB_NMP_VTYPE_B    = 0x2,       ///< B帧
    KLB_NMP_VTYPE_MAX  = 0xF        ///< MAX
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
