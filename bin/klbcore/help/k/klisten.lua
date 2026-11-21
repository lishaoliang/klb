--[[
-- Copyright(c) 2026, LGPL All Rights Reserved
-- @file   klisten.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C klisten
--   \n require("klisten")
--   \n C导出文件: ./klb/src_c/klua/klua_net/klua_klisten.c
--   \n 共享 listen + probe; wrap/unwrap 供 klist 跨线程传 sock
-- @version 0.2
--]]

local klisten = {}


-- @brief 监听端口
-- @param [in] port[number(int)]
-- @param [in] cfg[table]			[可选] 见内联
-- @return userdata|nil				成功为 handle; 失败为 nil
-- @return code[number|nil]			失败时 klb_netcode_e; 描述串用 knetcode.strerror(code)
-- @note 对象方法 `:co_accept` → proto, sock, msg, tls; `:set_mux_opts(cfg)` 热更新 tls_as_*
--   \n cfg = {
--   \n   tls_as_http[boolean]	[可选] 默认 true
--   \n   tls_as_mnp[boolean]	[可选] 默认 false
--   \n   tls_as_rtsp[boolean]	[可选] 默认 false; TLS 投递 RTSP(rtsps)
--   \n   max_connect[number]	[可选] listen backlog; 默认 20
--   \n }
klisten.listen = function (port, cfg)
	local listen = {}

	-- @brief 关闭监听 (幂等)
	-- @return 无
	-- @note __gc / __close 亦调此
	listen.close = function (self)
		return
	end

	-- @brief 接收一个已完成协议探测的连接
	-- @return proto[string|nil]	http/smp/mnp/smprpc/mnprpc/...; 退出为 nil
	-- @return sock[lightuserdata|nil]	netmux sock; 退出为 nil
	-- @return msg[string]		"exit" 表示环境退出; 其余为空串
	-- @return tls[boolean]		该 sock 是否为 TLS
	-- @note 仅协程内调用; sock 所有权交给调用方 (wrap 或 drop)
	listen.co_accept = function (self)
		return nil, nil, "exit", false
	end

	-- @brief 监听端口
	-- @return [number(int)]
	listen.port = function (self)
		return 0
	end

	-- @brief 配置的绑定地址 (INADDR_ANY 为空串)
	-- @return [string]
	listen.host = function (self)
		return ''
	end

	-- @brief 实际本地地址
	-- @return [string|nil] ip
	-- @return [number(int)|nil] port
	listen.local_addr = function (self)
		return nil, nil
	end

	-- @brief 监听 native fd
	-- @return [number(int)|nil]
	listen.native_fd = function (self)
		return nil
	end

	-- @brief 以新绑定地址重开监听 (同端口; 既有连接不受影响)
	-- @param [in] host[string]	[可选] 新绑定地址; nil/"" = INADDR_ANY
	-- @return [boolean] true.成功
	-- @return nil, code[number(int)]	失败 (已关闭为 INVAL)
	listen.rebind = function (self, host)
		return true
	end

	return listen
end


-- @brief 热更新已 open mux 的 tls_as_* (不重 bind)
-- @param [in] self[userdata]
-- @param [in] cfg[table]			tls_as_http / tls_as_mnp / tls_as_rtsp 任选
-- @return ok[boolean]|nil
-- @return code[number|nil]
klisten.set_mux_opts = function (self, cfg)
	return nil, nil
end


-- @brief 丢弃未消费的 netmux sock (关 fd)
-- @param [in] sock[lightuserdata]
-- @return 无
klisten.drop = function (sock)
	return
end


-- @brief 取 netmux sock (accept 所得) 的对端 IPv4 地址
-- @param [in] sock[lightuserdata]
-- @return ip[string|nil]			点分十进制; 无 sock 为 nil
-- @return port[number|nil]			对端端口 (主机序); 无 sock 为 nil
klisten.sock_addr = function (sock)
	return nil, nil
end


-- @brief 取 netmux sock 的 native fd
-- @param [in] sock[lightuserdata]
-- @return [number(int)|nil]	fd; 无 sock 为 nil
klisten.sock_fd = function (sock)
	return nil
end


-- @brief 取 netmux sock 探测协议名 (与 co_accept 首返回值同形)
-- @param [in] sock[lightuserdata]
-- @return proto[string|nil]		http/smp/mnp/smprpc/mnprpc/...; 无 sock 为 nil
-- @note kmuxlist.pop 后可据此区分同队多协议 (如 RPC smp/mnp)
klisten.sock_protocol = function (sock)
	return nil
end


-- @brief 将 netmux sock 封为 klb_obj (klist 传递); 取得所有权
-- @param [in] sock[lightuserdata]
-- @return [lightuserdata|nil] klb_obj_t*
klisten.wrap = function (sock)
	return nil
end


-- @brief 从 klb_obj 还原 netmux sock (供 khttp/krtsp.new_serve)
-- @param [in] obj[lightuserdata]	klb_obj_t*
-- @return [lightuserdata|nil]
klisten.unwrap = function (obj)
	return nil
end


-- @brief 销毁未 unwrap 的 sock obj (关 fd)
-- @param [in] obj[lightuserdata]
-- @return 无
klisten.destroy_obj = function (obj)
	return
end


return klisten
