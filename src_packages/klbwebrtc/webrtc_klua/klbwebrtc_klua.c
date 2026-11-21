// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwebrtc_klua.c
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klbwebrtc 的 Lua 绑定 (扩展库 "kwebrtc")
/// @version 0.8
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.2 实现 peer 工厂 / offer-answer / 远端 SDP / 候选 / SDP 解析; 0.3 增加 stats / set_dtls_cert / DataChannel 打开与收发; 0.4 增加媒体面绑定: RTP/H.264 收发 + RTCP/NACK 发送; 0.5 增加会话事件绑定: event_poll; 0.6 增加媒体协商与 RTCP 报告绑定: add_media / SR / RR / TWCC; 0.7 增加 twcc_flush 绑定 / stats 增加码率与丢包率字段
///////////////////////////////////////////////////////////////////////////
#include "klbwebrtc/klbwebrtc.h"
#include "klbwebrtc/core/klb_webrtc.h"
#include "klbnet/klb_netcode.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klbmem/klb_mem.h"
#include <string.h>


/// @def   KLUA_KWEBRTC_HANDLE
/// @brief Lua userdata 元表名
#define KLUA_KWEBRTC_HANDLE             "KWEBRTC_PEER_HANDLE*"

/// @def   KLUA_KWEBRTC_SDP_MAX
/// @brief SDP 文本缓冲长度
#define KLUA_KWEBRTC_SDP_MAX            8192

/// @def   KLUA_KWEBRTC_RECV_MAX
/// @brief 单次接收缓冲长度
#define KLUA_KWEBRTC_RECV_MAX           2048

/// @def   KLUA_KWEBRTC_MEDIA_MAX
/// @brief 媒体面单次接收默认缓冲长度
#define KLUA_KWEBRTC_MEDIA_MAX          (64 * 1024)

/// @def   KLUA_KWEBRTC_MEDIA_LIMIT
/// @brief 媒体面单次接收缓冲上限
#define KLUA_KWEBRTC_MEDIA_LIMIT        (256 * 1024)


/// @struct klua_kwebrtc_t
/// @brief  kwebrtc 会话的 Lua 包装
typedef struct klua_kwebrtc_t_
{
    lua_State*          L;          ///< Lua 状态
    klb_webrtc_peer_t*  p_peer;     ///< 会话
}klua_kwebrtc_t;


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_kwebrtc(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


/// @brief 统一返回: 小于 0 压入 nil, code; 否则压入 true
static int push_rc_klua_kwebrtc(lua_State* L, int code)
{
    if (0 > code)
    {
        return push_err_klua_kwebrtc(L, code);
    }

    lua_pushboolean(L, 1);
    return 1;
}


////////////////////////////////////////
static klua_kwebrtc_t* new_klua_kwebrtc(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = (klua_kwebrtc_t*)lua_newuserdata(L, sizeof(klua_kwebrtc_t));
    KLB_MEMSET(p_kwebrtc, 0, sizeof(klua_kwebrtc_t));
    luaL_setmetatable(L, KLUA_KWEBRTC_HANDLE);
    return p_kwebrtc;
}


static klua_kwebrtc_t* to_klua_kwebrtc(lua_State* L, int index)
{
    klua_kwebrtc_t* p_kwebrtc = (klua_kwebrtc_t*)luaL_checkudata(L, index, KLUA_KWEBRTC_HANDLE);
    luaL_argcheck(L, NULL != p_kwebrtc, index, "'kwebrtc' expected");
    return p_kwebrtc;
}


static int klua_kwebrtc_tostring(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);

    lua_pushfstring(L, "kwebrtc:%p", p_kwebrtc->p_peer);
    return 1;
}


static int klua_kwebrtc_peer_close(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);

    if (NULL != p_kwebrtc->p_peer)
    {
        klb_webrtc_peer_destroy(p_kwebrtc->p_peer);
        p_kwebrtc->p_peer = NULL;
    }

    return 0;
}


////////////////////////////////////////
static int klua_kwebrtc_peer_create(lua_State* L)
{
    int role = KLB_WEBRTC_ROLE_OFFER;

    if (lua_istable(L, 1))
    {
        lua_getfield(L, 1, "role");         ///< @1.role 角色: "offer"/"answer" 或 0/1

        if (lua_isnumber(L, -1))
        {
            role = (int)lua_tointeger(L, -1);
        }
        else if (lua_isstring(L, -1))
        {
            const char* p_role = lua_tostring(L, -1);
            role = (0 == strcmp(p_role, "answer")) ? KLB_WEBRTC_ROLE_ANSWER : KLB_WEBRTC_ROLE_OFFER;
        }

        lua_pop(L, 1);
    }

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = (NULL != p_env) ? klua_netmulti_get(p_env) : NULL;

    if (NULL == p_netmulti)
    {
        return push_err_klua_kwebrtc(L, KLB_NETCODE_INVAL);
    }

    int code = 0;
    klb_webrtc_peer_t* p_peer = klb_webrtc_peer_create(p_netmulti, role, &code);
    if (NULL == p_peer)
    {
        return push_err_klua_kwebrtc(L, (0 != code) ? code : KLB_NETCODE_ERR);
    }

    klua_kwebrtc_t* p_kwebrtc = new_klua_kwebrtc(L);
    p_kwebrtc->L = L;
    p_kwebrtc->p_peer = p_peer;

    return 1;
}


static int klua_kwebrtc_create_offer(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    char sdp[KLUA_KWEBRTC_SDP_MAX];

    int ret = klb_webrtc_peer_create_offer(p_kwebrtc->p_peer, sdp, (int)sizeof(sdp));
    if (0 >= ret)
    {
        return push_err_klua_kwebrtc(L, (0 > ret) ? ret : KLB_NETCODE_ERR);
    }

    lua_pushlstring(L, sdp, (size_t)ret);
    return 1;
}


static int klua_kwebrtc_create_answer(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    const char* p_remote = luaL_checkstring(L, 2);      ///< @2. 远端 offer
    char sdp[KLUA_KWEBRTC_SDP_MAX];

    int ret = klb_webrtc_peer_create_answer(p_kwebrtc->p_peer, p_remote, sdp, (int)sizeof(sdp));
    if (0 >= ret)
    {
        return push_err_klua_kwebrtc(L, (0 > ret) ? ret : KLB_NETCODE_ERR);
    }

    lua_pushlstring(L, sdp, (size_t)ret);
    return 1;
}


static int klua_kwebrtc_set_remote_sdp(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    const char* p_sdp = luaL_checkstring(L, 2);         ///< @2. 远端 SDP

    return push_rc_klua_kwebrtc(L, klb_webrtc_peer_set_remote_sdp(p_kwebrtc->p_peer, p_sdp));
}


static int klua_kwebrtc_add_ice_candidate(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    const char* p_cand = luaL_checkstring(L, 2);        ///< @2. 候选串

    return push_rc_klua_kwebrtc(L, klb_webrtc_peer_add_ice_candidate(p_kwebrtc->p_peer, p_cand));
}


static int klua_kwebrtc_sdp_parse(lua_State* L)
{
    const char* p_sdp = luaL_checkstring(L, 1);         ///< @1. SDP 文本
    klb_webrtc_sdp_t info;

    int ret = klb_webrtc_sdp_parse(p_sdp, &info);
    if (0 > ret)
    {
        return push_err_klua_kwebrtc(L, ret);
    }

    lua_newtable(L);

    lua_pushstring(L, info.ufrag);
    lua_setfield(L, -2, "ufrag");

    lua_pushstring(L, info.pwd);
    lua_setfield(L, -2, "pwd");

    lua_newtable(L);
    for (int i = 0; i < info.candidate_num; i++)
    {
        lua_pushstring(L, info.candidates[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "candidates");

    return 1;
}


static int klua_kwebrtc_state(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);

    int ret = klb_webrtc_peer_state(p_kwebrtc->p_peer);
    if (0 > ret)
    {
        return push_err_klua_kwebrtc(L, ret);
    }

    lua_pushstring(L, klb_webrtc_state_name(ret));
    return 1;
}


static int klua_kwebrtc_send(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    size_t len = 0;
    const char* p_data = luaL_checklstring(L, 2, &len);     ///< @2. 应用数据

    return push_rc_klua_kwebrtc(L,
        klb_webrtc_peer_send(p_kwebrtc->p_peer, (const uint8_t*)p_data, (int)len));
}


static int klua_kwebrtc_recv(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    int cap = (int)luaL_optinteger(L, 2, KLUA_KWEBRTC_RECV_MAX);   ///< @2. 接收上限

    if ((0 >= cap) || (KLUA_KWEBRTC_RECV_MAX < cap))
    {
        cap = KLUA_KWEBRTC_RECV_MAX;
    }

    uint8_t buf[KLUA_KWEBRTC_RECV_MAX];
    int code = 0;

    int ret = klb_webrtc_peer_recv(p_kwebrtc->p_peer, buf, cap, &code);
    if (0 > ret)
    {
        if (KLB_NETCODE_AGAIN == code)
        {
            lua_pushnil(L);
            lua_pushinteger(L, code);
            return 2;
        }

        return push_err_klua_kwebrtc(L, (0 != code) ? code : ret);
    }

    lua_pushlstring(L, (const char*)buf, (size_t)ret);
    return 1;
}


static int klua_kwebrtc_stats(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    klb_webrtc_stats_t st;

    int ret = klb_webrtc_peer_stats(p_kwebrtc->p_peer, &st);
    if (0 > ret)
    {
        return push_err_klua_kwebrtc(L, ret);
    }

    lua_newtable(L);

    lua_pushstring(L, klb_webrtc_state_name(st.state));
    lua_setfield(L, -2, "state");

    lua_pushinteger(L, st.state);
    lua_setfield(L, -2, "state_code");

    lua_pushboolean(L, (0 != st.has_selected) ? 1 : 0);
    lua_setfield(L, -2, "has_selected");

    lua_pushinteger(L, st.local_count);
    lua_setfield(L, -2, "local_count");

    lua_pushinteger(L, st.remote_count);
    lua_setfield(L, -2, "remote_count");

    lua_pushinteger(L, st.pair_count);
    lua_setfield(L, -2, "pair_count");

    lua_pushboolean(L, (0 != st.dtls_ready) ? 1 : 0);
    lua_setfield(L, -2, "dtls_ready");

    lua_pushboolean(L, (0 != st.srtp_ready) ? 1 : 0);
    lua_setfield(L, -2, "srtp_ready");

    lua_pushstring(L, st.local_ip);
    lua_setfield(L, -2, "local_ip");

    lua_pushinteger(L, st.local_port);
    lua_setfield(L, -2, "local_port");

    lua_pushstring(L, st.remote_ip);
    lua_setfield(L, -2, "remote_ip");

    lua_pushinteger(L, st.remote_port);
    lua_setfield(L, -2, "remote_port");

    lua_pushinteger(L, (lua_Integer)st.check_sent);
    lua_setfield(L, -2, "check_sent");

    lua_pushinteger(L, (lua_Integer)st.check_recv);
    lua_setfield(L, -2, "check_recv");

    lua_pushinteger(L, (lua_Integer)st.data_sent_pkts);
    lua_setfield(L, -2, "data_sent_pkts");

    lua_pushinteger(L, (lua_Integer)st.data_sent_bytes);
    lua_setfield(L, -2, "data_sent_bytes");

    lua_pushinteger(L, (lua_Integer)st.data_recv_pkts);
    lua_setfield(L, -2, "data_recv_pkts");

    lua_pushinteger(L, (lua_Integer)st.data_recv_bytes);
    lua_setfield(L, -2, "data_recv_bytes");

    lua_pushinteger(L, (lua_Integer)st.srtp_tx_pkts);
    lua_setfield(L, -2, "srtp_tx_pkts");

    lua_pushinteger(L, (lua_Integer)st.srtp_rx_pkts);
    lua_setfield(L, -2, "srtp_rx_pkts");

    lua_pushinteger(L, (lua_Integer)st.dtls_tx_pkts);
    lua_setfield(L, -2, "dtls_tx_pkts");

    lua_pushinteger(L, (lua_Integer)st.dtls_rx_pkts);
    lua_setfield(L, -2, "dtls_rx_pkts");

    lua_pushinteger(L, (lua_Integer)st.rtp_ssrc_tx);
    lua_setfield(L, -2, "rtp_ssrc_tx");

    lua_pushinteger(L, (lua_Integer)st.rtp_ssrc_rx);
    lua_setfield(L, -2, "rtp_ssrc_rx");

    lua_pushinteger(L, (lua_Integer)st.rtp_tx_pkts);
    lua_setfield(L, -2, "rtp_tx_pkts");

    lua_pushinteger(L, (lua_Integer)st.rtp_rx_pkts);
    lua_setfield(L, -2, "rtp_rx_pkts");

    lua_pushinteger(L, (lua_Integer)st.rtcp_tx_pkts);
    lua_setfield(L, -2, "rtcp_tx_pkts");

    lua_pushinteger(L, (lua_Integer)st.rtcp_rx_pkts);
    lua_setfield(L, -2, "rtcp_rx_pkts");

    lua_pushinteger(L, (lua_Integer)st.nack_tx_pkts);
    lua_setfield(L, -2, "nack_tx_pkts");

    lua_pushinteger(L, (lua_Integer)st.nack_rx_pkts);
    lua_setfield(L, -2, "nack_rx_pkts");

    lua_pushinteger(L, (lua_Integer)st.retrans_pkts);
    lua_setfield(L, -2, "retrans_pkts");

    lua_pushinteger(L, (lua_Integer)st.rtp_lost_pkts);
    lua_setfield(L, -2, "rtp_lost_pkts");

    lua_pushinteger(L, (lua_Integer)st.jb_pending);
    lua_setfield(L, -2, "jb_pending");

    lua_pushinteger(L, (lua_Integer)st.jb_dup_pkts);
    lua_setfield(L, -2, "jb_dup_pkts");

    lua_pushinteger(L, (lua_Integer)st.jb_late_pkts);
    lua_setfield(L, -2, "jb_late_pkts");

    lua_pushinteger(L, (lua_Integer)st.jb_overflow_pkts);
    lua_setfield(L, -2, "jb_overflow_pkts");

    lua_pushinteger(L, (lua_Integer)st.jb_reorder_pkts);
    lua_setfield(L, -2, "jb_reorder_pkts");

    lua_pushinteger(L, (lua_Integer)st.target_bps);
    lua_setfield(L, -2, "target_bps");

    lua_pushinteger(L, (lua_Integer)st.twcc_loss_permille);
    lua_setfield(L, -2, "twcc_loss_permille");

    lua_pushinteger(L, (lua_Integer)st.twcc_fb_rounds);
    lua_setfield(L, -2, "twcc_fb_rounds");

    return 1;
}


static int klua_kwebrtc_set_dtls_cert(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    const char* p_cert = luaL_checkstring(L, 2);        ///< @2. 本端证书 PEM
    const char* p_key = luaL_checkstring(L, 3);         ///< @3. 本端私钥 PEM
    const char* p_ca = luaL_optstring(L, 4, NULL);      ///< @4. 对端校验 CA PEM; 可空
    int verify = (int)luaL_optinteger(L, 5, 0);         ///< @5. 是否校验对端证书

    return push_rc_klua_kwebrtc(L,
        klb_webrtc_peer_set_dtls_cert(p_kwebrtc->p_peer, p_cert, p_key, p_ca, verify));
}


static int klua_kwebrtc_dc_open(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);

    return push_rc_klua_kwebrtc(L, klb_webrtc_peer_dc_open(p_kwebrtc->p_peer));
}


static int klua_kwebrtc_dc_is_open(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);

    lua_pushboolean(L, (0 != klb_webrtc_peer_dc_is_open(p_kwebrtc->p_peer)) ? 1 : 0);
    return 1;
}


static int klua_kwebrtc_dc_send(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    size_t len = 0;
    const char* p_data = luaL_checklstring(L, 2, &len);     ///< @2. 消息内容
    int is_binary = (0 != lua_toboolean(L, 3)) ? 1 : 0;      ///< @3. 是否二进制

    return push_rc_klua_kwebrtc(L,
        klb_webrtc_peer_dc_send(p_kwebrtc->p_peer, (const uint8_t*)p_data, (int)len, is_binary));
}


static int klua_kwebrtc_dc_recv(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    int cap = (int)luaL_optinteger(L, 2, KLUA_KWEBRTC_RECV_MAX);   ///< @2. 接收上限

    if ((0 >= cap) || (KLUA_KWEBRTC_RECV_MAX < cap))
    {
        cap = KLUA_KWEBRTC_RECV_MAX;
    }

    uint8_t buf[KLUA_KWEBRTC_RECV_MAX];
    int code = 0;

    int ret = klb_webrtc_peer_dc_recv(p_kwebrtc->p_peer, buf, cap, &code);
    if (0 > ret)
    {
        return push_err_klua_kwebrtc(L, (0 != code) ? code : ret);
    }

    if (0 == ret)
    {
        lua_pushnil(L);
        lua_pushinteger(L, KLB_NETCODE_AGAIN);
        return 2;
    }

    lua_pushlstring(L, (const char*)buf, (size_t)ret);
    return 1;
}


static int recv_media_klua_kwebrtc(lua_State* L, bool is_h264)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    int cap = (int)luaL_optinteger(L, 2, KLUA_KWEBRTC_MEDIA_MAX);    ///< @2. 接收上限

    if ((0 >= cap) || (KLUA_KWEBRTC_MEDIA_LIMIT < cap))
    {
        cap = KLUA_KWEBRTC_MEDIA_MAX;
    }

    uint8_t* p_buf = KLB_MALLOC(uint8_t, cap, 0);
    if (NULL == p_buf)
    {
        return push_err_klua_kwebrtc(L, KLB_NETCODE_NOMEM);
    }

    int code = 0;
    int ret = is_h264
        ? klb_webrtc_peer_h264_recv(p_kwebrtc->p_peer, p_buf, cap, &code)
        : klb_webrtc_peer_rtp_recv(p_kwebrtc->p_peer, p_buf, cap, &code);

    if (0 > ret)
    {
        KLB_FREE(p_buf);
        return push_err_klua_kwebrtc(L, (0 != code) ? code : ret);
    }

    if (0 == ret)
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushinteger(L, KLB_NETCODE_AGAIN);
        return 2;
    }

    lua_pushlstring(L, (const char*)p_buf, (size_t)ret);
    KLB_FREE(p_buf);

    return 1;
}


static int klua_kwebrtc_rtp_send(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    size_t len = 0;
    const char* p_data = luaL_checklstring(L, 2, &len);     ///< @2. RTP 包

    return push_rc_klua_kwebrtc(L,
        klb_webrtc_peer_rtp_send(p_kwebrtc->p_peer, (const uint8_t*)p_data, (int)len));
}


static int klua_kwebrtc_h264_send(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    size_t len = 0;
    const char* p_nalu = luaL_checklstring(L, 2, &len);     ///< @2. NALU (可带起始码)
    uint32_t timestamp = (uint32_t)luaL_optinteger(L, 3, 0);    ///< @3. 时间戳
    int marker = (0 != lua_toboolean(L, 4)) ? 1 : 0;        ///< @4. 标记位

    return push_rc_klua_kwebrtc(L,
        klb_webrtc_peer_h264_send(p_kwebrtc->p_peer, (const uint8_t*)p_nalu, (int)len,
            timestamp, marker));
}


static int klua_kwebrtc_rtp_recv(lua_State* L)
{
    return recv_media_klua_kwebrtc(L, false);
}


static int klua_kwebrtc_h264_recv(lua_State* L)
{
    return recv_media_klua_kwebrtc(L, true);
}


static int klua_kwebrtc_rtcp_send(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    size_t len = 0;
    const char* p_data = luaL_checklstring(L, 2, &len);     ///< @2. RTCP 包

    return push_rc_klua_kwebrtc(L,
        klb_webrtc_peer_rtcp_send(p_kwebrtc->p_peer, (const uint8_t*)p_data, (int)len));
}


static int klua_kwebrtc_nack_send(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);                       ///< @2. 丢失序号数组

    int num = (int)lua_rawlen(L, 2);
    if ((0 >= num) || (KLB_WEBRTC_NACK_MAX < num))
    {
        return push_err_klua_kwebrtc(L, KLB_NETCODE_INVAL);
    }

    uint16_t seqs[KLB_WEBRTC_NACK_MAX];

    for (int i = 0; i < num; i++)
    {
        lua_rawgeti(L, 2, i + 1);
        seqs[i] = (uint16_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
    }

    return push_rc_klua_kwebrtc(L,
        klb_webrtc_peer_nack_send(p_kwebrtc->p_peer, seqs, num));
}


static int klua_kwebrtc_event_poll(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    klb_webrtc_event_t evt;

    int ret = klb_webrtc_peer_event_poll(p_kwebrtc->p_peer, &evt);
    if (0 > ret)
    {
        return push_err_klua_kwebrtc(L, ret);
    }

    if (0 == ret)
    {
        lua_pushnil(L);
        lua_pushinteger(L, KLB_NETCODE_AGAIN);
        return 2;
    }

    lua_newtable(L);

    lua_pushstring(L, evt.name);
    lua_setfield(L, -2, "type");

    lua_pushinteger(L, evt.code);
    lua_setfield(L, -2, "code");

    if (KLB_WEBRTC_EVT_ICE_STATE == evt.type)
    {
        lua_pushstring(L, klb_webrtc_state_name(evt.code));
        lua_setfield(L, -2, "state");
    }

    return 1;
}


static int klua_kwebrtc_add_media(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    const char* p_kind = luaL_checkstring(L, 2);            ///< @2. "video" / "audio"
    luaL_checktype(L, 3, LUA_TTABLE);                       ///< @3. 编码数组 {{pt=96, name="H264", clock_rate=90000}}

    int kind = (0 == strcmp(p_kind, "audio")) ? KLB_WEBRTC_KIND_AUDIO
        : KLB_WEBRTC_KIND_VIDEO;
    int num = (int)lua_rawlen(L, 3);
    if ((0 >= num) || (KLB_WEBRTC_CODEC_MAX < num))
    {
        return push_err_klua_kwebrtc(L, KLB_NETCODE_INVAL);
    }

    klb_webrtc_codec_t codecs[KLB_WEBRTC_CODEC_MAX];
    KLB_MEMSET(codecs, 0, sizeof(codecs));

    for (int i = 0; i < num; i++)
    {
        lua_rawgeti(L, 3, i + 1);
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return push_err_klua_kwebrtc(L, KLB_NETCODE_INVAL);
        }

        lua_getfield(L, -1, "pt");
        codecs[i].pt = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "name");
        const char* p_name = lua_tostring(L, -1);
        if (NULL != p_name)
        {
            size_t n = strlen(p_name);
            if ((KLB_WEBRTC_CODEC_NAME - 1) < n)
            {
                n = KLB_WEBRTC_CODEC_NAME - 1;
            }

            memcpy(codecs[i].name, p_name, n);
            codecs[i].name[n] = '\0';
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "clock_rate");
        codecs[i].clock_rate = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "channels");
        codecs[i].channels = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);

        lua_pop(L, 1);
    }

    return push_rc_klua_kwebrtc(L,
        klb_webrtc_peer_add_media(p_kwebrtc->p_peer, kind, codecs, num));
}


static int klua_kwebrtc_rtcp_sr_send(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);

    int code = 0;
    uint32_t lsr = klb_webrtc_rtcp_sr_send(p_kwebrtc->p_peer, &code);
    if (0 > code)
    {
        return push_err_klua_kwebrtc(L, code);
    }

    lua_pushinteger(L, (lua_Integer)lsr);
    return 1;
}


static int klua_kwebrtc_rtcp_rr_send(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);

    return push_rc_klua_kwebrtc(L, klb_webrtc_rtcp_rr_send(p_kwebrtc->p_peer));
}


static int klua_kwebrtc_twcc_send(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);                       ///< @2. 包状态数组 (0.未收到; 非 0.已收到)

    int num = (int)lua_rawlen(L, 2);
    if ((0 >= num) || (KLB_WEBRTC_TWCC_MAX < num))
    {
        return push_err_klua_kwebrtc(L, KLB_NETCODE_INVAL);
    }

    uint8_t status[KLB_WEBRTC_TWCC_MAX];
    KLB_MEMSET(status, 0, sizeof(status));

    for (int i = 0; i < num; i++)
    {
        lua_rawgeti(L, 2, i + 1);
        status[i] = (0 != lua_tointeger(L, -1)) ? 1 : 0;
        lua_pop(L, 1);
    }

    uint16_t base_seq = (uint16_t)luaL_checkinteger(L, 3);  ///< @3. 起始序号
    uint32_t ref_time = (uint32_t)luaL_optinteger(L, 4, 0); ///< @4. 参考时间 (64ms 单位)

    return push_rc_klua_kwebrtc(L,
        klb_webrtc_rtcp_twcc_send(p_kwebrtc->p_peer, status, num, base_seq, ref_time));
}


static int klua_kwebrtc_twcc_flush(lua_State* L)
{
    klua_kwebrtc_t* p_kwebrtc = to_klua_kwebrtc(L, 1);

    return push_rc_klua_kwebrtc(L, klb_webrtc_rtcp_twcc_flush(p_kwebrtc->p_peer));
}


int klua_open_kwebrtc(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "peer_create",        klua_kwebrtc_peer_create },
        { "peer_close",         klua_kwebrtc_peer_close },
        { "create_offer",       klua_kwebrtc_create_offer },
        { "create_answer",      klua_kwebrtc_create_answer },
        { "set_remote_sdp",     klua_kwebrtc_set_remote_sdp },
        { "add_ice_candidate",  klua_kwebrtc_add_ice_candidate },
        { "sdp_parse",          klua_kwebrtc_sdp_parse },
        { "state",              klua_kwebrtc_state },
        { "send",               klua_kwebrtc_send },
        { "recv",               klua_kwebrtc_recv },
        { "stats",              klua_kwebrtc_stats },
        { "set_dtls_cert",      klua_kwebrtc_set_dtls_cert },
        { "dc_open",            klua_kwebrtc_dc_open },
        { "dc_is_open",         klua_kwebrtc_dc_is_open },
        { "dc_send",            klua_kwebrtc_dc_send },
        { "dc_recv",            klua_kwebrtc_dc_recv },
        { "rtp_send",           klua_kwebrtc_rtp_send },
        { "h264_send",          klua_kwebrtc_h264_send },
        { "rtp_recv",           klua_kwebrtc_rtp_recv },
        { "h264_recv",          klua_kwebrtc_h264_recv },
        { "rtcp_send",          klua_kwebrtc_rtcp_send },
        { "nack_send",          klua_kwebrtc_nack_send },
        { "event_poll",         klua_kwebrtc_event_poll },
        { "add_media",          klua_kwebrtc_add_media },
        { "rtcp_sr_send",       klua_kwebrtc_rtcp_sr_send },
        { "rtcp_rr_send",       klua_kwebrtc_rtcp_rr_send },
        { "twcc_send",          klua_kwebrtc_twcc_send },
        { "twcc_flush",         klua_kwebrtc_twcc_flush },
        { NULL,                 NULL }
    };

    static luaL_Reg meta[] =
    {
        { "__gc",       klua_kwebrtc_peer_close },
        { "__tostring", klua_kwebrtc_tostring },
        { NULL,         NULL }
    };

    luaL_newmetatable(L, KLUA_KWEBRTC_HANDLE);
    luaL_setfuncs(L, meta, 0);
    lua_pop(L, 1);

    luaL_newlib(L, lib);
    return 1;
}


// end
