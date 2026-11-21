// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socks5_wire.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   SOCKS5 RFC1928/1929 缓冲区编解码 (包内)
/// @version 0.2
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SOCKS5_WIRE_H__
#define __KLB_SOCKS5_WIRE_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLB_SOCKS5_VER                 0x05
#define KLB_SOCKS5_AUTH_VER            0x01
#define KLB_SOCKS5_METHOD_NONE         0x00
#define KLB_SOCKS5_METHOD_USERPASS     0x02
#define KLB_SOCKS5_METHOD_NO_ACCEPT    0xFF
#define KLB_SOCKS5_CMD_CONNECT         0x01
#define KLB_SOCKS5_ATYP_IPV4           0x01
#define KLB_SOCKS5_ATYP_DOMAIN         0x03
#define KLB_SOCKS5_ATYP_IPV6           0x04
#define KLB_SOCKS5_REP_SUCCEEDED       0x00
#define KLB_SOCKS5_REP_CMD_NOT_SUPP    0x07
#define KLB_SOCKS5_REP_ATYP_NOT_SUPP   0x08
#define KLB_SOCKS5_HOST_MAX            255
#define KLB_SOCKS5_ENCODE_MAX          520


/// @brief 编码客户端 greeting
/// @param [in]  with_userpass  非 0 时同时声明 METHOD 0x00 与 0x02
/// @return 写入字节数; 失败 -1
int klb_socks5_encode_greeting(uint8_t* p_out, int out_max, int with_userpass);

/// @brief 解析服务端 method select (2 字节)
/// @return 0 成功; 1 不足; -1 坏帧
int klb_socks5_parse_method_select(const uint8_t* p, int len, uint8_t* p_method);

/// @brief 编码 RFC1929 user/pass 请求
int klb_socks5_encode_userpass(uint8_t* p_out, int out_max,
    const char* p_user, int user_len, const char* p_pass, int pass_len);

/// @brief 解析 RFC1929 状态 (2 字节)
int klb_socks5_parse_userpass_status(const uint8_t* p, int len, uint8_t* p_status);

/// @brief 编码 CONNECT 请求 (含 VER/CMD/RSV)
int klb_socks5_encode_connect(uint8_t* p_out, int out_max,
    const char* p_host, int host_len, uint16_t port);

/// @brief 编码 method select 应答
int klb_socks5_encode_method_select(uint8_t* p_out, int out_max, uint8_t method);

/// @brief 解析客户端 greeting
/// @param [out] p_methods  方法表 (可 NULL 仅取 nmethods)
/// @param [out] p_consumed 已消费字节
/// @return 0 成功; 1 不足; -1 坏帧
int klb_socks5_parse_greeting(const uint8_t* p, int len,
    uint8_t* p_methods, int methods_max, int* p_nmethods, int* p_consumed);

/// @brief 按策略选 METHOD: want_userpass 非 0 仅接受 0x02, 否则仅 0x00
/// @return 选中 METHOD; 无则 0xFF
uint8_t klb_socks5_choose_method(const uint8_t* p_methods, int nmethods, int want_userpass);

/// @brief 解析 RFC1929 请求
/// @return 0 成功; 1 不足; -1 坏帧
int klb_socks5_parse_userpass_req(const uint8_t* p, int len,
    char* p_user, int user_max, int* p_user_len,
    char* p_pass, int pass_max, int* p_pass_len, int* p_consumed);

/// @brief 编码 RFC1929 状态应答
int klb_socks5_encode_userpass_status(uint8_t* p_out, int out_max, uint8_t status);

typedef struct klb_socks5_request_t_
{
    uint8_t     cmd;                            ///< CMD
    uint8_t     atyp;                           ///< ATYP
    char        host[KLB_SOCKS5_HOST_MAX + 1];  ///< 目标主机
    uint16_t    port;                           ///< 目标端口
    int         consumed;                       ///< 已消费字节
} klb_socks5_request_t;

/// @brief 解析 CONNECT 请求 (完整报文)
/// @return 0 成功; 1 不足; -1 坏帧
int klb_socks5_parse_request(const uint8_t* p, int len, klb_socks5_request_t* p_req);

typedef struct klb_socks5_reply_t_
{
    uint8_t     rep;                            ///< REP
    uint8_t     atyp;                           ///< ATYP
    char        host[KLB_SOCKS5_HOST_MAX + 1];  ///< BND 主机 (可空)
    uint16_t    port;                           ///< BND 端口
    int         consumed;                       ///< 已消费字节
} klb_socks5_reply_t;

/// @brief 解析 CONNECT 应答 (完整报文)
/// @return 0 成功; 1 不足; -1 坏帧
int klb_socks5_parse_reply(const uint8_t* p, int len, klb_socks5_reply_t* p_rsp);

/// @brief 编码 CONNECT 应答 (BND=0.0.0.0:0)
int klb_socks5_encode_reply(uint8_t* p_out, int out_max, uint8_t rep);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_SOCKS5_WIRE_H__

// end
