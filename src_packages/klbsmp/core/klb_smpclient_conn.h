// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_smpclient_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   SMP协议 客户端连接(klb SMP client connect)
/// @version 0.2
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SMPCLIENT_CONN_H__
#define __KLB_SMPCLIENT_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建SMP连接
/// @param [in] *p_netmulti       复用模块
/// @param [in] *p_socket         socket
/// @return klb_netconn_t* 连接对象
klb_netconn_t* klb_smpclient_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);


/// @brief SMP连接目标
/// @param [in] *p_netmulti       复用模块
/// @param [in] *p_host           主机
/// @param [in] port              端口
/// @param [in] tls               true=TLS(smps)
/// @return klb_netconn_t* 连接对象; 失败为 NULL
/// @param [out] *p_code          可选; 失败时 klb_netcode_e; 成功为 0
klb_netconn_t* klb_smpclient_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, bool tls, int* p_code);


/// @brief 释放连接
/// @param [in] *p_conn           连接对象
/// @return 无
void klb_smpclient_conn_free(klb_netconn_t* p_conn);


/// @brief 发送SMP数据(头 + 可选body)
/// @param [in] *p_conn           连接对象
/// @param [in] packtype          包类型: klb_mnp_packtype_e
/// @param [in] sequence          序号
/// @param [in] *p_head           数据头; 可为 NULL
/// @param [in] head_len          数据头长度
/// @param [in] *p_body           数据体; 可为 NULL
/// @param [in] body_len          数据体长度
/// @return int 0.成功; 非0.失败
int klb_smpclient_conn_send(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 按 buf 发送SMP数据
/// @param [in] *p_conn           连接对象
/// @param [in] *p_data           待发送缓存; 所有权转入连接
/// @return int 0.成功; 非0.失败
int klb_smpclient_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data);


/// @brief 写缓存 是否为空
/// @param [in] *p_conn           连接对象
/// @return bool true.空; false.还有数据
bool klb_smpclient_wbuf_is_empty(klb_netconn_t* p_conn);


/// @brief 写缓存 未发送字节数
/// @param [in] *p_conn           连接对象
/// @return int64_t 待发送字节数; 0 表示已排空
int64_t klb_smpclient_wbuf_size(klb_netconn_t* p_conn);


/// @brief 设置本连接 TEXT 是否压缩(选择权在客户端; Lua 无感)
/// @param [in] *p_conn           连接对象
/// @param [in] enable            true=发送 TEXT 使用 ZIP 压缩
/// @return 无
/// @note 本端无 zlib 时强制关闭; 收到压缩而本端未开启则协议断连
void klb_smpclient_conn_set_compress(klb_netconn_t* p_conn, bool enable);


/// @brief 设置本端发送加码并发送 CRYPTO_PSK OPTIONS(固定密钥无握手)
/// @param [in] *p_conn           连接对象
/// @param [in] sequence          OPTIONS 序号
/// @param [in] format            klb_mnp_crypto_e; 不可为 NULL
/// @param [in] *p_key            密钥材料
/// @param [in] key_size          密钥长度
/// @return int 0.成功; 非0.失败(失败时 TX 状态不变)
/// @note 对端收包后更新其 RX; PING/PONG/OPTIONS 自身不加码
int klb_smpclient_conn_send_crypto_psk(klb_netconn_t* p_conn, int sequence,
    uint32_t format, const uint8_t* p_key, uint32_t key_size);


/// @brief 连接策略: 设 TX 加码并立即发 CRYPTO_PSK(可供 Lua opts.crypto)
/// @param [in] *p_conn           连接对象
/// @param [in] format            klb_mnp_crypto_e; 不可为 NULL
/// @param [in] *p_key            密钥; NULL 则按 key_size 随机生成
/// @param [in] key_size          密钥长度; 须 >0
/// @param [in] rekey_after       >0 时每成功发送 N 次受控业务帧后自动换钥; 0=关闭
/// @return int 0.成功; 非0.失败
int klb_smpclient_conn_set_crypto(klb_netconn_t* p_conn,
    uint32_t format, const uint8_t* p_key, uint32_t key_size, uint32_t rekey_after);



void klb_smpclient_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms);
void klb_smpclient_conn_set_max_msg(klb_netconn_t* p_conn, int64_t max_msg);

#ifdef __cplusplus
}
#endif

#endif // __KLB_SMPCLIENT_CONN_H__

// end
