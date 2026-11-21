// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_ice.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   ICE 代理 (RFC 8445, 含 STUN/TURN 通道); host/srflx/relay 候选 + 连通性检查
/// @version 0.4
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.2 实现 host 候选收集 / STUN Binding / 候选对检查; 补本地 pwd/候选枚举与远端凭据接口; 0.3 增加 TURN 客户端 / srflx/relay 候选 / ICE restart
/// @warning TURN 为同步最小实现; IPv6 候选未接入
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_ICE_H__
#define __KLB_ICE_H__

#include "klb_type.h"
#include "klbnet/klb_netmulti.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @enum  klb_ice_role_e
/// @brief ICE 角色 (RFC 8445)
typedef enum klb_ice_role_e_
{
    KLB_ICE_ROLE_CONTROLLED  = 0,       ///< 被动方
    KLB_ICE_ROLE_CONTROLLING = 1,       ///< 主动方
}klb_ice_role_e;


/// @enum  klb_ice_state_e
/// @brief ICE 连通状态
typedef enum klb_ice_state_e_
{
    KLB_ICE_STATE_NEW       = 0,        ///< 初始
    KLB_ICE_STATE_GATHERING = 1,        ///< 收集候选
    KLB_ICE_STATE_CHECKING  = 2,        ///< 连通性检查
    KLB_ICE_STATE_CONNECTED = 3,        ///< 已连通
    KLB_ICE_STATE_FAILED    = 4,        ///< 失败
    KLB_ICE_STATE_CLOSED    = 5,        ///< 已关闭
}klb_ice_state_e;


/// @enum  klb_ice_cand_type_e
/// @brief 候选类型 (SDP typ)
typedef enum klb_ice_cand_type_e_
{
    KLB_ICE_CAND_HOST   = 0,            ///< host
    KLB_ICE_CAND_SRFLX  = 1,            ///< srflx
    KLB_ICE_CAND_RELAY  = 2,            ///< relay
}klb_ice_cand_type_e;


/// @struct klb_ice_stats_t
/// @brief  ICE 运行统计 (getStats 子集)
typedef struct klb_ice_stats_t_
{
    int         state;                  ///< klb_ice_state_e
    int         local_count;            ///< 本地候选数
    int         remote_count;           ///< 远端候选数
    int         pair_count;             ///< 候选对数
    int         has_selected;           ///< 1.已选中工作对; 0.未选中
    char        local_ip[64];           ///< 工作对本地 IP
    int         local_port;             ///< 工作对本地端口
    int         local_type;             ///< 工作对本地候选类型: klb_ice_cand_type_e
    uint32_t    local_priority;         ///< 工作对本地候选优先级
    char        remote_ip[64];          ///< 工作对远端 IP
    int         remote_port;            ///< 工作对远端端口
    int         remote_type;            ///< 工作对远端候选类型: klb_ice_cand_type_e
    uint32_t    remote_priority;        ///< 工作对远端候选优先级
    uint64_t    check_sent;             ///< 连通性检查发送数
    uint64_t    check_recv;             ///< 连通性检查接收数
    uint64_t    data_sent_pkts;         ///< 应用数据发送包数
    uint64_t    data_sent_bytes;        ///< 应用数据发送字节数
    uint64_t    data_recv_pkts;         ///< 应用数据接收包数
    uint64_t    data_recv_bytes;        ///< 应用数据接收字节数
}klb_ice_stats_t;


/// @brief ICE 代理 (不透明)
typedef struct klb_ice_agent_t_ klb_ice_agent_t;


/// @brief TURN 会话 (不透明)
typedef struct klb_ice_turn_t_ klb_ice_turn_t;


/// @struct klb_ice_turn_cfg_t
/// @brief  TURN 服务器配置
typedef struct klb_ice_turn_cfg_t_
{
    const char*     p_server;       ///< 服务器 IP/域名
    int             port;           ///< 端口; 0 取 3478
    const char*     p_username;     ///< 长期凭据用户名; 可 NULL
    const char*     p_password;     ///< 长期凭据密码; 可 NULL
}klb_ice_turn_cfg_t;


/// @brief 创建 ICE 代理
/// @param [in] *p_netmulti     复用模块; 不可为 NULL
/// @param [in] role            角色: klb_ice_role_e
/// @param [out] *p_code        可选; 失败时 klb_netcode_e; 成功为 0
/// @return klb_ice_agent_t* NULL.失败; 非 NULL.代理
KLB_API klb_ice_agent_t* klb_ice_agent_create(klb_netmulti_t* p_netmulti, int role, int* p_code);


/// @brief 销毁 ICE 代理
/// @param [in] *p_agent        代理; 可为 NULL
/// @return 无
KLB_API void klb_ice_agent_destroy(klb_ice_agent_t* p_agent);


/// @brief 取本地 ufrag
/// @param [in]  *p_agent       代理
/// @param [out] *p_buf         输出缓存
/// @param [in]  cap            缓存大小
/// @return int >0.写入长度; 小于 0.klb_netcode_e
KLB_API int klb_ice_agent_local_ufrag(klb_ice_agent_t* p_agent, char* p_buf, int cap);


/// @brief 取本地候选数 (须先 gather)
/// @param [in] *p_agent        代理
/// @return int 大于等于 0.候选数; 小于 0.klb_netcode_e
KLB_API int klb_ice_agent_local_count(klb_ice_agent_t* p_agent);


/// @brief 取本地候选 SDP 行 (须先 gather)
/// @param [in]  *p_agent       代理
/// @param [in]  index          候选下标
/// @param [out] *p_buf         输出缓存
/// @param [in]  cap            缓存大小
/// @return int >0.写入长度; 小于 0.klb_netcode_e
/// @note 输出形如 "candidate:1 1 udp 2130706431 192.0.2.2 5000 typ host"
KLB_API int klb_ice_agent_local_candidate(klb_ice_agent_t* p_agent, int index,
                                          char* p_buf, int cap);


/// @brief 取本地 pwd
/// @param [in]  *p_agent       代理
/// @param [out] *p_buf         输出缓存
/// @param [in]  cap            缓存大小
/// @return int >0.写入长度; 小于 0.klb_netcode_e
/// @note 供上层组装 SDP a=ice-pwd 行
KLB_API int klb_ice_agent_local_pwd(klb_ice_agent_t* p_agent, char* p_buf, int cap);


/// @brief 设置远端 ufrag/pwd
/// @param [in] *p_agent        代理
/// @param [in] *p_ufrag        远端 ufrag
/// @param [in] *p_pwd          远端 pwd
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 也可在 klb_ice_agent_add_remote_candidate 中传 a=ice-ufrag / a=ice-pwd 行
KLB_API int klb_ice_agent_set_remote_credentials(klb_ice_agent_t* p_agent,
    const char* p_ufrag, const char* p_pwd);


/// @brief 添加远端候选 (SDP candidate 行)
/// @param [in] *p_agent        代理
/// @param [in] *p_candidate    候选串
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_agent_add_remote_candidate(klb_ice_agent_t* p_agent, const char* p_candidate);


/// @brief 设置 STUN 服务器 (用于收集 srflx 候选)
/// @param [in] *p_agent        代理
/// @param [in] *p_host         服务器 IP/域名
/// @param [in] port            端口; 0 取 3478
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 须在 gather 前调用
KLB_API int klb_ice_agent_set_stun_server(klb_ice_agent_t* p_agent, const char* p_host, int port);


/// @brief 设置 TURN 服务器 (用于收集 relay 候选)
/// @param [in] *p_agent        代理
/// @param [in] *p_host         服务器 IP/域名
/// @param [in] port            端口; 0 取 3478
/// @param [in] *p_username     长期凭据用户名; 可 NULL
/// @param [in] *p_password     长期凭据密码; 可 NULL
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 须在 gather 前调用
KLB_API int klb_ice_agent_set_turn_server(klb_ice_agent_t* p_agent, const char* p_host, int port,
                                          const char* p_username, const char* p_password);


/// @brief 开始收集本地候选 (host / srflx / relay)
/// @param [in] *p_agent        代理
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_agent_gather(klb_ice_agent_t* p_agent);


/// @brief ICE restart: 重生成本地 ufrag/pwd 并复位候选对
/// @param [in] *p_agent        代理
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 保留本地 socket 与候选; 远端凭据/候选须由上层重新交换后再次 start
KLB_API int klb_ice_agent_restart(klb_ice_agent_t* p_agent);


/// @brief 开始连通性检查
/// @param [in] *p_agent        代理
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_agent_start(klb_ice_agent_t* p_agent);


/// @brief 取当前状态
/// @param [in] *p_agent        代理
/// @return int klb_ice_state_e; 小于 0.klb_netcode_e
KLB_API int klb_ice_agent_get_state(klb_ice_agent_t* p_agent);


/// @brief 取运行统计 (工作对 / 收发计数)
/// @param [in]  *p_agent       代理
/// @param [out] *p_stats       输出统计
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_agent_stats(klb_ice_agent_t* p_agent, klb_ice_stats_t* p_stats);


/// @brief 经选中候选对发送应用数据
/// @param [in] *p_agent        代理
/// @param [in] *p_data         数据
/// @param [in] len             长度
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_agent_send(klb_ice_agent_t* p_agent, const uint8_t* p_data, int len);


/// @brief 接收应用数据
/// @param [in]  *p_agent       代理
/// @param [out] *p_buf         输出缓存
/// @param [in]  cap            缓存大小
/// @param [out] *p_code        可选; klb_netcode_e
/// @return int >0.字节数; 小于 0.klb_netcode_e
KLB_API int klb_ice_agent_recv(klb_ice_agent_t* p_agent, uint8_t* p_buf, int cap, int* p_code);


/// @brief 创建 TURN 会话
/// @param [in]  *p_cfg         配置; 不可为 NULL
/// @param [out] *p_code        可选; 失败时 klb_netcode_e; 成功为 0
/// @return klb_ice_turn_t* NULL.失败; 非 NULL.会话
KLB_API klb_ice_turn_t* klb_ice_turn_create(const klb_ice_turn_cfg_t* p_cfg, int* p_code);


/// @brief 销毁 TURN 会话
/// @param [in] *p_turn         会话; 可为 NULL
/// @return 无
KLB_API void klb_ice_turn_destroy(klb_ice_turn_t* p_turn);


/// @brief 申请中继地址 (Allocate; 自动处理 401 长期凭据挑战)
/// @param [in] *p_turn         会话
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_turn_allocate(klb_ice_turn_t* p_turn);


/// @brief 刷新分配 (Refresh)
/// @param [in] *p_turn         会话
/// @param [in] lifetime        生命周期 (秒); 0 取 600
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_turn_refresh(klb_ice_turn_t* p_turn, int lifetime);


/// @brief 安装对端权限 (CreatePermission)
/// @param [in] *p_turn         会话
/// @param [in] *p_peer_ip      对端 IP
/// @param [in] peer_port       对端端口
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_turn_create_permission(klb_ice_turn_t* p_turn,
    const char* p_peer_ip, int peer_port);


/// @brief 绑定通道 (ChannelBind)
/// @param [in] *p_turn         会话
/// @param [in] *p_peer_ip      对端 IP
/// @param [in] peer_port       对端端口
/// @param [in] channel         通道号; 0 自动分配
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_turn_channel_bind(klb_ice_turn_t* p_turn,
    const char* p_peer_ip, int peer_port, int channel);


/// @brief 取中继地址 (XOR-RELAYED-ADDRESS; 须先 allocate)
/// @param [in]  *p_turn        会话
/// @param [out] *p_ip          输出 IP 文本
/// @param [in]  ip_cap         IP 缓存大小
/// @param [out] *p_port        输出端口; 可为 NULL
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_turn_relayed_address(klb_ice_turn_t* p_turn, char* p_ip, int ip_cap, int* p_port);


/// @brief 经中继发送数据 (ChannelData 或 Send Indication)
/// @param [in] *p_turn         会话
/// @param [in] *p_peer_ip      对端 IP
/// @param [in] peer_port       对端端口
/// @param [in] *p_data         数据
/// @param [in] len             长度
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_ice_turn_send(klb_ice_turn_t* p_turn, const char* p_peer_ip, int peer_port,
                              const uint8_t* p_data, int len);


/// @brief 经中继接收数据
/// @param [in]  *p_turn        会话
/// @param [out] *p_buf         输出缓存
/// @param [in]  cap            缓存大小
/// @param [out] *p_peer_ip     对端 IP 输出; 可为 NULL
/// @param [in]  ip_cap         对端 IP 缓存大小
/// @param [out] *p_peer_port   对端端口输出; 可为 NULL
/// @param [out] *p_code        可选; klb_netcode_e
/// @return int >0.字节数; 0.暂无数据; 小于 0.klb_netcode_e
KLB_API int klb_ice_turn_recv(klb_ice_turn_t* p_turn, uint8_t* p_buf, int cap,
                              char* p_peer_ip, int ip_cap, int* p_peer_port, int* p_code);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_ICE_H__

// end
