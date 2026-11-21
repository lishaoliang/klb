--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  代理 hop WebSocket 出站 (ws/wss + alua-tunnel; 帧在 C)
-- @note   经 klbcore.klbws.connect; 默认 path=/alua-tunnel; 握手/opcode/mask 仅 C
-- @history 修改历史
--  \n [2026-10] 创建文件 (N5 P1 / M1); 帧下沉 C (klbws/kws); 废弃 Lua ws.frame wire; HELLO/CONNECT 经 klbtunnel.hello_connect
--]]


local tunnel_client = require("klbcore.klbtunnel.client")
local crypto = require("klbcore.klbtunnel.crypto")


local M = {}


M.DEFAULT_PATH = "/alua-tunnel"


local function _auth_token(opts)
	if type(opts) ~= "table" then
		return ""
	end
	if type(opts.token) == "string" and opts.token ~= "" then
		return opts.token
	end
	if type(opts.auth) == "table" then
		if type(opts.auth.token) == "string" then
			return opts.auth.token
		end
		local user = tostring(opts.auth.user or "")
		local pass = tostring(opts.auth.pass or "")
		if user ~= "" or pass ~= "" then
			return user .. ":" .. pass
		end
	end
	return ""
end


local function _disconnect_klb(client)
	if client ~= nil and type(client.disconnect) == "function" then
		pcall(function ()
			client:disconnect()
		end)
	end
end


local function _wait_open(client, rounds)
	local n = tonumber(rounds) or 16
	for _ = 1, n do
		local msg, head, body = client:co_recv()
		if msg == "open" then
			return true
		end
		if msg == "error" or msg == "exit" or msg == "close" then
			return false, "ws early " .. tostring(msg)
		end
		_ = head
		_ = body
	end
	return false, "ws open timeout"
end


local function _session_stream(client, label)
	local data_leftover = ""
	local stream = {
		kind = label,
		_ws = client,
	}

	stream.idle_touch_ms = function ()
		local c = stream._ws
		if c ~= nil and type(c.last_recv_ms) == "function" then
			local ok, v = pcall(function ()
				return c:last_recv_ms()
			end)
			if ok and type(v) == "number" and v > 0 then
				return v
			end
		end
		return stream._idle_touch_ms
	end

	stream.close = function ()
		_disconnect_klb(client)
		stream._ws = nil
	end

	stream.send = function (_, data)
		if stream._ws == nil then
			return false, "closed"
		end

		local rc = stream._ws:send_binary(data or "")
		return 0 == rc
	end

	stream.recv = function (_, max_n)
		if stream._ws == nil then
			return nil, "closed"
		end

		max_n = tonumber(max_n) or 4096
		if #data_leftover > 0 then
			local out = data_leftover
			if max_n > 0 and #out > max_n then
				data_leftover = string.sub(out, max_n + 1)
				return string.sub(out, 1, max_n)
			end

			data_leftover = ""
			return out
		end

		for _ = 1, 32 do
			local msg, head, body = stream._ws:co_recv()
			if msg == "binary" or msg == "text" then
				body = body or ""
				if max_n > 0 and #body > max_n then
					data_leftover = string.sub(body, max_n + 1)
					body = string.sub(body, 1, max_n)
				end

				return body
			elseif msg == "pong" or msg == "open" then
				-- C 吞心跳
			elseif msg == "close" or msg == "error" or msg == "exit" then
				return nil, tostring(msg)
			end
			_ = head
		end

		return nil, "timeout"
	end

	return stream
end


local function _wrap_conn(stream, proxy_host, proxy_port, dest_host, dest_port, kind)
	local conn = {
		kind = kind or "ws-tunnel",
		proxy_host = proxy_host,
		proxy_port = tonumber(proxy_port),
		dest_host = dest_host,
		dest_port = tonumber(dest_port),
		tcp = nil,
	}

	conn.idle_touch_ms = function ()
		if type(stream) == "table" and type(stream.idle_touch_ms) == "function" then
			local ok, v = pcall(stream.idle_touch_ms, stream)
			if ok and type(v) == "number" then
				return v
			end
		end
		return conn._idle_touch_ms
	end

	conn.close = function ()
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		end
	end

	conn.send = function (_, data)
		if stream == nil then
			return false, "closed"
		end
		return stream:send(data)
	end

	conn.recv = function (_, max_n, timeout_ms)
		if stream == nil then
			return nil, "closed"
		end
		_ = timeout_ms
		return stream:recv(max_n or 4096)
	end

	return conn
end


local function _run_in_co(timeout_ms, fn)
	local co_util = require("klbagent.core.co_util")
	return co_util.co_run(timeout_ms, fn, "ws hop")
end


local function _klb_open(host, port, path, use_tls, token, ssid)
	local ok_w, klbws = pcall(require, "klbcore.klbws")
	if not ok_w or type(klbws) ~= "table" or type(klbws.connect) ~= "function" then
		return nil, "klbws.connect unavailable"
	end

	local opts = {
		tls = use_tls == true,
	}
	local headers = {}
	if type(token) == "string" and token ~= "" then
		headers["X-Alua-Token"] = token
	end

	-- 证明票须同送 alua_ssid (2026-11 ssid 契约)
	if type(ssid) == "string" and ssid ~= "" then
		headers["X-Alua-Ssid"] = ssid
	end

	if next(headers) ~= nil then
		opts.headers = headers
	end

	local client = klbws.connect(host, tonumber(port), path, opts)
	if client == nil then
		return nil, "ws connect fail"
	end

	local ok_o, err_o = _wait_open(client, 16)
	if not ok_o then
		_disconnect_klb(client)
		return nil, err_o
	end

	return client, nil
end


-- @brief 仅 WebSocket 握手探活 (不发隧道帧)
M.handshake = function (proxy_host, proxy_port, opts)
	if type(proxy_host) ~= "string" or proxy_host == "" then
		return false, "bad proxy_host"
	end
	if tonumber(proxy_port) == nil then
		return false, "bad proxy_port"
	end
	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	local timeout_ms = 5000
	local use_tls = false
	local path = M.DEFAULT_PATH
	local token = ""
	local ssid = ""
	if type(opts) == "table" then
		if opts.timeout_ms ~= nil then
			timeout_ms = tonumber(opts.timeout_ms) or timeout_ms
		end
		use_tls = (opts.tls == true) or (opts.wss == true)
		if type(opts.path) == "string" and opts.path ~= "" then
			path = opts.path
		end
		token = _auth_token(opts)
		if type(opts.alua_ssid) == "string" then
			ssid = opts.alua_ssid
		end
	end

	return _run_in_co(timeout_ms, function ()
		local client, err_c = _klb_open(proxy_host, proxy_port, path, use_tls, token, ssid)
		if client == nil then
			return false, err_c
		end
		_disconnect_klb(client)
		return true
	end)
end


-- @brief 经 ws/wss hop 打开到目标的隧道 (须在 kco 协程内)
M.connect = function (proxy_host, proxy_port, dest_host, dest_port, opts)
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

	if type(opts) == "table" and opts.tcp ~= nil then
		return false, "ws/wss hop cannot reuse prior tcp"
	end

	local use_tls = false
	local path = M.DEFAULT_PATH
	local token = ""
	local ssid = ""
	if type(opts) == "table" then
		use_tls = (opts.tls == true) or (opts.wss == true)
		if type(opts.path) == "string" and opts.path ~= "" then
			path = opts.path
		end
		token = _auth_token(opts)
		if type(opts.alua_ssid) == "string" then
			ssid = opts.alua_ssid
		end
	end

	local client, err_c = _klb_open(proxy_host, proxy_port, path, use_tls, token, ssid)
	if client == nil then
		return false, err_c
	end

	local stream = _session_stream(client, use_tls and "wss-tunnel" or "ws-tunnel")

	local hc_opts = opts
	if type(opts) ~= "table" then
		hc_opts = { token = token }
	else
		hc_opts = {}
		for k, v in pairs(opts) do
			hc_opts[k] = v
		end
		hc_opts.token = token
	end

	local ok_h, code_h, info = tunnel_client.hello_connect(
		tunnel_client.io_from_stream(stream),
		dest_host, tonumber(dest_port), hc_opts)
	if not ok_h then
		stream.close()
		return false, tostring(info or code_h)
	end

	local session_key = (type(info) == "table") and info.session_key or nil
	local kind = use_tls and "wss-tunnel" or "ws-tunnel"
	if session_key ~= nil then
		local cconn = crypto.wrap_conn(stream, session_key)
		cconn.proxy_host = proxy_host
		cconn.proxy_port = tonumber(proxy_port)
		cconn.dest_host = dest_host
		cconn.dest_port = tonumber(dest_port)
		cconn.crypto = true
		cconn.kind = kind
		return true, cconn
	end

	return true, _wrap_conn(stream, proxy_host, proxy_port, dest_host, dest_port, kind)
end


return M

-- end
