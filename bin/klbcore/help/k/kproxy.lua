--[[
-- Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kproxy.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kproxy, 裸 TCP/TLS 连接与中继 (代理用)
--   \n require("kproxy")
--   \n C导出文件: ./klb/src_c/klua/klua_net/klua_kproxy.c
--   \n             ./klb/src_c/klua/klua_net/klua_kproxy_co.c
--   \n 失败形状为字符串描述 (非数字错误码): nil/false, "err"
-- @version 0.1
--]]

local kproxy = {}


-- @brief 是否可用
-- @return [boolean] 恒为 true
kproxy.available = function ()
	return true
end


-- @brief 双向中继两个 kproxy.tcp 对象
-- @param [in] a[userdata]		kproxy.tcp
-- @param [in] b[userdata]		kproxy.tcp
-- @param [in] opts[table]		[可选] 中继参数 (见 C 侧实现)
-- @return [boolean] true.成功
-- @return false, err[string]	失败: "need a and b" / "opts must be table"
--   \n "closed" / "select fail" / "timeout"
kproxy.relay = function (a, b, opts)
	return false, "timeout"
end


-- @brief 建立裸 TCP 监听
-- @param [in] host[string]	绑定地址
-- @param [in] port[number(int)]	监听端口
-- @param [in] backlog[number(int)]	[可选] backlog
-- @return listener[userdata]	监听对象
-- @return nil, err[string]	失败
kproxy.listen = function (host, port, backlog)
	local listener = {}

	-- @brief 关闭监听
	-- @return 无
	-- @note __gc / __close 亦调此
	listener.close = function (self)
		return
	end

	-- @brief 同步 accept
	-- @param [in] timeout_ms[number(int)]	[可选] 超时; 缺省一直阻塞
	-- @return tcp[userdata]	连接对象
	-- @return nil, "timeout"	超时
	-- @return nil, err[string]	其它失败
	listener.accept = function (self, timeout_ms)
		return nil, "timeout"
	end

	-- @brief 协程内 accept (内部异步驱动)
	-- @return tcp[userdata]
	-- @return nil, err[string]	失败
	-- @note 仅协程内调用
	listener.co_accept = function (self)
		return nil, "async attach fail"
	end

	-- @brief 本地地址
	-- @return [string|nil] ip
	-- @return [number(int)|nil] port
	listener.local_addr = function (self)
		return nil, nil
	end

	return listener
end


-- @brief 同 listen (保留别名)
-- @param [in] host[string]	绑定地址
-- @param [in] port[number(int)]	监听端口
-- @param [in] backlog[number(int)]	[可选] backlog
-- @return listener[userdata]	监听对象 (方法同 kproxy.listen)
-- @return nil, err[string]	失败
kproxy.listen_raw = function (host, port, backlog)
	local listener = {}

	return listener
end


-- @brief 同步建立 TCP 连接
-- @param [in] host[string]	目标主机
-- @param [in] port[number(int)]	目标端口 (1..65535)
-- @param [in] timeout_ms[number(int)]	[可选] 连接超时; 默认 15000 (下限钳到 1)
-- @return tcp[userdata]	连接对象
-- @return nil, err[string]	失败: "bad host" / "bad port" / "connect fail"
kproxy.tcp_connect = function (host, port, timeout_ms)
	local tcp = {}

	-- @brief 关闭 (幂等)
	-- @return 无
	-- @note __gc / __close 亦调此
	tcp.close = function (self)
		return
	end

	-- @brief 同步发送
	-- @param [in] data[string]	数据
	-- @return [boolean] true.成功
	-- @return false, err[string]	失败: "closed" / "send fail" / "send eof"
	--   \n TLS: "tls send timeout" / "tls send fail"
	tcp.send = function (self, data)
		return true
	end

	-- @brief 同步接收
	-- @param [in] max_n[number(int)]	最大字节数
	-- @param [in] timeout_ms[number(int)]	超时毫秒
	-- @return data[string]
	-- @return nil, err[string]	失败: "closed" / "bad max" / "timeout" / "recv fail"
	--   \n "eof" / "select fail" / "tls recv fail"
	tcp.recv = function (self, max_n, timeout_ms)
		return nil, "timeout"
	end

	-- @brief 预读 (不消费)
	-- @param [in] max_n[number(int)]	最大字节数
	-- @return data[string]
	-- @return nil, err[string]	失败: "closed" / "bad max" / "peek fail" / "eof"
	tcp.recv_peek = function (self, max_n)
		return nil, "eof"
	end

	-- @brief 本地地址
	-- @return [string|nil] ip
	-- @return [number(int)|nil] port
	tcp.local_addr = function (self)
		return nil, nil
	end

	-- @brief 对端地址
	-- @return [string|nil] ip
	-- @return [number(int)|nil] port
	tcp.peer_addr = function (self)
		return nil, nil
	end

	-- @brief 交出底层监听 socket (供 ksmp/kmnp.new_serve)
	-- @return sock[lightuserdata]	klb_netlisten_socket_t*
	-- @return nil, err[string]	失败
	tcp.steal_listen_socket = function (self)
		return nil, "closed"
	end

	-- @brief 就地升级 TLS (同步)
	-- @param [in] hostname[string|table]	[可选] 主机名; 或 opts 表 (如 {hostname=, ca=, pin=})
	-- @param [in] timeout_ms[number(int)]	[可选] 握手超时
	-- @return [boolean] true.成功
	-- @return false, err[string]	失败: "closed" / "tls create fail" / "tls opts bad"
	--   \n "tls handshake fail" / "tls handshake timeout" / "tls pin mismatch"
	tcp.upgrade_tls = function (self, hostname, timeout_ms)
		return false, "tls handshake timeout"
	end

	-- @brief 协程内接收
	-- @param [in] max_n[number(int)]	最大字节数
	-- @return [string]	数据
	-- @return nil, err[string]	失败
	-- @note 仅协程内调用; 同一时刻只允许一个挂起者
	tcp.co_recv = function (self, max_n)
		return ""
	end

	-- @brief 协程内发送 (写缓存背压)
	-- @param [in] data[string]	数据
	-- @return [boolean] true.成功
	-- @return false, err[string]	失败
	-- @note 仅协程内调用
	tcp.co_send = function (self, data)
		return true
	end

	-- @brief 协程内升级 TLS
	-- @param [in] opts[table]	[可选] 如 {hostname=}
	-- @return [boolean] true.成功
	-- @return false, err[string]	失败
	-- @note 仅协程内调用
	tcp.co_upgrade_tls = function (self, opts)
		return true
	end

	return tcp
end


-- @brief 协程内建立连接 (异步)
-- @param [in] host[string]	目标主机
-- @param [in] port[number(int)]	目标端口
-- @return tcp[userdata]	连接对象
-- @return nil, err[string]	失败: "bad host/port" / 连接错误
-- @note 仅协程内调用
kproxy.co_connect = function (host, port)
	local tcp = {}

	return tcp
end


-- @brief 自有隧道连接 (保留导出, 当前不实现)
-- @param [in] ...	[任意] 忽略
-- @return nil, "use_lua_tunnel_frame"
-- @note 正式握手在 packages/proxy/tunnel (Lua 明文帧); 本轮 C 侧不加密
kproxy.tunnel_connect = function (...)
	return nil, "use_lua_tunnel_frame"
end


return kproxy
