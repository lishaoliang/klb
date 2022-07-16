///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mnp_help.h
/// @brief   media net protocol help, mnp协议相关帮助函数
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNP_HELP_H__
#define __KLB_MNP_HELP_H__

#include "klb_type.h"
#include "klbbase/klb_mnp.h"
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


KLB_API klb_buf_t* klb_mnp_pack_heart();

KLB_API klb_buf_t* klb_mnp_pack_text(uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);

KLB_API klb_buf_t* klb_mnp_pack_binary(uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);

KLB_API klb_buf_t* klb_mnp_pack_media();


KLB_API klb_buf_t* klb_mnp_pack_data(klb_mnp_packtype_e packtype, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


#ifdef __cplusplus
}
#endif

#endif // __KLB_MNP_H__
//end
