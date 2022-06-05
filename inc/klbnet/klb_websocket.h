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



#ifdef __cplusplus
}
#endif

#endif // __KLB_WEBSOCKET_H__
//end
