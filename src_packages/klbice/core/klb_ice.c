// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_ice.c
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   ICE 代理实现 (RFC 8445; host/srflx/relay 候选 + STUN/TURN)
/// @version 0.4
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.2 实现 host 候选收集 / STUN Binding / 候选对检查; 0.3 增加 TURN 客户端 / srflx、relay 候选 / ICE restart
/// @warning TURN 为同步最小实现; IPv6 候选未接入
///////////////////////////////////////////////////////////////////////////
#include "klbice/core/klb_ice.h"
#include "klbnet/klb_netcode.h"

#if defined(__KLB_MBEDTLS__)

#include "klbnet/klb_socket_udp.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rand.h"
#include "klbplatform/klb_time.h"
#include "klbplatform/klb_thread.h"
#include "mbedtls/md.h"
#include "mbedtls/md5.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#endif


//////////////////////////////////////////////////////////////////////////
// 前置定义
//////////////////////////////////////////////////////////////////////////

/// @def   KLB_ICE_MAX_LOCAL
/// @brief 本地候选上限 (host / srflx / relay)
#define KLB_ICE_MAX_LOCAL              16

/// @def   KLB_ICE_MAX_REMOTE
/// @brief 远端候选上限
#define KLB_ICE_MAX_REMOTE             16

/// @def   KLB_ICE_MAX_PAIR
/// @brief 候选对上限 (本地 x 远端)
#define KLB_ICE_MAX_PAIR               (KLB_ICE_MAX_LOCAL * KLB_ICE_MAX_REMOTE)

/// @def   KLB_ICE_UFRAG_LEN
/// @brief 本地 ufrag 长度 (4~8 字符; 取 8)
#define KLB_ICE_UFRAG_LEN              8

/// @def   KLB_ICE_PWD_LEN
/// @brief 本地 pwd 长度 (22 字符)
#define KLB_ICE_PWD_LEN                22

/// @def   KLB_ICE_STUN_BUF
/// @brief STUN 收发缓冲 (MTU 内)
#define KLB_ICE_STUN_BUF               1500

/// @def   KLB_ICE_STUN_HEADER_LEN
/// @brief STUN 头部长度
#define KLB_ICE_STUN_HEADER_LEN        20

/// @def   KLB_ICE_TXID_LEN
/// @brief STUN 事务 ID 长度
#define KLB_ICE_TXID_LEN               12

/// @def   KLB_ICE_STUN_COOKIE
/// @brief STUN magic cookie
#define KLB_ICE_STUN_COOKIE            0x2112A442u

/// @def   KLB_ICE_RETRANS_MS
/// @brief 连通性检查重发间隔 (毫秒)
#define KLB_ICE_RETRANS_MS             500

/// @def   KLB_ICE_RETRANS_MAX
/// @brief 连通性检查最多发送次数
#define KLB_ICE_RETRANS_MAX            7

/// @def   KLB_ICE_HOST_TYPE_PREF
/// @brief host 候选类型优先级 (RFC 8445)
#define KLB_ICE_HOST_TYPE_PREF         126u

/// @def   KLB_ICE_SRFLX_TYPE_PREF
/// @brief srflx 候选类型优先级 (RFC 8445)
#define KLB_ICE_SRFLX_TYPE_PREF        100u

/// @def   KLB_ICE_RELAY_TYPE_PREF
/// @brief relay 候选类型优先级 (RFC 8445)
#define KLB_ICE_RELAY_TYPE_PREF        0u

/// @def   KLB_ICE_LOCAL_PREF
/// @brief 本地优先级 (RFC 8445)
#define KLB_ICE_LOCAL_PREF             65535u

/// @def   KLB_ICE_HOST_PRIORITY
/// @brief host 候选优先级 (RFC 8445 公式)
#define KLB_ICE_HOST_PRIORITY          ((KLB_ICE_HOST_TYPE_PREF << 24) \
                                        | (KLB_ICE_LOCAL_PREF << 8) | 255u)

/// @def   KLB_ICE_SRFLX_PRIORITY
/// @brief srflx 候选优先级 (RFC 8445 公式)
#define KLB_ICE_SRFLX_PRIORITY         ((KLB_ICE_SRFLX_TYPE_PREF << 24) \
                                        | (KLB_ICE_LOCAL_PREF << 8) | 255u)

/// @def   KLB_ICE_RELAY_PRIORITY
/// @brief relay 候选优先级 (RFC 8445 公式)
#define KLB_ICE_RELAY_PRIORITY         ((KLB_ICE_RELAY_TYPE_PREF << 24) \
                                        | (KLB_ICE_LOCAL_PREF << 8) | 255u)

/// @def   KLB_ICE_STUN_TYPE_REQUEST
/// @brief Binding Request 消息类型
#define KLB_ICE_STUN_TYPE_REQUEST      0x0001

/// @def   KLB_ICE_STUN_TYPE_SUCCESS
/// @brief Binding Success Response 消息类型
#define KLB_ICE_STUN_TYPE_SUCCESS      0x0101

/// @def   KLB_ICE_STUN_TYPE_ERROR
/// @brief Binding Error Response 消息类型
#define KLB_ICE_STUN_TYPE_ERROR        0x0111

/// @def   KLB_ICE_ATTR_USERNAME
/// @brief STUN 属性 USERNAME
#define KLB_ICE_ATTR_USERNAME          0x0006

/// @def   KLB_ICE_ATTR_MI
/// @brief STUN 属性 MESSAGE-INTEGRITY
#define KLB_ICE_ATTR_MI                0x0008

/// @def   KLB_ICE_ATTR_XOR_MAPPED
/// @brief STUN 属性 XOR-MAPPED-ADDRESS
#define KLB_ICE_ATTR_XOR_MAPPED        0x0020

/// @def   KLB_ICE_ATTR_PRIORITY
/// @brief STUN 属性 PRIORITY
#define KLB_ICE_ATTR_PRIORITY          0x0024

/// @def   KLB_ICE_ATTR_USE_CANDIDATE
/// @brief STUN 属性 USE-CANDIDATE
#define KLB_ICE_ATTR_USE_CANDIDATE     0x0025

/// @def   KLB_ICE_ATTR_FINGERPRINT
/// @brief STUN 属性 FINGERPRINT
#define KLB_ICE_ATTR_FINGERPRINT       0x8028

/// @def   KLB_ICE_ATTR_ICE_CONTROLLED
/// @brief STUN 属性 ICE-CONTROLLED
#define KLB_ICE_ATTR_ICE_CONTROLLED    0x8029

/// @def   KLB_ICE_ATTR_ICE_CONTROLLING
/// @brief STUN 属性 ICE-CONTROLLING
#define KLB_ICE_ATTR_ICE_CONTROLLING   0x802A

/// @def   KLB_ICE_FINGERPRINT_XOR
/// @brief FINGERPRINT 异或常量 ("STUN")
#define KLB_ICE_FINGERPRINT_XOR        0x5354554Eu

/// @def   KLB_ICE_REMOTE_CAND_MAX
/// @brief 远端 ufrag/pwd 缓存长度
#define KLB_ICE_REMOTE_CRED_MAX        128

/// @def   KLB_ICE_ATTR_ERROR_CODE
/// @brief STUN 属性 ERROR-CODE
#define KLB_ICE_ATTR_ERROR_CODE        0x0009

/// @def   KLB_ICE_ATTR_CHANNEL_NUMBER
/// @brief TURN 属性 CHANNEL-NUMBER
#define KLB_ICE_ATTR_CHANNEL_NUMBER    0x000C

/// @def   KLB_ICE_ATTR_LIFETIME
/// @brief TURN 属性 LIFETIME
#define KLB_ICE_ATTR_LIFETIME          0x000D

/// @def   KLB_ICE_ATTR_XOR_PEER
/// @brief TURN 属性 XOR-PEER-ADDRESS
#define KLB_ICE_ATTR_XOR_PEER          0x0012

/// @def   KLB_ICE_ATTR_DATA
/// @brief TURN 属性 DATA
#define KLB_ICE_ATTR_DATA              0x0013

/// @def   KLB_ICE_ATTR_REALM
/// @brief TURN 属性 REALM
#define KLB_ICE_ATTR_REALM             0x0014

/// @def   KLB_ICE_ATTR_NONCE
/// @brief TURN 属性 NONCE
#define KLB_ICE_ATTR_NONCE             0x0015

/// @def   KLB_ICE_ATTR_XOR_RELAYED
/// @brief TURN 属性 XOR-RELAYED-ADDRESS
#define KLB_ICE_ATTR_XOR_RELAYED       0x0016

/// @def   KLB_ICE_ATTR_REQUESTED_TRANSPORT
/// @brief TURN 属性 REQUESTED-TRANSPORT
#define KLB_ICE_ATTR_REQUESTED_TRANSPORT 0x0019

/// @def   KLB_ICE_STUN_TYPE_ALLOCATE_REQ
/// @brief TURN Allocate 请求
#define KLB_ICE_STUN_TYPE_ALLOCATE_REQ 0x0003

/// @def   KLB_ICE_STUN_TYPE_ALLOCATE_OK
/// @brief TURN Allocate 成功应答
#define KLB_ICE_STUN_TYPE_ALLOCATE_OK  0x0103

/// @def   KLB_ICE_STUN_TYPE_ALLOCATE_ERR
/// @brief TURN Allocate 错误应答
#define KLB_ICE_STUN_TYPE_ALLOCATE_ERR 0x0113

/// @def   KLB_ICE_STUN_TYPE_REFRESH_REQ
/// @brief TURN Refresh 请求
#define KLB_ICE_STUN_TYPE_REFRESH_REQ  0x0004

/// @def   KLB_ICE_STUN_TYPE_REFRESH_OK
/// @brief TURN Refresh 成功应答
#define KLB_ICE_STUN_TYPE_REFRESH_OK   0x0104

/// @def   KLB_ICE_STUN_TYPE_PERM_REQ
/// @brief TURN CreatePermission 请求
#define KLB_ICE_STUN_TYPE_PERM_REQ     0x0008

/// @def   KLB_ICE_STUN_TYPE_PERM_OK
/// @brief TURN CreatePermission 成功应答
#define KLB_ICE_STUN_TYPE_PERM_OK      0x0108

/// @def   KLB_ICE_STUN_TYPE_CHBIND_REQ
/// @brief TURN ChannelBind 请求
#define KLB_ICE_STUN_TYPE_CHBIND_REQ   0x0009

/// @def   KLB_ICE_STUN_TYPE_CHBIND_OK
/// @brief TURN ChannelBind 成功应答
#define KLB_ICE_STUN_TYPE_CHBIND_OK    0x0109

/// @def   KLB_ICE_STUN_TYPE_SEND_IND
/// @brief TURN Send Indication
#define KLB_ICE_STUN_TYPE_SEND_IND     0x0016

/// @def   KLB_ICE_STUN_TYPE_DATA_IND
/// @brief TURN Data Indication
#define KLB_ICE_STUN_TYPE_DATA_IND     0x0017

/// @def   KLB_ICE_TURN_MSG_MAX
/// @brief TURN 收发缓冲
#define KLB_ICE_TURN_MSG_MAX           2048

/// @def   KLB_ICE_TURN_TIMEOUT_MS
/// @brief TURN 单次事务超时 (毫秒)
#define KLB_ICE_TURN_TIMEOUT_MS        800

/// @def   KLB_ICE_TURN_RETRY
/// @brief TURN 事务重试次数
#define KLB_ICE_TURN_RETRY             3

/// @def   KLB_ICE_TURN_POLL_MS
/// @brief TURN 收包轮询间隔 (毫秒)
#define KLB_ICE_TURN_POLL_MS           5

/// @def   KLB_ICE_TURN_CHANNEL_MIN
/// @brief TURN 通道号下限
#define KLB_ICE_TURN_CHANNEL_MIN       0x4000

/// @def   KLB_ICE_TURN_CHANNEL_MAX
/// @brief TURN 通道号上限
#define KLB_ICE_TURN_CHANNEL_MAX       0x7FFF

/// @def   KLB_ICE_TURN_MAX_PEER
/// @brief TURN 已授权对端上限
#define KLB_ICE_TURN_MAX_PEER          8


/// @struct klb_ice_local_t
/// @brief  本地候选 (host / srflx / relay)
typedef struct klb_ice_local_t_
{
    char            ip[64];         ///< 候选地址文本
    int             port;           ///< 候选端口 (主机序)
    uint32_t        priority;       ///< 候选优先级
    int             type;           ///< klb_ice_cand_type_e
    bool            owns_socket;    ///< 是否由本候选负责销毁 socket
    klb_socket_t*   p_socket;       ///< 绑定该地址的 UDP socket
    klb_ice_turn_t* p_turn;         ///< relay 候选所用 TURN 会话; host/srflx 为 NULL
}klb_ice_local_t;


/// @struct klb_ice_turn_peer_t
/// @brief  TURN 已授权对端
typedef struct klb_ice_turn_peer_t_
{
    char            ip[64];         ///< 对端 IP
    int             port;           ///< 对端端口
    int             channel;        ///< 通道号; 0 未绑定
}klb_ice_turn_peer_t;


/// @struct klb_ice_turn_t_
/// @brief  TURN 会话 (对外不透明)
struct klb_ice_turn_t_
{
    klb_socket_t*       p_socket;       ///< 与服务器通信的 UDP socket
    struct sockaddr_in  server;         ///< 服务器地址
    char                username[64];   ///< 长期凭据用户名
    char                password[64];   ///< 长期凭据密码
    char                realm[128];     ///< REALM
    char                nonce[128];     ///< NONCE
    uint8_t             key[16];        ///< MD5(username:realm:password)
    bool                key_valid;      ///< 长期凭据 key 已就绪
    char                relay_ip[64];   ///< 中继地址
    int                 relay_port;     ///< 中继端口
    int                 lifetime;       ///< 分配生命周期 (秒)
    int                 next_channel;   ///< 下一个可用通道号
    int                 peer_count;     ///< 已授权对端数
    klb_ice_turn_peer_t peers[KLB_ICE_TURN_MAX_PEER];   ///< 对端表
};


/// @struct klb_ice_remote_t
/// @brief  远端候选
typedef struct klb_ice_remote_t_
{
    char                foundation[32]; ///< foundation
    int                 component;      ///< 分量号
    uint32_t            priority;       ///< 候选优先级
    int                 type;           ///< klb_ice_cand_type_e
    char                ip[64];         ///< IPv4 文本
    int                 port;           ///< 端口 (主机序)
    struct sockaddr_in  addr;           ///< 已解析地址
}klb_ice_remote_t;


/// @struct klb_ice_pair_t
/// @brief  候选对
typedef struct klb_ice_pair_t_
{
    int             local_idx;          ///< 本地候选下标
    int             remote_idx;         ///< 远端候选下标
    uint64_t        priority;           ///< 对优先级
    bool            use_candidate;      ///< 已发 USE-CANDIDATE
    bool            succeeded;          ///< 连通性检查成功
    bool            abandoned;          ///< 重发耗尽
    bool            selected;           ///< 已选为工作对
    int             retrans;            ///< 已发送次数
    uint32_t        last_send_ms;       ///< 上次发送滴答
    uint8_t         txid[KLB_ICE_TXID_LEN]; ///< 最近事务 ID
    char            mapped_ip[64];      ///< XOR-MAPPED-ADDRESS IP
    int             mapped_port;        ///< XOR-MAPPED-ADDRESS 端口
}klb_ice_pair_t;


/// @struct klb_ice_agent_t
/// @brief  ICE 代理内部结构
struct klb_ice_agent_t_
{
    klb_netmulti_t* p_netmulti;         ///< 复用模块 (仅保存)
    int             role;               ///< klb_ice_role_e
    int             state;              ///< klb_ice_state_e
    uint64_t        tie_breaker;        ///< 角色冲突裁决
    char            local_ufrag[16];    ///< 本地 ufrag
    char            local_pwd[32];      ///< 本地 pwd
    char            remote_ufrag[KLB_ICE_REMOTE_CRED_MAX]; ///< 远端 ufrag
    char            remote_pwd[KLB_ICE_REMOTE_CRED_MAX];   ///< 远端 pwd
    klb_ice_local_t locals[KLB_ICE_MAX_LOCAL];   ///< 本地候选集
    int             local_count;        ///< 本地候选数
    klb_ice_remote_t remotes[KLB_ICE_MAX_REMOTE]; ///< 远端候选集
    int             remote_count;       ///< 远端候选数
    klb_ice_pair_t  pairs[KLB_ICE_MAX_PAIR];      ///< 候选对集
    int             pair_count;         ///< 候选对数
    int             selected;           ///< 选中对下标; -1 无
    char            stun_host[128];     ///< STUN 服务器 (srflx 收集); 空表未配置
    int             stun_port;          ///< STUN 端口
    char            turn_host[128];     ///< TURN 服务器 (relay 收集); 空表未配置
    int             turn_port;          ///< TURN 端口
    char            turn_user[64];      ///< TURN 用户名
    char            turn_pass[64];      ///< TURN 密码
    uint64_t        stat_check_sent;    ///< 连通性检查发送数
    uint64_t        stat_check_recv;    ///< 连通性检查接收数
    uint64_t        stat_data_sent_pkts;    ///< 应用数据发送包数
    uint64_t        stat_data_sent_bytes;   ///< 应用数据发送字节数
    uint64_t        stat_data_recv_pkts;    ///< 应用数据接收包数
    uint64_t        stat_data_recv_bytes;   ///< 应用数据接收字节数
};


//////////////////////////////////////////////////////////////////////////
// 基础工具
//////////////////////////////////////////////////////////////////////////

/// @brief 写入 16 位大端
static void put_u16_klb_ice(uint8_t* p_buf, uint16_t val)
{
    p_buf[0] = (uint8_t)(val >> 8);
    p_buf[1] = (uint8_t)(val & 0xFF);
}


/// @brief 读取 16 位大端
static uint16_t get_u16_klb_ice(const uint8_t* p_buf)
{
    return (uint16_t)(((uint16_t)p_buf[0] << 8) | (uint16_t)p_buf[1]);
}


/// @brief 写入 32 位大端
static void put_u32_klb_ice(uint8_t* p_buf, uint32_t val)
{
    p_buf[0] = (uint8_t)(val >> 24);
    p_buf[1] = (uint8_t)((val >> 16) & 0xFF);
    p_buf[2] = (uint8_t)((val >> 8) & 0xFF);
    p_buf[3] = (uint8_t)(val & 0xFF);
}


/// @brief 读取 32 位大端
static uint32_t get_u32_klb_ice(const uint8_t* p_buf)
{
    return ((uint32_t)p_buf[0] << 24) | ((uint32_t)p_buf[1] << 16)
        | ((uint32_t)p_buf[2] << 8) | (uint32_t)p_buf[3];
}


/// @brief 写入 64 位大端
static void put_u64_klb_ice(uint8_t* p_buf, uint64_t val)
{
    put_u32_klb_ice(p_buf, (uint32_t)(val >> 32));
    put_u32_klb_ice(p_buf + 4, (uint32_t)(val & 0xFFFFFFFFu));
}


/// @brief 字符串忽略大小写比较
/// @return bool true.相等; false.不等
static bool str_ieq_klb_ice(const char* p_a, const char* p_b)
{
    while ((0 != *p_a) && (0 != *p_b))
    {
        if (tolower((unsigned char)*p_a) != tolower((unsigned char)*p_b))
        {
            return false;
        }

        p_a++;
        p_b++;
    }

    return (0 == *p_a) && (0 == *p_b);
}


/// @brief 拷贝一个空白分隔的 token
/// @param [out] *p_dest    输出缓存
/// @param [in]  cap        缓存大小
/// @param [in]  *p_src     源串 (允许前导空白)
/// @return int 大于等于 0.长度; 小于 0.klb_netcode_e
static int copy_token_klb_ice(char* p_dest, int cap, const char* p_src)
{
    while ((0 != *p_src) && (0 != isspace((unsigned char)*p_src)))
    {
        p_src++;
    }

    int len = 0;
    while ((0 != *p_src) && (0 == isspace((unsigned char)*p_src)))
    {
        if ((len + 1) >= cap)
        {
            return KLB_NETCODE_MSGSIZE;
        }

        p_dest[len] = *p_src;
        len++;
        p_src++;
    }

    p_dest[len] = '\0';

    return len;
}


//////////////////////////////////////////////////////////////////////////
// CRC32 / HMAC-SHA1
//////////////////////////////////////////////////////////////////////////

static uint32_t s_crc32_table[256];
static bool s_crc32_ready = false;


/// @brief 初始化 CRC32 表 (IEEE 802.3, 反射多项式 0xEDB88320)
static void crc32_init_klb_ice(void)
{
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t crc = i;

        for (int k = 0; k < 8; k++)
        {
            if (0 != (crc & 1u))
            {
                crc = 0xEDB88320u ^ (crc >> 1);
            }
            else
            {
                crc = crc >> 1;
            }
        }

        s_crc32_table[i] = crc;
    }

    s_crc32_ready = true;
}


/// @brief 计算 CRC32
/// @param [in] *p_data     数据
/// @param [in] len         长度
/// @return uint32_t CRC32 值
static uint32_t crc32_klb_ice(const uint8_t* p_data, int len)
{
    if (!s_crc32_ready)
    {
        crc32_init_klb_ice();
    }

    uint32_t crc = 0xFFFFFFFFu;
    for (int i = 0; i < len; i++)
    {
        crc = s_crc32_table[(crc ^ p_data[i]) & 0xFFu] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFFu;
}


/// @brief 计算 HMAC-SHA1
/// @param [in]  *p_key     key 文本; NULL 视为空
/// @param [in]  *p_data    输入
/// @param [in]  len        输入长度
/// @param [out] *p_out     20 字节输出
/// @return int 0.成功; 小于 0.klb_netcode_e
static int hmac_sha1_klb_ice(const char* p_key, const uint8_t* p_data, int len, uint8_t* p_out)
{
    const mbedtls_md_info_t* p_md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (NULL == p_md)
    {
        return KLB_NETCODE_NOTSUP;
    }

    static const uint8_t s_empty = 0;
    const uint8_t* p_key_data = (const uint8_t*)p_key;
    size_t key_len = 0;

    if (NULL != p_key)
    {
        key_len = strlen(p_key);
    }
    else
    {
        p_key_data = &s_empty;
    }

    int ret = mbedtls_md_hmac(p_md, p_key_data, key_len, p_data, (size_t)len, p_out);
    if (0 != ret)
    {
        return KLB_NETCODE_ERR;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// STUN 编码
//////////////////////////////////////////////////////////////////////////

/// @brief 写 STUN 头部 (length 先置 0)
static void stun_put_header_klb_ice(uint8_t* p_buf, uint16_t type, const uint8_t* p_txid)
{
    put_u16_klb_ice(p_buf, type);
    put_u16_klb_ice(p_buf + 2, 0);
    put_u32_klb_ice(p_buf + 4, KLB_ICE_STUN_COOKIE);
    memcpy(p_buf + 8, p_txid, KLB_ICE_TXID_LEN);
}


/// @brief 追加一个 STUN 属性 (含 4 字节对齐填充)
/// @param [in]  *p_buf     消息缓冲
/// @param [in]  cap        缓冲容量
/// @param [in,out] *p_pos  当前写入位置
/// @param [in]  type       属性类型
/// @param [in]  *p_val      属性值; 可为 NULL
/// @param [in]  len        属性值长度
/// @return int 0.成功; 小于 0.klb_netcode_e
static int stun_put_attr_klb_ice(uint8_t* p_buf, int cap, int* p_pos,
    uint16_t type, const uint8_t* p_val, int len)
{
    int total = 4 + ((len + 3) & ~3);

    if ((*p_pos + total) > cap)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    put_u16_klb_ice(p_buf + *p_pos, type);
    put_u16_klb_ice(p_buf + *p_pos + 2, (uint16_t)len);

    if (0 < len)
    {
        memcpy(p_buf + *p_pos + 4, p_val, (size_t)len);
    }

    int pad = total - 4 - len;
    if (0 < pad)
    {
        memset(p_buf + *p_pos + 4 + len, 0, (size_t)pad);
    }

    *p_pos += total;

    return 0;
}


/// @brief 追加 MESSAGE-INTEGRITY
/// @param [in]  *p_buf     消息缓冲
/// @param [in]  cap        缓冲容量
/// @param [in,out] *p_pos  当前写入位置
/// @param [in]  *p_pwd     key 文本
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 计算前把头部 length 置为 "到 MI 结束" 的值 (RFC 5389)
static int stun_add_mi_klb_ice(uint8_t* p_buf, int cap, int* p_pos, const char* p_pwd)
{
    if ((*p_pos + 24) > cap)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    put_u16_klb_ice(p_buf + 2, (uint16_t)((*p_pos - KLB_ICE_STUN_HEADER_LEN) + 24));

    uint8_t digest[20];
    int ret = hmac_sha1_klb_ice(p_pwd, p_buf, *p_pos, digest);
    if (0 != ret)
    {
        return ret;
    }

    put_u16_klb_ice(p_buf + *p_pos, KLB_ICE_ATTR_MI);
    put_u16_klb_ice(p_buf + *p_pos + 2, 20);
    memcpy(p_buf + *p_pos + 4, digest, 20);
    *p_pos += 24;

    return 0;
}


/// @brief 追加 FINGERPRINT 并封口头部 length
/// @param [in]  *p_buf     消息缓冲
/// @param [in]  cap        缓冲容量
/// @param [in,out] *p_pos  当前写入位置
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note CRC32 覆盖 FINGERPRINT 之前的内容; 头部 length 已含 FINGERPRINT
static int stun_add_fingerprint_klb_ice(uint8_t* p_buf, int cap, int* p_pos)
{
    if ((*p_pos + 8) > cap)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    put_u16_klb_ice(p_buf + 2, (uint16_t)((*p_pos - KLB_ICE_STUN_HEADER_LEN) + 8));

    uint32_t crc = crc32_klb_ice(p_buf, *p_pos) ^ KLB_ICE_FINGERPRINT_XOR;

    put_u16_klb_ice(p_buf + *p_pos, KLB_ICE_ATTR_FINGERPRINT);
    put_u16_klb_ice(p_buf + *p_pos + 2, 4);
    put_u32_klb_ice(p_buf + *p_pos + 4, crc);
    *p_pos += 8;

    put_u16_klb_ice(p_buf + 2, (uint16_t)(*p_pos - KLB_ICE_STUN_HEADER_LEN));

    return 0;
}


/// @brief 写 XOR-MAPPED-ADDRESS 值
/// @param [out] *p_val     输出值缓存 (至少 20 字节)
/// @param [in]  *p_from    来源地址
/// @param [in]  *p_txid    事务 ID (IPv6 用)
/// @return int 大于 0.值长度; 小于 0.klb_netcode_e
static int stun_put_xor_mapped_klb_ice(uint8_t* p_val, const struct sockaddr* p_from,
    const uint8_t* p_txid)
{
    p_val[0] = 0;

    if (AF_INET == p_from->sa_family)
    {
        const struct sockaddr_in* p_in = (const struct sockaddr_in*)p_from;

        p_val[1] = 0x01;
        put_u16_klb_ice(p_val + 2, (uint16_t)(ntohs(p_in->sin_port) ^ (uint16_t)(KLB_ICE_STUN_COOKIE >> 16)));
        put_u32_klb_ice(p_val + 4, ntohl(p_in->sin_addr.s_addr) ^ KLB_ICE_STUN_COOKIE);

        return 8;
    }
    else if (AF_INET6 == p_from->sa_family)
    {
        const struct sockaddr_in6* p_in6 = (const struct sockaddr_in6*)p_from;

        p_val[1] = 0x02;
        put_u16_klb_ice(p_val + 2, (uint16_t)(ntohs(p_in6->sin6_port) ^ (uint16_t)(KLB_ICE_STUN_COOKIE >> 16)));

        for (int i = 0; i < 4; i++)
        {
            p_val[4 + i] = p_in6->sin6_addr.s6_addr[i]
                ^ (uint8_t)(KLB_ICE_STUN_COOKIE >> (24 - (8 * i)));
        }

        for (int i = 0; i < KLB_ICE_TXID_LEN; i++)
        {
            p_val[8 + i] = p_in6->sin6_addr.s6_addr[4 + i] ^ p_txid[i];
        }

        return 20;
    }

    return KLB_NETCODE_AFNOSUPPORT;
}


//////////////////////////////////////////////////////////////////////////
// STUN 解析
//////////////////////////////////////////////////////////////////////////

/// @brief 判定是否 STUN 报文
/// @return bool true.是; false.否
static bool stun_is_message_klb_ice(const uint8_t* p_buf, int len)
{
    if (KLB_ICE_STUN_HEADER_LEN > len)
    {
        return false;
    }

    if (0 != (p_buf[0] & 0xC0))
    {
        return false;
    }

    return KLB_ICE_STUN_COOKIE == get_u32_klb_ice(p_buf + 4);
}


/// @brief 查找属性
/// @param [in]  *p_buf     消息
/// @param [in]  len        消息长度
/// @param [in]  type       属性类型
/// @param [out] **pp_val   属性值; 可为 NULL
/// @param [out] *p_vlen    属性值长度; 可为 NULL
/// @return int 0.找到; 小于 0.klb_netcode_e
static int stun_find_attr_klb_ice(const uint8_t* p_buf, int len, uint16_t type,
    const uint8_t** pp_val, int* p_vlen)
{
    int msg_end = KLB_ICE_STUN_HEADER_LEN + (int)get_u16_klb_ice(p_buf + 2);
    if (msg_end > len)
    {
        msg_end = len;
    }

    int off = KLB_ICE_STUN_HEADER_LEN;
    while ((off + 4) <= msg_end)
    {
        uint16_t attr_type = get_u16_klb_ice(p_buf + off);
        int attr_len = (int)get_u16_klb_ice(p_buf + off + 2);
        int next = off + 4 + ((attr_len + 3) & ~3);

        if (next > msg_end)
        {
            break;
        }

        if (type == attr_type)
        {
            if (NULL != pp_val)
            {
                *pp_val = p_buf + off + 4;
            }

            if (NULL != p_vlen)
            {
                *p_vlen = attr_len;
            }

            return 0;
        }

        off = next;
    }

    return KLB_NETCODE_NOENT;
}


/// @brief 校验 MESSAGE-INTEGRITY
/// @param [in,out] *p_buf  消息 (临时改 length 后还原)
/// @param [in]  len        消息长度
/// @param [in]  *p_pwd     key 文本
/// @param [out] *p_found   是否含 MI; 可为 NULL
/// @return int 0.校验通过(或无需校验); 小于 0.klb_netcode_e
/// @note p_pwd 为 NULL 或空串时不做校验, 仅报告是否存在 MI
static int stun_check_mi_klb_ice(uint8_t* p_buf, int len, const char* p_pwd, bool* p_found)
{
    if (NULL != p_found)
    {
        *p_found = false;
    }

    const uint8_t* p_val = NULL;
    int vlen = 0;
    int ret = stun_find_attr_klb_ice(p_buf, len, KLB_ICE_ATTR_MI, &p_val, &vlen);
    if (0 != ret)
    {
        return 0;
    }

    if (NULL != p_found)
    {
        *p_found = true;
    }

    if (20 > vlen)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    if ((NULL == p_pwd) || (0 == p_pwd[0]))
    {
        return 0;
    }

    int attr_off = (int)(p_val - p_buf) - 4;

    uint16_t saved_len = get_u16_klb_ice(p_buf + 2);
    put_u16_klb_ice(p_buf + 2, (uint16_t)((attr_off - KLB_ICE_STUN_HEADER_LEN) + 24));

    uint8_t digest[20];
    ret = hmac_sha1_klb_ice(p_pwd, p_buf, attr_off, digest);

    put_u16_klb_ice(p_buf + 2, saved_len);

    if (0 != ret)
    {
        return ret;
    }

    if (0 != memcmp(digest, p_val, 20))
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    return 0;
}


/// @brief 解析 XOR-MAPPED-ADDRESS
/// @param [in]  *p_val     属性值
/// @param [in]  len        属性值长度
/// @param [in]  *p_txid    事务 ID
/// @param [out] *p_ip      IP 文本
/// @param [in]  ip_cap     IP 缓存大小
/// @param [out] *p_port    端口 (主机序)
/// @return int 0.成功; 小于 0.klb_netcode_e
static int stun_parse_xor_mapped_klb_ice(const uint8_t* p_val, int len,
    const uint8_t* p_txid, char* p_ip, int ip_cap, int* p_port)
{
    if (2 > len)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    int family = p_val[1];
    if (0x01 == family)
    {
        if (8 > len)
        {
            return KLB_NETCODE_PROTO_PARSE;
        }

        struct in_addr addr;
        addr.s_addr = htonl(get_u32_klb_ice(p_val + 4) ^ KLB_ICE_STUN_COOKIE);

        if (NULL == inet_ntop(AF_INET, &addr, p_ip, (socklen_t)ip_cap))
        {
            return KLB_NETCODE_ERR;
        }

        if (NULL != p_port)
        {
            *p_port = (int)(get_u16_klb_ice(p_val + 2) ^ (uint16_t)(KLB_ICE_STUN_COOKIE >> 16));
        }

        return 0;
    }
    else if (0x02 == family)
    {
        if (20 > len)
        {
            return KLB_NETCODE_PROTO_PARSE;
        }

        uint8_t raw[16];
        for (int i = 0; i < 4; i++)
        {
            raw[i] = p_val[4 + i] ^ (uint8_t)(KLB_ICE_STUN_COOKIE >> (24 - (8 * i)));
        }

        for (int i = 0; i < KLB_ICE_TXID_LEN; i++)
        {
            raw[4 + i] = p_val[8 + i] ^ p_txid[i];
        }

        struct in6_addr addr6;
        memcpy(addr6.s6_addr, raw, 16);

        if (NULL == inet_ntop(AF_INET6, &addr6, p_ip, (socklen_t)ip_cap))
        {
            return KLB_NETCODE_ERR;
        }

        if (NULL != p_port)
        {
            *p_port = (int)(get_u16_klb_ice(p_val + 2) ^ (uint16_t)(KLB_ICE_STUN_COOKIE >> 16));
        }

        return 0;
    }

    return KLB_NETCODE_AFNOSUPPORT;
}


//////////////////////////////////////////////////////////////////////////
// TURN / STUN 扩展辅助
//////////////////////////////////////////////////////////////////////////

/// @def   KLB_ICE_STUN_TYPE_REFRESH_ERR
/// @brief TURN Refresh 错误应答
#define KLB_ICE_STUN_TYPE_REFRESH_ERR  0x0114

/// @def   KLB_ICE_STUN_TYPE_PERM_ERR
/// @brief TURN CreatePermission 错误应答
#define KLB_ICE_STUN_TYPE_PERM_ERR     0x0118

/// @def   KLB_ICE_STUN_TYPE_CHBIND_ERR
/// @brief TURN ChannelBind 错误应答
#define KLB_ICE_STUN_TYPE_CHBIND_ERR   0x0119

/// @def   KLB_ICE_TURN_DEFAULT_PORT
/// @brief TURN/STUN 默认端口
#define KLB_ICE_TURN_DEFAULT_PORT      3478


/// @brief 经本地候选发送 (relay 候选走 TURN); 定义见下
/// @param [in] *p_local    本地候选
/// @param [in] *p_data     数据
/// @param [in] len         长度
/// @param [in] *p_to       远端地址
/// @return int 0.成功; 小于 0.klb_netcode_e
static int send_to_local_klb_ice(klb_ice_local_t* p_local, const uint8_t* p_data, int len,
    const struct sockaddr_in* p_to);


/// @brief 生成 12 字节 STUN 事务 ID
/// @param [out] *p_txid    事务 ID 输出
/// @return 无
static void stun_new_txid_klb_ice(uint8_t* p_txid)
{
    for (int i = 0; i < KLB_ICE_TXID_LEN; i++)
    {
        p_txid[i] = (uint8_t)(rand() & 0xFF);
    }
}


/// @brief 计算 HMAC-SHA1 (原始字节 key)
/// @param [in]  *p_key     密钥
/// @param [in]  key_len    密钥长度
/// @param [in]  *p_data    输入数据
/// @param [in]  len        输入长度
/// @param [out] *p_out     20 字节输出
/// @return int 0.成功; 小于 0.klb_netcode_e
static int hmac_sha1_raw_klb_ice(const uint8_t* p_key, int key_len,
                                 const uint8_t* p_data, int len, uint8_t* p_out)
{
    const mbedtls_md_info_t* p_md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (NULL == p_md)
    {
        return KLB_NETCODE_NOTSUP;
    }

    int ret = mbedtls_md_hmac(p_md, p_key, (size_t)key_len, p_data, (size_t)len, p_out);
    if (0 != ret)
    {
        return KLB_NETCODE_ERR;
    }

    return 0;
}


/// @brief 追加 MESSAGE-INTEGRITY (原始字节 key; TURN 长期凭据)
/// @param [in]  *p_buf     消息缓冲
/// @param [in]  cap        缓冲容量
/// @param [in,out] *p_pos  当前写入位置
/// @param [in]  *p_key     密钥
/// @param [in]  key_len    密钥长度
/// @return int 0.成功; 小于 0.klb_netcode_e
static int stun_add_mi_raw_klb_ice(uint8_t* p_buf, int cap, int* p_pos,
                                   const uint8_t* p_key, int key_len)
{
    if ((*p_pos + 24) > cap)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    put_u16_klb_ice(p_buf + 2, (uint16_t)((*p_pos - KLB_ICE_STUN_HEADER_LEN) + 24));

    uint8_t digest[20];
    int ret = hmac_sha1_raw_klb_ice(p_key, key_len, p_buf, *p_pos, digest);
    if (0 != ret)
    {
        return ret;
    }

    put_u16_klb_ice(p_buf + *p_pos, KLB_ICE_ATTR_MI);
    put_u16_klb_ice(p_buf + *p_pos + 2, 20);
    memcpy(p_buf + *p_pos + 4, digest, 20);
    *p_pos += 24;

    return 0;
}


/// @brief 读取任意文本属性
/// @param [in]  *p_buf     消息
/// @param [in]  len        消息长度
/// @param [in]  type       属性类型
/// @param [out] *p_out     输出缓存
/// @param [in]  cap        缓存大小
/// @return int 0.成功; 小于 0.klb_netcode_e
static int stun_get_text_klb_ice(const uint8_t* p_buf, int len, uint16_t type,
                                 char* p_out, int cap)
{
    const uint8_t* p_val = NULL;
    int vlen = 0;

    if (0 != stun_find_attr_klb_ice(p_buf, len, type, &p_val, &vlen))
    {
        return KLB_NETCODE_NOENT;
    }

    if (0 >= cap)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    if ((cap - 1) < vlen)
    {
        vlen = cap - 1;
    }

    memcpy(p_out, p_val, (size_t)vlen);
    p_out[vlen] = '\0';

    return 0;
}


/// @brief 取 ERROR-CODE 数值
/// @param [in] *p_buf      消息
/// @param [in] len          消息长度
/// @return int 错误码; 0 表未携带
static int stun_get_error_code_klb_ice(const uint8_t* p_buf, int len)
{
    const uint8_t* p_val = NULL;
    int vlen = 0;

    if (0 != stun_find_attr_klb_ice(p_buf, len, KLB_ICE_ATTR_ERROR_CODE, &p_val, &vlen))
    {
        return 0;
    }

    if (4 > vlen)
    {
        return 0;
    }

    return ((int)(p_val[2] & 0x07) * 100) + (int)p_val[3];
}


/// @brief 追加以 IP/端口描述的 XOR 地址属性
/// @param [in]  *p_buf     消息缓冲
/// @param [in]  cap        缓冲容量
/// @param [in,out] *p_pos  当前写入位置
/// @param [in]  type       属性类型
/// @param [in]  *p_ip      对端 IP
/// @param [in]  port       对端端口 (主机序)
/// @param [in]  *p_txid    事务 ID
/// @return int 0.成功; 小于 0.klb_netcode_e
static int turn_put_xor_addr_klb_ice(uint8_t* p_buf, int cap, int* p_pos,
    uint16_t type, const char* p_ip, int port, const uint8_t* p_txid)
{
    struct sockaddr_in addr;
    uint8_t val[20];

    memset(&addr, 0, sizeof(addr));

    if (0 != klb_socket_resolve_v4(p_ip, port, &addr))
    {
        return KLB_NETCODE_DNS_FAIL;
    }

    int vlen = stun_put_xor_mapped_klb_ice(val, (const struct sockaddr*)&addr, p_txid);
    if (0 > vlen)
    {
        return vlen;
    }

    return stun_put_attr_klb_ice(p_buf, cap, p_pos, type, val, vlen);
}


/// @brief 计算 TURN 长期凭据密钥 MD5(username:realm:password)
/// @param [in,out] *p_turn TURN 会话
/// @return int 0.成功; 小于 0.klb_netcode_e
static int turn_make_key_klb_ice(klb_ice_turn_t* p_turn)
{
    char text[320];

    int len = snprintf(text, sizeof(text), "%s:%s:%s",
        p_turn->username, p_turn->realm, p_turn->password);
    if ((0 > len) || ((int)sizeof(text) <= len))
    {
        return KLB_NETCODE_MSGSIZE;
    }

    int ret = mbedtls_md5((const unsigned char*)text, (size_t)len, p_turn->key);
    if (0 != ret)
    {
        return KLB_NETCODE_ERR;
    }

    p_turn->key_valid = true;

    return 0;
}


/// @brief 追加 TURN 长期凭据属性 (USERNAME/REALM/NONCE) 与 MESSAGE-INTEGRITY
/// @param [in]  *p_turn    会话
/// @param [in]  *p_buf     消息缓冲
/// @param [in]  cap        缓冲容量
/// @param [in,out] *p_pos  当前写入位置
/// @return int 0.成功; 小于 0.klb_netcode_e
static int turn_add_auth_klb_ice(klb_ice_turn_t* p_turn, uint8_t* p_buf, int cap, int* p_pos)
{
    if (!p_turn->key_valid)
    {
        return KLB_NETCODE_ACCES;
    }

    int ret = stun_put_attr_klb_ice(p_buf, cap, p_pos, KLB_ICE_ATTR_USERNAME,
        (const uint8_t*)p_turn->username, (int)strlen(p_turn->username));
    if (0 != ret)
    {
        return ret;
    }

    ret = stun_put_attr_klb_ice(p_buf, cap, p_pos, KLB_ICE_ATTR_REALM,
        (const uint8_t*)p_turn->realm, (int)strlen(p_turn->realm));
    if (0 != ret)
    {
        return ret;
    }

    ret = stun_put_attr_klb_ice(p_buf, cap, p_pos, KLB_ICE_ATTR_NONCE,
        (const uint8_t*)p_turn->nonce, (int)strlen(p_turn->nonce));
    if (0 != ret)
    {
        return ret;
    }

    return stun_add_mi_raw_klb_ice(p_buf, cap, p_pos, p_turn->key,
        (int)sizeof(p_turn->key));
}


/// @brief TURN 事务: 发送请求并等待匹配事务 ID 的应答 (同步轮询)
/// @param [in]  *p_turn    会话
/// @param [in]  *p_req     请求消息
/// @param [in]  req_len    请求长度
/// @param [in]  ok_type    成功应答类型
/// @param [in]  err_type   错误应答类型
/// @param [out] *p_rsp     应答缓冲
/// @param [in]  cap        应答缓冲容量
/// @param [out] *p_rsp_len 应答长度; 可为 NULL
/// @return int 0.收到应答; 小于 0.klb_netcode_e
/// @note 收到成功或错误应答均返回 0, 由调用方按类型区分
static int turn_transact_klb_ice(klb_ice_turn_t* p_turn, const uint8_t* p_req, int req_len,
    uint16_t ok_type, uint16_t err_type, uint8_t* p_rsp, int cap, int* p_rsp_len)
{
    const uint8_t* p_txid = p_req + 8;

    for (int attempt = 0; attempt < KLB_ICE_TURN_RETRY; attempt++)
    {
        int ret = klb_socket_udp_sendto(p_turn->p_socket, p_req, req_len,
            (const struct sockaddr*)&p_turn->server, (int)sizeof(p_turn->server));
        if (0 != ret)
        {
            return ret;
        }

        uint32_t start = klb_tick_count();
        while ((klb_tick_count() - start) < KLB_ICE_TURN_TIMEOUT_MS)
        {
            struct sockaddr_storage from;
            int from_len = (int)sizeof(from);
            uint8_t buf[KLB_ICE_TURN_MSG_MAX];

            memset(&from, 0, sizeof(from));

            int n = klb_socket_udp_recvfrom(p_turn->p_socket, buf, (int)sizeof(buf),
                (struct sockaddr*)&from, &from_len);
            if (0 > n)
            {
                klb_sleep(KLB_ICE_TURN_POLL_MS);
                continue;
            }

            if (KLB_ICE_STUN_HEADER_LEN > n)
            {
                continue;
            }

            if (!stun_is_message_klb_ice(buf, n))
            {
                continue;
            }

            if (0 != memcmp(buf + 8, p_txid, KLB_ICE_TXID_LEN))
            {
                continue;
            }

            uint16_t type = get_u16_klb_ice(buf);
            if ((ok_type != type) && (err_type != type))
            {
                continue;
            }

            if (cap < n)
            {
                n = cap;
            }

            memcpy(p_rsp, buf, (size_t)n);

            if (NULL != p_rsp_len)
            {
                *p_rsp_len = n;
            }

            return 0;
        }
    }

    return KLB_NETCODE_TIMEDOUT;
}


/// @brief 查找已授权对端
/// @return klb_ice_turn_peer_t* NULL.未找到; 非 NULL.对端
static klb_ice_turn_peer_t* turn_find_peer_klb_ice(klb_ice_turn_t* p_turn,
    const char* p_ip, int port)
{
    for (int i = 0; i < p_turn->peer_count; i++)
    {
        klb_ice_turn_peer_t* p_peer = &p_turn->peers[i];

        if ((port == p_peer->port) && (0 == strcmp(p_peer->ip, p_ip)))
        {
            return p_peer;
        }
    }

    return NULL;
}


/// @brief 记录已授权对端
/// @return klb_ice_turn_peer_t* NULL.表满; 非 NULL.对端
static klb_ice_turn_peer_t* turn_add_peer_klb_ice(klb_ice_turn_t* p_turn,
    const char* p_ip, int port)
{
    klb_ice_turn_peer_t* p_peer = turn_find_peer_klb_ice(p_turn, p_ip, port);
    if (NULL != p_peer)
    {
        return p_peer;
    }

    if (KLB_ICE_TURN_MAX_PEER <= p_turn->peer_count)
    {
        return NULL;
    }

    p_peer = &p_turn->peers[p_turn->peer_count];
    memset(p_peer, 0, sizeof(*p_peer));
    snprintf(p_peer->ip, sizeof(p_peer->ip), "%s", p_ip);
    p_peer->port = port;
    p_turn->peer_count++;

    return p_peer;
}


//////////////////////////////////////////////////////////////////////////
// 代理内部流程
//////////////////////////////////////////////////////////////////////////

/// @brief 计算候选对优先级 (RFC 8445 6.1.2.3)
static uint64_t pair_priority_klb_ice(const klb_ice_agent_t* p_agent,
    uint32_t local_prio, uint32_t remote_prio)
{
    uint32_t g = local_prio;
    uint32_t d = remote_prio;

    if (KLB_ICE_ROLE_CONTROLLED == p_agent->role)
    {
        g = remote_prio;
        d = local_prio;
    }

    uint32_t min = (g < d) ? g : d;
    uint32_t max = (g < d) ? d : g;
    uint64_t tie = (g > d) ? 1u : 0u;

    return ((uint64_t)min << 32) + ((uint64_t)max << 1) + tie;
}


/// @brief 源地址是否匹配远端候选
static bool remote_match_klb_ice(const klb_ice_remote_t* p_remote, const struct sockaddr* p_from)
{
    if (AF_INET != p_from->sa_family)
    {
        return false;
    }

    const struct sockaddr_in* p_in = (const struct sockaddr_in*)p_from;

    if (p_remote->port != (int)ntohs(p_in->sin_port))
    {
        return false;
    }

    return p_remote->addr.sin_addr.s_addr == p_in->sin_addr.s_addr;
}


/// @brief 按事务 ID 查找候选对
/// @return int 大于等于 0.下标; 小于 0.KLB_NETCODE_NOENT
static int find_pair_by_txid_klb_ice(const klb_ice_agent_t* p_agent, const uint8_t* p_txid)
{
    for (int i = 0; i < p_agent->pair_count; i++)
    {
        if (0 == memcmp(p_agent->pairs[i].txid, p_txid, KLB_ICE_TXID_LEN))
        {
            return i;
        }
    }

    return KLB_NETCODE_NOENT;
}


/// @brief 按本地 socket 下标 + 来源地址查找候选对
/// @return int 大于等于 0.下标; 小于 0.KLB_NETCODE_NOENT
static int find_pair_by_source_klb_ice(const klb_ice_agent_t* p_agent, int local_idx,
    const struct sockaddr* p_from)
{
    for (int i = 0; i < p_agent->pair_count; i++)
    {
        const klb_ice_pair_t* p_pair = &p_agent->pairs[i];

        if (local_idx != p_pair->local_idx)
        {
            continue;
        }

        if (remote_match_klb_ice(&p_agent->remotes[p_pair->remote_idx], p_from))
        {
            return i;
        }
    }

    return KLB_NETCODE_NOENT;
}


/// @brief 选定候选对
static void select_pair_klb_ice(klb_ice_agent_t* p_agent, int idx)
{
    for (int i = 0; i < p_agent->pair_count; i++)
    {
        p_agent->pairs[i].selected = false;
    }

    p_agent->pairs[idx].selected = true;
    p_agent->selected = idx;
    p_agent->state = KLB_ICE_STATE_CONNECTED;
}


/// @brief 构造 Binding Request
/// @param [in]  *p_agent   代理
/// @param [in]  *p_pair    候选对
/// @param [in]  use_candidate  是否带 USE-CANDIDATE
/// @param [out] *p_buf     消息缓冲
/// @param [in]  cap        缓冲容量
/// @return int 大于 0.消息长度; 小于 0.klb_netcode_e
static int build_request_klb_ice(klb_ice_agent_t* p_agent, klb_ice_pair_t* p_pair,
    bool use_candidate, uint8_t* p_buf, int cap)
{
    uint8_t txid[KLB_ICE_TXID_LEN];

    for (int i = 0; i < KLB_ICE_TXID_LEN; i++)
    {
        txid[i] = (uint8_t)(rand() & 0xFF);
    }

    stun_put_header_klb_ice(p_buf, KLB_ICE_STUN_TYPE_REQUEST, txid);

    int pos = KLB_ICE_STUN_HEADER_LEN;
    char username[KLB_ICE_REMOTE_CRED_MAX + 20];
    snprintf(username, sizeof(username), "%s:%s", p_agent->remote_ufrag, p_agent->local_ufrag);

    int ret = stun_put_attr_klb_ice(p_buf, cap, &pos, KLB_ICE_ATTR_USERNAME,
        (const uint8_t*)username, (int)strlen(username));
    if (0 != ret)
    {
        return ret;
    }

    uint8_t val[8];
    put_u32_klb_ice(val, p_agent->locals[p_pair->local_idx].priority);

    ret = stun_put_attr_klb_ice(p_buf, cap, &pos, KLB_ICE_ATTR_PRIORITY, val, 4);
    if (0 != ret)
    {
        return ret;
    }

    put_u64_klb_ice(val, p_agent->tie_breaker);
    ret = stun_put_attr_klb_ice(p_buf, cap, &pos,
        (KLB_ICE_ROLE_CONTROLLING == p_agent->role) ? KLB_ICE_ATTR_ICE_CONTROLLING : KLB_ICE_ATTR_ICE_CONTROLLED,
        val, 8);
    if (0 != ret)
    {
        return ret;
    }

    if (use_candidate)
    {
        ret = stun_put_attr_klb_ice(p_buf, cap, &pos, KLB_ICE_ATTR_USE_CANDIDATE, NULL, 0);
        if (0 != ret)
        {
            return ret;
        }
    }

    ret = stun_add_mi_klb_ice(p_buf, cap, &pos, p_agent->remote_pwd);
    if (0 != ret)
    {
        return ret;
    }

    ret = stun_add_fingerprint_klb_ice(p_buf, cap, &pos);
    if (0 != ret)
    {
        return ret;
    }

    memcpy(p_pair->txid, txid, KLB_ICE_TXID_LEN);

    return pos;
}


/// @brief 构造 Binding Success Response
/// @param [in]  *p_agent   代理
/// @param [in]  *p_req     请求消息
/// @param [in]  *p_from    请求来源
/// @param [out] *p_buf     消息缓冲
/// @param [in]  cap        缓冲容量
/// @return int 大于 0.消息长度; 小于 0.klb_netcode_e
static int build_response_klb_ice(klb_ice_agent_t* p_agent, const uint8_t* p_req,
    const struct sockaddr* p_from, uint8_t* p_buf, int cap)
{
    const uint8_t* p_txid = p_req + 8;

    stun_put_header_klb_ice(p_buf, KLB_ICE_STUN_TYPE_SUCCESS, p_txid);

    int pos = KLB_ICE_STUN_HEADER_LEN;
    uint8_t val[20];
    int vlen = stun_put_xor_mapped_klb_ice(val, p_from, p_txid);
    if (0 > vlen)
    {
        return vlen;
    }

    int ret = stun_put_attr_klb_ice(p_buf, cap, &pos, KLB_ICE_ATTR_XOR_MAPPED, val, vlen);
    if (0 != ret)
    {
        return ret;
    }

    ret = stun_add_mi_klb_ice(p_buf, cap, &pos, p_agent->local_pwd);
    if (0 != ret)
    {
        return ret;
    }

    ret = stun_add_fingerprint_klb_ice(p_buf, cap, &pos);
    if (0 != ret)
    {
        return ret;
    }

    return pos;
}


/// @brief 发送一个 Binding Request 并更新重发状态
/// @return int 0.成功; 小于 0.klb_netcode_e
static int send_request_klb_ice(klb_ice_agent_t* p_agent, int idx, bool use_candidate)
{
    klb_ice_pair_t* p_pair = &p_agent->pairs[idx];
    klb_ice_local_t* p_local = &p_agent->locals[p_pair->local_idx];
    klb_ice_remote_t* p_remote = &p_agent->remotes[p_pair->remote_idx];

    uint8_t buf[KLB_ICE_STUN_BUF];
    int len = build_request_klb_ice(p_agent, p_pair, use_candidate, buf, (int)sizeof(buf));
    if (0 > len)
    {
        return len;
    }

    int ret = send_to_local_klb_ice(p_local, buf, len, &p_remote->addr);
    if (0 != ret)
    {
        return ret;
    }

    p_pair->use_candidate = use_candidate;
    p_pair->last_send_ms = klb_tick_count();
    p_agent->stat_check_sent++;

    return 0;
}


/// @brief 选出已成功对中优先级最高者并发送 USE-CANDIDATE
static void nominate_best_klb_ice(klb_ice_agent_t* p_agent)
{
    int best = -1;
    uint64_t best_prio = 0;

    for (int i = 0; i < p_agent->pair_count; i++)
    {
        klb_ice_pair_t* p_pair = &p_agent->pairs[i];

        if (!p_pair->succeeded)
        {
            continue;
        }

        if ((-1 == best) || (p_pair->priority > best_prio))
        {
            best = i;
            best_prio = p_pair->priority;
        }
    }

    if (-1 == best)
    {
        return;
    }

    if (p_agent->pairs[best].use_candidate)
    {
        return;
    }

    if (0 == send_request_klb_ice(p_agent, best, true))
    {
        p_agent->pairs[best].retrans = 1;
    }
}


/// @brief 处理一条 STUN 报文
/// @param [in] *p_agent    代理
/// @param [in] local_idx   接收 socket 下标
/// @param [in,out] *p_buf  报文 (MI 校验会临时改 length)
/// @param [in] len         报文长度
/// @param [in] *p_from     来源地址
/// @return int 0.已处理; 小于 0.klb_netcode_e (忽略)
static int handle_stun_klb_ice(klb_ice_agent_t* p_agent, int local_idx, uint8_t* p_buf,
    int len, const struct sockaddr* p_from)
{
    uint16_t type = get_u16_klb_ice(p_buf);

    p_agent->stat_check_recv++;

    if (KLB_ICE_STUN_TYPE_REQUEST == type)
    {
        bool found = false;
        int ret = stun_check_mi_klb_ice(p_buf, len, p_agent->local_pwd, &found);
        if ((0 != ret) || (!found))
        {
            return KLB_NETCODE_PROTO_PARSE;
        }

        int idx = find_pair_by_source_klb_ice(p_agent, local_idx, p_from);
        if (0 <= idx)
        {
            p_agent->pairs[idx].succeeded = true;
        }

        uint8_t rsp[KLB_ICE_STUN_BUF];
        int rsp_len = build_response_klb_ice(p_agent, p_buf, p_from, rsp, (int)sizeof(rsp));
        if (0 < rsp_len)
        {
            klb_ice_local_t* p_local = &p_agent->locals[local_idx];

            if (AF_INET == p_from->sa_family)
            {
                send_to_local_klb_ice(p_local, rsp, rsp_len,
                    (const struct sockaddr_in*)p_from);
            }
        }

        if ((0 <= idx) && (KLB_ICE_ROLE_CONTROLLED == p_agent->role))
        {
            const uint8_t* p_val = NULL;
            int vlen = 0;

            if (0 == stun_find_attr_klb_ice(p_buf, len, KLB_ICE_ATTR_USE_CANDIDATE, &p_val, &vlen))
            {
                select_pair_klb_ice(p_agent, idx);
            }
        }

        return 0;
    }
    else if (KLB_ICE_STUN_TYPE_SUCCESS == type)
    {
        bool found = false;
        int ret = stun_check_mi_klb_ice(p_buf, len, p_agent->remote_pwd, &found);
        if (0 != ret)
        {
            return ret;
        }

        if ((!found) && (0 != p_agent->remote_pwd[0]))
        {
            return KLB_NETCODE_PROTO_PARSE;
        }

        int idx = find_pair_by_txid_klb_ice(p_agent, p_buf + 8);
        if (0 > idx)
        {
            return KLB_NETCODE_NOENT;
        }

        const uint8_t* p_val = NULL;
        int vlen = 0;
        ret = stun_find_attr_klb_ice(p_buf, len, KLB_ICE_ATTR_XOR_MAPPED, &p_val, &vlen);
        if (0 != ret)
        {
            return ret;
        }

        klb_ice_pair_t* p_pair = &p_agent->pairs[idx];
        ret = stun_parse_xor_mapped_klb_ice(p_val, vlen, p_buf + 8,
            p_pair->mapped_ip, (int)sizeof(p_pair->mapped_ip), &p_pair->mapped_port);
        if (0 != ret)
        {
            return ret;
        }

        p_pair->succeeded = true;

        if (KLB_ICE_ROLE_CONTROLLING == p_agent->role)
        {
            if (p_pair->use_candidate)
            {
                select_pair_klb_ice(p_agent, idx);
            }
            else
            {
                nominate_best_klb_ice(p_agent);
            }
        }

        return 0;
    }
    else if (KLB_ICE_STUN_TYPE_ERROR == type)
    {
        int idx = find_pair_by_txid_klb_ice(p_agent, p_buf + 8);
        if (0 <= idx)
        {
            p_agent->pairs[idx].abandoned = true;
        }

        return 0;
    }

    return KLB_NETCODE_PROTO_UNEXPECTED;
}


/// @brief 重发定时泵 (非阻塞)
/// @note 所有对耗尽重发且无成功时置 FAILED
static void pump_retrans_klb_ice(klb_ice_agent_t* p_agent)
{
    if (KLB_ICE_STATE_CHECKING != p_agent->state)
    {
        return;
    }

    uint32_t now = klb_tick_count();
    bool pending = false;
    bool any_succeeded = false;

    for (int i = 0; i < p_agent->pair_count; i++)
    {
        klb_ice_pair_t* p_pair = &p_agent->pairs[i];

        if (p_pair->succeeded)
        {
            any_succeeded = true;
            continue;
        }

        if (p_pair->abandoned)
        {
            continue;
        }

        if ((now - p_pair->last_send_ms) < KLB_ICE_RETRANS_MS)
        {
            pending = true;
            continue;
        }

        if (p_pair->retrans >= KLB_ICE_RETRANS_MAX)
        {
            p_pair->abandoned = true;
            continue;
        }

        if (0 == send_request_klb_ice(p_agent, i, p_pair->use_candidate))
        {
            p_pair->retrans++;
        }

        pending = true;
    }

    if ((!pending) && (!any_succeeded) && (-1 == p_agent->selected))
    {
        p_agent->state = KLB_ICE_STATE_FAILED;
    }
}


/// @brief 加入一个本地候选 (通用)
/// @param [in] *p_agent    代理
/// @param [in] type        候选类型 klb_ice_cand_type_e
/// @param [in] *p_ip       候选地址文本
/// @param [in] port        候选端口 (主机序)
/// @param [in] priority    候选优先级
/// @param [in] *p_socket   承载该候选的 socket; host 候选为本候选所有
/// @param [in] owns_socket 是否由本候选负责销毁 socket
/// @param [in] *p_turn     relay 候选所用 TURN 会话; 其它为 NULL
/// @return int 0.成功; 小于 0.klb_netcode_e
static int add_local_ex_klb_ice(klb_ice_agent_t* p_agent, int type, const char* p_ip,
    int port, uint32_t priority, klb_socket_t* p_socket, bool owns_socket,
    klb_ice_turn_t* p_turn)
{
    if (KLB_ICE_MAX_LOCAL <= p_agent->local_count)
    {
        return KLB_NETCODE_LIMIT;
    }

    for (int i = 0; i < p_agent->local_count; i++)
    {
        klb_ice_local_t* p_old = &p_agent->locals[i];

        if ((type != p_old->type) || (port != p_old->port))
        {
            continue;
        }

        if (0 == strcmp(p_old->ip, p_ip))
        {
            return KLB_NETCODE_EXIST;
        }
    }

    klb_ice_local_t* p_local = &p_agent->locals[p_agent->local_count];

    memset(p_local, 0, sizeof(*p_local));
    snprintf(p_local->ip, sizeof(p_local->ip), "%s", p_ip);
    p_local->port = port;
    p_local->priority = priority;
    p_local->type = type;
    p_local->owns_socket = owns_socket;
    p_local->p_socket = p_socket;
    p_local->p_turn = p_turn;
    p_agent->local_count++;

    return 0;
}


/// @brief 加入一个本地 host 候选
/// @return int 0.成功; 小于 0.klb_netcode_e
static int add_local_klb_ice(klb_ice_agent_t* p_agent, const char* p_ip)
{
    if (KLB_ICE_MAX_LOCAL <= p_agent->local_count)
    {
        return KLB_NETCODE_LIMIT;
    }

    for (int i = 0; i < p_agent->local_count; i++)
    {
        if ((KLB_ICE_CAND_HOST == p_agent->locals[i].type)
            && (0 == strcmp(p_agent->locals[i].ip, p_ip)))
        {
            return KLB_NETCODE_EXIST;
        }
    }

    klb_socket_t* p_socket = klb_socket_udp_open(KLB_SOCKET_FAMILY_V4, p_ip, 0, 0);
    if (NULL == p_socket)
    {
        return KLB_NETCODE_BIND_FAIL;
    }

    int port = klb_socket_local_port(p_socket);
    if (0 >= port)
    {
        klb_socket_destroy(p_socket);
        return KLB_NETCODE_BIND_FAIL;
    }

    return add_local_ex_klb_ice(p_agent, KLB_ICE_CAND_HOST, p_ip, port,
        KLB_ICE_HOST_PRIORITY, p_socket, true, NULL);
}


/// @brief 枚举非 loopback IPv4 地址并建 host 候选
/// @return int 大于等于 0.候选数; 小于 0.klb_netcode_e
static int gather_local_klb_ice(klb_ice_agent_t* p_agent)
{
#if defined(_WIN32)

    char host[256] = { 0 };
    if (0 != gethostname(host, (int)sizeof(host)))
    {
        return KLB_NETCODE_ERR;
    }

    struct addrinfo hints;
    struct addrinfo* p_res = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (0 != getaddrinfo(host, NULL, &hints, &p_res))
    {
        return KLB_NETCODE_DNS_FAIL;
    }

    for (struct addrinfo* p_ai = p_res; NULL != p_ai; p_ai = p_ai->ai_next)
    {
        char ip[64] = { 0 };
        const struct sockaddr_in* p_in = (const struct sockaddr_in*)p_ai->ai_addr;

        if (NULL == inet_ntop(AF_INET, &p_in->sin_addr, ip, (socklen_t)sizeof(ip)))
        {
            continue;
        }

        if (0 == strcmp(ip, "0.0.0.0"))
        {
            continue;
        }

        if (0 == strncmp(ip, "127.", 4))
        {
            continue;
        }

        add_local_klb_ice(p_agent, ip);
    }

    freeaddrinfo(p_res);

#else

    struct ifaddrs* p_list = NULL;
    if (0 != getifaddrs(&p_list))
    {
        return KLB_NETCODE_ERR;
    }

    for (struct ifaddrs* p_ifa = p_list; NULL != p_ifa; p_ifa = p_ifa->ifa_next)
    {
        if (NULL == p_ifa->ifa_addr)
        {
            continue;
        }

        if (AF_INET != p_ifa->ifa_addr->sa_family)
        {
            continue;
        }

        if (0 != (p_ifa->ifa_flags & IFF_LOOPBACK))
        {
            continue;
        }

        if (0 == (p_ifa->ifa_flags & IFF_UP))
        {
            continue;
        }

        char ip[64] = { 0 };
        const struct sockaddr_in* p_in = (const struct sockaddr_in*)p_ifa->ifa_addr;

        if (NULL == inet_ntop(AF_INET, &p_in->sin_addr, ip, (socklen_t)sizeof(ip)))
        {
            continue;
        }

        if (0 == strncmp(ip, "127.", 4))
        {
            continue;
        }

        add_local_klb_ice(p_agent, ip);
    }

    freeifaddrs(p_list);

#endif

    return p_agent->local_count;
}


/// @brief 经本地候选发送数据 (relay 走 TURN 封装)
/// @param [in] *p_local    本地候选
/// @param [in] *p_data     数据
/// @param [in] len          长度
/// @param [in] *p_to        目标地址
/// @return int 0.成功; 小于 0.klb_netcode_e
static int send_to_local_klb_ice(klb_ice_local_t* p_local, const uint8_t* p_data, int len,
    const struct sockaddr_in* p_to)
{
    if (NULL != p_local->p_turn)
    {
        char ip[64] = { 0 };

        if (NULL == inet_ntop(AF_INET, &p_to->sin_addr, ip, (socklen_t)sizeof(ip)))
        {
            return KLB_NETCODE_ERR;
        }

        return klb_ice_turn_send(p_local->p_turn, ip, (int)ntohs(p_to->sin_port), p_data, len);
    }

    if (NULL == p_local->p_socket)
    {
        return KLB_NETCODE_NOTCONN;
    }

    return klb_socket_udp_sendto(p_local->p_socket, p_data, len,
        (const struct sockaddr*)p_to, (int)sizeof(struct sockaddr_in));
}


/// @brief 向 STUN 服务器查询本端映射地址 (srflx)
/// @param [in]  *p_socket  用于发送的 host socket
/// @param [in]  *p_server  STUN 服务器地址
/// @param [out] *p_ip      映射 IP 文本
/// @param [in]  ip_cap     IP 缓存大小
/// @param [out] *p_port    映射端口 (主机序)
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 同步轮询; 会消费 socket 上无关报文
static int stun_query_mapped_klb_ice(klb_socket_t* p_socket, const struct sockaddr_in* p_server,
    char* p_ip, int ip_cap, int* p_port)
{
    uint8_t req[128];
    uint8_t txid[KLB_ICE_TXID_LEN];
    uint8_t rsp[KLB_ICE_STUN_BUF];

    stun_new_txid_klb_ice(txid);
    stun_put_header_klb_ice(req, KLB_ICE_STUN_TYPE_REQUEST, txid);

    int pos = KLB_ICE_STUN_HEADER_LEN;
    int ret = stun_add_fingerprint_klb_ice(req, (int)sizeof(req), &pos);
    if (0 != ret)
    {
        return ret;
    }

    for (int attempt = 0; attempt < 2; attempt++)
    {
        ret = klb_socket_udp_sendto(p_socket, req, pos,
            (const struct sockaddr*)p_server, (int)sizeof(*p_server));
        if (0 != ret)
        {
            return ret;
        }

        uint32_t start = klb_tick_count();
        while ((klb_tick_count() - start) < KLB_ICE_TURN_TIMEOUT_MS)
        {
            struct sockaddr_storage from;
            int from_len = (int)sizeof(from);

            memset(&from, 0, sizeof(from));

            int n = klb_socket_udp_recvfrom(p_socket, rsp, (int)sizeof(rsp),
                (struct sockaddr*)&from, &from_len);
            if (0 > n)
            {
                klb_sleep(KLB_ICE_TURN_POLL_MS);
                continue;
            }

            if (KLB_ICE_STUN_HEADER_LEN > n)
            {
                continue;
            }

            if (!stun_is_message_klb_ice(rsp, n))
            {
                continue;
            }

            if (0 != memcmp(rsp + 8, txid, KLB_ICE_TXID_LEN))
            {
                continue;
            }

            if (KLB_ICE_STUN_TYPE_SUCCESS != get_u16_klb_ice(rsp))
            {
                continue;
            }

            const uint8_t* p_val = NULL;
            int vlen = 0;

            if (0 != stun_find_attr_klb_ice(rsp, n, KLB_ICE_ATTR_XOR_MAPPED, &p_val, &vlen))
            {
                continue;
            }

            return stun_parse_xor_mapped_klb_ice(p_val, vlen, txid, p_ip, ip_cap, p_port);
        }
    }

    return KLB_NETCODE_TIMEDOUT;
}


/// @brief 收集 srflx 候选 (复用各 host socket)
/// @param [in] *p_agent    代理
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 未配置 STUN 服务器时直接返回 0
static int gather_srflx_klb_ice(klb_ice_agent_t* p_agent)
{
    struct sockaddr_in server;
    int base = p_agent->local_count;

    if (0 == p_agent->stun_host[0])
    {
        return 0;
    }

    memset(&server, 0, sizeof(server));

    if (0 != klb_socket_resolve_v4(p_agent->stun_host, p_agent->stun_port, &server))
    {
        return KLB_NETCODE_DNS_FAIL;
    }

    for (int i = 0; i < base; i++)
    {
        klb_ice_local_t* p_local = &p_agent->locals[i];
        char ip[64] = { 0 };
        int port = 0;

        if ((KLB_ICE_CAND_HOST != p_local->type) || (NULL == p_local->p_socket))
        {
            continue;
        }

        if (0 != stun_query_mapped_klb_ice(p_local->p_socket, &server, ip, (int)sizeof(ip), &port))
        {
            continue;
        }

        add_local_ex_klb_ice(p_agent, KLB_ICE_CAND_SRFLX, ip, port,
            KLB_ICE_SRFLX_PRIORITY, p_local->p_socket, false, NULL);
    }

    return 0;
}


/// @brief 收集 relay 候选 (TURN Allocate)
/// @param [in] *p_agent    代理
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 未配置 TURN 服务器时直接返回 0
static int gather_relay_klb_ice(klb_ice_agent_t* p_agent)
{
    klb_ice_turn_cfg_t cfg;
    int code = 0;

    if (0 == p_agent->turn_host[0])
    {
        return 0;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.p_server = p_agent->turn_host;
    cfg.port = p_agent->turn_port;
    cfg.p_username = (0 == p_agent->turn_user[0]) ? NULL : p_agent->turn_user;
    cfg.p_password = (0 == p_agent->turn_pass[0]) ? NULL : p_agent->turn_pass;

    klb_ice_turn_t* p_turn = klb_ice_turn_create(&cfg, &code);
    if (NULL == p_turn)
    {
        return (0 != code) ? code : KLB_NETCODE_ERR;
    }

    int ret = klb_ice_turn_allocate(p_turn);
    if (0 != ret)
    {
        klb_ice_turn_destroy(p_turn);
        return ret;
    }

    char ip[64] = { 0 };
    int port = 0;

    ret = klb_ice_turn_relayed_address(p_turn, ip, (int)sizeof(ip), &port);
    if (0 != ret)
    {
        klb_ice_turn_destroy(p_turn);
        return ret;
    }

    ret = add_local_ex_klb_ice(p_agent, KLB_ICE_CAND_RELAY, ip, port,
        KLB_ICE_RELAY_PRIORITY, p_turn->p_socket, false, p_turn);
    if (0 != ret)
    {
        klb_ice_turn_destroy(p_turn);
        return ret;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 对外 API
//////////////////////////////////////////////////////////////////////////

klb_ice_agent_t* klb_ice_agent_create(klb_netmulti_t* p_netmulti, int role, int* p_code)
{
    if ((NULL == p_netmulti)
        || ((KLB_ICE_ROLE_CONTROLLED != role) && (KLB_ICE_ROLE_CONTROLLING != role)))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    klb_ice_agent_t* p_agent = KLB_MALLOCZ(klb_ice_agent_t, 1, 0);

    p_agent->p_netmulti = p_netmulti;
    p_agent->role = role;
    p_agent->state = KLB_ICE_STATE_NEW;
    p_agent->selected = -1;
    p_agent->tie_breaker = ((uint64_t)(uint32_t)rand() << 32) | (uint64_t)(uint32_t)rand();

    klb_rand_string(p_agent->local_ufrag, KLB_ICE_UFRAG_LEN, true);
    klb_rand_string(p_agent->local_pwd, KLB_ICE_PWD_LEN, true);

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_agent;
}


void klb_ice_agent_destroy(klb_ice_agent_t* p_agent)
{
    if (NULL == p_agent)
    {
        return;
    }

    for (int i = 0; i < p_agent->local_count; i++)
    {
        klb_ice_local_t* p_local = &p_agent->locals[i];

        // relay 候选的 socket 归 TURN 会话所有
        if (NULL != p_local->p_turn)
        {
            klb_ice_turn_destroy(p_local->p_turn);
            p_local->p_turn = NULL;
        }
        else if (p_local->owns_socket && (NULL != p_local->p_socket))
        {
            klb_socket_destroy(p_local->p_socket);
        }

        p_local->p_socket = NULL;
    }

    p_agent->local_count = 0;
    p_agent->selected = -1;
    p_agent->state = KLB_ICE_STATE_CLOSED;

    KLB_FREE(p_agent);
}


int klb_ice_agent_local_ufrag(klb_ice_agent_t* p_agent, char* p_buf, int cap)
{
    if ((NULL == p_agent) || (NULL == p_buf))
    {
        return KLB_NETCODE_INVAL;
    }

    int len = (int)strlen(p_agent->local_ufrag);
    if (cap <= len)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    memcpy(p_buf, p_agent->local_ufrag, (size_t)len);
    p_buf[len] = '\0';

    return len;
}


int klb_ice_agent_add_remote_candidate(klb_ice_agent_t* p_agent, const char* p_candidate)
{
    if ((NULL == p_agent) || (NULL == p_candidate))
    {
        return KLB_NETCODE_INVAL;
    }

    const char* p_line = p_candidate;
    while ((0 != *p_line) && (0 != isspace((unsigned char)*p_line)))
    {
        p_line++;
    }

    if (0 == strncmp(p_line, "a=", 2))
    {
        p_line += 2;
    }

    // 兼容 SDP 凭据行: 便于在仅有候选 API 时配置远端 ufrag/pwd
    if (0 == strncmp(p_line, "ice-ufrag:", 10))
    {
        return (0 <= copy_token_klb_ice(p_agent->remote_ufrag,
            (int)sizeof(p_agent->remote_ufrag), p_line + 10)) ? 0 : KLB_NETCODE_MSGSIZE;
    }
    else if (0 == strncmp(p_line, "ice-pwd:", 8))
    {
        return (0 <= copy_token_klb_ice(p_agent->remote_pwd,
            (int)sizeof(p_agent->remote_pwd), p_line + 8)) ? 0 : KLB_NETCODE_MSGSIZE;
    }

    if (0 != strncmp(p_line, "candidate:", 10))
    {
        return KLB_NETCODE_INVAL;
    }

    char foundation[32] = { 0 };
    char transport[16] = { 0 };
    char ip[64] = { 0 };
    char ctype[16] = { 0 };
    unsigned int priority = 0;
    int component = 0;
    int port = 0;

    int n = sscanf(p_line, "candidate:%31s %d %15s %u %63s %d typ %15s",
        foundation, &component, transport, &priority, ip, &port, ctype);
    if (7 != n)
    {
        return KLB_NETCODE_INVAL;
    }

    if (!str_ieq_klb_ice(transport, "UDP"))
    {
        return KLB_NETCODE_NOTSUP;
    }

    if ((0 >= port) || (65535 < port))
    {
        return KLB_NETCODE_INVAL;
    }

    if (KLB_ICE_MAX_REMOTE <= p_agent->remote_count)
    {
        return KLB_NETCODE_LIMIT;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    if (0 != klb_socket_resolve_v4(ip, port, &addr))
    {
        return KLB_NETCODE_DNS_FAIL;
    }

    klb_ice_remote_t* p_remote = &p_agent->remotes[p_agent->remote_count];

    snprintf(p_remote->foundation, sizeof(p_remote->foundation), "%s", foundation);
    p_remote->component = component;
    p_remote->priority = (uint32_t)priority;
    p_remote->port = port;
    p_remote->addr = addr;
    snprintf(p_remote->ip, sizeof(p_remote->ip), "%s", ip);

    if (str_ieq_klb_ice(ctype, "srflx"))
    {
        p_remote->type = KLB_ICE_CAND_SRFLX;
    }
    else if (str_ieq_klb_ice(ctype, "relay"))
    {
        p_remote->type = KLB_ICE_CAND_RELAY;
    }
    else
    {
        p_remote->type = KLB_ICE_CAND_HOST;
    }

    p_agent->remote_count++;

    return 0;
}


int klb_ice_agent_gather(klb_ice_agent_t* p_agent)
{
    if (NULL == p_agent)
    {
        return KLB_NETCODE_INVAL;
    }

    if (0 < p_agent->local_count)
    {
        return 0;
    }

    p_agent->state = KLB_ICE_STATE_GATHERING;

    int ret = gather_local_klb_ice(p_agent);
    if (0 > ret)
    {
        p_agent->state = KLB_ICE_STATE_FAILED;
        return ret;
    }

    if (0 == ret)
    {
        p_agent->state = KLB_ICE_STATE_FAILED;
        return KLB_NETCODE_NOENT;
    }

    // srflx / relay 收集失败不致命, 保留已收集的 host 候选
    gather_srflx_klb_ice(p_agent);
    gather_relay_klb_ice(p_agent);

    return 0;
}


int klb_ice_agent_restart(klb_ice_agent_t* p_agent)
{
    if (NULL == p_agent)
    {
        return KLB_NETCODE_INVAL;
    }

    klb_rand_string(p_agent->local_ufrag, KLB_ICE_UFRAG_LEN, true);
    klb_rand_string(p_agent->local_pwd, KLB_ICE_PWD_LEN, true);
    p_agent->tie_breaker = ((uint64_t)(uint32_t)rand() << 32) | (uint64_t)(uint32_t)rand();

    KLB_MEMSET(p_agent->remote_ufrag, 0, sizeof(p_agent->remote_ufrag));
    KLB_MEMSET(p_agent->remote_pwd, 0, sizeof(p_agent->remote_pwd));

    p_agent->remote_count = 0;
    p_agent->pair_count = 0;
    p_agent->selected = -1;
    p_agent->state = KLB_ICE_STATE_NEW;

    return 0;
}


int klb_ice_agent_set_stun_server(klb_ice_agent_t* p_agent, const char* p_host, int port)
{
    if ((NULL == p_agent) || (NULL == p_host) || (0 == p_host[0]))
    {
        return KLB_NETCODE_INVAL;
    }

    if ((0 > port) || (65535 < port))
    {
        return KLB_NETCODE_INVAL;
    }

    snprintf(p_agent->stun_host, sizeof(p_agent->stun_host), "%s", p_host);
    p_agent->stun_port = (0 == port) ? KLB_ICE_TURN_DEFAULT_PORT : port;

    return 0;
}


int klb_ice_agent_set_turn_server(klb_ice_agent_t* p_agent, const char* p_host, int port,
                                  const char* p_username, const char* p_password)
{
    if ((NULL == p_agent) || (NULL == p_host) || (0 == p_host[0]))
    {
        return KLB_NETCODE_INVAL;
    }

    if ((0 > port) || (65535 < port))
    {
        return KLB_NETCODE_INVAL;
    }

    snprintf(p_agent->turn_host, sizeof(p_agent->turn_host), "%s", p_host);
    p_agent->turn_port = (0 == port) ? KLB_ICE_TURN_DEFAULT_PORT : port;

    KLB_MEMSET(p_agent->turn_user, 0, sizeof(p_agent->turn_user));
    KLB_MEMSET(p_agent->turn_pass, 0, sizeof(p_agent->turn_pass));

    if (NULL != p_username)
    {
        snprintf(p_agent->turn_user, sizeof(p_agent->turn_user), "%s", p_username);
    }

    if (NULL != p_password)
    {
        snprintf(p_agent->turn_pass, sizeof(p_agent->turn_pass), "%s", p_password);
    }

    return 0;
}


int klb_ice_agent_start(klb_ice_agent_t* p_agent)
{
    if (NULL == p_agent)
    {
        return KLB_NETCODE_INVAL;
    }

    if ((0 == p_agent->local_count) || (0 == p_agent->remote_count))
    {
        return KLB_NETCODE_NOENT;
    }

    if ((KLB_ICE_STATE_CHECKING == p_agent->state) || (KLB_ICE_STATE_CONNECTED == p_agent->state))
    {
        return 0;
    }

    // step1. 生成全部候选对
    int best = -1;
    uint64_t best_prio = 0;

    p_agent->pair_count = 0;
    p_agent->selected = -1;

    for (int li = 0; li < p_agent->local_count; li++)
    {
        for (int ri = 0; ri < p_agent->remote_count; ri++)
        {
            klb_ice_pair_t* p_pair = &p_agent->pairs[p_agent->pair_count];

            memset(p_pair, 0, sizeof(*p_pair));
            p_pair->local_idx = li;
            p_pair->remote_idx = ri;
            p_pair->mapped_port = 0;
            p_pair->priority = pair_priority_klb_ice(p_agent,
                p_agent->locals[li].priority, p_agent->remotes[ri].priority);

            if ((-1 == best) || (p_pair->priority > best_prio))
            {
                best = p_agent->pair_count;
                best_prio = p_pair->priority;
            }

            p_agent->pair_count++;
        }
    }

    if (0 == p_agent->pair_count)
    {
        return KLB_NETCODE_NOENT;
    }

    // step2. 进入检查态, 最高优先级对由 controlling 侧提名
    p_agent->state = KLB_ICE_STATE_CHECKING;

    for (int i = 0; i < p_agent->pair_count; i++)
    {
        bool use_candidate = (KLB_ICE_ROLE_CONTROLLING == p_agent->role) && (best == i);

        if (0 == send_request_klb_ice(p_agent, i, use_candidate))
        {
            p_agent->pairs[i].retrans = 1;
        }
    }

    return 0;
}


int klb_ice_agent_get_state(klb_ice_agent_t* p_agent)
{
    if (NULL == p_agent)
    {
        return KLB_NETCODE_INVAL;
    }

    return p_agent->state;
}


int klb_ice_agent_stats(klb_ice_agent_t* p_agent, klb_ice_stats_t* p_stats)
{
    if ((NULL == p_agent) || (NULL == p_stats))
    {
        return KLB_NETCODE_INVAL;
    }

    memset(p_stats, 0, sizeof(*p_stats));

    p_stats->state = p_agent->state;
    p_stats->local_count = p_agent->local_count;
    p_stats->remote_count = p_agent->remote_count;
    p_stats->pair_count = p_agent->pair_count;
    p_stats->has_selected = (0 <= p_agent->selected) ? 1 : 0;
    p_stats->check_sent = p_agent->stat_check_sent;
    p_stats->check_recv = p_agent->stat_check_recv;
    p_stats->data_sent_pkts = p_agent->stat_data_sent_pkts;
    p_stats->data_sent_bytes = p_agent->stat_data_sent_bytes;
    p_stats->data_recv_pkts = p_agent->stat_data_recv_pkts;
    p_stats->data_recv_bytes = p_agent->stat_data_recv_bytes;

    if (0 <= p_agent->selected)
    {
        klb_ice_pair_t* p_pair = &p_agent->pairs[p_agent->selected];
        klb_ice_local_t* p_local = &p_agent->locals[p_pair->local_idx];
        klb_ice_remote_t* p_remote = &p_agent->remotes[p_pair->remote_idx];

        snprintf(p_stats->local_ip, sizeof(p_stats->local_ip), "%s", p_local->ip);
        p_stats->local_port = p_local->port;
        p_stats->local_type = p_local->type;
        p_stats->local_priority = p_local->priority;

        snprintf(p_stats->remote_ip, sizeof(p_stats->remote_ip), "%s", p_remote->ip);
        p_stats->remote_port = p_remote->port;
        p_stats->remote_type = p_remote->type;
        p_stats->remote_priority = p_remote->priority;
    }

    return 0;
}


int klb_ice_agent_send(klb_ice_agent_t* p_agent, const uint8_t* p_data, int len)
{
    if ((NULL == p_agent) || (NULL == p_data) || (0 > len))
    {
        return KLB_NETCODE_INVAL;
    }

    if ((0 > p_agent->selected) || (KLB_ICE_STATE_CONNECTED != p_agent->state))
    {
        return KLB_NETCODE_NOTCONN;
    }

    klb_ice_pair_t* p_pair = &p_agent->pairs[p_agent->selected];
    klb_ice_local_t* p_local = &p_agent->locals[p_pair->local_idx];
    klb_ice_remote_t* p_remote = &p_agent->remotes[p_pair->remote_idx];

    int ret = send_to_local_klb_ice(p_local, p_data, len, &p_remote->addr);
    if (0 == ret)
    {
        p_agent->stat_data_sent_pkts++;
        p_agent->stat_data_sent_bytes += (uint64_t)len;
    }

    return ret;
}


int klb_ice_agent_recv(klb_ice_agent_t* p_agent, uint8_t* p_buf, int cap, int* p_code)
{
    if ((NULL == p_agent) || (NULL == p_buf) || (0 >= cap))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return KLB_NETCODE_INVAL;
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    pump_retrans_klb_ice(p_agent);

    for (int li = 0; li < p_agent->local_count; li++)
    {
        klb_ice_local_t* p_local = &p_agent->locals[li];

        if ((NULL == p_local->p_socket) && (NULL == p_local->p_turn))
        {
            continue;
        }

        while (true)
        {
            uint8_t buf[KLB_ICE_STUN_BUF];
            struct sockaddr_storage from;
            int from_len = (int)sizeof(from);
            int n = 0;

            memset(&from, 0, sizeof(from));

            if (NULL != p_local->p_turn)
            {
                // relay 候选: 由 TURN 会话解出真实对端
                struct sockaddr_in* p_in = (struct sockaddr_in*)&from;
                char peer_ip[64] = { 0 };
                int peer_port = 0;
                int code = 0;

                n = klb_ice_turn_recv(p_local->p_turn, buf, (int)sizeof(buf),
                    peer_ip, (int)sizeof(peer_ip), &peer_port, &code);
                if (0 >= n)
                {
                    break;
                }

                p_in->sin_family = AF_INET;
                p_in->sin_port = htons((uint16_t)peer_port);

                if (1 != inet_pton(AF_INET, peer_ip, &p_in->sin_addr))
                {
                    continue;
                }

                from_len = (int)sizeof(struct sockaddr_in);
            }
            else
            {
                n = klb_socket_udp_recvfrom(p_local->p_socket, buf, (int)sizeof(buf),
                    (struct sockaddr*)&from, &from_len);
                if (0 > n)
                {
                    break;
                }
            }

            if (0 == n)
            {
                continue;
            }

            if (stun_is_message_klb_ice(buf, n))
            {
                handle_stun_klb_ice(p_agent, li, buf, n, (const struct sockaddr*)&from);
                continue;
            }

            if (0 > p_agent->selected)
            {
                continue;
            }

            klb_ice_pair_t* p_pair = &p_agent->pairs[p_agent->selected];

            if ((li != p_pair->local_idx)
                || (!remote_match_klb_ice(&p_agent->remotes[p_pair->remote_idx],
                    (const struct sockaddr*)&from)))
            {
                continue;
            }

            int copy_len = (n > cap) ? cap : n;

            memcpy(p_buf, buf, (size_t)copy_len);

            p_agent->stat_data_recv_pkts++;
            p_agent->stat_data_recv_bytes += (uint64_t)copy_len;

            return copy_len;
        }
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 本地凭据与候选枚举
//////////////////////////////////////////////////////////////////////////

/// @brief 取本地候选数
/// @param [in] *p_agent    代理
/// @return int 大于等于 0.候选数; 小于 0.klb_netcode_e
int klb_ice_agent_local_count(klb_ice_agent_t* p_agent)
{
    if (NULL == p_agent)
    {
        return KLB_NETCODE_INVAL;
    }

    return p_agent->local_count;
}


/// @brief 取本地候选 SDP 行
/// @param [in]  *p_agent   代理
/// @param [in]  index      候选下标
/// @param [out] *p_buf     输出缓存
/// @param [in]  cap        缓存大小
/// @return int 大于 0.写入长度; 小于 0.klb_netcode_e
int klb_ice_agent_local_candidate(klb_ice_agent_t* p_agent, int index,
                                  char* p_buf, int cap)
{
    if ((NULL == p_agent) || (NULL == p_buf) || (0 > index) || (p_agent->local_count <= index))
    {
        return KLB_NETCODE_INVAL;
    }

    klb_ice_local_t* p_local = &p_agent->locals[index];
    const char* p_type = "host";

    if (KLB_ICE_CAND_SRFLX == p_local->type)
    {
        p_type = "srflx";
    }
    else if (KLB_ICE_CAND_RELAY == p_local->type)
    {
        p_type = "relay";
    }

    int len = snprintf(p_buf, (size_t)cap, "candidate:%d 1 udp %u %s %d typ %s",
        index + 1, (unsigned int)p_local->priority, p_local->ip, p_local->port, p_type);
    if ((0 > len) || (cap <= len))
    {
        return KLB_NETCODE_MSGSIZE;
    }

    return len;
}


/// @brief 取本地 pwd
/// @param [in]  *p_agent   代理
/// @param [out] *p_buf     输出缓存
/// @param [in]  cap        缓存大小
/// @return int 大于等于 0.写入长度; 小于 0.klb_netcode_e
/// @note 供上层组装 SDP a=ice-pwd 行
int klb_ice_agent_local_pwd(klb_ice_agent_t* p_agent, char* p_buf, int cap)
{
    if ((NULL == p_agent) || (NULL == p_buf))
    {
        return KLB_NETCODE_INVAL;
    }

    int len = (int)strlen(p_agent->local_pwd);
    if (cap <= len)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    memcpy(p_buf, p_agent->local_pwd, (size_t)len);
    p_buf[len] = '\0';

    return len;
}


/// @brief 设置远端 ufrag/pwd
/// @param [in] *p_agent    代理
/// @param [in] *p_ufrag    远端 ufrag
/// @param [in] *p_pwd      远端 pwd
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 亦可在 add_remote_candidate 中传 a=ice-ufrag / a=ice-pwd 行
int klb_ice_agent_set_remote_credentials(klb_ice_agent_t* p_agent,
    const char* p_ufrag, const char* p_pwd)
{
    if ((NULL == p_agent) || (NULL == p_ufrag) || (NULL == p_pwd))
    {
        return KLB_NETCODE_INVAL;
    }

    if ((0 > copy_token_klb_ice(p_agent->remote_ufrag,
            (int)sizeof(p_agent->remote_ufrag), p_ufrag))
        || (0 > copy_token_klb_ice(p_agent->remote_pwd,
            (int)sizeof(p_agent->remote_pwd), p_pwd)))
    {
        return KLB_NETCODE_MSGSIZE;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// TURN 对外 API
//////////////////////////////////////////////////////////////////////////

klb_ice_turn_t* klb_ice_turn_create(const klb_ice_turn_cfg_t* p_cfg, int* p_code)
{
    if (NULL != p_code)
    {
        *p_code = 0;
    }

    if ((NULL == p_cfg) || (NULL == p_cfg->p_server) || (0 == p_cfg->p_server[0]))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    if ((0 > p_cfg->port) || (65535 < p_cfg->port))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    int port = (0 == p_cfg->port) ? KLB_ICE_TURN_DEFAULT_PORT : p_cfg->port;

    klb_ice_turn_t* p_turn = KLB_MALLOCZ(klb_ice_turn_t, 1, 0);

    if (0 != klb_socket_resolve_v4(p_cfg->p_server, port, &p_turn->server))
    {
        KLB_FREE(p_turn);

        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_DNS_FAIL;
        }

        return NULL;
    }

    p_turn->p_socket = klb_socket_udp_open(KLB_SOCKET_FAMILY_V4, NULL, 0, 0);
    if (NULL == p_turn->p_socket)
    {
        KLB_FREE(p_turn);

        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_BIND_FAIL;
        }

        return NULL;
    }

    if (NULL != p_cfg->p_username)
    {
        snprintf(p_turn->username, sizeof(p_turn->username), "%s", p_cfg->p_username);
    }

    if (NULL != p_cfg->p_password)
    {
        snprintf(p_turn->password, sizeof(p_turn->password), "%s", p_cfg->p_password);
    }

    p_turn->lifetime = 600;
    p_turn->next_channel = KLB_ICE_TURN_CHANNEL_MIN;

    return p_turn;
}


void klb_ice_turn_destroy(klb_ice_turn_t* p_turn)
{
    if (NULL == p_turn)
    {
        return;
    }

    if (NULL != p_turn->p_socket)
    {
        klb_socket_destroy(p_turn->p_socket);
        p_turn->p_socket = NULL;
    }

    KLB_FREE(p_turn);
}


int klb_ice_turn_allocate(klb_ice_turn_t* p_turn)
{
    uint8_t req[512];
    uint8_t rsp[KLB_ICE_TURN_MSG_MAX];
    uint8_t transport[4] = { 17, 0, 0, 0 };     // 17 = UDP
    bool tried_auth = false;

    if (NULL == p_turn)
    {
        return KLB_NETCODE_INVAL;
    }

    tried_auth = p_turn->key_valid;

    for (int pass = 0; pass < 2; pass++)
    {
        uint8_t txid[KLB_ICE_TXID_LEN];

        stun_new_txid_klb_ice(txid);
        stun_put_header_klb_ice(req, KLB_ICE_STUN_TYPE_ALLOCATE_REQ, txid);

        int pos = KLB_ICE_STUN_HEADER_LEN;
        int ret = stun_put_attr_klb_ice(req, (int)sizeof(req), &pos,
            KLB_ICE_ATTR_REQUESTED_TRANSPORT, transport, 4);
        if (0 != ret)
        {
            return ret;
        }

        if (tried_auth)
        {
            ret = turn_add_auth_klb_ice(p_turn, req, (int)sizeof(req), &pos);
            if (0 != ret)
            {
                return ret;
            }
        }

        ret = stun_add_fingerprint_klb_ice(req, (int)sizeof(req), &pos);
        if (0 != ret)
        {
            return ret;
        }

        int rsp_len = 0;
        ret = turn_transact_klb_ice(p_turn, req, pos, KLB_ICE_STUN_TYPE_ALLOCATE_OK,
            KLB_ICE_STUN_TYPE_ALLOCATE_ERR, rsp, (int)sizeof(rsp), &rsp_len);
        if (0 != ret)
        {
            return ret;
        }

        if (KLB_ICE_STUN_TYPE_ALLOCATE_OK == get_u16_klb_ice(rsp))
        {
            const uint8_t* p_val = NULL;
            int vlen = 0;

            ret = stun_find_attr_klb_ice(rsp, rsp_len, KLB_ICE_ATTR_XOR_RELAYED, &p_val, &vlen);
            if (0 != ret)
            {
                return KLB_NETCODE_PROTO_PARSE;
            }

            ret = stun_parse_xor_mapped_klb_ice(p_val, vlen, rsp + 8,
                p_turn->relay_ip, (int)sizeof(p_turn->relay_ip), &p_turn->relay_port);
            if (0 != ret)
            {
                return ret;
            }

            p_val = NULL;
            vlen = 0;

            if ((0 == stun_find_attr_klb_ice(rsp, rsp_len, KLB_ICE_ATTR_LIFETIME, &p_val, &vlen))
                && (4 <= vlen))
            {
                p_turn->lifetime = (int)get_u32_klb_ice(p_val);
            }

            return 0;
        }

        int err = stun_get_error_code_klb_ice(rsp, rsp_len);
        if ((401 != err) || tried_auth)
        {
            return (401 == err) ? KLB_NETCODE_ACCES : KLB_NETCODE_PROTO_UNEXPECTED;
        }

        if ((0 != stun_get_text_klb_ice(rsp, rsp_len, KLB_ICE_ATTR_REALM,
                p_turn->realm, (int)sizeof(p_turn->realm)))
            || (0 != stun_get_text_klb_ice(rsp, rsp_len, KLB_ICE_ATTR_NONCE,
                p_turn->nonce, (int)sizeof(p_turn->nonce))))
        {
            return KLB_NETCODE_PROTO_PARSE;
        }

        ret = turn_make_key_klb_ice(p_turn);
        if (0 != ret)
        {
            return ret;
        }

        tried_auth = true;
    }

    return KLB_NETCODE_ACCES;
}


int klb_ice_turn_refresh(klb_ice_turn_t* p_turn, int lifetime)
{
    uint8_t req[512];
    uint8_t rsp[KLB_ICE_TURN_MSG_MAX];
    uint8_t txid[KLB_ICE_TXID_LEN];
    uint8_t val[4];

    if (NULL == p_turn)
    {
        return KLB_NETCODE_INVAL;
    }

    if (!p_turn->key_valid)
    {
        return KLB_NETCODE_ACCES;
    }

    if (0 >= lifetime)
    {
        lifetime = 600;
    }

    stun_new_txid_klb_ice(txid);
    stun_put_header_klb_ice(req, KLB_ICE_STUN_TYPE_REFRESH_REQ, txid);

    int pos = KLB_ICE_STUN_HEADER_LEN;
    put_u32_klb_ice(val, (uint32_t)lifetime);

    int ret = stun_put_attr_klb_ice(req, (int)sizeof(req), &pos, KLB_ICE_ATTR_LIFETIME, val, 4);
    if (0 != ret)
    {
        return ret;
    }

    ret = turn_add_auth_klb_ice(p_turn, req, (int)sizeof(req), &pos);
    if (0 != ret)
    {
        return ret;
    }

    ret = stun_add_fingerprint_klb_ice(req, (int)sizeof(req), &pos);
    if (0 != ret)
    {
        return ret;
    }

    int rsp_len = 0;
    ret = turn_transact_klb_ice(p_turn, req, pos, KLB_ICE_STUN_TYPE_REFRESH_OK,
        KLB_ICE_STUN_TYPE_REFRESH_ERR, rsp, (int)sizeof(rsp), &rsp_len);
    if (0 != ret)
    {
        return ret;
    }

    if (KLB_ICE_STUN_TYPE_REFRESH_OK != get_u16_klb_ice(rsp))
    {
        return KLB_NETCODE_PROTO_UNEXPECTED;
    }

    p_turn->lifetime = lifetime;

    return 0;
}


int klb_ice_turn_create_permission(klb_ice_turn_t* p_turn, const char* p_peer_ip, int peer_port)
{
    uint8_t req[512];
    uint8_t rsp[KLB_ICE_TURN_MSG_MAX];
    uint8_t txid[KLB_ICE_TXID_LEN];

    if ((NULL == p_turn) || (NULL == p_peer_ip))
    {
        return KLB_NETCODE_INVAL;
    }

    if ((0 >= peer_port) || (65535 < peer_port))
    {
        return KLB_NETCODE_INVAL;
    }

    if (!p_turn->key_valid)
    {
        return KLB_NETCODE_ACCES;
    }

    stun_new_txid_klb_ice(txid);
    stun_put_header_klb_ice(req, KLB_ICE_STUN_TYPE_PERM_REQ, txid);

    int pos = KLB_ICE_STUN_HEADER_LEN;
    int ret = turn_put_xor_addr_klb_ice(req, (int)sizeof(req), &pos,
        KLB_ICE_ATTR_XOR_PEER, p_peer_ip, peer_port, txid);
    if (0 != ret)
    {
        return ret;
    }

    ret = turn_add_auth_klb_ice(p_turn, req, (int)sizeof(req), &pos);
    if (0 != ret)
    {
        return ret;
    }

    ret = stun_add_fingerprint_klb_ice(req, (int)sizeof(req), &pos);
    if (0 != ret)
    {
        return ret;
    }

    int rsp_len = 0;
    ret = turn_transact_klb_ice(p_turn, req, pos, KLB_ICE_STUN_TYPE_PERM_OK,
        KLB_ICE_STUN_TYPE_PERM_ERR, rsp, (int)sizeof(rsp), &rsp_len);
    if (0 != ret)
    {
        return ret;
    }

    if (KLB_ICE_STUN_TYPE_PERM_OK != get_u16_klb_ice(rsp))
    {
        return KLB_NETCODE_PROTO_UNEXPECTED;
    }

    if (NULL == turn_add_peer_klb_ice(p_turn, p_peer_ip, peer_port))
    {
        return KLB_NETCODE_LIMIT;
    }

    return 0;
}


int klb_ice_turn_channel_bind(klb_ice_turn_t* p_turn, const char* p_peer_ip, int peer_port,
                              int channel)
{
    uint8_t req[512];
    uint8_t rsp[KLB_ICE_TURN_MSG_MAX];
    uint8_t txid[KLB_ICE_TXID_LEN];
    uint8_t val[4];
    int chan = channel;

    if ((NULL == p_turn) || (NULL == p_peer_ip))
    {
        return KLB_NETCODE_INVAL;
    }

    if ((0 >= peer_port) || (65535 < peer_port))
    {
        return KLB_NETCODE_INVAL;
    }

    if (!p_turn->key_valid)
    {
        return KLB_NETCODE_ACCES;
    }

    if (0 >= chan)
    {
        if (KLB_ICE_TURN_CHANNEL_MAX < p_turn->next_channel)
        {
            return KLB_NETCODE_LIMIT;
        }

        chan = p_turn->next_channel;
    }
    else if ((KLB_ICE_TURN_CHANNEL_MIN > chan) || (KLB_ICE_TURN_CHANNEL_MAX < chan))
    {
        return KLB_NETCODE_INVAL;
    }

    stun_new_txid_klb_ice(txid);
    stun_put_header_klb_ice(req, KLB_ICE_STUN_TYPE_CHBIND_REQ, txid);

    int pos = KLB_ICE_STUN_HEADER_LEN;
    put_u16_klb_ice(val, (uint16_t)chan);
    put_u16_klb_ice(val + 2, 0);

    int ret = stun_put_attr_klb_ice(req, (int)sizeof(req), &pos,
        KLB_ICE_ATTR_CHANNEL_NUMBER, val, 4);
    if (0 != ret)
    {
        return ret;
    }

    ret = turn_put_xor_addr_klb_ice(req, (int)sizeof(req), &pos,
        KLB_ICE_ATTR_XOR_PEER, p_peer_ip, peer_port, txid);
    if (0 != ret)
    {
        return ret;
    }

    ret = turn_add_auth_klb_ice(p_turn, req, (int)sizeof(req), &pos);
    if (0 != ret)
    {
        return ret;
    }

    ret = stun_add_fingerprint_klb_ice(req, (int)sizeof(req), &pos);
    if (0 != ret)
    {
        return ret;
    }

    int rsp_len = 0;
    ret = turn_transact_klb_ice(p_turn, req, pos, KLB_ICE_STUN_TYPE_CHBIND_OK,
        KLB_ICE_STUN_TYPE_CHBIND_ERR, rsp, (int)sizeof(rsp), &rsp_len);
    if (0 != ret)
    {
        return ret;
    }

    if (KLB_ICE_STUN_TYPE_CHBIND_OK != get_u16_klb_ice(rsp))
    {
        return KLB_NETCODE_PROTO_UNEXPECTED;
    }

    klb_ice_turn_peer_t* p_peer = turn_add_peer_klb_ice(p_turn, p_peer_ip, peer_port);
    if (NULL == p_peer)
    {
        return KLB_NETCODE_LIMIT;
    }

    p_peer->channel = chan;

    if (chan == p_turn->next_channel)
    {
        p_turn->next_channel++;
    }

    return 0;
}


int klb_ice_turn_relayed_address(klb_ice_turn_t* p_turn, char* p_ip, int ip_cap, int* p_port)
{
    if ((NULL == p_turn) || (NULL == p_ip) || (0 >= ip_cap))
    {
        return KLB_NETCODE_INVAL;
    }

    if (0 == p_turn->relay_ip[0])
    {
        return KLB_NETCODE_NOENT;
    }

    int len = (int)strlen(p_turn->relay_ip);
    if (ip_cap <= len)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    memcpy(p_ip, p_turn->relay_ip, (size_t)len + 1);

    if (NULL != p_port)
    {
        *p_port = p_turn->relay_port;
    }

    return 0;
}


int klb_ice_turn_send(klb_ice_turn_t* p_turn, const char* p_peer_ip, int peer_port,
                      const uint8_t* p_data, int len)
{
    if ((NULL == p_turn) || (NULL == p_peer_ip) || (NULL == p_data) || (0 > len))
    {
        return KLB_NETCODE_INVAL;
    }

    if ((0 == len) || (KLB_ICE_TURN_MSG_MAX < len))
    {
        return KLB_NETCODE_MSGSIZE;
    }

    klb_ice_turn_peer_t* p_peer = turn_find_peer_klb_ice(p_turn, p_peer_ip, peer_port);

    if (NULL == p_peer)
    {
        // 惰性安装权限
        int ret = klb_ice_turn_create_permission(p_turn, p_peer_ip, peer_port);
        if (0 != ret)
        {
            return ret;
        }

        p_peer = turn_find_peer_klb_ice(p_turn, p_peer_ip, peer_port);
        if (NULL == p_peer)
        {
            return KLB_NETCODE_NOENT;
        }
    }

    uint8_t buf[KLB_ICE_TURN_MSG_MAX];
    int total = 0;

    if (0 < p_peer->channel)
    {
        // ChannelData: 2 字节通道号 + 2 字节长度 + 载荷
        if ((int)sizeof(buf) < (len + 4))
        {
            return KLB_NETCODE_MSGSIZE;
        }

        put_u16_klb_ice(buf, (uint16_t)p_peer->channel);
        put_u16_klb_ice(buf + 2, (uint16_t)len);
        memcpy(buf + 4, p_data, (size_t)len);
        total = len + 4;
    }
    else
    {
        uint8_t txid[KLB_ICE_TXID_LEN];

        stun_new_txid_klb_ice(txid);
        stun_put_header_klb_ice(buf, KLB_ICE_STUN_TYPE_SEND_IND, txid);

        int pos = KLB_ICE_STUN_HEADER_LEN;
        int ret = turn_put_xor_addr_klb_ice(buf, (int)sizeof(buf), &pos,
            KLB_ICE_ATTR_XOR_PEER, p_peer_ip, peer_port, txid);
        if (0 != ret)
        {
            return ret;
        }

        ret = stun_put_attr_klb_ice(buf, (int)sizeof(buf), &pos, KLB_ICE_ATTR_DATA, p_data, len);
        if (0 != ret)
        {
            return ret;
        }

        // Send Indication 不携带 MESSAGE-INTEGRITY, 仅封口头部 length
        put_u16_klb_ice(buf + 2, (uint16_t)(pos - KLB_ICE_STUN_HEADER_LEN));
        total = pos;
    }

    return klb_socket_udp_sendto(p_turn->p_socket, buf, total,
        (const struct sockaddr*)&p_turn->server, (int)sizeof(p_turn->server));
}


int klb_ice_turn_recv(klb_ice_turn_t* p_turn, uint8_t* p_buf, int cap,
                      char* p_peer_ip, int ip_cap, int* p_peer_port, int* p_code)
{
    if ((NULL == p_turn) || (NULL == p_buf) || (0 >= cap))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return KLB_NETCODE_INVAL;
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    while (true)
    {
        uint8_t buf[KLB_ICE_TURN_MSG_MAX];
        struct sockaddr_storage from;
        int from_len = (int)sizeof(from);

        memset(&from, 0, sizeof(from));

        int n = klb_socket_udp_recvfrom(p_turn->p_socket, buf, (int)sizeof(buf),
            (struct sockaddr*)&from, &from_len);
        if (0 > n)
        {
            return 0;
        }

        if (0 == n)
        {
            continue;
        }

        // ChannelData: 首字节 0x40~0x7F
        if ((0x40 <= buf[0]) && (0x7F >= buf[0]))
        {
            if (4 > n)
            {
                continue;
            }

            int channel = (int)get_u16_klb_ice(buf);
            int dlen = (int)get_u16_klb_ice(buf + 2);

            if ((4 + dlen) > n)
            {
                dlen = n - 4;
            }

            for (int i = 0; i < p_turn->peer_count; i++)
            {
                if (channel != p_turn->peers[i].channel)
                {
                    continue;
                }

                if (NULL != p_peer_ip)
                {
                    snprintf(p_peer_ip, (size_t)ip_cap, "%s", p_turn->peers[i].ip);
                }

                if (NULL != p_peer_port)
                {
                    *p_peer_port = p_turn->peers[i].port;
                }

                break;
            }

            if (dlen > cap)
            {
                dlen = cap;
            }

            memcpy(p_buf, buf + 4, (size_t)dlen);

            return dlen;
        }

        if (!stun_is_message_klb_ice(buf, n))
        {
            continue;
        }

        if (KLB_ICE_STUN_TYPE_DATA_IND != get_u16_klb_ice(buf))
        {
            continue;
        }

        const uint8_t* p_val = NULL;
        int vlen = 0;

        if (0 == stun_find_attr_klb_ice(buf, n, KLB_ICE_ATTR_XOR_PEER, &p_val, &vlen))
        {
            char ip[64] = { 0 };
            int port = 0;

            if (0 == stun_parse_xor_mapped_klb_ice(p_val, vlen, buf + 8,
                ip, (int)sizeof(ip), &port))
            {
                if (NULL != p_peer_ip)
                {
                    snprintf(p_peer_ip, (size_t)ip_cap, "%s", ip);
                }

                if (NULL != p_peer_port)
                {
                    *p_peer_port = port;
                }
            }
        }

        p_val = NULL;
        vlen = 0;

        if (0 != stun_find_attr_klb_ice(buf, n, KLB_ICE_ATTR_DATA, &p_val, &vlen))
        {
            continue;
        }

        if (vlen > cap)
        {
            vlen = cap;
        }

        memcpy(p_buf, p_val, (size_t)vlen);

        return vlen;
    }
}

#else

klb_ice_agent_t* klb_ice_agent_create(klb_netmulti_t* p_netmulti, int role, int* p_code)
{
    (void)p_netmulti;
    (void)role;

    if (NULL != p_code)
    {
        *p_code = KLB_NETCODE_NOTSUP;
    }

    return NULL;
}


void klb_ice_agent_destroy(klb_ice_agent_t* p_agent)
{
    (void)p_agent;
}


int klb_ice_agent_local_ufrag(klb_ice_agent_t* p_agent, char* p_buf, int cap)
{
    (void)p_agent;
    (void)p_buf;
    (void)cap;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_local_count(klb_ice_agent_t* p_agent)
{
    (void)p_agent;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_local_candidate(klb_ice_agent_t* p_agent, int index, char* p_buf, int cap)
{
    (void)p_agent;
    (void)index;
    (void)p_buf;
    (void)cap;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_local_pwd(klb_ice_agent_t* p_agent, char* p_buf, int cap)
{
    (void)p_agent;
    (void)p_buf;
    (void)cap;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_set_remote_credentials(klb_ice_agent_t* p_agent,
    const char* p_ufrag, const char* p_pwd)
{
    (void)p_agent;
    (void)p_ufrag;
    (void)p_pwd;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_add_remote_candidate(klb_ice_agent_t* p_agent, const char* p_candidate)
{
    (void)p_agent;
    (void)p_candidate;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_set_stun_server(klb_ice_agent_t* p_agent, const char* p_host, int port)
{
    (void)p_agent;
    (void)p_host;
    (void)port;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_set_turn_server(klb_ice_agent_t* p_agent, const char* p_host, int port,
                                  const char* p_username, const char* p_password)
{
    (void)p_agent;
    (void)p_host;
    (void)port;
    (void)p_username;
    (void)p_password;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_gather(klb_ice_agent_t* p_agent)
{
    (void)p_agent;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_restart(klb_ice_agent_t* p_agent)
{
    (void)p_agent;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_start(klb_ice_agent_t* p_agent)
{
    (void)p_agent;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_get_state(klb_ice_agent_t* p_agent)
{
    (void)p_agent;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_stats(klb_ice_agent_t* p_agent, klb_ice_stats_t* p_stats)
{
    (void)p_agent;
    (void)p_stats;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_send(klb_ice_agent_t* p_agent, const uint8_t* p_data, int len)
{
    (void)p_agent;
    (void)p_data;
    (void)len;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_agent_recv(klb_ice_agent_t* p_agent, uint8_t* p_buf, int cap, int* p_code)
{
    (void)p_agent;
    (void)p_buf;
    (void)cap;

    if (NULL != p_code)
    {
        *p_code = KLB_NETCODE_NOTSUP;
    }

    return KLB_NETCODE_NOTSUP;
}


klb_ice_turn_t* klb_ice_turn_create(const klb_ice_turn_cfg_t* p_cfg, int* p_code)
{
    (void)p_cfg;

    if (NULL != p_code)
    {
        *p_code = KLB_NETCODE_NOTSUP;
    }

    return NULL;
}


void klb_ice_turn_destroy(klb_ice_turn_t* p_turn)
{
    (void)p_turn;
}


int klb_ice_turn_allocate(klb_ice_turn_t* p_turn)
{
    (void)p_turn;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_turn_refresh(klb_ice_turn_t* p_turn, int lifetime)
{
    (void)p_turn;
    (void)lifetime;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_turn_create_permission(klb_ice_turn_t* p_turn, const char* p_peer_ip, int peer_port)
{
    (void)p_turn;
    (void)p_peer_ip;
    (void)peer_port;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_turn_channel_bind(klb_ice_turn_t* p_turn, const char* p_peer_ip, int peer_port, int channel)
{
    (void)p_turn;
    (void)p_peer_ip;
    (void)peer_port;
    (void)channel;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_turn_relayed_address(klb_ice_turn_t* p_turn, char* p_ip, int ip_cap, int* p_port)
{
    (void)p_turn;
    (void)p_ip;
    (void)ip_cap;
    (void)p_port;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_turn_send(klb_ice_turn_t* p_turn, const char* p_peer_ip, int peer_port,
                      const uint8_t* p_data, int len)
{
    (void)p_turn;
    (void)p_peer_ip;
    (void)peer_port;
    (void)p_data;
    (void)len;

    return KLB_NETCODE_NOTSUP;
}


int klb_ice_turn_recv(klb_ice_turn_t* p_turn, uint8_t* p_buf, int cap,
                      char* p_peer_ip, int ip_cap, int* p_peer_port, int* p_code)
{
    (void)p_turn;
    (void)p_buf;
    (void)cap;
    (void)p_peer_ip;
    (void)ip_cap;
    (void)p_peer_port;

    if (NULL != p_code)
    {
        *p_code = KLB_NETCODE_NOTSUP;
    }

    return KLB_NETCODE_NOTSUP;
}

#endif

// end
