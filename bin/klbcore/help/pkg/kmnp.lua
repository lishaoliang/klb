--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kmnp.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kmnp, MNP 连接 (文本/二进制/媒体 + RPC)
--   \n require("kmnp")
--   \n C导出文件: ./klb/src_packages/klbmnp/mnp_klua/klbmnp_klua.c
--   \n             ./klb/src_packages/klbmnp/mnp_klua/klbmnp_rpc_klua.c
--   \n 媒体帧: lightuserdata(klb_buf_t*) = [klb_mnp_media_t][码流]
--   \n 失败统一为 nil, code[klb_netcode_e]; 描述串用 knetcode.strerror(code)
--   \n RPC 常量(脚本侧自带): RPC_LUA=0x10 / RPC_JSON=0x11;
--   \n   method: POST=0x03 / NOTIFY=0x0A / REQUEST=0x10 / RESPONSE=0x11
-- @version 0.1
--]]

local kmnp = {}


-- @brief 连接服务端
-- @param [in] host[string]	目标主机
-- @param [in] port[number(int)]	目标端口
-- @param [in] opts[table]		[可选] tls[boolean]; crypto[table]{format, key, key_size, rekey_after}
-- @return conn[userdata]	连接对象 (兼服务端对象, is_client=false)
-- @return nil, code[number(int)]	失败 (无 netmulti/crypto 字段非法为 INVAL)
-- @note 对象方法: disconnect / send_text / send_binary / co_recv / free_media
--   \n           peer / set_timeout / set_max_msg / last_recv_ms
kmnp.connect = function (host, port, opts)
	local conn = {}

	-- @brief 断开 (幂等); 先唤醒挂起的 co_recv ("exit")
	-- @return 无
	-- @note __gc / __close 亦调此
	conn.disconnect = function (self)
		return
	end

	-- @brief 发送文本帧
	-- @param [in] head[string]	头部
	-- @param [in] body[string]	[可选] 主体
	-- @return [number(int)] 0.成功; 非0.错误码 (已断开恒 1)
	conn.send_text = function (self, head, body)
		return 0
	end

	-- @brief 发送二进制帧
	-- @param [in] head[string]	头部
	-- @param [in] body[string]	[可选] 主体
	-- @return [number(int)] 0.成功; 非0.错误码
	conn.send_binary = function (self, head, body)
		return 0
	end

	-- @brief 接收一条事件/媒体
	-- @return kind[string]	"text" / "binary" / "media" / "error" / "exit"
	-- @return a[string|lightuserdata|number]	text/binary 为 head; media 为 klb_buf_t*; error 为 code
	-- @return b[string]	text/binary 为 body; 其它为空串/nil
	-- @note 仅协程内调用; 同一时刻只允许一个等待者
	--   \n 注意: kmnp 无 send_media, 只能收媒体
	conn.co_recv = function (self)
		return "exit", "", ""
	end

	-- @brief 释放 co_recv 得到的媒体帧
	-- @param [in] buf[lightuserdata]	klb_buf_t*
	-- @return 无
	conn.free_media = function (self, buf)
		return
	end

	-- @brief 对端地址
	-- @return [string] "ip:port"; 客户端侧恒为空串
	conn.peer = function (self)
		return ''
	end

	-- @brief 设置超时
	-- @param [in] read_ms[number(int)]	[可选] 读空闲超时; 默认 0=关闭
	-- @param [in] write_ms[number(int)]	[可选] 写队列超时; 默认 0=关闭
	-- @param [in] idle_ms[number(int)]	[可选] 全空闲超时; 默认 0=关闭
	-- @return [number(int)] 0.成功; INVAL.无连接
	-- @note 亦支持单表形式 set_timeout{read=,write=,idle=}; 单位毫秒
	conn.set_timeout = function (self, read_ms, write_ms, idle_ms)
		return 0
	end

	-- @brief 设置单条消息重组上限
	-- @param [in] bytes[number(int)]	上限字节; <=0 不限; 默认 16MB
	-- @return [number(int)] 0.成功; INVAL.无连接
	conn.set_max_msg = function (self, bytes)
		return 0
	end

	-- @brief 最近收包毫秒时间 (含 PING/PONG)
	-- @return [number(int)] 无连接为 0
	conn.last_recv_ms = function (self)
		return 0
	end

	return conn
end


-- @brief RPC 连接 (客户端)
-- @param [in] host[string]	目标主机
-- @param [in] port[number(int)]	目标端口
-- @param [in] opts[table]		[可选] 只认 tls[boolean] (无 crypto)
-- @return client[userdata]	RPC 客户端对象
-- @return nil, code[number(int)]	失败
-- @note 对象方法: disconnect / post / notify / co_recv / co_call / co_wait / status
kmnp.connect_rpc = function (host, port, opts)
	local client = {}

	-- @brief 断开
	-- @return 无
	client.disconnect = function (self)
		return
	end

	-- @brief 单向 POST
	-- @param [in] rpctype[number(int)]	0x10=LUA(变参按 map 序列化) / 0x11=JSON(单串)
	-- @param [in] ...	[任意] 载荷
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

	-- @brief 接收一条 POST/NOTIFY 消息
	-- @return ...	LUA 模式为解包后的多返回值; JSON 模式为单个 string
	-- @return nil	断连或环境退出
	-- @note 仅协程内调用
	client.co_recv = function (self)
		return
	end

	-- @brief REQUEST 并等待 RESPONSE
	-- @param [in] rpctype[number(int)]	0x10 / 0x11
	-- @param [in] ...	[任意] 载荷; 末尾的 number 实参按 timeout_ms 解析
	-- @return ...	回应的多返回值 (LUA) 或 string (JSON)
	-- @return nil	发送失败 / 断连 / 超时
	-- @note 仅协程内调用; 支持多 in-flight, 序号域 [1000,1000000] 回绕
	--   \n 超时后底层停止收发, 需 disconnect 重建连接
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


-- @brief 监听端口 (普通 MNP)
-- @param [in] port[number(int)]	监听端口
-- @param [in] opts[table]		[可选] host / family("ipv4"|"ipv6"|"dual") / max_connect(默认20)
--   \n reuseport / nodelay / keepalive
-- @return listen[userdata]	监听对象
-- @return nil, code[number(int)]	失败
kmnp.listen = function (port, opts)
	local listen = {}

	-- @brief 关闭监听
	-- @return 无
	listen.close = function (self)
		return
	end

	-- @brief 接收一个连接
	-- @return conn[userdata]	连接对象 (对象方法同 connect 返回对象)
	-- @return kind[string]	"ok" / "exit"
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
-- @param [in] opts[table]		[可选] 同 listen
-- @return listen[userdata]	监听对象
-- @return nil, code[number(int)]	失败
kmnp.listen_rpc = function (port, opts)
	local listen = {}

	-- @brief 关闭监听
	-- @return 无
	listen.close = function (self)
		return
	end

	-- @brief 接收一个 RPC 连接
	-- @return serverpc[userdata]	RPC 服务端对象
	-- @return nil			环境退出 (注意: 只有 1 个返回值)
	-- @note 仅协程内调用
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


-- @brief 以已探测的 sock 建 MNP 服务端
-- @param [in] sock[lightuserdata]	klb_netlisten_socket_t* (来自 klisten:co_accept)
-- @param [in] opts[table]			[可选] tls[boolean](默认继承 sock->tls) / cert / key
--   \n ping_interval_ms / ping_timeout_ms
-- @return serve[userdata]	服务端对象 (对象方法同 connect 返回对象)
-- @return nil, code[number(int)]	失败 (TLS_CERT / TLS_HANDSHAKE / NOMEM / INVAL)
-- @note 所有权: C 侧取 fd 后释放该结构, 传入指针立即失效
kmnp.new_serve = function (sock, opts)
	local serve = {}

	return serve
end


-- @brief 以已探测的 sock 建 RPC 服务端
-- @param [in] sock[lightuserdata]	klb_netlisten_socket_t*
-- @param [in] opts[table]			[可选] tls / cert / key (无 ping 选项)
-- @return serverpc[userdata]	RPC 服务端对象
-- @return nil, code[number(int)]	失败
-- @note 所有权同 new_serve
--   \n RPC 服务端对象方法: disconnect / post / notify / response / co_recv / status / peer
kmnp.new_serve_rpc = function (sock, opts)
	local serverpc = {}

	-- @brief 断开
	-- @return 无
	serverpc.disconnect = function (self)
		return
	end

	-- @brief 单向 POST
	-- @param [in] rpctype[number(int)]	0x10 / 0x11
	-- @param [in] ...	[任意] 载荷
	-- @return [number(int)] 0.成功; 非0.错误码
	serverpc.post = function (self, rpctype, ...)
		return 0
	end

	-- @brief 单向 NOTIFY
	-- @param [in] rpctype[number(int)]	0x10 / 0x11
	-- @param [in] ...	[任意] 载荷
	-- @return [number(int)] 0.成功; 非0.错误码
	serverpc.notify = function (self, rpctype, ...)
		return 0
	end

	-- @brief 对 REQUEST 回 RESPONSE
	-- @param [in] rpctype[number(int)]	0x10 / 0x11
	-- @param [in] sequence[number(int)]	取自 status()
	-- @param [in] ...	[任意] 载荷
	-- @return [number(int)] 0.成功; 非0.错误码
	-- @note co_recv 的载荷不含 sequence, 必须紧接 status() 取号
	serverpc.response = function (self, rpctype, sequence, ...)
		return 0
	end

	-- @brief 接收一条 POST/NOTIFY/REQUEST
	-- @return ...	LUA 为多返回值; JSON 为 string
	-- @return nil	断连或环境退出
	-- @note 仅协程内调用
	serverpc.co_recv = function (self)
		return
	end

	-- @brief 最近一次收包状态 (含可回应的 sequence)
	-- @return [table] rpctype, method, sequence
	serverpc.status = function (self)
		return {}
	end

	-- @brief 对端地址
	-- @return [string] "ip:port"
	serverpc.peer = function (self)
		return ''
	end

	return serverpc
end


return kmnp
