--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  自有隧道入口服务端 (N3-1 明文; N3-2 PSK; N3-3 listen)
-- @note   魔数 0xA7 分流; HELLO/CONNECT 后由 entry 中继; listen 复用 kproxy
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 明文帧握手 + CONNECT; N3-2 PSK 会话密钥; N3-3 is_magic / listen / start(tunnel-only); serve_conn_packet (mnps 整包 HELLO/CONNECT)
--]]
local klbcode = require("klbcore.klbcode")
local base = klbcode.base


local kproxy_io = require("klbcore.klbproxy.io")

local frame = require("klbcore.klbtunnel.frame")
local crypto = require("klbcore.klbtunnel.crypto")


local M = {}


M.MAGIC0 = frame.MAGIC0


-- @brief 编码并发送一帧
-- @param [in]  sock[any]		send
-- @param [in]  typ[number]
-- @param [in]  payload[string|nil]
-- @return [boolean] ok
-- @return [string|nil] err
local function _send_frame(sock, typ, payload)
	local wire, err = frame.encode(typ, payload)
	if wire == nil then
		return false, err or "encode fail"
	end
	return kproxy_io.send(sock, wire)
end


-- @brief 是否 alua-tunnel 魔数首字节
-- @param [in]  first[number|string|nil]
-- @return [boolean]
M.is_magic = function (first)
	local b0 = nil
	if type(first) == "number" then
		b0 = math.floor(first)
	elseif type(first) == "string" and #first > 0 then
		b0 = string.byte(first, 1)
	end
	return b0 == frame.MAGIC0
end


-- @brief 真 listen (kproxy.listen 包装; 供同口/专口入口)
-- @param [in]  host[string]
-- @param [in]  port[number]
-- @param [in]  backlog[number|nil]
-- @return [userdata|nil] listener
-- @return [string|nil] err
local function _listen(host, port, backlog)
	if type(host) ~= "string" or host == "" then
		return nil, "bad host"
	end

	if tonumber(port) == nil then
		return nil, "bad port"
	end

	local ok_kp, kproxy = pcall(require, "kproxy")
	if not ok_kp or type(kproxy) ~= "table" or type(kproxy.listen) ~= "function" then
		return nil, "kproxy.listen unavailable"
	end

	if backlog ~= nil then
		return kproxy.listen(host, tonumber(port), tonumber(backlog))
	end

	return kproxy.listen(host, tonumber(port))
end


-- @brief 仅 alua-tunnel 协议的入口循环 (内部复用 entry.start)
-- @param [in]  cfg[table|nil]
-- @param [in]  opts[table|nil]
-- @return [boolean] ok
-- @return [string|nil] err
local function _start(cfg, opts)
	local config = require("klbagent.packages.proxy.config")
	local entry = require("klbagent.packages.proxy.entry")

	if cfg == nil then
		cfg = config.defaults()
	end

	if type(cfg) ~= "table" then
		return false, "bad cfg"
	end

	if type(cfg.listen) ~= "table" then
		cfg.listen = {}
	end

	-- 专口: 只开 alua-tunnel; 同口分流仍走 entry + demux
	cfg.listen.protocols = { "alua-tunnel" }

	return entry.start(cfg, opts)
end


local function _auth_mode(opts)
	if type(opts) ~= "table" or type(opts.auth) ~= "table" then
		return "none"
	end

	local mode = opts.auth.mode
	if type(mode) == "string" and mode ~= "" then
		return string.lower(mode)
	end

	return "none"
end


-- @brief 常量时间比较 (避免 token 逐字节短路)
-- @param [in]  a[string]
-- @param [in]  b[string]
-- @return equal[boolean]
local function _token_eq(a, b)
	if "string" ~= type(a) or "string" ~= type(b) then
		return false
	end

	local la = #a
	local lb = #b
	local n = la
	if lb > n then
		n = lb
	end

	local diff = la ~ lb
	for i = 1, n do
		local ca = (i <= la) and string.byte(a, i) or 0
		local cb = (i <= lb) and string.byte(b, i) or 0
		diff = diff | (ca ~ cb)
	end

	return 0 == diff
end


local function _check_token(token, opts)
	local mode = _auth_mode(opts)
	if mode == "none" or mode == "" then
		return true
	end

	if type(opts) ~= "table" or type(opts.auth) ~= "table" then
		return false, "auth rejected"
	end

	local expire = tonumber(opts.auth.expire_ms)
	if nil ~= expire and 0 < expire then
		local now = math.floor(os.time() * 1000)
		if expire <= now then
			return false, "token expired"
		end
	end

	local expect = ""
	if type(opts.auth.token) == "string" then
		expect = opts.auth.token
	else
		local user = tostring(opts.auth.user or "")
		local pass = tostring(opts.auth.pass or "")
		expect = user .. ":" .. pass
	end

	if _token_eq(tostring(token or ""), expect) then
		return true
	end

	return false, "auth rejected"
end


-- @brief 解析隧道握手至目标已解析 (尚未发 CONNECT_OK)
-- @param [in]  sock[any]
-- @param [in]  opts[table|nil]	`{ auth, key, prefix }` prefix 含已读魔数首字节
-- @return [boolean] ok
-- @return [table|string] `{ host, port, session_key? }` 或 err
local function _serve_conn(sock, opts)
	if sock == nil or type(sock.send) ~= "function" or type(sock.recv) ~= "function" then
		return false, "need sock"
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	local prefix = ""
	if type(opts) == "table" and type(opts.prefix) == "string" then
		prefix = opts.prefix
	end

	-- HELLO
	local ok_h, msg = frame.decode(sock, prefix)
	if not ok_h then
		return false, msg
	end

	if msg.type ~= frame.T_HELLO then
		_send_frame(sock, frame.T_HELLO_NAK, "need hello")
		return false, "need hello"
	end

	local hello, err_p = crypto.parse_hello_payload(msg.payload)
	if hello == nil then
		_send_frame(sock, frame.T_HELLO_NAK, err_p or "bad hello")
		return false, err_p or "bad hello"
	end

	local ok_a, err_a = _check_token(hello.token, opts)
	if not ok_a then
		_send_frame(sock, frame.T_HELLO_NAK, err_a or "auth fail")
		return false, err_a or "auth fail"
	end

	local session_key = nil
	local hello_ok_payload = ""

	if hello.crypto then
		local psk = crypto.psk_from_opts(opts)
		if psk == nil then
			_send_frame(sock, frame.T_HELLO_NAK, "no psk")
			return false, "no psk"
		end

		local ok_pl, nonce_s = crypto.encode_hello_ok_crypto(nil)
		if ok_pl == nil then
			_send_frame(sock, frame.T_HELLO_NAK, "nonce fail")
			return false, "nonce fail"
		end

		hello_ok_payload = ok_pl
		session_key = crypto.derive_session_key(psk, hello.nonce_c, nonce_s)
	end

	local ok_s = _send_frame(sock, frame.T_HELLO_OK, hello_ok_payload)
	if not ok_s then
		return false, "hello ok send fail"
	end

	-- CONNECT
	ok_h, msg = frame.decode(sock, "")
	if not ok_h then
		return false, msg
	end

	if msg.type ~= frame.T_CONNECT then
		_send_frame(sock, frame.T_CONNECT_NAK, "need connect")
		return false, "need connect"
	end

	local host, port, err_c = frame.parse_connect_payload(msg.payload)
	if host == nil then
		_send_frame(sock, frame.T_CONNECT_NAK, err_c or "bad target")
		return false, err_c or "bad target"
	end

	return true, {
		host = host,
		port = port,
		kind = "alua-tunnel",
		session_key = session_key,
		crypto = session_key ~= nil,
	}
end


-- @brief 整包消息流上 HELLO/CONNECT (跳过鉴权; 供 mnps 已 TEXT alua.auth 后)
-- @param [in]  stream[table]	须有 send/recv; 每次 recv 为一帧载荷
-- @param [in]  opts[table|nil]	现行忽略; 预留
-- @return [boolean] ok
-- @return [table|string] `{ host, port, kind }` 或 err
local function _serve_conn_packet(stream, opts)
	_ = opts
	if stream == nil or type(stream.send) ~= "function" or type(stream.recv) ~= "function" then
		return false, "need stream"
	end

	local carry = ""

	local function recv_msg()
		while true do
			local ok_t, msg, rest = frame.try_decode(carry)
			if ok_t then
				carry = rest or ""
				return msg
			end

			if msg ~= "short" then
				return nil, msg
			end

			local data, err = stream:recv(65535)
			if type(data) ~= "string" or #data == 0 then
				return nil, err or "eof"
			end

			carry = carry .. data
		end
	end

	local msg, err_r = recv_msg()
	if msg == nil then
		return false, err_r
	end

	if msg.type ~= frame.T_HELLO then
		_send_frame(stream, frame.T_HELLO_NAK, "need hello")
		return false, "need hello"
	end

	if not _send_frame(stream, frame.T_HELLO_OK, "") then
		return false, "hello ok send fail"
	end

	msg, err_r = recv_msg()
	if msg == nil then
		return false, err_r
	end

	if msg.type ~= frame.T_CONNECT then
		_send_frame(stream, frame.T_CONNECT_NAK, "need connect")
		return false, "need connect"
	end

	local host, port, err_c = frame.parse_connect_payload(msg.payload)
	if host == nil then
		_send_frame(stream, frame.T_CONNECT_NAK, err_c or "bad target")
		return false, err_c or "bad target"
	end

	return true, { host = host, port = port, kind = "alua-tunnel" }
end


-- @brief 发送 CONNECT 应答
-- @param [in]  sock[any]
-- @param [in]  ok[boolean]
-- @param [in]  err[string|nil]
-- @return [boolean] ok
-- @return [string|nil] err
local function _reply(sock, ok, err)
	if sock == nil or type(sock.send) ~= "function" then
		return false, "need sock"
	end

	local msg
	if ok then
		msg = frame.encode(frame.T_CONNECT_OK, "")
	else
		msg = frame.encode(frame.T_CONNECT_NAK, tostring(err or "fail"))
	end

	if msg == nil then
		return false, "encode fail"
	end

	local ok_s = kproxy_io.send(sock, msg)
	if not ok_s then
		return false, "reply send fail"
	end

	return true
end


-- @brief 真 listen (公开形态; 失败 nil, code, err)
M.listen = function (host, port, backlog)
	local listener, err = _listen(host, port, backlog)
	if listener ~= nil then
		return listener
	end

	return nil, ((("number" == type(err)) and err) or base.IO), tostring(err or "")
end


-- @brief 仅 alua-tunnel 协议的入口循环 (公开形态; 失败 false, code, err)
M.start = function (cfg, opts)
	local ok, err = _start(cfg, opts)
	if ok then
		return true, err
	end

	return false, ((("number" == type(err)) and err) or base.IO), tostring(err or "")
end


-- @brief 解析隧道握手至目标已解析 (公开形态; 失败 false, code, err)
M.serve_conn = function (sock, opts)
	local ok, req = _serve_conn(sock, opts)
	if ok then
		return true, req
	end

	return false, ((("number" == type(req)) and req) or base.IO), tostring(req)
end


-- @brief 整包消息流上 HELLO/CONNECT (公开形态; 失败 false, code, err)
M.serve_conn_packet = function (stream, opts)
	local ok, req = _serve_conn_packet(stream, opts)
	if ok then
		return true, req
	end

	return false, ((("number" == type(req)) and req) or base.IO), tostring(req)
end


-- @brief 发送 CONNECT 应答 (公开形态; 失败 false, code, err)
M.reply = function (sock, ok, err)
	local rok, err_r = _reply(sock, ok, err)
	if rok then
		return true
	end

	return false, ((("number" == type(err_r)) and err_r) or base.IO), tostring(err_r)
end


return M

-- end
