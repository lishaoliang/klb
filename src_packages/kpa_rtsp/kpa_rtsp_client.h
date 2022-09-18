///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    kpa_rtsp_client.h
/// @brief   rtsp client
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KPA_RTSP_CLIENT_H__
#define __KPA_RTSP_CLIENT_H__

#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_coroutine.h"
#include "klbnet/klb_multiplex.h"
#include "klbnet/klb_socket.h"

#include "kpa_rtsp/kpa_rtsp_client_inter.h"


typedef struct kpa_rtsp_client_t_
{
    klua_ex_coroutine_t*        p_ex_co;
    klb_multiplex_t*            p_multi;

    kpa_rtsp_client_inter_t*    p_inter;
}kpa_rtsp_client_t;


void kpa_rtsp_client_createmeta(lua_State* L);

int kpa_rtsp_client_connect(lua_State* L);


#endif // __KPA_RTSP_CLIENT_H__
//end
