--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  kproxy 异步 IO 门面 (优先 co_*; 非协程回退同步 API)
-- @note   klbcore 真源; proxy 薄封装再导出; 探活可 opts.async=false
-- @history 修改历史
--  \n [2026-10] 创建 (M1 迁泵); 从 proxy 迁入 klbcore
--]]


local M = {}


local function _kproxy()
	local ok, mod = pcall(require, "kproxy")
	if ok and type(mod) == "table" then
		return mod
	end

	return nil
end


-- @brief 是否强制同步 (opts.async==false)
-- @param [in]  opts[table|nil]
-- @return [boolean]
local function _force_sync(opts)
	return type(opts) == "table" and opts.async == false
end


-- @brief TCP 连接 (协程内优先 co_connect)
-- @param [in]  host[string]
-- @param [in]  port[number]
-- @param [in]  timeout_ms[number|nil]	仅同步路径使用
-- @param [in]  opts[table|nil]			`{ async=false }` 强制同步
-- @return [userdata|nil] tcp
-- @return [string|nil] err
M.tcp_connect = function (host, port, timeout_ms, opts)
	local kproxy = _kproxy()
	if kproxy == nil then
		return nil, "kproxy unavailable"
	end

	port = tonumber(port)
	if type(host) ~= "string" or host == "" or port == nil then
		return nil, "bad host/port"
	end

	if not _force_sync(opts) and type(kproxy.co_connect) == "function" then
		local ok_c, tcp, err_c = pcall(kproxy.co_connect, host, port)
		if ok_c then
			return tcp, err_c
		end
		-- 非协程: 回退同步
	end

	if type(kproxy.tcp_connect) ~= "function" then
		return nil, "kproxy.tcp_connect unavailable"
	end

	return kproxy.tcp_connect(host, port, tonumber(timeout_ms) or 15000)
end


-- @brief 发送 (userdata 优先 co_send)
-- @param [in]  sock[any]
-- @param [in]  data[string]
-- @return [boolean] ok
-- @return [string|nil] err
M.send = function (sock, data)
	if sock == nil then
		return false, "closed"
	end

	if type(sock) == "userdata" and type(sock.co_send) == "function" then
		local ok_c, a, b = pcall(sock.co_send, sock, data)
		if ok_c then
			return a, b
		end
	end

	if type(sock.send) == "function" then
		return sock:send(data)
	end

	return false, "no send"
end


-- @brief 接收 (userdata 优先 co_recv)
-- @param [in]  sock[any]
-- @param [in]  max_n[number]
-- @param [in]  timeout_ms[number|nil]	仅同步路径
-- @return [string|nil] data
-- @return [string|nil] err
M.recv = function (sock, max_n, timeout_ms)
	if sock == nil then
		return nil, "closed"
	end

	max_n = tonumber(max_n) or 4096

	if type(sock) == "userdata" and type(sock.co_recv) == "function" then
		local ok_c, a, b = pcall(sock.co_recv, sock, max_n)
		if ok_c then
			return a, b
		end
	end

	if type(sock.recv) == "function" then
		if timeout_ms ~= nil then
			return sock:recv(max_n, timeout_ms)
		end

		return sock:recv(max_n)
	end

	return nil, "no recv"
end


-- @brief 窥探接收 (MSG_PEEK, 不消费); 供 demux 后 steal 给 ksmp/kmnp
-- @param [in]  sock[any]
-- @param [in]  max_n[number]
-- @return [string|nil] data
-- @return [string|nil] err
M.recv_peek = function (sock, max_n)
	if sock == nil then
		return nil, "closed"
	end

	max_n = tonumber(max_n) or 4
	if type(sock) == "userdata" and type(sock.recv_peek) == "function" then
		return sock:recv_peek(max_n)
	end

	return nil, "recv_peek unavailable"
end


-- @brief TLS 升级 (优先 co_upgrade_tls)
-- @param [in]  tcp[userdata]
-- @param [in]  hostname_or_opts[string|table|nil]	主机名或 `{ hostname/sni, verify, ca_pem, alpn, pin_sha256, timeout_ms }`
-- @param [in]  timeout_ms[number|nil]
-- @return [boolean] ok
-- @return [string|nil] err
M.upgrade_tls = function (tcp, hostname_or_opts, timeout_ms)
	if tcp == nil then
		return false, "closed"
	end

	local arg2 = hostname_or_opts
	if type(hostname_or_opts) == "table" and timeout_ms ~= nil and hostname_or_opts.timeout_ms == nil then
		arg2 = {}
		for k, v in pairs(hostname_or_opts) do
			arg2[k] = v
		end
		arg2.timeout_ms = timeout_ms
	end

	if type(tcp) == "userdata" and type(tcp.co_upgrade_tls) == "function" then
		local ok_c, a, b = pcall(tcp.co_upgrade_tls, tcp, arg2)
		if ok_c then
			return a, b
		end
	end

	if type(tcp.upgrade_tls) == "function" then
		return tcp:upgrade_tls(arg2, timeout_ms)
	end

	return false, "upgrade_tls unavailable"
end


-- @brief listener accept (优先 co_accept)
-- @param [in]  listener[userdata]
-- @param [in]  timeout_ms[number|nil]	仅同步路径
-- @return [userdata|nil] client
-- @return [string|nil] err
M.accept = function (listener, timeout_ms)
	if listener == nil then
		return nil, "no listener"
	end

	if type(listener.co_accept) == "function" then
		local ok_c, a, b = pcall(listener.co_accept, listener)
		if ok_c then
			return a, b
		end
	end

	if type(listener.accept) == "function" then
		return listener:accept(tonumber(timeout_ms) or 1000)
	end

	return nil, "no accept"
end


-- @brief 是否具备 co_* 绑定
-- @return [boolean]
M.co_available = function ()
	local kproxy = _kproxy()
	if kproxy == nil then
		return false
	end

	return type(kproxy.co_connect) == "function"
end


return M

-- end
