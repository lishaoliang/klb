///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_lsm.h
/// @brief   klb live stream media framework: 实时流媒体框架: 服务端
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_LSM_H__
#define __KLB_LSM_H__

#include "klb_type.h"
#include "net/klb_ncm.h"
#include "platform/klb_thread.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLB_LSM_NCM_MAX             64


typedef struct klb_lsm_t_
{
    int             ncm_num;                    ///< ncm数目
    klb_ncm_t*      p_ncm[KLB_LSM_NCM_MAX];     ///< ncm

    klb_thread_t*   p_thread_listen;            ///< 监听线程
    klb_thread_t*   p_thread_txt;               ///< 文本/二进制处理线程
    klb_thread_t*   p_thread_media;             ///< 媒体处理线程
}klb_lsm_t;


klb_lsm_t* klb_lsm_create();
void klb_lsm_destroy(klb_lsm_t* p_lsm);

int klb_lsm_start(klb_lsm_t* p_lsm);
int klb_lsm_stop(klb_lsm_t* p_lsm);


#ifdef __cplusplus
}
#endif

#endif // __KLB_LSM_H__
//end
