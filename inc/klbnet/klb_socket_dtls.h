// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socket_dtls.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   DTLS over UDP 会话 (RFC 6347, mbedTLS datagram 传输)
/// @version 0.6
/// @history 修改历史
///  \n [2026] 创建文件
///  \n [2026-10] 0.2 实现 datagram 传输 / 超时重传 / 握手分片 / 证书指纹导出; 0.3 DTLS-SRTP 密钥导出落地 (RFC 5764 EXTRACTOR-dtls_srtp); 0.4 增加 klb_socket_dtls_srtp_export_peer (本端接收方向); 0.5 增加抽象 IO 传输 (承载于 ICE 已选通道)
/// @warning 服务端一段会话绑定一个 UDP fd; 未 connect 的 fd 以首个报文源地址为对端
///  \n IO 模式: 不持有 fd; 服务端使用固定 client transport id (单会话 P2P)
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SOCKET_DTLS_H__
#define __KLB_SOCKET_DTLS_H__

#include "klbnet/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_socket_dtls_cfg_t
/// @brief  DTLS 配置
typedef struct klb_socket_dtls_cfg_t_
{
    const char*     p_cert_pem;         ///< 本端证书 (PEM); 服务端必需
    const char*     p_key_pem;          ///< 本端私钥 (PEM); 服务端必需
    const char*     p_ca_pem;           ///< 对端校验 CA (PEM); 可 NULL
    int             verify_peer;        ///< 0(false).不校验; 1(true).校验对端证书
}klb_socket_dtls_cfg_t;


/// @struct klb_socket_dtls_io_t
/// @brief  DTLS 抽象传输回调; 用于承载在非 fd 通道 (如 ICE 已选通道) 之上
/// @note  send/recv 返回大于 0 为字节数; 返回 0 或小于 0 视为暂时不可读写
typedef struct klb_socket_dtls_io_t_
{
    int  (*send)(void* p_ctx, const uint8_t* p_data, int len);      ///< 发送一个 DTLS 记录
    int  (*recv)(void* p_ctx, uint8_t* p_buf, int cap);             ///< 接收一个 DTLS 记录
    void (*close)(void* p_ctx);                                     ///< 可选; 会话销毁时回调
    void* p_ctx;                                                    ///< 回调上下文
}klb_socket_dtls_io_t;


/// @brief 在抽象 IO 上创建 DTLS 会话 (不持有 fd)
/// @param [in]  is_server      0(false).客户端; 1(true).服务端
/// @param [in]  *p_io          传输回调; send/recv 不可为 NULL
/// @param [in]  *p_cfg         配置; 服务端不可为 NULL
/// @param [out] *p_code        可选; 失败时 klb_netcode_e; 成功为 0
/// @return klb_socket_t* NULL.失败; 非 NULL.DTLS socket
KLB_API klb_socket_t* klb_socket_dtls_io_open(bool is_server, const klb_socket_dtls_io_t* p_io,
    const klb_socket_dtls_cfg_t* p_cfg, int* p_code);


/// @brief 在已有 UDP socket 上创建 DTLS 客户端
/// @param [in]  *p_socket      已 connect 的 UDP socket; 成功后被 DTLS 会话接管
/// @param [in]  *p_cfg         配置; 可为 NULL
/// @param [out] *p_code        可选; 失败时 klb_netcode_e; 成功为 0
/// @return klb_socket_t* NULL.失败 (桩恒 NULL); 非 NULL.DTLS socket
KLB_API klb_socket_t* klb_socket_dtls_client_open(klb_socket_t* p_socket, const klb_socket_dtls_cfg_t* p_cfg, int* p_code);


/// @brief 在已有 UDP socket 上创建 DTLS 服务端
/// @param [in]  *p_socket      已 bind 的 UDP socket; 成功后被 DTLS 会话接管
/// @param [in]  *p_cfg         配置; 不可为 NULL
/// @param [out] *p_code        可选; 失败时 klb_netcode_e; 成功为 0
/// @return klb_socket_t* NULL.失败 (桩恒 NULL); 非 NULL.DTLS socket
KLB_API klb_socket_t* klb_socket_dtls_server_open(klb_socket_t* p_socket, const klb_socket_dtls_cfg_t* p_cfg, int* p_code);


/// @brief 推进 DTLS 握手
/// @param [in] *p_socket       DTLS socket
/// @return int 0.完成; 1.继续; 小于 0.klb_netcode_e
KLB_API int klb_socket_dtls_handshake(klb_socket_t* p_socket);


/// @brief 导出 DTLS-SRTP 密钥材料 (RFC 5764, 本端发送方向)
/// @param [in]  *p_socket      DTLS socket; 须握手完成且双方已协商 use_srtp
/// @param [out] *p_key         输出主密钥 (客户端取 client_write_key, 服务端取 server_write_key)
/// @param [in]  key_cap        密钥缓存大小; 至少 16
/// @param [out] *p_salt        输出主盐 (对应方向的 write_salt)
/// @param [in]  salt_cap       盐缓存大小; 至少 14
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 对端方向用 klb_socket_dtls_srtp_export_peer 获取
KLB_API int klb_socket_dtls_srtp_export(klb_socket_t* p_socket, uint8_t* p_key, int key_cap,
                                        uint8_t* p_salt, int salt_cap);


/// @brief 导出 DTLS-SRTP 密钥材料 (RFC 5764, 对端发送方向 = 本端接收方向)
/// @param [in]  *p_socket      DTLS socket; 须握手完成且双方已协商 use_srtp
/// @param [out] *p_key         输出主密钥 (客户端取 server_write_key, 服务端取 client_write_key)
/// @param [in]  key_cap        密钥缓存大小; 至少 16
/// @param [out] *p_salt        输出主盐 (对应方向的 write_salt)
/// @param [in]  salt_cap       盐缓存大小; 至少 14
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_dtls_srtp_export_peer(klb_socket_t* p_socket, uint8_t* p_key, int key_cap,
                                             uint8_t* p_salt, int salt_cap);


/// @brief 计算证书 PEM 的 SHA-256 指纹 (SDP a=fingerprint 用, 无需建立会话)
/// @param [in]  *p_cert_pem   证书 PEM
/// @param [out] *p_text       输出文本
/// @param [in]  cap           缓冲容量; 至少 104 字节
/// @return int >0.文本长度; 小于 0.klb_netcode_e
/// @note 输出形如 "sha-256 AB:CD:..."
KLB_API int klb_socket_dtls_cert_fingerprint(const char* p_cert_pem, char* p_text, int cap);


/// @brief 导出对端证书 SHA-256 指纹 (SDP a=fingerprint 形如 "sha-256 AB:CD:...")
/// @param [in]  *p_socket      DTLS socket; 须握手完成
/// @param [out] *p_text        输出文本缓冲
/// @param [in]  cap            缓冲容量; 至少 104 字节
/// @return int >0.文本长度; 小于 0.klb_netcode_e
/// @note 客户端用于校验服务端证书; 服务端用于校验客户端证书; 未握手/无对端证书返回失败
KLB_API int klb_socket_dtls_fingerprint(klb_socket_t* p_socket, char* p_text, int cap);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_SOCKET_DTLS_H__

// end
