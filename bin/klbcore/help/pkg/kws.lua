--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kws.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kws, WebSocket 客户端/服务端 + RPC
--   \n require("kws")
--   \n C导出文件: ./klb/src_packages/klbws/ws_klua/klbws_klua.c
--   \n             ./klb/src_packages/klbws/ws_klua/klbws_rpc_klua.c
--   \n 失败统一为 nil, code[klb_netcode_e]; 描述串用 knetcode.strerror(code)
--   \n 单条消息上限默认 1MiB (set_max_payload 可调, 仅客户端)
--   \n close code: 1000.正常 / 1002.协议错 / 1007.UTF8错 / 1009.过大 / 1005.无状态
-- @version 0.1
--]]

local kws = {}


-- @brief 连接 WebSocket 服务端
-- @param [in] host[string]	目标主机
-- @param [in] port[number(int)]	目标端口
-- @param [in] path[string]	[可选] 请求路径
-- @param [in] opts[table]		[可选] 见下方; 若第 3 参为 table 则视为 opts
-- @return conn[userdata]	连接对象
-- @return nil, code[number(int)]	失败 (INVAL / CONNECT_FAIL / 握手码)
-- @note opts = {
--   \n   tls[boolean]		默认 false
--   \n   protocol[string]	Sec-WebSocket-Protocol; 长度 1..127, 禁 CR/LF
--   \n   headers[table]		额外请求头 name=value; 禁 CR/LF; 保留头被静默跳过
--   \n }
kws.connect = function (host, port, path, opts)
	local conn = {}

	-- @brief 断开 (幂等)
	-- @return 无
	-- @note __gc / __close 亦调此
	conn.disconnect = function (self)
		return
	end

	-- @brief 发送文本帧
	-- @param [in] s[string]	文本
	-- @return [number(int)] 0.成功; 非0.失败 (无连接恒 1)
	conn.send_text = function (self, s)
		return 0
	end

	-- @brief 发送二进制帧
	-- @param [in] s[string]	二进制数据
	-- @return [number(int)] 0.成功; 非0.失败
	-- @note 客户端自动掩码
	conn.send_binary = function (self, s)
		return 0
	end

	-- @brief 发送 PING
	-- @param [in] payload[string]	[可选] 载荷; >125B 时不发送
	-- @return [number(int)] 0.成功; 1.失败/超长
	conn.send_ping = function (self, payload)
		return 0
	end

	-- @brief 发送 CLOSE
	-- @param [in] code[number(int)]	[可选] 关闭码; 默认 1000
	-- @param [in] reason[string]	[可选] 原因; <=123B 且 UTF-8 合法
	-- @return [number(int)] 0.成功; 1.参数非法或失败
	conn.send_close = function (self, code, reason)
		return 0
	end

	-- @brief 对端地址
	-- @return [string] 客户端为 "host:port"; 服务端为 "ip:port"
	conn.peer = function (self)
		return ''
	end

	-- @brief 写缓存未发送字节数
	-- @return [number(int)]
	conn.buffered_amount = function (self)
		return 0
	end

	-- @brief 接收一条事件/数据
	-- @return kind[string]	"open" / "close" / "pong" / "text" / "http" / "binary" / "error" / "exit"
	-- @return a[string|number]	head(或 close code / error code)
	-- @return b[string]	body(或 reason)
	-- @note 仅协程内调用; 同一时刻只允许一个等待者
	--   \n 队列优先级: evt(open/close/pong) > text/http > binary > error
	conn.co_recv = function (self)
		return "exit", "", ""
	end

	-- @brief 设置超时 (仅客户端生效, 服务端调用返回 0)
	-- @param [in] read_ms[number(int)]	[可选] 默认 0=关闭
	-- @param [in] write_ms[number(int)]	[可选] 默认 0=关闭
	-- @param [in] idle_ms[number(int)]	[可选] 默认 0=关闭
	-- @return [number(int)] 0.成功; INVAL.无连接
	conn.set_timeout = function (self, read_ms, write_ms, idle_ms)
		return 0
	end

	-- @brief 设置单条消息上限 (仅客户端生效)
	-- @param [in] bytes[number(int)]	[可选] 字节; <=0 复位为默认 1MiB
	-- @return [number(int)] 0.成功; INVAL.无连接
	conn.set_max_payload = function (self, bytes)
		return 0
	end

	-- @brief 最近收包毫秒时间 (含 PING/PONG)
	-- @return [number(int)] 无连接为 0
	conn.last_recv_ms = function (self)
		return 0
	end

	return conn
end


-- @brief RPC 连接 (客户端; WS BINARY 承载)
-- @param [in] host[string]	目标主机
-- @param [in] port[number(int)]	目标端口
-- @param [in] opts[table]		[可选] tls[boolean] / path[string](默认 "/rpc") / protocol[string]
-- @return client[userdata]	RPC 客户端对象
-- @return nil, code[number(int)]	失败
-- @note 对象方法: disconnect / post / notify / co_recv / co_call / co_wait / status
kws.connect_rpc = function (host, port, opts)
	local client = {}

	-- @brief 断开
	-- @return 无
	client.disconnect = function (self)
		return
	end

	-- @brief 单向 POST
	-- @param [in] rpctype[number(int)]	0x10=LUA / 0x11=JSON
	-- @param [in] ...	[任意] 载荷 (JSON 模式取单个字符串)
	-- @return [number(int)] 0.成功; 非0.错误码
	client.post = function (self, rpctype, ...)
		return 0
	end

	-- @brief 单向 NOTIFY
	-- @param [in] rpctype[number(int)]	同上
	-- @param [in] ...	[任意] 载荷
	-- @return [number(int)] 0.成功; 非0.错误码
	client.notify = function (self, rpctype, ...)
		return 0
	end

	-- @brief 接收一条 POST/NOTIFY
	-- @return [table|string]	LUA 为解包值; JSON 为 string
	-- @return nil	断连或环境退出
	-- @note 仅协程内调用
	client.co_recv = function (self)
		return
	end

	-- @brief REQUEST 并等待 RESPONSE
	-- @param [in] rpctype[number(int)]	0x10 / 0x11
	-- @param [in] ...	[任意] 载荷
	-- @return [table|string]	回应值
	-- @return nil	发送失败 / 断连 / 超时 (序号不匹配的响应被丢弃)
	-- @note 仅协程内调用
	client.co_call = function (self, rpctype, ...)
		return
	end

	-- @brief 等待写缓存排空 (背压)
	-- @return nil	写缓存已空, 或断连
	-- @note 仅协程内调用
	client.co_wait = function (self)
		return nil
	end

	-- @brief 最近一次收发状态
	-- @return [table] rpctype, method, sequence, res_rpctype, res_method, res_sequence
	--   \n 字符串值: "LUA"/"JSON", "POST"/"NOTIFY"/"REQUEST"/"RESPONSE"
	client.status = function (self)
		return {}
	end

	return client
end


-- @brief 监听端口
-- @param [in] port[number(int)]	监听端口
-- @param [in] opts[table]		[可选]
-- @return listen[userdata]	监听对象
-- @return nil, code[number(int)]	失败 (TLS_CERT / bind 码)
-- @note opts = {
--   \n   tls[boolean] / cert[string] / key[string]
--   \n   path[string]				<256; 空=任意路径
--   \n   protocol[string]			<128
--   \n   ping_interval_ms[number] / ping_timeout_ms[number]	任一出现即启用服务端心跳
--   \n   host / max_connect(默认20) / family / reuseport / nodelay / keepalive
--   \n }
kws.listen = function (port, opts)
	local listen = {}

	-- @brief 关闭监听
	-- @return 无
	listen.close = function (self)
		return
	end

	-- @brief 接收一个连接
	-- @return conn[userdata|nil]	连接对象
	-- @return kind[string]	"ok" / "error" / "exit"
	-- @note 仅协程内调用
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


-- @brief 监听端口 (RPC 服务端)
-- @param [in] port[number(int)]	监听端口
-- @param [in] opts[table]		[可选] 同 listen; path 默认 "/rpc"; 无 ping 选项
-- @return listen[userdata]	监听对象
-- @return nil, code[number(int)]	失败
-- @note 注意: path/protocol 非法时只返回 nil 一个值
kws.listen_rpc = function (port, opts)
	local listen = {}

	-- @brief 关闭监听
	-- @return 无
	listen.close = function (self)
		return
	end

	-- @brief 接收一个 RPC 连接
	-- @return serverpc[userdata]	RPC 服务端对象
	-- @return nil			失败/退出 (只有 1 个返回值)
	-- @note 仅协程内调用
	--   \n serverpc 方法: disconnect / post(rpctype, ...) / notify(rpctype, ...)
	--   \n   response(rpctype, sequence, ...) / co_recv() / status()
	--   \n co_recv 载荷不含 sequence, 要回应须紧接 status() 取号
	listen.co_accept = function (self)
		return nil
	end

	-- @brief 监听端口
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


-- @brief 计算 Sec-WebSocket-Accept (服务端握手辅助)
-- @param [in] key[string]	Sec-WebSocket-Key
-- @return [string]	base64(SHA1(key + GUID))
-- @return nil		失败
kws.accept = function (key)
	return ''
end


-- @brief 接管已探测的 sock, 建 WS 连接 (服务端)
-- @param [in] sock[lightuserdata]	klb_netlisten_socket_t* (来自 klisten:co_accept)
-- @param [in] opts[table]			[可选] ping_interval_ms / ping_timeout_ms
-- @return conn[userdata]	连接对象
-- @return nil, code[number(int)]	失败 (INVAL / NOMEM)
-- @note 所有权: C 侧取 fd 后释放该结构, 传入指针立即失效
kws.adopt = function (sock, opts)
	local conn = {}

	return conn
end


return kws
