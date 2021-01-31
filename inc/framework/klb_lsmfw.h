///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_lsmfw.h
/// @brief   klb live stream media framework: 实时流媒体框架: 服务端
///          go实现: "./klb/src/kframework/klsmfw"
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_LSMFW_H__
#define __KLB_LSMFW_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


int klb_lsmfw_init(const char* p_cfg);
void klb_lsmfw_quit();
int klb_lsmfw_start();
void klb_lsmfw_stop();

int klb_lsmfw_listen();
int klb_lsmfw_listen_tls();
int klb_lsmfw_close_listen();

int klb_lsmfw_set_new_conn_cb();
int klb_lsmfw_set_receiver_cb();
int klb_lsmfw_set_disconnect_cb();

int klb_lsmfw_switch_protocol();
int klb_lsmfw_close_conn(int id);
int klb_lsmfw_send_txt(int id);
int klb_lsmfw_send_file(int id);

int klb_lsmfw_open_stream(int id);
int klb_lsmfw_close_stream(int id);

int klb_lsmfw_push_media(uint32_t chnn, uint32_t sidx);


#ifdef __cplusplus
}
#endif

#endif // __KLB_LSMFW_H__
//end
