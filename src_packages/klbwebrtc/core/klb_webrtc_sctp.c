// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_webrtc_sctp.c
/// @brief   WebRTC DataChannel 的 SCTP/DCEP 最小子集实现
/// @version 0.1
/// @history 修改历史
///  \n [2026-10] 创建文件: CRC32c / 四向握手 / DATA / DCEP OPEN+ACK
/// @warning 单流单通道; 不做分片/SACK/重传; CRC32c 按 RFC 4960 小端存储
///////////////////////////////////////////////////////////////////////////
#include "klbwebrtc/core/klb_webrtc_sctp.h"
#include "klbnet/klb_netcode.h"
#include "klbplatform/klb_time.h"
#include "klbmem/klb_mem.h"
#include <string.h>


/// @def   KLB_WEBRTC_DC_PORT
/// @brief SCTP 端口; 对齐 SDP a=sctp-port:5000
#define KLB_WEBRTC_DC_PORT              5000

/// @def   KLB_WEBRTC_DC_MTU
/// @brief 单包上限; 对齐 DTLS MTU
#define KLB_WEBRTC_DC_MTU               1200

/// @def   KLB_WEBRTC_DC_QUEUE_SLOTS
/// @brief 待发控制/分片包槽位
#define KLB_WEBRTC_DC_QUEUE_SLOTS       32

/// @def   KLB_WEBRTC_DC_SENT_SLOTS
/// @brief 已发 DATA 缓存槽位 (供 SACK 确认与 RTO 重传)
#define KLB_WEBRTC_DC_SENT_SLOTS        32

/// @def   KLB_WEBRTC_DC_RTO_MS
/// @brief 重传超时 (毫秒)
#define KLB_WEBRTC_DC_RTO_MS            1000

/// @def   KLB_WEBRTC_DC_RTO_MAX
/// @brief 同一 DATA 最大重传次数
#define KLB_WEBRTC_DC_RTO_MAX           5

/// @def   KLB_WEBRTC_DC_SACK_BITS
/// @brief 接收位图窗口位数
#define KLB_WEBRTC_DC_SACK_BITS         64

/// @def   KLB_WEBRTC_DC_SACK_GAPS
/// @brief 单次 SACK 可携带的缺口块上限
#define KLB_WEBRTC_DC_SACK_GAPS         8

/// @def   KLB_WEBRTC_DC_FLAG_E
/// @brief DATA chunk 标志: 消息尾片
#define KLB_WEBRTC_DC_FLAG_E            0x01

/// @def   KLB_WEBRTC_DC_FLAG_B
/// @brief DATA chunk 标志: 消息首片
#define KLB_WEBRTC_DC_FLAG_B            0x02

/// @def   KLB_WEBRTC_DC_COMMON_HEADER
/// @brief SCTP 公共头长度
#define KLB_WEBRTC_DC_COMMON_HEADER     12

/// @def   KLB_WEBRTC_DC_CHUNK_HEADER
/// @brief chunk 头长度
#define KLB_WEBRTC_DC_CHUNK_HEADER      4

/// @def   KLB_WEBRTC_DC_INIT_LEN
/// @brief INIT/INIT_ACK 固定载荷长度
#define KLB_WEBRTC_DC_INIT_LEN          16

/// @def   KLB_WEBRTC_DC_COOKIE_LEN
/// @brief COOKIE_ECHO 载荷长度
#define KLB_WEBRTC_DC_COOKIE_LEN        8

/// @def   KLB_WEBRTC_DC_DATA_HEADER
/// @brief DATA chunk 固定头长度
#define KLB_WEBRTC_DC_DATA_HEADER       12

/// @def   KLB_WEBRTC_DC_DCEP_OPEN_LEN
/// @brief DCEP OPEN 载荷长度
#define KLB_WEBRTC_DC_DCEP_OPEN_LEN     12

/// @def   KLB_WEBRTC_DC_ARWND
/// @brief 接收窗口通告值
#define KLB_WEBRTC_DC_ARWND             262144

/// @def   KLB_WEBRTC_DC_CRC32C_POLY
/// @brief CRC32c 反向多项式 (RFC 4960 附录 B)
#define KLB_WEBRTC_DC_CRC32C_POLY       0x82F63B78u

/// @def   KLB_WEBRTC_DC_CHUNK_DATA
/// @brief chunk 类型: DATA
#define KLB_WEBRTC_DC_CHUNK_DATA        0

/// @def   KLB_WEBRTC_DC_CHUNK_INIT
/// @brief chunk 类型: INIT
#define KLB_WEBRTC_DC_CHUNK_INIT        1

/// @def   KLB_WEBRTC_DC_CHUNK_INIT_ACK
/// @brief chunk 类型: INIT_ACK
#define KLB_WEBRTC_DC_CHUNK_INIT_ACK    2

/// @def   KLB_WEBRTC_DC_CHUNK_SACK
/// @brief chunk 类型: SACK
#define KLB_WEBRTC_DC_CHUNK_SACK        3

/// @def   KLB_WEBRTC_DC_CHUNK_COOKIE_ECHO
/// @brief chunk 类型: COOKIE_ECHO
#define KLB_WEBRTC_DC_CHUNK_COOKIE_ECHO 10

/// @def   KLB_WEBRTC_DC_CHUNK_COOKIE_ACK
/// @brief chunk 类型: COOKIE_ACK
#define KLB_WEBRTC_DC_CHUNK_COOKIE_ACK  11

/// @def   KLB_WEBRTC_DC_DCEP_ACK
/// @brief DCEP 消息类型: ACK
#define KLB_WEBRTC_DC_DCEP_ACK          0x02

/// @def   KLB_WEBRTC_DC_DCEP_OPEN
/// @brief DCEP 消息类型: OPEN
#define KLB_WEBRTC_DC_DCEP_OPEN         0x03


/// @enum  klb_webrtc_dc_state_e
/// @brief DataChannel 关联状态
typedef enum klb_webrtc_dc_state_e_
{
    KLB_WEBRTC_DC_STATE_CLOSED       = 0,   ///< 未发起
    KLB_WEBRTC_DC_STATE_COOKIE_WAIT  = 1,   ///< 已发 INIT, 等 INIT_ACK
    KLB_WEBRTC_DC_STATE_COOKIE_ECHOED = 2,  ///< 已发 COOKIE_ECHO, 等 COOKIE_ACK
    KLB_WEBRTC_DC_STATE_ESTABLISHED  = 3,   ///< 关联已建立
}klb_webrtc_dc_state_e;


/// @struct klb_webrtc_dc_pkt_t
/// @brief  待发控制包
typedef struct klb_webrtc_dc_pkt_t_
{
    int     len;                                    ///< 报文长度
    uint8_t data[KLB_WEBRTC_DC_MTU];                ///< 报文内容
}klb_webrtc_dc_pkt_t;


/// @struct klb_webrtc_dc_sent_t
/// @brief  已发 DATA 缓存项 (SACK 确认 / RTO 重传)
typedef struct klb_webrtc_dc_sent_t_
{
    bool        used;                               ///< 槽位占用
    int         retries;                            ///< 已重传次数
    uint32_t    tsn;                                ///< TSN
    uint16_t    stream_id;                          ///< 流号
    uint16_t    stream_seq;                         ///< 流内序号
    int         flags;                              ///< DATA chunk 标志
    int         ppid;                               ///< 载荷协议号
    int         len;                                ///< 载荷长度
    uint32_t    sent_ms;                            ///< 最近发送时刻
    uint8_t*    p_data;                             ///< 载荷副本
}klb_webrtc_dc_sent_t;


/// @struct klb_webrtc_dc_t_
/// @brief  DataChannel 会话 (对外不透明)
struct klb_webrtc_dc_t_
{
    bool        is_client;                          ///< 1.SCTP 客户端; 0.服务端
    bool        open;                               ///< DCEP 打开完成 (兼容字段)
    bool        dcep_sent;                          ///< 已发 DCEP OPEN
    int         state;                              ///< klb_webrtc_dc_state_e
    int         stream_id;                          ///< 默认发送流号
    uint32_t    want_mask;                          ///< 待打开流位图
    uint32_t    open_mask;                          ///< 已打开流位图
    uint16_t    local_port;                         ///< 本地端口
    uint16_t    remote_port;                        ///< 对端端口
    uint32_t    local_vtag;                         ///< 本端校验标签
    uint32_t    remote_vtag;                        ///< 对端校验标签
    uint32_t    local_tsn;                          ///< 本端下一 TSN
    uint16_t    stream_seq[KLB_WEBRTC_DC_STREAMS];  ///< 各流下一序号
    int         head;                               ///< 队列出队下标
    int         count;                              ///< 队列元素数
    int         sent_head;                          ///< 已发缓存写入下标
    klb_webrtc_dc_pkt_t slots[KLB_WEBRTC_DC_QUEUE_SLOTS];   ///< 待发控制包槽位
    klb_webrtc_dc_sent_t sent[KLB_WEBRTC_DC_SENT_SLOTS];    ///< 已发 DATA 缓存

    bool        rx_started;                         ///< 已建立接收 TSN 基线
    uint32_t    rx_cum_tsn;                         ///< 累计确认 TSN
    uint64_t    rx_bits;                            ///< rx_cum_tsn+1 起的接收位图
    bool        rx_busy;                            ///< 分片重组进行中
    int         rx_stream;                          ///< 重组消息流号
    int         rx_ppid;                            ///< 重组消息 PPID
    int         rx_len;                             ///< 已重组长度
    int         rx_cap;                             ///< 重组缓冲容量
    uint8_t*    p_rx;                               ///< 分片重组缓冲
};


/// @brief 读大端 16 位
static uint16_t get_u16_be_klb_webrtc_dc(const uint8_t* p_data)
{
    return (uint16_t)(((uint16_t)p_data[0] << 8) | (uint16_t)p_data[1]);
}


/// @brief 读大端 32 位
static uint32_t get_u32_be_klb_webrtc_dc(const uint8_t* p_data)
{
    return ((uint32_t)p_data[0] << 24) | ((uint32_t)p_data[1] << 16)
         | ((uint32_t)p_data[2] << 8) | (uint32_t)p_data[3];
}


/// @brief 写大端 16 位
static void put_u16_be_klb_webrtc_dc(uint8_t* p_data, uint16_t val)
{
    p_data[0] = (uint8_t)((val >> 8) & 0xFF);
    p_data[1] = (uint8_t)(val & 0xFF);
}


/// @brief 写大端 32 位
static void put_u32_be_klb_webrtc_dc(uint8_t* p_data, uint32_t val)
{
    p_data[0] = (uint8_t)((val >> 24) & 0xFF);
    p_data[1] = (uint8_t)((val >> 16) & 0xFF);
    p_data[2] = (uint8_t)((val >> 8) & 0xFF);
    p_data[3] = (uint8_t)(val & 0xFF);
}


/// @brief CRC32c 增量计算
/// @param [in] crc         上一轮值; 首轮传 0xFFFFFFFF
/// @param [in] *p_data     数据
/// @param [in] len         长度
/// @return uint32_t 未取反的中间值
static uint32_t update_crc32c_klb_webrtc_dc(uint32_t crc, const uint8_t* p_data, int len)
{
    for (int i = 0; i < len; i++)
    {
        crc ^= (uint32_t)p_data[i];

        for (int bit = 0; bit < 8; bit++)
        {
            if (0 != (crc & 1u))
            {
                crc = (crc >> 1) ^ KLB_WEBRTC_DC_CRC32C_POLY;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}


/// @brief 计算整包 CRC32c (校验和字段视为 0)
static uint32_t calc_crc32c_klb_webrtc_dc(const uint8_t* p_data, int len)
{
    static const uint8_t zero[4] = { 0, 0, 0, 0 };

    uint32_t crc = update_crc32c_klb_webrtc_dc(0xFFFFFFFFu, p_data, 8);
    crc = update_crc32c_klb_webrtc_dc(crc, zero, 4);
    crc = update_crc32c_klb_webrtc_dc(crc, p_data + 12, len - 12);

    return crc ^ 0xFFFFFFFFu;
}


/// @brief 写入校验和 (小端)
static void seal_pkt_klb_webrtc_dc(uint8_t* p_pkt, int len)
{
    uint32_t crc = calc_crc32c_klb_webrtc_dc(p_pkt, len);

    p_pkt[8] = (uint8_t)(crc & 0xFF);
    p_pkt[9] = (uint8_t)((crc >> 8) & 0xFF);
    p_pkt[10] = (uint8_t)((crc >> 16) & 0xFF);
    p_pkt[11] = (uint8_t)((crc >> 24) & 0xFF);
}


/// @brief 校验整包 CRC32c
static bool check_pkt_klb_webrtc_dc(const uint8_t* p_pkt, int len)
{
    uint32_t stored = (uint32_t)p_pkt[8] | ((uint32_t)p_pkt[9] << 8)
        | ((uint32_t)p_pkt[10] << 16) | ((uint32_t)p_pkt[11] << 24);

    return (stored == calc_crc32c_klb_webrtc_dc(p_pkt, len)) ? true : false;
}


/// @brief 写 SCTP 公共头
/// @param [in] *p_dc       会话
/// @param [in] vtag        校验标签
/// @param [out] *p_out     输出缓冲
/// @param [in] cap         容量
/// @return int 大于 0.载荷起点; 小于 0.klb_netcode_e
static int begin_pkt_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, uint32_t vtag, uint8_t* p_out, int cap)
{
    if (KLB_WEBRTC_DC_COMMON_HEADER > cap)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    put_u16_be_klb_webrtc_dc(p_out + 0, p_dc->local_port);
    put_u16_be_klb_webrtc_dc(p_out + 2, p_dc->remote_port);
    put_u32_be_klb_webrtc_dc(p_out + 4, vtag);
    KLB_MEMSET(p_out + 8, 0, 4);

    return KLB_WEBRTC_DC_COMMON_HEADER;
}


/// @brief 追加一个 chunk 头
/// @param [out] *p_out     输出缓冲
/// @param [in] cap         容量
/// @param [in] pos         当前写入位置
/// @param [in] type        chunk 类型
/// @param [in] flags       chunk 标志
/// @param [in] payload_len 载荷长度
/// @return int 大于 0.载荷起点; 小于 0.klb_netcode_e
static int put_chunk_klb_webrtc_dc(uint8_t* p_out, int cap, int pos, int type, int flags, int payload_len)
{
    int total = KLB_WEBRTC_DC_CHUNK_HEADER + payload_len;
    int padded = (total + 3) & ~3;

    if ((0 > pos) || (cap < (pos + padded)))
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    p_out[pos + 0] = (uint8_t)type;
    p_out[pos + 1] = (uint8_t)flags;
    put_u16_be_klb_webrtc_dc(p_out + pos + 2, (uint16_t)total);

    KLB_MEMSET(p_out + pos + KLB_WEBRTC_DC_CHUNK_HEADER, 0,
        (size_t)(padded - KLB_WEBRTC_DC_CHUNK_HEADER));

    return pos + KLB_WEBRTC_DC_CHUNK_HEADER;
}


/// @brief 加入待发队列
/// @return int 0.成功; 小于 0.klb_netcode_e
static int queue_push_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, const uint8_t* p_data, int len)
{
    if ((0 > len) || (KLB_WEBRTC_DC_MTU < len))
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    if (KLB_WEBRTC_DC_QUEUE_SLOTS <= p_dc->count)
    {
        return KLB_NETCODE_LIMIT;
    }

    int idx = (p_dc->head + p_dc->count) % KLB_WEBRTC_DC_QUEUE_SLOTS;

    memcpy(p_dc->slots[idx].data, p_data, (size_t)len);
    p_dc->slots[idx].len = len;
    p_dc->count++;

    return 0;
}


/// @brief 构建 INIT / INIT_ACK
/// @return int 0.成功; 小于 0.klb_netcode_e
static int build_init_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, bool is_ack,
                                    uint8_t* p_out, int cap, int* p_out_len)
{
    uint32_t vtag = (is_ack ? p_dc->remote_vtag : 0u);

    int pos = begin_pkt_klb_webrtc_dc(p_dc, vtag, p_out, cap);
    if (0 > pos)
    {
        return pos;
    }

    int type = (is_ack ? KLB_WEBRTC_DC_CHUNK_INIT_ACK : KLB_WEBRTC_DC_CHUNK_INIT);
    pos = put_chunk_klb_webrtc_dc(p_out, cap, pos, type, 0, KLB_WEBRTC_DC_INIT_LEN);
    if (0 > pos)
    {
        return pos;
    }

    put_u32_be_klb_webrtc_dc(p_out + pos + 0, p_dc->local_vtag);
    put_u32_be_klb_webrtc_dc(p_out + pos + 4, KLB_WEBRTC_DC_ARWND);
    put_u16_be_klb_webrtc_dc(p_out + pos + 8, KLB_WEBRTC_DC_STREAMS);    // outbound streams
    put_u16_be_klb_webrtc_dc(p_out + pos + 10, KLB_WEBRTC_DC_STREAMS);   // inbound streams
    put_u32_be_klb_webrtc_dc(p_out + pos + 12, p_dc->local_tsn);

    *p_out_len = pos + KLB_WEBRTC_DC_INIT_LEN;
    seal_pkt_klb_webrtc_dc(p_out, *p_out_len);

    return 0;
}


/// @brief 构建 COOKIE_ECHO
/// @return int 0.成功; 小于 0.klb_netcode_e
static int build_cookie_echo_klb_webrtc_dc(klb_webrtc_dc_t* p_dc,
                                           uint8_t* p_out, int cap, int* p_out_len)
{
    int pos = begin_pkt_klb_webrtc_dc(p_dc, p_dc->remote_vtag, p_out, cap);
    if (0 > pos)
    {
        return pos;
    }

    pos = put_chunk_klb_webrtc_dc(p_out, cap, pos, KLB_WEBRTC_DC_CHUNK_COOKIE_ECHO,
        0, KLB_WEBRTC_DC_COOKIE_LEN);
    if (0 > pos)
    {
        return pos;
    }

    put_u32_be_klb_webrtc_dc(p_out + pos + 0, p_dc->local_vtag);
    put_u32_be_klb_webrtc_dc(p_out + pos + 4, p_dc->local_tsn);

    *p_out_len = pos + KLB_WEBRTC_DC_COOKIE_LEN;
    seal_pkt_klb_webrtc_dc(p_out, *p_out_len);

    return 0;
}


/// @brief 构建 COOKIE_ACK
/// @return int 0.成功; 小于 0.klb_netcode_e
static int build_cookie_ack_klb_webrtc_dc(klb_webrtc_dc_t* p_dc,
                                          uint8_t* p_out, int cap, int* p_out_len)
{
    int pos = begin_pkt_klb_webrtc_dc(p_dc, p_dc->remote_vtag, p_out, cap);
    if (0 > pos)
    {
        return pos;
    }

    pos = put_chunk_klb_webrtc_dc(p_out, cap, pos, KLB_WEBRTC_DC_CHUNK_COOKIE_ACK, 0, 0);
    if (0 > pos)
    {
        return pos;
    }

    *p_out_len = pos;
    seal_pkt_klb_webrtc_dc(p_out, *p_out_len);

    return 0;
}


/// @brief 构建一个 DATA chunk 报文
/// @return int 0.成功; 小于 0.klb_netcode_e
static int build_data_pkt_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, uint32_t tsn, int stream_id,
                                        uint16_t stream_seq, int flags, int ppid,
                                        const uint8_t* p_msg, int len,
                                        uint8_t* p_out, int cap, int* p_out_len)
{
    int payload_len = KLB_WEBRTC_DC_DATA_HEADER + len;

    int pos = begin_pkt_klb_webrtc_dc(p_dc, p_dc->remote_vtag, p_out, cap);
    if (0 > pos)
    {
        return pos;
    }

    pos = put_chunk_klb_webrtc_dc(p_out, cap, pos, KLB_WEBRTC_DC_CHUNK_DATA, flags, payload_len);
    if (0 > pos)
    {
        return pos;
    }

    put_u32_be_klb_webrtc_dc(p_out + pos + 0, tsn);
    put_u16_be_klb_webrtc_dc(p_out + pos + 4, (uint16_t)stream_id);
    put_u16_be_klb_webrtc_dc(p_out + pos + 6, stream_seq);
    put_u32_be_klb_webrtc_dc(p_out + pos + 8, (uint32_t)ppid);

    if (0 < len)
    {
        memcpy(p_out + pos + KLB_WEBRTC_DC_DATA_HEADER, p_msg, (size_t)len);
    }

    *p_out_len = pos + payload_len;
    seal_pkt_klb_webrtc_dc(p_out, *p_out_len);

    return 0;
}


/// @brief 记录已发 DATA (供 SACK 确认与 RTO 重传)
/// @return 无
static void record_sent_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, uint32_t tsn, int stream_id,
                                      uint16_t stream_seq, int flags, int ppid,
                                      const uint8_t* p_msg, int len)
{
    klb_webrtc_dc_sent_t* p_slot = &p_dc->sent[p_dc->sent_head];

    p_dc->sent_head = (p_dc->sent_head + 1) % KLB_WEBRTC_DC_SENT_SLOTS;

    KLB_FREE(p_slot->p_data);
    KLB_MEMSET(p_slot, 0, sizeof(klb_webrtc_dc_sent_t));

    int n = (0 < len) ? len : 1;
    p_slot->p_data = KLB_MALLOC(uint8_t, n, 0);
    if (NULL == p_slot->p_data)
    {
        return;
    }

    if (0 < len)
    {
        memcpy(p_slot->p_data, p_msg, (size_t)len);
    }

    p_slot->used = true;
    p_slot->retries = 0;
    p_slot->tsn = tsn;
    p_slot->stream_id = (uint16_t)stream_id;
    p_slot->stream_seq = stream_seq;
    p_slot->flags = flags;
    p_slot->ppid = ppid;
    p_slot->len = len;
    p_slot->sent_ms = klb_tick_count();
}


/// @brief 按 TSN 确认已发 DATA 并释放缓存
/// @return 无
static void ack_sent_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, uint32_t tsn)
{
    for (int i = 0; i < KLB_WEBRTC_DC_SENT_SLOTS; i++)
    {
        klb_webrtc_dc_sent_t* p_slot = &p_dc->sent[i];

        if ((!p_slot->used) || (0 < (int32_t)(p_slot->tsn - tsn)))
        {
            continue;
        }

        KLB_FREE(p_slot->p_data);
        p_slot->used = false;
        p_slot->retries = 0;
    }
}


/// @brief 构建并记录一个单包 DATA 消息 (DCEP 等短消息)
/// @return int 0.成功; 小于 0.klb_netcode_e
static int build_data_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, int stream_id, const uint8_t* p_msg,
                                    int len, int ppid, uint8_t* p_out, int cap, int* p_out_len)
{
    if ((0 > len) || ((0 == len) && (NULL == p_msg)))
    {
        return KLB_NETCODE_EINVAL;
    }

    if ((0 > stream_id) || (KLB_WEBRTC_DC_STREAMS <= stream_id))
    {
        return KLB_NETCODE_EINVAL;
    }

    int flags = KLB_WEBRTC_DC_FLAG_B | KLB_WEBRTC_DC_FLAG_E;
    uint16_t seq = p_dc->stream_seq[stream_id];
    uint32_t tsn = p_dc->local_tsn;

    int ret = build_data_pkt_klb_webrtc_dc(p_dc, tsn, stream_id, seq, flags, ppid,
        p_msg, len, p_out, cap, p_out_len);
    if (0 > ret)
    {
        return ret;
    }

    record_sent_klb_webrtc_dc(p_dc, tsn, stream_id, seq, flags, ppid, p_msg, len);

    p_dc->local_tsn++;
    p_dc->stream_seq[stream_id]++;

    return 0;
}


/// @brief 构建并排队 DCEP 消息
/// @return int 0.成功; 小于 0.klb_netcode_e
static int queue_dcep_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, int stream_id, int type)
{
    uint8_t msg[KLB_WEBRTC_DC_DCEP_OPEN_LEN];
    int msg_len = 1;

    KLB_MEMSET(msg, 0, sizeof(msg));
    msg[0] = (uint8_t)type;

    // DCEP OPEN 为 12 字节固定头: type/channel-type/priority/reliability/label-len/proto-len
    if (KLB_WEBRTC_DC_DCEP_OPEN == type)
    {
        msg_len = KLB_WEBRTC_DC_DCEP_OPEN_LEN;
    }

    uint8_t pkt[KLB_WEBRTC_DC_MTU];
    int out_len = 0;

    int ret = build_data_klb_webrtc_dc(p_dc, stream_id, msg, msg_len, KLB_WEBRTC_DC_PPID_DCEP,
        pkt, (int)sizeof(pkt), &out_len);
    if (0 > ret)
    {
        return ret;
    }

    return queue_push_klb_webrtc_dc(p_dc, pkt, out_len);
}


/// @brief 处理 INIT
/// @return int 0.成功; 小于 0.klb_netcode_e
static int input_init_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, const uint8_t* p_payload, int len)
{
    if (KLB_WEBRTC_DC_INIT_LEN > len)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    p_dc->remote_vtag = get_u32_be_klb_webrtc_dc(p_payload + 0);
    p_dc->state = KLB_WEBRTC_DC_STATE_COOKIE_WAIT;

    uint8_t pkt[KLB_WEBRTC_DC_MTU];
    int out_len = 0;

    int ret = build_init_klb_webrtc_dc(p_dc, true, pkt, (int)sizeof(pkt), &out_len);
    if (0 > ret)
    {
        return ret;
    }

    return queue_push_klb_webrtc_dc(p_dc, pkt, out_len);
}


/// @brief 处理 INIT_ACK
/// @return int 0.成功; 小于 0.klb_netcode_e
static int input_init_ack_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, const uint8_t* p_payload, int len)
{
    if (KLB_WEBRTC_DC_INIT_LEN > len)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    p_dc->remote_vtag = get_u32_be_klb_webrtc_dc(p_payload + 0);
    p_dc->state = KLB_WEBRTC_DC_STATE_COOKIE_ECHOED;

    uint8_t pkt[KLB_WEBRTC_DC_MTU];
    int out_len = 0;

    int ret = build_cookie_echo_klb_webrtc_dc(p_dc, pkt, (int)sizeof(pkt), &out_len);
    if (0 > ret)
    {
        return ret;
    }

    return queue_push_klb_webrtc_dc(p_dc, pkt, out_len);
}


/// @brief 处理 COOKIE_ECHO
/// @return int 0.成功; 小于 0.klb_netcode_e
static int input_cookie_echo_klb_webrtc_dc(klb_webrtc_dc_t* p_dc)
{
    p_dc->state = KLB_WEBRTC_DC_STATE_ESTABLISHED;

    uint8_t pkt[KLB_WEBRTC_DC_MTU];
    int out_len = 0;

    int ret = build_cookie_ack_klb_webrtc_dc(p_dc, pkt, (int)sizeof(pkt), &out_len);
    if (0 > ret)
    {
        return ret;
    }

    return queue_push_klb_webrtc_dc(p_dc, pkt, out_len);
}


/// @brief 处理 COOKIE_ACK
/// @return int 0.成功; 小于 0.klb_netcode_e
static int input_cookie_ack_klb_webrtc_dc(klb_webrtc_dc_t* p_dc)
{
    p_dc->state = KLB_WEBRTC_DC_STATE_ESTABLISHED;

    // 客户端在关联建立后为所有待打开流发 DCEP OPEN
    if (p_dc->is_client)
    {
        for (int stream_id = 0; stream_id < KLB_WEBRTC_DC_STREAMS; stream_id++)
        {
            uint32_t bit = (uint32_t)1 << stream_id;
            if (0 == (p_dc->want_mask & bit))
            {
                continue;
            }

            int ret = queue_dcep_klb_webrtc_dc(p_dc, stream_id, KLB_WEBRTC_DC_DCEP_OPEN);
            if (0 > ret)
            {
                return ret;
            }

            p_dc->want_mask &= ~bit;
        }

        p_dc->dcep_sent = true;
    }

    return 0;
}


/// @brief 追加分片数据到重组缓冲 (按需扩容)
/// @return int 0.成功; 小于 0.klb_netcode_e
static int append_rx_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, const uint8_t* p_data, int len)
{
    if (0 >= len)
    {
        return 0;
    }

    int need = p_dc->rx_len + len;
    if (KLB_WEBRTC_DC_MSG_MAX < need)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    if (p_dc->rx_cap < need)
    {
        int cap = (0 < p_dc->rx_cap) ? p_dc->rx_cap : 1024;
        while (cap < need)
        {
            cap *= 2;
        }

        if (KLB_WEBRTC_DC_MSG_MAX < cap)
        {
            cap = KLB_WEBRTC_DC_MSG_MAX;
        }

        uint8_t* p_new = KLB_MALLOC(uint8_t, cap, 0);
        if (NULL == p_new)
        {
            return KLB_NETCODE_ENOMEM;
        }

        if (NULL != p_dc->p_rx)
        {
            memcpy(p_new, p_dc->p_rx, (size_t)p_dc->rx_len);
            KLB_FREE(p_dc->p_rx);
        }

        p_dc->p_rx = p_new;
        p_dc->rx_cap = cap;
    }

    memcpy(p_dc->p_rx + p_dc->rx_len, p_data, (size_t)len);
    p_dc->rx_len = need;

    return 0;
}


/// @brief 重置分片重组状态
/// @return 无
static void reset_rx_klb_webrtc_dc(klb_webrtc_dc_t* p_dc)
{
    p_dc->rx_busy = false;
    p_dc->rx_len = 0;
    p_dc->rx_stream = 0;
    p_dc->rx_ppid = 0;
}


/// @brief 处理 DATA chunk
/// @return int 0.成功; 小于 0.klb_netcode_e
static int input_data_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, int flags, const uint8_t* p_payload,
                                    int len, uint8_t* p_msg, int cap, int* p_msg_len,
                                    int* p_stream_id)
{
    if (KLB_WEBRTC_DC_DATA_HEADER > len)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    int stream_id = (int)get_u16_be_klb_webrtc_dc(p_payload + 4);
    uint32_t ppid = get_u32_be_klb_webrtc_dc(p_payload + 8);
    const uint8_t* p_body = p_payload + KLB_WEBRTC_DC_DATA_HEADER;
    int blen = len - KLB_WEBRTC_DC_DATA_HEADER;

    bool begin = (0 != (flags & KLB_WEBRTC_DC_FLAG_B)) ? true : false;
    bool end = (0 != (flags & KLB_WEBRTC_DC_FLAG_E)) ? true : false;

    // DCEP: 必为单包消息
    if (KLB_WEBRTC_DC_PPID_DCEP == (int)ppid)
    {
        if ((!begin) || (!end) || (0 >= blen))
        {
            return 0;
        }

        if ((0 <= stream_id) && (KLB_WEBRTC_DC_STREAMS > stream_id))
        {
            if (KLB_WEBRTC_DC_DCEP_OPEN == p_body[0])
            {
                p_dc->open_mask |= (uint32_t)1 << stream_id;

                int ret = queue_dcep_klb_webrtc_dc(p_dc, stream_id, KLB_WEBRTC_DC_DCEP_ACK);
                if (0 > ret)
                {
                    return ret;
                }
            }
            else if (KLB_WEBRTC_DC_DCEP_ACK == p_body[0])
            {
                p_dc->open_mask |= (uint32_t)1 << stream_id;
            }
        }

        return 0;
    }

    // 应用消息: 分片重组
    if (begin)
    {
        reset_rx_klb_webrtc_dc(p_dc);

        p_dc->rx_busy = true;
        p_dc->rx_stream = stream_id;
        p_dc->rx_ppid = (int)ppid;
    }
    else if (!p_dc->rx_busy)
    {
        // 首片未到: 丢弃该分片
        return 0;
    }

    if (0 > append_rx_klb_webrtc_dc(p_dc, p_body, blen))
    {
        reset_rx_klb_webrtc_dc(p_dc);

        return KLB_NETCODE_EMSGSIZE;
    }

    if (!end)
    {
        return 0;
    }

    if (p_dc->rx_len > cap)
    {
        reset_rx_klb_webrtc_dc(p_dc);

        return KLB_NETCODE_EMSGSIZE;
    }

    if (0 < p_dc->rx_len)
    {
        memcpy(p_msg, p_dc->p_rx, (size_t)p_dc->rx_len);
    }

    *p_msg_len = p_dc->rx_len;
    if (NULL != p_stream_id)
    {
        *p_stream_id = p_dc->rx_stream;
    }

    reset_rx_klb_webrtc_dc(p_dc);

    return 0;
}


/// @brief 记录收到的 TSN
/// @param [in] *p_dc       会话
/// @param [in] tsn         收到的 TSN
/// @return bool true.新数据; false.重复数据
static bool track_tsn_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, uint32_t tsn)
{
    if (!p_dc->rx_started)
    {
        p_dc->rx_started = true;
        p_dc->rx_cum_tsn = tsn - 1;
    }

    int32_t diff = (int32_t)(tsn - p_dc->rx_cum_tsn);
    if (0 >= diff)
    {
        return false;
    }

    if (1 == diff)
    {
        p_dc->rx_cum_tsn = tsn;

        while (0 != (p_dc->rx_bits & 1u))
        {
            p_dc->rx_cum_tsn++;
            p_dc->rx_bits >>= 1;
        }

        return true;
    }

    if (KLB_WEBRTC_DC_SACK_BITS < diff)
    {
        return true;
    }

    uint64_t mask = (uint64_t)1 << (diff - 1);
    if (0 != (p_dc->rx_bits & mask))
    {
        return false;
    }

    p_dc->rx_bits |= mask;

    return true;
}


/// @brief 构建并排队 SACK
/// @return int 0.成功; 小于 0.klb_netcode_e
static int queue_sack_klb_webrtc_dc(klb_webrtc_dc_t* p_dc)
{
    uint16_t gap_start[KLB_WEBRTC_DC_SACK_GAPS];
    uint16_t gap_end[KLB_WEBRTC_DC_SACK_GAPS];
    int gaps = 0;

    for (int i = 0; i < KLB_WEBRTC_DC_SACK_BITS; )
    {
        if (0 == (p_dc->rx_bits & ((uint64_t)1 << i)))
        {
            i++;
            continue;
        }

        int j = i;
        while ((j < KLB_WEBRTC_DC_SACK_BITS) && (0 != (p_dc->rx_bits & ((uint64_t)1 << j))))
        {
            j++;
        }

        if (gaps < KLB_WEBRTC_DC_SACK_GAPS)
        {
            gap_start[gaps] = (uint16_t)(i + 1);
            gap_end[gaps] = (uint16_t)j;
            gaps++;
        }

        i = j;
    }

    int payload_len = 12 + gaps * 4;

    uint8_t pkt[KLB_WEBRTC_DC_MTU];
    int pos = begin_pkt_klb_webrtc_dc(p_dc, p_dc->remote_vtag, pkt, (int)sizeof(pkt));
    if (0 > pos)
    {
        return pos;
    }

    pos = put_chunk_klb_webrtc_dc(pkt, (int)sizeof(pkt), pos, KLB_WEBRTC_DC_CHUNK_SACK,
        0, payload_len);
    if (0 > pos)
    {
        return pos;
    }

    put_u32_be_klb_webrtc_dc(pkt + pos + 0, p_dc->rx_cum_tsn);
    put_u32_be_klb_webrtc_dc(pkt + pos + 4, KLB_WEBRTC_DC_ARWND);
    put_u16_be_klb_webrtc_dc(pkt + pos + 8, (uint16_t)gaps);
    put_u16_be_klb_webrtc_dc(pkt + pos + 10, 0);

    for (int i = 0; i < gaps; i++)
    {
        put_u16_be_klb_webrtc_dc(pkt + pos + 12 + i * 4, gap_start[i]);
        put_u16_be_klb_webrtc_dc(pkt + pos + 14 + i * 4, gap_end[i]);
    }

    int out_len = pos + payload_len;
    seal_pkt_klb_webrtc_dc(pkt, out_len);

    return queue_push_klb_webrtc_dc(p_dc, pkt, out_len);
}


/// @brief 处理 SACK chunk (确认已发 DATA)
/// @return int 0.成功; 小于 0.klb_netcode_e
static int input_sack_klb_webrtc_dc(klb_webrtc_dc_t* p_dc, const uint8_t* p_payload, int len)
{
    if (12 > len)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    uint32_t cum = get_u32_be_klb_webrtc_dc(p_payload + 0);
    ack_sent_klb_webrtc_dc(p_dc, cum);

    int gaps = (int)get_u16_be_klb_webrtc_dc(p_payload + 8);
    int off = 12;

    for (int i = 0; i < gaps; i++)
    {
        if ((off + 4) > len)
        {
            break;
        }

        uint16_t start = get_u16_be_klb_webrtc_dc(p_payload + off);
        uint16_t end = get_u16_be_klb_webrtc_dc(p_payload + off + 2);
        off += 4;

        for (int k = (int)start; k <= (int)end; k++)
        {
            ack_sent_klb_webrtc_dc(p_dc, cum + (uint32_t)k);
        }
    }

    return 0;
}


klb_webrtc_dc_t* klb_webrtc_dc_create(bool is_client, int* p_code)
{
    klb_webrtc_dc_t* p_dc = KLB_MALLOCZ(klb_webrtc_dc_t, 1, 0);
    if (NULL == p_dc)
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_ENOMEM;
        }

        return NULL;
    }

    p_dc->is_client = is_client;
    p_dc->state = KLB_WEBRTC_DC_STATE_CLOSED;
    p_dc->local_port = KLB_WEBRTC_DC_PORT;
    p_dc->remote_port = KLB_WEBRTC_DC_PORT;

    uint32_t seed = (uint32_t)klb_tick_count();
    p_dc->local_vtag = 0x4B4C4200u ^ (seed << 8);
    p_dc->local_tsn = 1000u + (seed & 0xFFu);

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_dc;
}


void klb_webrtc_dc_destroy(klb_webrtc_dc_t* p_dc)
{
    if (NULL == p_dc)
    {
        return;
    }

    for (int i = 0; i < KLB_WEBRTC_DC_SENT_SLOTS; i++)
    {
        KLB_FREE(p_dc->sent[i].p_data);
    }

    KLB_FREE(p_dc->p_rx);
    KLB_FREE(p_dc);
}


int klb_webrtc_dc_is_open(klb_webrtc_dc_t* p_dc)
{
    return klb_webrtc_dc_is_open_stream(p_dc, 0);
}


int klb_webrtc_dc_is_open_stream(klb_webrtc_dc_t* p_dc, int stream_id)
{
    if ((NULL == p_dc) || (0 > stream_id) || (KLB_WEBRTC_DC_STREAMS <= stream_id))
    {
        return 0;
    }

    if (KLB_WEBRTC_DC_STATE_ESTABLISHED != p_dc->state)
    {
        return 0;
    }

    return (0 != (p_dc->open_mask & ((uint32_t)1 << stream_id))) ? 1 : 0;
}


int klb_webrtc_dc_open(klb_webrtc_dc_t* p_dc)
{
    if (NULL == p_dc)
    {
        return KLB_NETCODE_EINVAL;
    }

    // 服务端等待对端 INIT
    if (!p_dc->is_client)
    {
        return 0;
    }

    if (KLB_WEBRTC_DC_STATE_CLOSED != p_dc->state)
    {
        return 0;
    }

    p_dc->want_mask |= 1u;
    p_dc->stream_id = 0;

    uint8_t pkt[KLB_WEBRTC_DC_MTU];
    int out_len = 0;

    int ret = build_init_klb_webrtc_dc(p_dc, false, pkt, (int)sizeof(pkt), &out_len);
    if (0 > ret)
    {
        return ret;
    }

    ret = queue_push_klb_webrtc_dc(p_dc, pkt, out_len);
    if (0 > ret)
    {
        return ret;
    }

    p_dc->state = KLB_WEBRTC_DC_STATE_COOKIE_WAIT;

    return 0;
}


int klb_webrtc_dc_open_stream(klb_webrtc_dc_t* p_dc, int stream_id)
{
    if ((NULL == p_dc) || (0 > stream_id) || (KLB_WEBRTC_DC_STREAMS <= stream_id))
    {
        return KLB_NETCODE_EINVAL;
    }

    if (!p_dc->is_client)
    {
        return 0;
    }

    uint32_t bit = (uint32_t)1 << stream_id;

    // 关联建立前记入待打开集合
    if (KLB_WEBRTC_DC_STATE_ESTABLISHED != p_dc->state)
    {
        p_dc->want_mask |= bit;

        return 0;
    }

    if (0 != (p_dc->open_mask & bit))
    {
        return 0;
    }

    return queue_dcep_klb_webrtc_dc(p_dc, stream_id, KLB_WEBRTC_DC_DCEP_OPEN);
}


int klb_webrtc_dc_output_stream(klb_webrtc_dc_t* p_dc, int stream_id,
                                const uint8_t* p_msg, int len, int ppid,
                                uint8_t* p_out, int cap, int* p_out_len)
{
    if ((NULL == p_dc) || ((0 < len) && (NULL == p_msg)) || (NULL == p_out)
        || (NULL == p_out_len))
    {
        return KLB_NETCODE_EINVAL;
    }

    if ((0 > stream_id) || (KLB_WEBRTC_DC_STREAMS <= stream_id))
    {
        return KLB_NETCODE_EINVAL;
    }

    if (KLB_WEBRTC_DC_STATE_ESTABLISHED != p_dc->state)
    {
        return KLB_NETCODE_EAGAIN;
    }

    if ((0 > len) || (KLB_WEBRTC_DC_MSG_MAX < len))
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    int frag_max = KLB_WEBRTC_DC_MTU - KLB_WEBRTC_DC_COMMON_HEADER
        - KLB_WEBRTC_DC_CHUNK_HEADER - KLB_WEBRTC_DC_DATA_HEADER;

    // 单片消息: 直接构建
    if (len <= frag_max)
    {
        return build_data_klb_webrtc_dc(p_dc, stream_id, p_msg, len, ppid, p_out, cap, p_out_len);
    }

    int frag_num = (len + frag_max - 1) / frag_max;

    // 首片走调用方缓冲, 余片须有足够队列槽位
    if ((frag_num - 1) > (KLB_WEBRTC_DC_QUEUE_SLOTS - p_dc->count))
    {
        return KLB_NETCODE_LIMIT;
    }

    int off = 0;

    for (int i = 0; i < frag_num; i++)
    {
        int n = len - off;
        if (frag_max < n)
        {
            n = frag_max;
        }

        int flags = 0;
        if (0 == i)
        {
            flags |= KLB_WEBRTC_DC_FLAG_B;
        }

        if ((frag_num - 1) == i)
        {
            flags |= KLB_WEBRTC_DC_FLAG_E;
        }

        uint16_t seq = p_dc->stream_seq[stream_id];
        uint32_t tsn = p_dc->local_tsn;

        uint8_t pkt[KLB_WEBRTC_DC_MTU];
        uint8_t* p_dst = (0 == i) ? p_out : pkt;
        int dst_cap = (0 == i) ? cap : (int)sizeof(pkt);
        int dst_len = 0;

        int ret = build_data_pkt_klb_webrtc_dc(p_dc, tsn, stream_id, seq, flags, ppid,
            p_msg + off, n, p_dst, dst_cap, &dst_len);
        if (0 > ret)
        {
            return ret;
        }

        record_sent_klb_webrtc_dc(p_dc, tsn, stream_id, seq, flags, ppid, p_msg + off, n);

        p_dc->local_tsn++;
        p_dc->stream_seq[stream_id]++;

        if (0 == i)
        {
            *p_out_len = dst_len;
        }
        else
        {
            ret = queue_push_klb_webrtc_dc(p_dc, pkt, dst_len);
            if (0 > ret)
            {
                return ret;
            }
        }

        off += n;
    }

    return 0;
}


int klb_webrtc_dc_output(klb_webrtc_dc_t* p_dc, const uint8_t* p_msg, int len, int ppid,
                         uint8_t* p_out, int cap, int* p_out_len)
{
    if (NULL == p_dc)
    {
        return KLB_NETCODE_EINVAL;
    }

    return klb_webrtc_dc_output_stream(p_dc, p_dc->stream_id, p_msg, len, ppid,
        p_out, cap, p_out_len);
}


int klb_webrtc_dc_input_ex(klb_webrtc_dc_t* p_dc, const uint8_t* p_data, int len,
                           uint8_t* p_msg, int cap, int* p_msg_len, int* p_stream_id)
{
    if ((NULL == p_dc) || (NULL == p_data) || (KLB_WEBRTC_DC_COMMON_HEADER > len))
    {
        return KLB_NETCODE_EINVAL;
    }

    if (NULL != p_msg_len)
    {
        *p_msg_len = 0;
    }

    if (!check_pkt_klb_webrtc_dc(p_data, len))
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    uint32_t vtag = get_u32_be_klb_webrtc_dc(p_data + 4);
    int pos = KLB_WEBRTC_DC_COMMON_HEADER;

    while ((pos + KLB_WEBRTC_DC_CHUNK_HEADER) <= len)
    {
        int type = p_data[pos + 0];
        int flags = p_data[pos + 1];
        int clen = (int)get_u16_be_klb_webrtc_dc(p_data + pos + 2);

        if ((KLB_WEBRTC_DC_CHUNK_HEADER > clen) || ((pos + clen) > len))
        {
            return KLB_NETCODE_PROTO_PARSE;
        }

        const uint8_t* p_payload = p_data + pos + KLB_WEBRTC_DC_CHUNK_HEADER;
        int plen = clen - KLB_WEBRTC_DC_CHUNK_HEADER;
        int ret = 0;

        switch (type)
        {
        case KLB_WEBRTC_DC_CHUNK_INIT:
            ret = input_init_klb_webrtc_dc(p_dc, p_payload, plen);
            break;
        case KLB_WEBRTC_DC_CHUNK_INIT_ACK:
            ret = input_init_ack_klb_webrtc_dc(p_dc, p_payload, plen);
            break;
        case KLB_WEBRTC_DC_CHUNK_COOKIE_ECHO:
            ret = input_cookie_echo_klb_webrtc_dc(p_dc);
            break;
        case KLB_WEBRTC_DC_CHUNK_COOKIE_ACK:
            ret = input_cookie_ack_klb_webrtc_dc(p_dc);
            break;
        case KLB_WEBRTC_DC_CHUNK_DATA:
            if ((0 != p_dc->remote_vtag) && (vtag != p_dc->local_vtag))
            {
                break;
            }

            if (track_tsn_klb_webrtc_dc(p_dc, get_u32_be_klb_webrtc_dc(p_payload + 0)))
            {
                ret = input_data_klb_webrtc_dc(p_dc, flags, p_payload, plen,
                    p_msg, cap, p_msg_len, p_stream_id);
            }

            // 每个新 DATA 回一个 SACK
            (void)queue_sack_klb_webrtc_dc(p_dc);
            break;
        case KLB_WEBRTC_DC_CHUNK_SACK:
            if ((0 != p_dc->remote_vtag) && (vtag != p_dc->local_vtag))
            {
                break;
            }

            ret = input_sack_klb_webrtc_dc(p_dc, p_payload, plen);
            break;
        default:
            break;
        }

        if (0 > ret)
        {
            return ret;
        }

        pos += (clen + 3) & ~3;
    }

    return 0;
}


int klb_webrtc_dc_input(klb_webrtc_dc_t* p_dc, const uint8_t* p_data, int len,
                        uint8_t* p_msg, int cap, int* p_msg_len)
{
    return klb_webrtc_dc_input_ex(p_dc, p_data, len, p_msg, cap, p_msg_len, NULL);
}


int klb_webrtc_dc_poll(klb_webrtc_dc_t* p_dc, uint8_t* p_out, int cap, int* p_out_len)
{
    if ((NULL == p_dc) || (NULL == p_out) || (NULL == p_out_len))
    {
        return KLB_NETCODE_EINVAL;
    }

    *p_out_len = 0;

    if (0 < p_dc->count)
    {
        klb_webrtc_dc_pkt_t* p_pkt = &p_dc->slots[p_dc->head];
        int n = (p_pkt->len > cap) ? cap : p_pkt->len;

        memcpy(p_out, p_pkt->data, (size_t)n);
        *p_out_len = n;

        p_dc->head = (p_dc->head + 1) % KLB_WEBRTC_DC_QUEUE_SLOTS;
        p_dc->count--;

        return 0;
    }

    // 队列空: 扫描已发缓存, 超时重传或放弃
    uint32_t now = (uint32_t)klb_tick_count();

    for (int i = 0; i < KLB_WEBRTC_DC_SENT_SLOTS; i++)
    {
        klb_webrtc_dc_sent_t* p_slot = &p_dc->sent[i];

        if (!p_slot->used)
        {
            continue;
        }

        if (KLB_WEBRTC_DC_RTO_MAX <= p_slot->retries)
        {
            KLB_FREE(p_slot->p_data);
            p_slot->used = false;
            continue;
        }

        if ((uint32_t)(now - p_slot->sent_ms) < (uint32_t)KLB_WEBRTC_DC_RTO_MS)
        {
            continue;
        }

        int ret = build_data_pkt_klb_webrtc_dc(p_dc, p_slot->tsn, (int)p_slot->stream_id,
            p_slot->stream_seq, p_slot->flags, p_slot->ppid,
            p_slot->p_data, p_slot->len, p_out, cap, p_out_len);
        if (0 > ret)
        {
            return ret;
        }

        p_slot->sent_ms = now;
        p_slot->retries++;

        return 0;
    }

    return 0;
}


// end
