--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   khttp.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C khttp, HTTP 客户端/服务端 + RPC
--   \n require("khttp")
--   \n C导出文件: ./klb/src_packages/klbhttp/http_klua/klbhttp_klua.c
--   \n             ./klb/src_packages/klbhttp/http_klua/klbhttp_rpc_klua.c
--   \n 失败统一为 nil, code[klb_netcode_e]; 描述串用 knetcode.strerror(code)
--   \n 文本包走 MNP TEXT 帧 (head=HTTP 头, body=报文体), 非 HTTP 协议栈
-- @version 0.1
--]]

local khttp = {}


-- @brief 建立 HTTP 连接 (客户端)
-- @param [in] host[string]	目标主机
-- @param [in] port[number(int)]	目标端口
-- @param [in] opts[table]		[可选] tls[boolean]; 默认 false
-- @return handle[userdata]	连接对象 (client/server 共用类型)
-- @return nil, code[number(int)]	失败 (CONNECT_FAIL 等)
-- @note 对象方法: disconnect / send / send_file / co_recv / tls / peer
--   \n           set_timeout / set_max_body / set_connect / upgrade_tls / upgrade
khttp.connect = function (host, port, opts)
	local handle = {}

	-- @brief 关闭连接 (幂等); 不主动唤醒挂起的 co_recv
	-- @return 无
	-- @note __gc / __close 亦调此
	handle.disconnect = function (self)
		return
	end

	-- @brief 发送一个文本包
	-- @param [in] head[string]	头部 (HTTP 请求行 + 头域)
	-- @param [in] body[string]	[可选] 主体
	-- @return [number(int)] 0.成功; 非0.错误码
	-- @note 底层走 klb_netconn_send_text(seq=0), 非 HTTP 协议栈组包
	handle.send = function (self, head, body)
		return 0
	end

	-- @brief 发送头部 + 文件内容
	-- @param [in] head[string]	头部
	-- @param [in] path[string]	文件路径
	-- @param [in] opts[table]		[可选] offset[number]=0; length[number]=0(0 表示只发头)
	-- @return [number(int)] 0.成功; 非0.错误码
	-- @note 先开文件再入队, 开失败不发头
	handle.send_file = function (self, head, path, opts)
		return 0
	end

	-- @brief 接收一个文本包
	-- @return kind[string]	"text" / "error"
	-- @return head[string|number]	text 为头部; error 为错误码
	-- @return body[string]	text 为主体
	-- @note 仅协程内调用; 同一时刻只允许一个等待者
	handle.co_recv = function (self)
		return "text", "", ""
	end

	-- @brief 当前连接是否 TLS
	-- @return [boolean]
	handle.tls = function (self)
		return false
	end

	-- @brief 对端地址
	-- @return [string] "ip:port" (缓冲 128B)
	handle.peer = function (self)
		return ''
	end

	-- @brief 设置超时
	-- @param [in] read_ms[number(int)|table]	读空闲超时; 或表 {read=,write=,idle=}
	-- @param [in] write_ms[number(int)]	[可选] 写队列超时
	-- @param [in] idle_ms[number(int)]	[可选] 全空闲超时
	-- @return [number(int)] 0.成功; INVAL.无连接
	-- @note 单位毫秒; 0=关闭该维度
	handle.set_timeout = function (self, read_ms, write_ms, idle_ms)
		return 0
	end

	-- @brief 设置收包体上限
	-- @param [in] max_body[number(int)]	[可选] 字节; 0=不限
	-- @return [number(int)] 0.成功; INVAL.无连接
	-- @note 超限上抛 PROTO_TOO_LARGE
	handle.set_max_body = function (self, max_body)
		return 0
	end

	-- @brief 置/清 CONNECT 2xx 无体应答模式 (正向代理隧道; 仅客户端)
	-- @param [in] connect[boolean]	[可选] 默认 false
	-- @return [number(int)] 0.成功; INVAL.服务端或无连接
	handle.set_connect = function (self, connect)
		return 0
	end

	-- @brief 明文连接就地升级 TLS (fd 不变; 仅客户端)
	-- @return [number(int)] 0.成功; INVAL.服务端或无连接
	handle.upgrade_tls = function (self)
		return 0
	end

	-- @brief 升级为 WebSocket (仅服务端)
	-- @return ws[userdata]	kws 连接对象
	-- @return nil, code[number(int)]	失败: INVAL(非服务端/无连接)
	--   \n BUSY(写缓存非空) / NOTSUP(未注册 kws.adopt) / take 返回码
	-- @note 成功后本 handle 失效 (所有权转移到 kws)
	handle.upgrade = function (self)
		return nil, -22
	end

	return handle
end


-- @brief RPC 连接 (客户端)
-- @param [in] host[string]	目标主机
-- @param [in] port[number(int)]	目标端口
-- @param [in] opts[table]		[可选] tls[boolean]
-- @return client[userdata]	RPC 客户端对象
-- @return nil, code[number(int)]	失败
-- @note 对象方法: disconnect / post / notify / co_recv / co_call / co_wait / status
khttp.connect_rpc = function (host, port, opts)
	local client = {}

	-- @brief 关闭 (幂等)
	-- @return 无
	client.disconnect = function (self)
		return
	end

	-- @brief 单向 POST
	-- @param [in] rpctype[number(int)]	0x10=LUA / 0x11=JSON
	-- @param [in] ...	[任意] 载荷 (JSON 模式必须单个字符串)
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
	-- @return ...	LUA 为解包后的多返回值; JSON 为单个 string
	-- @note 仅协程内调用
	client.co_recv = function (self)
		return
	end

	-- @brief REQUEST 并等待 RESPONSE
	-- @param [in] rpctype[number(int)]	0x10 / 0x11
	-- @param [in] ...	[任意] 载荷
	-- @return ...	回应值
	-- @return nil	发送失败 / 断连 / 超时
	-- @note 仅协程内调用; 序号域 [1000,1000000] 回绕
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
	client.status = function (self)
		return {}
	end

	return client
end


-- @brief 监听端口
-- @param [in] port[number(int)]	监听端口
-- @param [in] opts[table]		[可选] 见下方
-- @return listen[userdata]	监听对象
-- @return nil, code[number(int)]	失败 (TLS_CERT / BIND_FAIL / 配置非法 INVAL)
-- @note opts = {
--   \n   tls[boolean](默认 false) / plain[boolean](默认 false; 仅 tls 时有意义)
--   \n   cert[string] / key[string]
--   \n   host / max_connect(默认20) / family / reuseport / nodelay / keepalive
--   \n }
--   \n accept 必经协议探测, 探测通过才入待 accept 队列
khttp.listen = function (port, opts)
	local listen = {}

	-- @brief 关闭监听
	-- @return 无
	listen.close = function (self)
		return
	end

	-- @brief 热更新 TLS 证书 (不重 bind)
	-- @param [in] cfg[table]	{ cert=, key= }; 两者必须非空
	-- @return [boolean] true.成功; false.已关闭/参数空/建 ctx 失败
	-- @note 仅新 accept 用新 ctx, 旧连接仍持旧 ctx
	listen.reload_tls = function (self, cfg)
		return false
	end

	-- @brief 接收一个连接
	-- @return handle[userdata|nil]	连接对象
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
-- @param [in] opts[table]		[可选] 同 listen
-- @return listen[userdata]	监听对象
-- @return nil, code[number(int)]	失败
-- @note 与 listen 的差异: 无 reload_tls
khttp.listen_rpc = function (port, opts)
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


-- @brief 以已探测的 sock 建 HTTP 服务端 (可开 TLS)
-- @param [in] sock[lightuserdata]	klb_netlisten_socket_t* (来自 klisten:co_accept)
-- @param [in] opts[table]			[可选] tls[boolean](默认继承 sock->tls) / cert / key
-- @return handle[userdata]	连接对象 (服务端角色; 方法同 connect 返回对象)
-- @return nil, code[number(int)]	失败 (INVAL / TLS_CERT / TLS_HANDSHAKE / NOMEM)
-- @note 所有权: C 侧取 fd 后释放该结构, 传入指针立即失效
khttp.new_serve = function (sock, opts)
	local handle = {}

	return handle
end


return khttp
