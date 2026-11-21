--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  自有隧道出站客户端 (N3-1 明文; N3-2 PSK 密封)
-- @note   hop.type=alua-tunnel; 有 PSK 时 HELLO 交换 nonce 后 DATA 帧
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 明文帧握手 + CONNECT; N3-2 PSK 会话 + wrap_conn; hello_connect / io_* 供 smp/ws 复用
--]]


local frame = require("klbcore.klbtunnel.frame")
local crypto = require("klbcore.klbtunnel.crypto")
local kproxy_io = require("klbcore.klbproxy.io")
local errcode = require("klbcore.klbtunnel.errcode")


local M = {}


local function _close_tcp(tcp)
	if tcp ~= nil and type(tcp.close) == "function" then
		pcall(function ()
			tcp:close()
		end)
	end
end


local function _wrap_conn(tcp, proxy_host, proxy_port, dest_host, dest_port)
	local conn = {
		kind = "alua-tunnel",
		proxy_host = proxy_host,
		proxy_port = tonumber(proxy_port),
		dest_host = dest_host,
		dest_port = tonumber(dest_port),
		tcp = tcp,
	}

	conn.close = function ()
		_close_tcp(conn.tcp)
		conn.tcp = nil
	end

	conn.send = function (_, data)
		if conn.tcp == nil then
			return false, "closed"
		end
		return kproxy_io.send(conn.tcp, data)
	end

	conn.recv = function (_, max_n)
		if conn.tcp == nil then
			return nil, "closed"
		end
		return kproxy_io.recv(conn.tcp, max_n or 4096)
	end

	return conn
end


local function _auth_token(opts)
	if type(opts) ~= "table" then
		return ""
	end

	if type(opts.token) == "string" and opts.token ~= "" then
		return opts.token
	end

	if type(opts.auth) ~= "table" then
		return ""
	end

	if type(opts.auth.token) == "string" then
		return opts.auth.token
	end

	local user = tostring(opts.auth.user or "")
	local pass = tostring(opts.auth.pass or "")
	if user == "" and pass == "" then
		return ""
	end

	return user .. ":" .. pass
end


-- @brief TCP 字节流 → hello_connect IO
-- @param [in]  tcp[any]	kproxy.tcp
-- @return [table] io
M.io_from_tcp = function (tcp)
	return {
		send = function (wire)
			return kproxy_io.send(tcp, wire)
		end,
		recv_msg = function ()
			return frame.decode(tcp, "")
		end,
		close = function ()
			_close_tcp(tcp)
		end,
	}
end


-- @brief 已有 send/recv 字节流 → hello_connect IO (整包一帧)
-- @param [in]  stream[table]	`send(data)` / `recv(max_n)`
-- @return [table] io
-- @note 每连接保留 carry, 多帧 / 粘包时逐帧消费
M.io_from_stream = function (stream)
	local carry = ""
	return {
		send = function (wire)
			local ok = stream:send(wire)
			return ok == true or ok == 0
		end,
		recv_msg = function ()
			while true do
				local ok_t, msg, rest = frame.try_decode(carry)
				if ok_t then
					carry = rest or ""
					return true, msg
				end

				if msg ~= "short" then
					return false, tostring(msg or "frame fail")
				end

				local data, err = stream:recv(65535)
				if type(data) ~= "string" or #data == 0 then
					return false, err or "eof"
				end

				carry = carry .. data
			end
		end,
		close = function ()
			if "function" == type(stream.close) then
				pcall(function ()
					stream:close()
				end)
			end
		end,
	}
end


-- @brief klbmnp/klbsmp 会话 → hello_connect IO (BINARY head=alua-tunnel)
-- @param [in]  client[table]	send_binary / co_recv
-- @param [in]  head[string|nil]	默认 `alua-tunnel`
-- @return [table] io
M.io_from_klb_binary = function (client, head)
	local h = head
	if type(h) ~= "string" or h == "" then
		h = "alua-tunnel"
	end

	local carry = ""

	return {
		send = function (wire)
			local rc = client:send_binary(h, wire or "")
			return 0 == rc
		end,
		recv_msg = function ()
			for _ = 1, 16 do
				local ok_t, msg, rest = frame.try_decode(carry)
				if ok_t then
					carry = rest or ""
					return true, msg
				end

				if msg ~= "short" then
					return false, tostring(msg or "frame fail")
				end

				local msg_t, rh, body = client:co_recv()
				if msg_t == "binary" then
					if rh == nil or rh == "" or rh == h then
						carry = carry .. (body or "")
					end
				elseif msg_t == "pong" or msg_t == "media" then
					if msg_t == "media" and type(client.free_media) == "function" then
						pcall(function ()
							client:free_media(rh)
						end)
					end
				elseif msg_t == "error" or msg_t == "exit" or msg_t == "close" then
					return false, "tunnel early " .. tostring(msg_t)
				end
			end
			return false, "tunnel recv timeout"
		end,
		close = function ()
			if "function" == type(client.disconnect) then
				pcall(function ()
					client:disconnect()
				end)
			end
		end,
	}
end


-- @brief 在已建通道上 HELLO/CONNECT (帧 API 唯一入口)
-- @param [in]  io[table]		`{ send(wire)->ok, recv_msg()->ok,msg|err }`
-- @param [in]  dest_host[string]
-- @param [in]  dest_port[number]
-- @param [in]  opts[table|nil]	`{ token, auth, key, skip_crypto, timeout_ms }`
-- @return [boolean] ok
-- @return [table|string] `{ session_key }` 或 err
-- @note skip_crypto=true 时不做隧道 PSK (mnps 等已有层加密)
local function _hello_connect(io, dest_host, dest_port, opts)
	if type(io) ~= "table" or type(io.send) ~= "function" or type(io.recv_msg) ~= "function" then
		return false, "need io"
	end

	if type(dest_host) ~= "string" or dest_host == "" then
		return false, "bad dest_host"
	end

	if tonumber(dest_port) == nil then
		return false, "bad dest_port"
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	local token = _auth_token(opts)
	local skip_crypto = (type(opts) == "table" and opts.skip_crypto == true)
	local psk = nil
	if not skip_crypto then
		psk = crypto.psk_from_opts(opts)
	end

	local nonce_c = nil
	local session_key = nil
	local hello_wire
	if psk ~= nil then
		local hp, nc_or_err = crypto.encode_hello_crypto(token, nil)
		if hp == nil then
			return false, tostring(nc_or_err or "hello crypto encode fail")
		end
		nonce_c = nc_or_err
		hello_wire = frame.encode(frame.T_HELLO, hp)
	else
		hello_wire = frame.encode_hello(token)
	end

	if hello_wire == nil then
		return false, "hello encode fail"
	end

	local ok_s = io.send(hello_wire)
	if not ok_s then
		return false, "hello send fail"
	end

	local ok_h, msg = io.recv_msg()
	if not ok_h then
		return false, tostring(msg)
	end

	if msg.type == frame.T_HELLO_NAK then
		return false, "hello nak: " .. tostring(msg.payload)
	end

	if msg.type ~= frame.T_HELLO_OK then
		return false, "unexpected hello rsp"
	end

	if psk ~= nil then
		local ok_info = crypto.parse_hello_ok_payload(msg.payload)
		if not ok_info.crypto or type(ok_info.nonce_s) ~= "string" then
			return false, "hello ok need crypto nonce"
		end
		session_key = crypto.derive_session_key(psk, nonce_c, ok_info.nonce_s)
	end

	local connect_wire = frame.encode_connect(dest_host, tonumber(dest_port))
	if connect_wire == nil then
		return false, "connect encode fail"
	end

	ok_s = io.send(connect_wire)
	if not ok_s then
		return false, "connect send fail"
	end

	ok_h, msg = io.recv_msg()
	if not ok_h then
		return false, tostring(msg)
	end

	if msg.type == frame.T_CONNECT_NAK then
		return false, "connect nak: " .. tostring(msg.payload)
	end

	if msg.type ~= frame.T_CONNECT_OK then
		return false, "unexpected connect rsp"
	end

	return true, { session_key = session_key }
end


-- @brief 为握手挂超时; 到期关闭 io 以唤醒阻塞读
-- @param [in]  io[table]		hello_connect IO
-- @param [in]  ms[number|nil]	超时毫秒; 省略/非正不挂
-- @return [table|nil]			`{ done, expired }` 状态; 未挂为 nil
local function _arm_timeout(io, ms)
	if "number" ~= type(ms) or 0 >= ms then
		return nil
	end

	local ok_k, kco = pcall(require, "kco")
	if not ok_k or "table" ~= type(kco) or "function" ~= type(kco.timeout) then
		return nil
	end

	local state = { done = false, expired = false }
	kco.timeout(ms, function ()
		if state.done then
			return
		end

		state.expired = true
		if "table" == type(io) and "function" == type(io.close) then
			pcall(io.close)
		end
	end)

	return state
end


-- @brief 带超时的 HELLO/CONNECT
-- @return [boolean] ok
-- @return [table|string] 成功 `{ session_key }`; 失败 err
local function _hello_connect_timed(io, dest_host, dest_port, opts)
	local ms = nil
	if "table" == type(opts) then
		ms = tonumber(opts.timeout_ms)
	end

	local state = _arm_timeout(io, ms)
	local ok, info = _hello_connect(io, dest_host, dest_port, opts)

	if nil ~= state then
		state.done = true
		if state.expired and not ok then
			return false, "timeout"
		end
	end

	return ok, info
end


-- @brief 经 alua-tunnel 代理连接到目标
-- @param [in]  proxy_host[string]
-- @param [in]  proxy_port[number]
-- @param [in]  dest_host[string]
-- @param [in]  dest_port[number]
-- @param [in]  opts[table|nil]	`{ timeout_ms, tcp, key, auth={token|user,pass|key} }`
-- @return [boolean] ok
-- @return [table|string] conn 或 err
local function _connect(proxy_host, proxy_port, dest_host, dest_port, opts)
	if type(proxy_host) ~= "string" or proxy_host == "" then
		return false, "bad proxy_host"
	end

	if tonumber(proxy_port) == nil then
		return false, "bad proxy_port"
	end

	if type(dest_host) ~= "string" or dest_host == "" then
		return false, "bad dest_host"
	end

	if tonumber(dest_port) == nil then
		return false, "bad dest_port"
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	local timeout_ms = 15000
	local tcp = nil
	if type(opts) == "table" then
		if opts.timeout_ms ~= nil then
			timeout_ms = tonumber(opts.timeout_ms) or timeout_ms
		end
		if opts.tcp ~= nil then
			tcp = opts.tcp
		end
	end

	if tcp == nil then
		local err_c
		tcp, err_c = kproxy_io.tcp_connect(proxy_host, tonumber(proxy_port), timeout_ms, opts)
		if tcp == nil then
			return false, err_c or "connect fail"
		end
	end

	local function fail(msg)
		_close_tcp(tcp)
		return false, msg
	end

	local ok_h, info = _hello_connect_timed(M.io_from_tcp(tcp), dest_host, dest_port, opts)
	if not ok_h then
		return fail(tostring(info))
	end

	local session_key = (type(info) == "table") and info.session_key or nil
	if session_key ~= nil then
		local cconn = crypto.wrap_conn(tcp, session_key)
		cconn.proxy_host = proxy_host
		cconn.proxy_port = tonumber(proxy_port)
		cconn.dest_host = dest_host
		cconn.dest_port = tonumber(dest_port)
		cconn.crypto = true
		return true, cconn
	end

	return true, _wrap_conn(tcp, proxy_host, proxy_port, dest_host, dest_port)
end


-- @brief 在已建通道上 HELLO/CONNECT (公开形态)
-- @return ok[boolean]
-- @return [table|number] 成功为 `{ session_key }`; 失败为错误码
-- @return [string] 失败描述
M.hello_connect = function (io, dest_host, dest_port, opts)
	local ok, info = _hello_connect_timed(io, dest_host, dest_port, opts)
	if ok then
		return true, info
	end

	return false, errcode.of(info), tostring(info)
end


-- @brief 经 alua-tunnel 代理连接到目标 (公开形态)
-- @return ok[boolean]
-- @return [table|number] 成功为 conn; 失败为错误码
-- @return [string] 失败描述
M.connect = function (proxy_host, proxy_port, dest_host, dest_port, opts)
	local ok, conn = _connect(proxy_host, proxy_port, dest_host, dest_port, opts)
	if ok then
		return true, conn
	end

	return false, errcode.of(conn), tostring(conn)
end


return M

-- end

