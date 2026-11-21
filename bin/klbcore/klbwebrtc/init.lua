--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
--
-- @file   init.lua
-- @brief  klbwebrtc WebRTC 会话包装
-- @stability experimental
-- @impl evolving
-- @note   接线 kwebrtc 绑定面, 失败形状统一为 nil, code; 会话含 ICE + DTLS-SRTP + DataChannel
--]]


local kwebrtc = require("kwebrtc")
local kco = require("kco")


local mod = {}


-- @brief  创建 PeerConnection
-- @param  [in] cfg     配置表; cfg.role = "offer" / "answer"
-- @return peer 或 nil, code
-- @stability experimental
mod.peer_new = function (cfg)
	return kwebrtc.peer_create(cfg)
end


-- @brief  关闭 PeerConnection
-- @param  [in] peer    会话
-- @return 无
-- @stability experimental
mod.peer_close = function (peer)
	return kwebrtc.peer_close(peer)
end


-- @brief  生成本地 offer SDP
-- @param  [in] peer    会话
-- @return sdp 或 nil, code
-- @stability experimental
mod.create_offer = function (peer)
	return kwebrtc.create_offer(peer)
end


-- @brief  生成 answer SDP (内部完成远端 offer 解析与 ICE 启动)
-- @param  [in] peer        会话
-- @param  [in] remote_sdp  远端 offer
-- @return sdp 或 nil, code
-- @stability experimental
mod.create_answer = function (peer, remote_sdp)
	return kwebrtc.create_answer(peer, remote_sdp)
end


-- @brief  设置远端 SDP (写入 ICE 凭据与候选并启动连通性检查)
-- @param  [in] peer    会话
-- @param  [in] sdp     远端 SDP
-- @return true 或 nil, code
-- @stability experimental
mod.set_remote_sdp = function (peer, sdp)
	return kwebrtc.set_remote_sdp(peer, sdp)
end


-- @brief  添加远端 ICE 候选 (trickle)
-- @param  [in] peer         会话
-- @param  [in] candidate    候选串
-- @return true 或 nil, code
-- @stability experimental
mod.add_ice_candidate = function (peer, candidate)
	return kwebrtc.add_ice_candidate(peer, candidate)
end


-- @brief  解析 SDP 文本 (仅 ICE 凭据与候选子集)
-- @param  [in] text    SDP 文本
-- @return 表 {ufrag, pwd, candidates} 或 nil, code
-- @stability experimental
mod.sdp_parse = function (text)
	return kwebrtc.sdp_parse(text)
end


-- @brief  取连通状态
-- @param  [in] peer    会话
-- @return "new"/"gathering"/"checking"/"connected"/"failed"/"closed" 或 nil, code
-- @stability experimental
mod.state = function (peer)
	return kwebrtc.state(peer)
end


-- @brief  经已建立的通道发送数据
-- @param  [in] peer    会话
-- @param  [in] data    数据
-- @return true 或 nil, code
-- @stability experimental
mod.send = function (peer, data)
	return kwebrtc.send(peer, data)
end


-- @brief  接收数据 (暂无数据回 nil, -11)
-- @param  [in] peer    会话
-- @param  [in] cap     接收上限; 可省略
-- @return data 或 nil, code
-- @stability experimental
mod.recv = function (peer, cap)
	return kwebrtc.recv(peer, cap)
end


-- @brief  取会话统计 (ICE/DTLS/SRTP 计数与选中候选对)
-- @param  [in] peer    会话
-- @return 表 或 nil, code
-- @stability experimental
mod.stats = function (peer)
	return kwebrtc.stats(peer)
end


-- @brief  发送一个已构建的 RTP 包 (经 SRTP 保护)
-- @param  [in] peer    会话
-- @param  [in] data    RTP 包
-- @return true 或 nil, code
-- @stability experimental
mod.rtp_send = function (peer, data)
	return kwebrtc.rtp_send(peer, data)
end


-- @brief  发送一个 H.264 NALU (自动单包 / FU-A 分片)
-- @param  [in] peer       会话
-- @param  [in] nalu       NALU; 可带 Annex-B 起始码
-- @param  [in] timestamp  RTP 时间戳 (90 kHz)
-- @param  [in] marker     标记位; 非 0 时最后一包置 M
-- @return true 或 nil, code
-- @stability experimental
mod.h264_send = function (peer, nalu, timestamp, marker)
	return kwebrtc.h264_send(peer, nalu, timestamp, marker)
end


-- @brief  接收一个有序 RTP 包 (暂无数据回 nil, -11)
-- @param  [in] peer    会话
-- @param  [in] cap     接收上限; 可省略
-- @return data 或 nil, code
-- @stability experimental
mod.rtp_recv = function (peer, cap)
	return kwebrtc.rtp_recv(peer, cap)
end


-- @brief  接收一个 H.264 NALU (暂无完整 NALU 回 nil, -11)
-- @param  [in] peer    会话
-- @param  [in] cap     接收上限; 可省略
-- @return nalu 或 nil, code
-- @stability experimental
mod.h264_recv = function (peer, cap)
	return kwebrtc.h264_recv(peer, cap)
end


-- @brief  发送一个已构建的 RTCP 包 (经 SRTCP 保护)
-- @param  [in] peer    会话
-- @param  [in] data    RTCP 包
-- @return true 或 nil, code
-- @stability experimental
mod.rtcp_send = function (peer, data)
	return kwebrtc.rtcp_send(peer, data)
end


-- @brief  发送 RTCP NACK 请求重传
-- @param  [in] peer    会话
-- @param  [in] seqs    丢失序号数组
-- @return true 或 nil, code
-- @stability experimental
mod.nack_send = function (peer, seqs)
	return kwebrtc.nack_send(peer, seqs)
end


-- @brief  取一个待处理会话事件 (无事件回 nil, -11)
-- @param  [in] peer    会话
-- @return 表 {type, code[, state]} 或 nil, code
-- @stability experimental
mod.event_poll = function (peer)
	return kwebrtc.event_poll(peer)
end


-- @brief  协程: 等待下一个会话事件
-- @param  [in] peer         会话
-- @param  [in] timeout_ms   超时毫秒; 默认 5000
-- @param  [in] interval_ms  轮询间隔; 默认 10
-- @return 表 {type, code[, state]} / nil, -110
-- @stability experimental
mod.co_wait_event = function (peer, timeout_ms, interval_ms)
	timeout_ms = tonumber(timeout_ms) or 5000
	interval_ms = tonumber(interval_ms) or 10

	while timeout_ms > 0 do
		local evt, code = kwebrtc.event_poll(peer)
		if evt ~= nil then
			return evt
		end

		if code ~= -11 then
			return nil, code
		end

		kco.co_sleep(interval_ms)
		timeout_ms = timeout_ms - interval_ms
	end

	return nil, -110
end


-- @brief  配置 DTLS 证书 (须在 create_offer / create_answer 之前)
-- @param  [in] peer         会话
-- @param  [in] cert_pem     本端证书 PEM
-- @param  [in] key_pem      本端私钥 PEM
-- @param  [in] ca_pem       对端校验 CA PEM; 可省略
-- @param  [in] verify_peer  是否校验对端证书
-- @return true 或 nil, code
-- @stability experimental
mod.set_dtls_cert = function (peer, cert_pem, key_pem, ca_pem, verify_peer)
	return kwebrtc.set_dtls_cert(peer, cert_pem, key_pem, ca_pem, verify_peer)
end


-- @brief  打开 DataChannel (须 DTLS 就绪)
-- @param  [in] peer    会话
-- @return true 或 nil, code
-- @stability experimental
mod.dc_open = function (peer)
	return kwebrtc.dc_open(peer)
end


-- @brief  DataChannel 是否已打开
-- @param  [in] peer    会话
-- @return true / false
-- @stability experimental
mod.dc_is_open = function (peer)
	return kwebrtc.dc_is_open(peer)
end


-- @brief  经 DataChannel 发送消息
-- @param  [in] peer       会话
-- @param  [in] data       消息内容
-- @param  [in] is_binary  是否二进制
-- @return true 或 nil, code
-- @stability experimental
mod.dc_send = function (peer, data, is_binary)
	return kwebrtc.dc_send(peer, data, is_binary)
end


-- @brief  经 DataChannel 接收消息 (暂无数据回 nil, -11)
-- @param  [in] peer    会话
-- @param  [in] cap     接收上限; 可省略
-- @return data 或 nil, code
-- @stability experimental
mod.dc_recv = function (peer, cap)
	return kwebrtc.dc_recv(peer, cap)
end


-- @brief  协程: 等待 ICE 连通
-- @param  [in] peer         会话
-- @param  [in] timeout_ms   超时毫秒; 默认 5000
-- @param  [in] interval_ms  轮询间隔; 默认 10
-- @return true / false, "failed"/"closed" / nil, -110
-- @stability experimental
mod.co_wait_connected = function (peer, timeout_ms, interval_ms)
	timeout_ms = tonumber(timeout_ms) or 5000
	interval_ms = tonumber(interval_ms) or 10

	while timeout_ms > 0 do
		local st, code = kwebrtc.state(peer)
		if st == nil then
			return nil, code
		end

		if st == "connected" then
			return true
		end

		if st == "failed" or st == "closed" then
			return false, st
		end

		kco.co_sleep(interval_ms)
		timeout_ms = timeout_ms - interval_ms
	end

	return nil, -110
end


-- @brief  协程: 等待 DataChannel 打开
-- @param  [in] peer         会话
-- @param  [in] timeout_ms   超时毫秒; 默认 5000
-- @param  [in] interval_ms  轮询间隔; 默认 10
-- @return true / nil, -110
-- @stability experimental
mod.co_wait_dc = function (peer, timeout_ms, interval_ms)
	timeout_ms = tonumber(timeout_ms) or 5000
	interval_ms = tonumber(interval_ms) or 10

	while timeout_ms > 0 do
		if kwebrtc.dc_is_open(peer) then
			return true
		end

		kco.co_sleep(interval_ms)
		timeout_ms = timeout_ms - interval_ms
	end

	return nil, -110
end


-- @brief  协程: 带超时接收 DataChannel 消息
-- @param  [in] peer         会话
-- @param  [in] timeout_ms   超时毫秒; 默认 5000
-- @param  [in] cap          接收上限; 可省略
-- @return data / nil, code
-- @stability experimental
mod.co_dc_recv = function (peer, timeout_ms, cap)
	timeout_ms = tonumber(timeout_ms) or 5000

	while timeout_ms > 0 do
		local data, code = kwebrtc.dc_recv(peer, cap)
		if data ~= nil then
			return data
		end

		-- -11 暂无数据; -107 通道尚未打开
		if code ~= -11 and code ~= -107 then
			return nil, code
		end

		kco.co_sleep(10)
		timeout_ms = timeout_ms - 10
	end

	return nil, -110
end


-- @brief  增加一条媒体 m-line (video / audio; 须在 create_offer 之前)
-- @param  [in] peer    会话
-- @param  [in] kind    "video" / "audio"
-- @param  [in] codecs  编码数组; 元素 {pt=96, name="H264", clock_rate=90000, channels=1}
-- @return true 或 nil, code
-- @stability experimental
mod.add_media = function (peer, kind, codecs)
        return kwebrtc.add_media(peer, kind, codecs)
end


-- @brief  发送 RTCP SR (发送者报告)
-- @param  [in] peer    会话
-- @return lsr 或 nil, code; lsr 为 SR 的 NTP 中间 32 位
-- @stability experimental
mod.rtcp_sr_send = function (peer)
        return kwebrtc.rtcp_sr_send(peer)
end


-- @brief  发送 RTCP RR (接收者报告)
-- @param  [in] peer    会话
-- @return true 或 nil, code
-- @stability experimental
mod.rtcp_rr_send = function (peer)
        return kwebrtc.rtcp_rr_send(peer)
end


-- @brief  发送 RTCP TWCC 传输层拥塞控制反馈
-- @param  [in] peer       会话
-- @param  [in] status     包状态数组; 0.未收到; 非 0.已收到
-- @param  [in] base_seq   起始序号
-- @param  [in] ref_time   参考时间 (24 位, 64ms 单位); 可省略
-- @return true 或 nil, code
-- @stability experimental
mod.twcc_send = function (peer, status, base_seq, ref_time)
        return kwebrtc.twcc_send(peer, status, base_seq, ref_time)
end


--- @brief  冲刷当前 TWCC 窗口并发送反馈 (含到达时间增量)
--- @param  [in] peer       会话
--- @return true 或 nil, code
--- @stability experimental
mod.twcc_flush = function (peer)
        return kwebrtc.twcc_flush(peer)
end


return mod
