// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_tunnel_wire.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   alua-tunnel 帧编解码 (包内)
/// @version 0.2
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_TUNNEL_WIRE_H__
#define __KLB_TUNNEL_WIRE_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLB_TUNNEL_MAGIC0          0xA7
#define KLB_TUNNEL_MAGIC1          0x01
#define KLB_TUNNEL_VER             0x01
#define KLB_TUNNEL_HDR_LEN         6
#define KLB_TUNNEL_T_HELLO         0x01
#define KLB_TUNNEL_T_HELLO_OK      0x02
#define KLB_TUNNEL_T_HELLO_NAK     0x03
#define KLB_TUNNEL_T_CONNECT       0x10
#define KLB_TUNNEL_T_CONNECT_OK    0x11
#define KLB_TUNNEL_T_CONNECT_NAK   0x12
#define KLB_TUNNEL_T_DATA          0x20
#define KLB_TUNNEL_HOST_MAX        255


typedef struct klb_tunnel_frame_t_
{
    uint8_t     type;           ///< 帧类型
    uint8_t     ver;            ///< 版本
    const uint8_t* p_payload;   ///< payload 指针 (指向输入缓冲)
    int         payload_len;    ///< payload 长度
    int         frame_len;      ///< 整帧长度 (6 + payload)
} klb_tunnel_frame_t;


/// @brief 编码一帧到 out; payload 可空
/// @return 写入字节数; 失败 -1
int klb_tunnel_encode(uint8_t* p_out, int out_max, uint8_t typ,
    const uint8_t* p_payload, int payload_len);

/// @brief 从缓冲尝试解一帧
/// @return 0 成功; 1 不足 (short); -1 坏帧
int klb_tunnel_try_decode(const uint8_t* p_buf, int buf_len, klb_tunnel_frame_t* p_frame);

/// @brief 编码 CONNECT payload: host_len(1)+host+port_be16
int klb_tunnel_encode_connect_payload(uint8_t* p_out, int out_max,
    const char* p_host, int host_len, uint16_t port);

/// @brief 解析 CONNECT payload
/// @return 0 成功; -1 失败
int klb_tunnel_parse_connect_payload(const uint8_t* p, int len,
    char* p_host, int host_max, uint16_t* p_port);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_TUNNEL_WIRE_H__

// end
