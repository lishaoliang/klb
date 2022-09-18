///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_flv_demux.h
/// @brief   FLV demux
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_FLV_DEMUX_H__
#define __KLB_FLV_DEMUX_H__


#include "klb_type.h"
#include "klbutil/klb_nlist.h"


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_flv_demux_t_ klb_flv_demux_t;


klb_flv_demux_t* klb_flv_demux_create();
void klb_flv_demux_destroy(klb_flv_demux_t* p_flv_demux);


/// @brief 解分离flv数据流
/// @param [in]  *p_flv_demux    模块
/// @param [in]  *p_data         流数据
/// @param [in]  data_len        数据长度
/// @param [out] *p_list_out 解析出来的数据帧,放入此链表中
/// @return 0.正常; -1. 数据格式错误
int klb_flv_demux_do(klb_flv_demux_t* p_flv_demux, const char* p_data, int data_len, klb_nlist_t* p_list_out);


#ifdef __cplusplus
}
#endif

#endif // __KLB_FLV_DEMUX_H__
//end
