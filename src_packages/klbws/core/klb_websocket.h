///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_websocket.h
/// @brief   ws/wss客户端
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WEBSOCKET_H__
#define __KLB_WEBSOCKET_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLB_WEBSOCKET_HEAD_MIN       2
#define KLB_WEBSOCKET_HEAD_MAX       14

#define KLB_WEBSOCKET_MAX_PAYLOAD    (1024 * 1024)   ///< 单条消息上限, 含分片合计

#define KLB_WEBSOCKET_PACK_OPEN      0x70            ///< 握手完成, 非 MNP 包类型
#define KLB_WEBSOCKET_PACK_CLOSE     0x71            ///< Close 帧, 载荷为 2 字节码 + reason
#define KLB_WEBSOCKET_PACK_PONG      0x72            ///< Pong 帧载荷

#define KLB_WEBSOCKET_CLOSE_NORMAL   1000
#define KLB_WEBSOCKET_CLOSE_PROTO    1002
#define KLB_WEBSOCKET_CLOSE_UTF8     1007
#define KLB_WEBSOCKET_CLOSE_TOOBIG   1009
#define KLB_WEBSOCKET_CLOSE_NOSTATUS 1005

typedef enum klb_websocket_opcode_e_
{
    KLB_WEBSOCKET_OPCODE_CONTINUATION = 0x0,    ///< %x0 a continuation frame
    KLB_WEBSOCKET_OPCODE_TEXT = 0x1,            ///< %x1 a text frame
    KLB_WEBSOCKET_OPCODE_BINARY = 0x2,          ///< %x2 a binary frame
    KLB_WEBSOCKET_OPCODE_CLOSE = 0x8,           ///< %x8 a connection close
    KLB_WEBSOCKET_OPCODE_PING = 0x9,            ///< %x9 a ping
    KLB_WEBSOCKET_OPCODE_PONG = 0xA,            ///< %xA a pong
}klb_websocket_opcode_e;


/* rfc6455 5.2. Base Framing Protocol
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
*/
typedef struct klb_websocket_t_
{
    uint8_t  fin : 1;           ///< FIN
    uint8_t  rsv1 : 1;
    uint8_t  rsv2 : 1;
    uint8_t  rsv3 : 1;
    uint8_t  opcode : 4;        ///< klb_websocket_opcode_e
    uint8_t  mask;              ///< 0.无mask; 1.有mask

    uint8_t  head_len;          ///< ws头长度[KLB_WEBSOCKET_HEAD_MIN, KLB_WEBSOCKET_HEAD_MAX]

    uint8_t  mask_key[4];       ///< mask=1有效
    uint64_t payload_len;       ///< 负载数据长度
}klb_websocket_t;


int klb_websocket_parse(klb_websocket_t* p_ws, const uint8_t* p_data, int data_len);

void klb_websocket_mask(const uint8_t* p_mask_key, uint8_t* p_data, int data_len);

klb_buf_t* klb_websocket_pack_fin(uint8_t opcode, const uint8_t* p_mask_key, const char* p_data, int data_len);


/// @brief 填充伪随机字节
/// @param [out] *p_out          输出
/// @param [in]  n               字节数
/// @return 无
void klb_websocket_random(uint8_t* p_out, int n);


/// @brief Base64 编码
/// @param [in]  *p_in           输入
/// @param [in]  in_len          输入长度
/// @param [out] *p_out          输出, 含结尾 0
/// @param [in]  out_cap         输出容量
/// @return int 0.成功; 非0.失败
int klb_websocket_b64(const uint8_t* p_in, int in_len, char* p_out, int out_cap);


/// @brief 计算 Sec-WebSocket-Accept
/// @param [in]  *p_key          Sec-WebSocket-Key
/// @param [out] *p_out          Accept 字符串
/// @param [in]  out_cap         输出容量, 至少 29
/// @return int 0.成功; 非0.失败
int klb_websocket_accept(const char* p_key, char* p_out, int out_cap);


/// @brief 生成 4 字节掩码
/// @param [out] key[4]          掩码
/// @return 无
void klb_websocket_mask_key(uint8_t key[4]);


/// @brief 从握手原文取头字段
/// @param [in]  *p_header       头文本
/// @param [in]  *p_name         字段名
/// @param [out] *p_out          字段值
/// @param [in]  out_cap         输出容量
/// @return bool true.取到完整值; false.没有或被截断
bool klb_websocket_header_get(const char* p_header, const char* p_name, char* p_out, int out_cap);


/// @brief 头字段值是否包含记号(忽略大小写)
/// @param [in] *p_header        头文本
/// @param [in] *p_name          字段名
/// @param [in] *p_token         记号
/// @return bool true.包含
bool klb_websocket_header_has(const char* p_header, const char* p_name, const char* p_token);


/// @brief 帧头是否可接受
/// @param [in] *p_ws            已解析的帧头
/// @param [in] from_client      true.对端是客户端, 必须带 mask
/// @return bool true. text/binary/continuation/close/ping/pong; 数据帧允许 FIN=0
bool klb_websocket_frame_ok(const klb_websocket_t* p_ws, bool from_client);


/// @brief UTF-8 是否合法 (RFC 3629)
/// @param [in] *p_data          字节
/// @param [in] data_len         长度
/// @return bool true.合法或空
bool klb_websocket_utf8_ok(const uint8_t* p_data, int data_len);


/// @brief Close 载荷是否可接受
/// @param [in]  *p_data         载荷; 空表示无状态码
/// @param [in]  data_len        长度
/// @param [out] *p_code         解析出的关闭码; 空载荷为 1005
/// @return bool true.可接受
bool klb_websocket_close_ok(const uint8_t* p_data, int data_len, int* p_code);


/// @brief 关闭码是否允许出现在帧里
/// @param [in] code             关闭码
/// @return bool true.允许发送
bool klb_websocket_close_code_ok(int code);


/// @brief 子协议列表是否包含记号
/// @param [in] *p_list          逗号分隔列表
/// @param [in] *p_token         记号
/// @return bool true.包含
bool klb_websocket_protocol_has(const char* p_list, const char* p_token);



#ifdef __cplusplus
}
#endif

#endif // __KLB_WEBSOCKET_H__

// end
