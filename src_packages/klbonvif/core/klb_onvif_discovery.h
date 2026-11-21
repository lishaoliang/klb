// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_onvif_discovery.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   ONVIF WS-Discovery 组播搜索与服务 (UDP 239.255.255.250:3702)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_ONVIF_DISCOVERY_H__
#define __KLB_ONVIF_DISCOVERY_H__

#include "klb_type.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLB_ONVIF_DISCOVERY_MCAST_IP
/// @brief WS-Discovery 组播地址
#define KLB_ONVIF_DISCOVERY_MCAST_IP    "239.255.255.250"

/// @def   KLB_ONVIF_DISCOVERY_PORT
/// @brief WS-Discovery 端口
#define KLB_ONVIF_DISCOVERY_PORT        3702


/// @enum  klb_onvif_discovery_role_e
/// @brief Discovery 角色
typedef enum klb_onvif_discovery_role_e_
{
    KLB_ONVIF_DISCOVERY_SEARCH = 0,     ///< 搜索端 (发 Probe, 收 ProbeMatches)
    KLB_ONVIF_DISCOVERY_SERVE  = 1,     ///< 服务端 (加组播, 收 Probe, 回 ProbeMatches)
}klb_onvif_discovery_role_e;


/// @brief 打开 Discovery 组播连接并挂入 netmulti
/// @param [in] *p_netmulti     复用模块; 不可为 NULL
/// @param [in] role            角色: klb_onvif_discovery_role_e
/// @param [in] port            服务端绑定端口; 0 则用 KLB_ONVIF_DISCOVERY_PORT; 搜索端忽略
/// @param [out] *p_code        可选; 失败时 klb_netcode_e; 成功为 0
/// @return klb_netconn_t* 成功非 NULL; 失败 NULL
/// @note 载荷 (SOAP XML) 由调用方组装; 本模块只做 UDP 组播收发
klb_netconn_t* klb_onvif_discovery_open(klb_netmulti_t* p_netmulti, int role, uint16_t port, int* p_code);


/// @brief 释放 Discovery 连接
/// @param [in] *p_conn         连接; 可为 NULL
/// @return 无
void klb_onvif_discovery_free(klb_netconn_t* p_conn);


/// @brief 发送 UDP 报文
/// @param [in] *p_conn         Discovery 连接
/// @param [in] *p_data         载荷 (SOAP XML); 不可为 NULL
/// @param [in] len             载荷长度
/// @param [in] *p_ip           目标 IPv4; NULL 则发往组播地址
/// @param [in] port            目标端口; p_ip 为 NULL 时忽略, 使用组播端口
/// @return int 0 成功; 非 0 失败
int klb_onvif_discovery_send(klb_netconn_t* p_conn, const uint8_t* p_data, int len,
                             const char* p_ip, uint16_t port);


#ifdef __cplusplus
}
#endif

#endif // __KLB_ONVIF_DISCOVERY_H__

// end
