--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   krtsp.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C krtsp, RTSP 客户端/服务端 (MNP 承载)
--   \n require("krtsp")
--   \n C导出文件: ./klb/src_packages/klbrtsp/rtsp_klua/klbrtsp_klua.c
--   \n             ./klb/src_packages/klbrtsp/rtsp_klua/klbrtsp_serve_klua.c
--   \n 媒体帧: lightuserdata(klb_buf_t*) = [klb_mnp_media_t][码流]
--   \n send_media 转移所有权; co_recv 得到的帧须 free_media
--   \n 失败统一为 nil, code[klb_netcode_e]; 描述串用 knetcode.strerror(code)
-- @version 0.1
--]]

local krtsp = {}


-- @brief 连接 RTSP 服务端
-- @param [in] host[string]	目标主机
-- @param [in] port[number(int)]	目标端口
-- @return client[userdata]	客户端对象
-- @return nil, code[number(int)]	失败 (无 netmulti 为 INVAL; 建连失败兜底 CONNECT_FAIL)
krtsp.connect = function (host, port)
	local client = {}

	-- @brief 断开 (幂等); 会唤醒挂起的 co_recv ("exit")
	-- @return 无
	-- @note __gc / __close 亦调此
	client.disconnect = function (self)
		return
	end

	-- @brief 发送 RTSP 文本 (仅 head, 无 body 参数)
	-- @param [in] head[string]	报文头 (含请求/响应行)
	-- @return [number(int)] 0.成功; 非0.错误码 (已断开为 1)
	client.send_text = function (self, head)
		return 0
	end

	-- @brief 发送媒体帧
	-- @param [in] buf[lightuserdata]	klb_buf_t* (须自带 klb_mnp_media_t 头)
	-- @return [number(int)] 0.成功; 非0.错误码
	-- @note 所有权转移: 无论成败 C 侧都 unref, 调用后不得再 free_media
	client.send_media = function (self, buf)
		return 0
	end

	-- @brief 设置视频 payload type
	-- @param [in] pt[number(int)]	默认 96
	-- @return 无
	client.set_video_pt = function (self, pt)
		return
	end

	-- @brief 设置音频 payload type
	-- @param [in] pt[number(int)]	默认 97; 0=关闭音频
	-- @return 无
	client.set_audio_pt = function (self, pt)
		return
	end

	-- @brief 设置音频参数
	-- @param [in] samples[number(int)]	采样率; 默认 44100
	-- @param [in] tracks[number(int)]	[可选] 声道/轨数; 默认 2
	-- @param [in] bits[number(int)]		[可选] 位宽; 默认 2
	-- @return 无
	client.set_audio_info = function (self, samples, tracks, bits)
		return
	end

	-- @brief 设置超时
	-- @param [in] read_ms[number(int)]	[可选] 读空闲超时; 默认 0=不启用
	-- @param [in] write_ms[number(int)]	[可选] 写队列超时; 默认 0=不启用
	-- @param [in] idle_ms[number(int)]	[可选] 全空闲超时; 默认 0=不启用
	-- @return 无
	-- @note 单位毫秒; 超时上报 KLB_SOCKET_TIMEOUT
	client.set_timeout = function (self, read_ms, write_ms, idle_ms)
		return
	end

	-- @brief 打开本地 UDP 收流口
	-- @return [number(int)]	本地端口
	-- @return nil	失败
	client.open_udp = function (self)
		return 0
	end

	-- @brief 关闭本地 UDP 口
	-- @return 无
	client.close_udp = function (self)
		return
	end

	-- @brief UDP 打洞 (向对端偶数 RTP 端口)
	-- @param [in] host[string]	对端 IP
	-- @param [in] port[number(int)]	对端端口
	-- @return [boolean] true.成功
	client.udp_punch = function (self, host, port)
		return false
	end

	-- @brief 接收一条事件/媒体
	-- @return kind[string]	"text" / "media" / "exit"
	-- @return body[string|lightuserdata]	text 为报文; media 为 klb_buf_t*; exit 为 nil
	-- @note 仅协程内调用; 一次只返回一个元素; text 与 media 同时就绪优先 text
	--   \n 注意: 客户端是 "先 kind 后载荷", 与服务端相反
	client.co_recv = function (self)
		return "exit", nil
	end

	-- @brief 释放 co_recv 得到的媒体帧
	-- @param [in] buf[lightuserdata]	klb_buf_t*
	-- @return 无
	client.free_media = function (self, buf)
		return
	end

	-- @brief 调试: 打印媒体帧内容到 stdout
	-- @param [in] buf[lightuserdata]	klb_buf_t*
	-- @return 无
	client.dump_media = function (self, buf)
		return
	end

	return client
end


-- @brief 监听 RTSP 端口 (明文; TLS 须走 new_serve)
-- @param [in] port[number(int)]	监听端口
-- @param [in] opts[table]			[可选] host / max_connect(默认20) / family("ipv4"|"ipv6"|"dual")
--   \n reuseport / nodelay / keepalive
-- @return listen[userdata]	监听对象
-- @return nil, code[number(int)]	失败 (family 非法为 INVAL)
krtsp.listen = function (port, opts)
	local listen = {}

	-- @brief 关闭监听
	-- @return 无
	listen.close = function (self)
		return
	end

	-- @brief 接收一个连接
	-- @return [lightuserdata|nil]	klb_netlisten_socket_t*; nil 表示退出
	-- @return kind[string]	"ok" / "exit"
	-- @note 仅协程内调用; 得到的 sock 可喂给 krtsp.new_serve (所有权转移)
	listen.co_accept = function (self)
		return nil, "exit"
	end

	-- @brief 监听端口 (未 open 为 0)
	-- @return [number(int)]
	listen.port = function (self)
		return 0
	end

	-- @brief 本地地址
	-- @return [string|nil] ip
	-- @return [number(int)|nil] port
	listen.local_addr = function (self)
		return nil, nil
	end

	-- @brief 底层 fd
	-- @return [number(int)|nil]
	listen.native_fd = function (self)
		return nil
	end

	return listen
end


-- @brief 以已探测的 sock 建 RTSP 服务端 (可开 TLS)
-- @param [in] sock[lightuserdata]	klb_netlisten_socket_t* (来自 klisten/krtsp 的 co_accept)
-- @param [in] opts[table]			[可选] tls[boolean](默认继承 sock->tls) / cert[string] / key[string]
-- @return serve[userdata]	服务端对象
-- @return nil, code[number(int)]	失败 (TLS_CERT / TLS_HANDSHAKE / NOMEM / INVAL)
-- @note 所有权: C 侧取 fd 后释放该结构, 传入指针立即失效, 不可重复使用
krtsp.new_serve = function (sock, opts)
	local serve = {}

	-- @brief 断开 (幂等); 会唤醒挂起的 co_recv
	-- @return 无
	serve.disconnect = function (self)
		return
	end

	-- @brief 发送 RTSP 文本 (服务端有 body 参数)
	-- @param [in] head[string]	报文头
	-- @param [in] body[string]	[可选] 报文体
	-- @return [number(int)] 0.成功; 非0.错误码
	serve.send_text = function (self, head, body)
		return 0
	end

	-- @brief 发送媒体帧
	-- @param [in] buf[lightuserdata]	klb_buf_t*
	-- @return [number(int)] 0.成功; 非0.错误码
	-- @note 所有权转移, 调用后不得再 free_media
	serve.send_media = function (self, buf)
		return 0
	end

	-- @brief 单轨: TCP 交织传输 (RTCP 用 rtp_ch+1)
	-- @param [in] rtp_ch[number(int)]	RTP 通道号
	-- @return 无
	serve.setup_tcp = function (self, rtp_ch)
		return
	end

	-- @brief 单轨: UDP 传输
	-- @param [in] client_port[number(int)]	客户端端口
	-- @return [number(int)] 服务端端口
	-- @return nil	失败
	serve.setup_udp = function (self, client_port)
		return 0
	end

	-- @brief 设置超时
	-- @param [in] read_ms[number(int)]	[可选] 默认 0=不启用
	-- @param [in] write_ms[number(int)]	[可选] 默认 0=不启用
	-- @param [in] idle_ms[number(int)]	[可选] 默认 0=不启用
	-- @return 无
	serve.set_timeout = function (self, read_ms, write_ms, idle_ms)
		return
	end

	-- @brief 设置视频 payload type
	-- @param [in] pt[number(int)]	[可选] <=0 或 >=128 不生效
	-- @return 无
	serve.set_video_pt = function (self, pt)
		return
	end

	-- @brief 设置音频 payload type
	-- @param [in] pt[number(int)]	[可选] <=0 或 >=128 关闭音频
	-- @return 无
	serve.set_audio_pt = function (self, pt)
		return
	end

	-- @brief 按轨设置 payload type
	-- @param [in] track[number(int)]	轨索引
	-- @param [in] pt[number(int)]		[可选] payload type; 默认 0
	-- @param [in] is_audio[boolean]	[可选] true.音轨; 默认 false
	-- @return 无
	serve.set_track_pt = function (self, track, pt, is_audio)
		return
	end

	-- @brief 按轨 TCP 交织
	-- @param [in] track[number(int)]	轨索引
	-- @param [in] rtp_ch[number(int)]	RTP 通道号
	-- @return 无
	serve.setup_tcp_track = function (self, track, rtp_ch)
		return
	end

	-- @brief 按轨 UDP 传输
	-- @param [in] track[number(int)]	轨索引
	-- @param [in] client_port[number(int)]	客户端端口
	-- @return [number(int)] 服务端端口
	-- @return nil	失败
	serve.setup_udp_track = function (self, track, client_port)
		return 0
	end

	-- @brief 接收一条 RTSP 文本
	-- @return body[string]	报文
	-- @return kind[string]	"text" / "media" / "exit"
	-- @note 仅协程内调用; 注意: 服务端是 "先 body 后 kind", 与客户端相反
	--   \n RTSP 请求/响应语义完全由 Lua 侧解析, C 层不解协议
	serve.co_recv = function (self)
		return "", "exit"
	end

	return serve
end


return krtsp
