--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kwebrtc.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kwebrtc, WebRTC peer (轮询式, 无对象方法)
--   \n require("kwebrtc")
--   \n C导出文件: ./klb/src_packages/klbwebrtc/webrtc_klua/klbwebrtc_klua.c
--   \n 全部为模块级函数, 第 1 参恒为 peer; peer 无 __index, 不能用 peer:xxx(...)
--   \n 媒体帧一律为 Lua string (非 lightuserdata); 无协程要求, 靠 nil,AGAIN 轮询
--   \n 失败统一为 nil, code[klb_netcode_e] (写类成功返回 true)
--   \n 无数据统一为 nil, AGAIN(-11)
--   \n 典型顺序: set_dtls_cert -> add_media -> create_offer/create_answer
--   \n          -> set_remote_sdp / add_ice_candidate -> event_poll -> dc_open / rtp_send
-- @version 0.1
--]]

local kwebrtc = {}


-- @brief 创建 peer
-- @param [in] opts[table]	[可选] role = "offer"|"answer"|0|1; 默认 "offer"
-- @return peer[userdata]	peer 句柄
-- @return nil, code[number(int)]	失败 (无 netmulti 为 INVAL)
-- @note 销毁: peer_close 或 __gc; __gc / __tostring 之外无元方法
kwebrtc.peer_create = function (opts)
	return nil, 0
end


-- @brief 销毁 peer (幂等)
-- @param [in] peer[userdata]
-- @return 无
kwebrtc.peer_close = function (peer)
	return
end


-- @brief 生成 offer SDP
-- @param [in] peer[userdata]
-- @return [string]	SDP 文本 (内部缓冲 8192)
-- @return nil, code[number(int)]	失败
kwebrtc.create_offer = function (peer)
	return ''
end


-- @brief 生成 answer SDP
-- @param [in] peer[userdata]
-- @param [in] remote_offer[string]	对端 offer
-- @return [string]	SDP 文本
-- @return nil, code[number(int)]	失败
kwebrtc.create_answer = function (peer, remote_offer)
	return ''
end


-- @brief 设置远端 SDP
-- @param [in] peer[userdata]
-- @param [in] sdp[string]	远端 SDP
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	失败
kwebrtc.set_remote_sdp = function (peer, sdp)
	return true
end


-- @brief 添加远端 ICE 候选
-- @param [in] peer[userdata]
-- @param [in] cand[string]	candidate 行
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	失败
kwebrtc.add_ice_candidate = function (peer, cand)
	return true
end


-- @brief 解析 SDP
-- @param [in] sdp[string]	SDP 文本
-- @return [table] ufrag[string], pwd[string], candidates[table]
-- @return nil, code[number(int)]	失败
-- @note 仅暴露上列 3 个字段 (fingerprint/setup/mlines 未暴露)
kwebrtc.sdp_parse = function (sdp)
	return { ufrag = '', pwd = '', candidates = {} }
end


-- @brief ICE 状态
-- @param [in] peer[userdata]
-- @return [string] "new"/"gathering"/"checking"/"connected"/"failed"/"closed"
-- @return nil, code[number(int)]	失败
kwebrtc.state = function (peer)
	return "new"
end


-- @brief 发送应用数据 (DataChannel 未开时的普通通道)
-- @param [in] peer[userdata]
-- @param [in] data[string]
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	失败
kwebrtc.send = function (peer, data)
	return true
end


-- @brief 接收应用数据 (非阻塞)
-- @param [in] peer[userdata]
-- @param [in] cap[number(int)]	[可选] 最大字节; 默认 2048
-- @return [string]	数据
-- @return nil, code[number(int)]	暂无数据为 AGAIN
kwebrtc.recv = function (peer, cap)
	return nil, -11
end


-- @brief 统计信息
-- @param [in] peer[userdata]
-- @return [table] state, state_code, has_selected, local_count, remote_count, pair_count
--   \n dtls_ready, srtp_ready, local_ip, local_port, remote_ip, remote_port
--   \n check_sent, check_recv, data_sent_pkts, data_sent_bytes, data_recv_pkts, data_recv_bytes
--   \n srtp_tx_pkts, srtp_rx_pkts, dtls_tx_pkts, dtls_rx_pkts
--   \n rtp_ssrc_tx, rtp_ssrc_rx, rtp_tx_pkts, rtp_rx_pkts, rtcp_tx_pkts, rtcp_rx_pkts
--   \n nack_tx_pkts, nack_rx_pkts, retrans_pkts, rtp_lost_pkts
--   \n jb_pending, jb_dup_pkts, jb_late_pkts, jb_overflow_pkts, jb_reorder_pkts
--   \n target_bps, twcc_loss_permille, twcc_fb_rounds
-- @return nil, code[number(int)]	失败
kwebrtc.stats = function (peer)
	return {}
end


-- @brief 配置 DTLS 证书 (必须在 create_offer/create_answer 之前)
-- @param [in] peer[userdata]
-- @param [in] cert_pem[string]	证书 PEM
-- @param [in] key_pem[string]	私钥 PEM
-- @param [in] ca_pem[string]	[可选] CA PEM; 缺省 nil
-- @param [in] verify[number(int)]	[可选] 是否校验对端; 默认 0
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	失败
-- @note 未配置则 DTLS/SRTP 不启用
kwebrtc.set_dtls_cert = function (peer, cert_pem, key_pem, ca_pem, verify)
	return true
end


-- @brief 打开 DataChannel (须 dtls_ready 之后)
-- @param [in] peer[userdata]
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	未连通为 NOTCONN(-107)
kwebrtc.dc_open = function (peer)
	return true
end


-- @brief DataChannel 是否已打开
-- @param [in] peer[userdata]
-- @return [boolean]
kwebrtc.dc_is_open = function (peer)
	return false
end


-- @brief 经 DataChannel 发送
-- @param [in] peer[userdata]
-- @param [in] data[string]
-- @param [in] is_binary[boolean]	[可选] 默认 false
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	失败
kwebrtc.dc_send = function (peer, data, is_binary)
	return true
end


-- @brief 经 DataChannel 接收 (非阻塞)
-- @param [in] peer[userdata]
-- @param [in] cap[number(int)]	[可选] 最大字节; 默认 2048
-- @return [string]	数据
-- @return nil, code[number(int)]	暂无数据为 AGAIN
kwebrtc.dc_recv = function (peer, cap)
	return nil, -11
end


-- @brief 发送 RTP 包
-- @param [in] peer[userdata]
-- @param [in] rtp[string]	完整 RTP 包 (自带 12B 固定头; UDP 载荷 <= 1200B)
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	失败
kwebrtc.rtp_send = function (peer, rtp)
	return true
end


-- @brief 发送 H264 NALU (自动单包/FU-A 分片)
-- @param [in] peer[userdata]
-- @param [in] nalu[string]	NALU; 允许带 Annex-B 起始码
-- @param [in] timestamp[number(int)]	[可选] 90kHz RTP 时间戳; 默认 0
-- @param [in] marker[boolean]	[可选] 默认 false
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	失败
-- @note 序号由 C 层自增
kwebrtc.h264_send = function (peer, nalu, timestamp, marker)
	return true
end


-- @brief 接收 RTP 包 (非阻塞)
-- @param [in] peer[userdata]
-- @param [in] cap[number(int)]	[可选] 最大字节; 默认 65536; <=0 或 >256KiB 回落 64KiB
-- @return [string]	RTP 包
-- @return nil, code[number(int)]	暂无数据为 AGAIN
kwebrtc.rtp_recv = function (peer, cap)
	return nil, -11
end


-- @brief 接收 H264 帧 (非阻塞)
-- @param [in] peer[userdata]
-- @param [in] cap[number(int)]	[可选] 同 rtp_recv
-- @return [string]	H264 数据
-- @return nil, code[number(int)]	暂无数据为 AGAIN
kwebrtc.h264_recv = function (peer, cap)
	return nil, -11
end


-- @brief 发送 RTCP 包
-- @param [in] peer[userdata]
-- @param [in] rtcp[string]	完整 RTCP 包
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	失败
kwebrtc.rtcp_send = function (peer, rtcp)
	return true
end


-- @brief 发送 NACK
-- @param [in] peer[userdata]
-- @param [in] seqs[table]	序号数组; 1..16 项
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	项数越界为 INVAL
kwebrtc.nack_send = function (peer, seqs)
	return true
end


-- @brief 轮询事件
-- @param [in] peer[userdata]
-- @return [table] type[string], code[number(int)], state[string] (仅 ice_state 时附带)
--   \n type: "ice_state" / "dtls_ready" / "srtp_ready" / "dc_open"
-- @return nil, code[number(int)]	无事件为 AGAIN
-- @note 队列满时丢最旧事件
kwebrtc.event_poll = function (peer)
	return nil, -11
end


-- @brief 添加媒体描述 (必须在 create_offer 之前)
-- @param [in] peer[userdata]
-- @param [in] kind[string]	"video" / "audio" (其它值按 video)
-- @param [in] codecs[table]	1..4 项; 每项 {pt, name, clock_rate, channels}
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	项数越界为 INVAL
-- @note 最多 4 条 m-line
kwebrtc.add_media = function (peer, kind, codecs)
	return true
end


-- @brief 发送 RTCP SR
-- @param [in] peer[userdata]
-- @return [number(int)] lsr (SR 的 NTP 中间 32 位)
-- @return nil, code[number(int)]	失败
kwebrtc.rtcp_sr_send = function (peer)
	return 0
end


-- @brief 发送 RTCP RR
-- @param [in] peer[userdata]
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	失败
kwebrtc.rtcp_rr_send = function (peer)
	return true
end


-- @brief 发送 TWCC 反馈
-- @param [in] peer[userdata]
-- @param [in] status[table]	状态数组; 1..256 项
-- @param [in] base_seq[number(int)]	起始序号
-- @param [in] ref_time[number(int)]	[可选] 参考时间 (单位 64ms, 24 位); 默认 0
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	项数越界为 INVAL
kwebrtc.twcc_send = function (peer, status, base_seq, ref_time)
	return true
end


-- @brief 冲刷待发 TWCC
-- @param [in] peer[userdata]
-- @return [boolean] true.成功
-- @return nil, code[number(int)]	失败
kwebrtc.twcc_flush = function (peer)
	return true
end


return kwebrtc
