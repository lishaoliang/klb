#ifndef __KLB_RESOLUTION_H__
#define __KLB_RESOLUTION_H__


#include "klb_type.h"


typedef enum klb_resolution_e_
{
    KLB_RESOLUTION_BEGIN = 1000,

    // 4:3
    KLB_RESOLUTION_256x190,
    KLB_RESOLUTION_320x240,     ///< QVGA
    KLB_RESOLUTION_480x360,
    KLB_RESOLUTION_640x480,     ///< VGA
    KLB_RESOLUTION_768x576,     ///< PAL D1
    KLB_RESOLUTION_1024x768,    ///< XGA
    KLB_RESOLUTION_1400x1050,   ///< SXGA
    KLB_RESOLUTION_1600x1200,   ///< UXGA
    KLB_RESOLUTION_2048x1536,   ///< QXGA

    // 16:9
    KLB_RESOLUTION_256x144,
    KLB_RESOLUTION_384x216,
    KLB_RESOLUTION_480x272,     ///< WQVGA
    KLB_RESOLUTION_640x360,     ///< nHD
    KLB_RESOLUTION_960x540,     ///< qHD
    KLB_RESOLUTION_1280x720,    ///< HD 720P
    KLB_RESOLUTION_1600x900,    ///< HD+
    KLB_RESOLUTION_1920x1080,   ///< HDTV 1080P
    KLB_RESOLUTION_2560x1440,   ///< QHD
    KLB_RESOLUTION_3200x1800,   ///< WQXGA+
    KLB_RESOLUTION_3840x2160,   ///< UHD 4K
    KLB_RESOLUTION_5120x2880,   ///< UHD+
    KLB_RESOLUTION_7680x4320,   ///< UHD 8K
    KLB_RESOLUTION_9600x5400,   ///< UHD+
    KLB_RESOLUTION_10240x5760,  ///< UHD+ 8K
}klb_resolution_e;



/// @brief 分辨率尺寸
/// @param [in]  resolution     尺寸类型: klb_resolution_e
/// @param [out] *p_w            输出宽
/// @param [out] *p_h            输出高
/// @return bool true,false
bool klb_resolution_size(int resolution/*klb_resolution_e*/, int* p_w, int* p_h);


#endif // __KLB_RESOLUTION_H__
//end
