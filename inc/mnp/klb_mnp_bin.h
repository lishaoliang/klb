#ifndef __KLB_MNP_BIN_H__
#define __KLB_MNP_BIN_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#pragma pack(4)

/// @struct klb_mnp_binh_t
/// @brief  media net protocol, bin head
///  \n F包: [klb_mnp_t][klb_mnp_binh_t][extra][data...]
///  \n B包: [klb_mnp_t][klb_mnp_binh_t][extra][data...]
///  \n C包: [klb_mnp_t][data...]
///  \n E包: [klb_mnp_t][data...]
typedef struct klb_mnp_binh_t_
{
    uint16_t    extra;      ///< 附加数据长度
    uint16_t    resv;       ///< 0

    uint32_t    sequence;   ///< 序列号
    uint32_t    uid;        ///< 用户自定义ID(user defined id)
    //- 4 + 4 + 4 = 12 Byte
}klb_mnp_binh_t;

#pragma pack()


#ifdef __cplusplus
}
#endif

#endif // __KLB_MNP_BIN_H__
//end
