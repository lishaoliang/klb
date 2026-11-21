--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  SOCKS5 入口服务端握手 (RFC1928/1929; wire 优先 ksocks5)
-- @note   serve_conn 解析 CONNECT 目标; reply 发送应答; 中继由 entry/pump.relay
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); serve_conn 支持 opts.prefix (与 HTTP 同口分流); N1-1 mode=none 仅接受无鉴权方法; kproxy_io co_*; wire 下沉 ksocks5
--]]


local kproxy_io = require("klbcore.klbproxy.io")
local klbcode = require("klbcore.klbcode")
local base = klbcode.base


local M = {}


local function _load_wire()
	local ok, mod = pcall(require, "ksocks5")
	if ok and type(mod) == "table" and type(mod.encode_reply) == "function" then
		return mod
	end
	return nil
end


local W = _load_wire()


local function _byte(n)
	return string.char(n % 256)
end


local function _make_reader(sock, prefix)
	local buf = prefix or ""

	return function (n)
		local parts = {}
		local got = 0

		while got < n do
			if #buf > 0 then
				local take = math.min(n - got, #buf)
				parts[#parts + 1] = string.sub(buf, 1, take)
				buf = string.sub(buf, take + 1)
				got = got + take
			else
				local chunk, err = kproxy_io.recv(sock, n - got)
				if type(chunk) ~= "string" or #chunk == 0 then
					return nil, err or "eof"
				end

				parts[#parts + 1] = chunk
				got = got + #chunk
			end
		end

		return table.concat(parts)
	end
end


local function _recv_exact(sock, n)
	local parts = {}
	local got = 0

	while got < n do
		local chunk, err = kproxy_io.recv(sock, n - got)
		if type(chunk) ~= "string" or #chunk == 0 then
			return nil, err or "eof"
		end

		parts[#parts + 1] = chunk
		got = got + #chunk
	end

	return table.concat(parts)
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


local function _do_userpass(sock, opts, recv_exact)
	recv_exact = recv_exact or function (n)
		return _recv_exact(sock, n)
	end

	local hdr, err = recv_exact(2)
	if hdr == nil then
		return false, err or "auth hdr fail"
	end

	if string.byte(hdr, 1) ~= 0x01 then
		return false, "bad auth ver"
	end

	local ulen = string.byte(hdr, 2)
	local user, err_u = recv_exact(ulen)
	if user == nil then
		return false, err_u or "auth user fail"
	end

	local plen_b, err_p = recv_exact(1)
	if plen_b == nil then
		return false, err_p or "auth plen fail"
	end

	local plen = string.byte(plen_b, 1)
	local pass, err_w = recv_exact(plen)
	if pass == nil then
		return false, err_w or "auth pass fail"
	end

	if W ~= nil then
		local parsed = W.parse_userpass(hdr .. user .. plen_b .. pass)
		if parsed == nil then
			return false, "bad auth"
		end
		user = parsed.user
		pass = parsed.pass
	end

	local auth_ok = false
	if type(opts) == "table" and type(opts.auth) == "table" then
		local ok_a, acl = pcall(require, "klbagent.packages.proxy.acl")
		if ok_a and type(acl) == "table" and type(acl.check_credentials) == "function" then
			auth_ok = acl.check_credentials(opts.auth, user, pass, nil)
		else
			local expect_user = tostring(opts.auth.user or "")
			local expect_pass = tostring(opts.auth.pass or "")
			auth_ok = (user == expect_user and pass == expect_pass)
		end
	end

	if auth_ok then
		local ok_msg = nil
		if W ~= nil then
			ok_msg = W.encode_userpass_status(0x00)
		else
			ok_msg = _byte(0x01) .. _byte(0x00)
		end

		local ok_s = kproxy_io.send(sock, ok_msg)
		if not ok_s then
			return false, "auth ok send fail"
		end
		return true
	end

	local nak = nil
	if W ~= nil then
		nak = W.encode_userpass_status(0x01)
	else
		nak = _byte(0x01) .. _byte(0x01)
	end
	kproxy_io.send(sock, nak)
	return false, "auth rejected"
end


local function _parse_addr(sock, atyp, recv_exact)
	recv_exact = recv_exact or function (n)
		return _recv_exact(sock, n)
	end

	if atyp == 0x01 then
		local raw, err = recv_exact(4)
		if raw == nil then
			return nil, nil, err
		end

		local a, b, c, d = string.byte(raw, 1, 4)
		return string.format("%d.%d.%d.%d", a, b, c, d), nil, nil
	end

	if atyp == 0x03 then
		local len_b, err_l = recv_exact(1)
		if len_b == nil then
			return nil, nil, err_l
		end

		local n = string.byte(len_b, 1)
		local host, err_h = recv_exact(n)
		if host == nil then
			return nil, nil, err_h
		end

		return host, nil, nil
	end

	if atyp == 0x04 then
		local raw, err = recv_exact(16)
		if raw == nil then
			return nil, nil, err
		end

		local parts = {}
		for i = 1, 16, 2 do
			parts[#parts + 1] = string.format("%02x%02x", string.byte(raw, i), string.byte(raw, i + 1))
		end
		return table.concat(parts, ":"), nil, nil
	end

	return nil, nil, "bad atyp"
end


-- @brief 处理 SOCKS5 握手至 CONNECT 目标已解析 (尚未发 CONNECT 应答)
-- @param [in]  sock[any]		须有 send/recv
-- @param [in]  opts[table|nil]	`{ auth = { mode, user, pass }, prefix[string] }`
-- @return [boolean] ok
-- @return [table|string] `{ host, port }` 或 err
local function _serve_conn_impl(sock, opts)
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

	local recv_exact = _make_reader(sock, prefix)

	local hdr, err = recv_exact(2)
	if hdr == nil then
		return false, err or "greeting fail"
	end

	if string.byte(hdr, 1) ~= 0x05 then
		return false, "bad socks ver"
	end

	local nmethods = string.byte(hdr, 2)
	local methods, err_m = recv_exact(nmethods)
	if methods == nil then
		return false, err_m or "methods fail"
	end

	local mode = _auth_mode(opts)
	local want_user = (mode == "userpass" or mode == "user" or mode == "password" or mode == "basic")
	local chosen = 0xFF

	if W ~= nil then
		chosen = W.choose_method(methods, want_user)
	else
		local function has_method(code)
			for i = 1, #methods do
				if string.byte(methods, i) == code then
					return true
				end
			end
			return false
		end

		if want_user then
			if has_method(0x02) then
				chosen = 0x02
			end
		else
			if has_method(0x00) then
				chosen = 0x00
			end
		end
	end

	local select_msg = nil
	if W ~= nil then
		select_msg = W.encode_method_select(chosen)
	else
		select_msg = _byte(0x05) .. _byte(chosen)
	end

	if chosen == 0xFF then
		kproxy_io.send(sock, select_msg)
		return false, "no acceptable method"
	end

	local ok_s = kproxy_io.send(sock, select_msg)
	if not ok_s then
		return false, "method send fail"
	end

	if chosen == 0x02 then
		local ok_a, err_a = _do_userpass(sock, opts, recv_exact)
		if not ok_a then
			return false, err_a or "auth fail"
		end
	end

	local req, err_r = recv_exact(4)
	if req == nil then
		return false, err_r or "connect hdr fail"
	end

	if string.byte(req, 1) ~= 0x05 then
		return false, "bad connect ver"
	end

	local cmd = string.byte(req, 2)
	local atyp = string.byte(req, 4)

	if W ~= nil then
		local rest = nil
		if atyp == 0x01 then
			rest = recv_exact(4 + 2)
		elseif atyp == 0x03 then
			local len_b = recv_exact(1)
			if len_b == nil then
				return false, "bad addr"
			end
			local n = string.byte(len_b, 1)
			local body = recv_exact(n + 2)
			if body == nil then
				return false, "bad addr"
			end
			rest = len_b .. body
		elseif atyp == 0x04 then
			rest = recv_exact(16 + 2)
		else
			M.reply(sock, 0x08)
			return false, "bad atyp"
		end

		if rest == nil then
			return false, "bad addr"
		end

		local parsed = W.parse_request(req .. rest)
		if parsed == nil then
			M.reply(sock, 0x08)
			return false, "bad addr"
		end

		if parsed.cmd ~= 0x01 then
			M.reply(sock, 0x07)
			return false, "cmd not connect"
		end

		return true, { host = parsed.host, port = parsed.port, atyp = parsed.atyp }
	end

	if cmd ~= 0x01 then
		M.reply(sock, 0x07)
		return false, "cmd not connect"
	end

	local host, _, err_h = _parse_addr(sock, atyp, recv_exact)
	if host == nil then
		M.reply(sock, 0x08)
		return false, err_h or "bad addr"
	end

	local port_b, err_p = recv_exact(2)
	if port_b == nil then
		return false, err_p or "port fail"
	end

	local port = string.byte(port_b, 1) * 256 + string.byte(port_b, 2)
	return true, { host = host, port = port, atyp = atyp }
end


-- @brief 入站握手 (失败形状收口)
-- @return [boolean|nil] ok
-- @return [table|number] `{ host, port }` 或错误码
-- @return [string|nil] errstr
M.serve_conn = function (sock, opts)
	local ok, req = _serve_conn_impl(sock, opts)
	if ok then
		return true, req
	end

	return nil, ((("number" == type(req)) and req) or base.IO), req
end


-- @brief 编码 BND.ADDR/PORT (IPv4 或域名; 失败回 0.0.0.0:0)
-- @param [in]  bnd[table|nil]	`{ host[string], port[number] }`
-- @return [string] atyp+addr+port
local function _encode_bnd(bnd)
	local host = "0.0.0.0"
	local port = 0
	if type(bnd) == "table" then
		if type(bnd.host) == "string" and bnd.host ~= "" then
			host = bnd.host
		end
		port = tonumber(bnd.port) or 0
	end

	local a, b, c, d = string.match(host, "^(%d+)%.(%d+)%.(%d+)%.(%d+)$")
	if a ~= nil then
		local na = tonumber(a)
		local nb = tonumber(b)
		local nc = tonumber(c)
		local nd = tonumber(d)
		if na ~= nil and nb ~= nil and nc ~= nil and nd ~= nil
			and na <= 255 and nb <= 255 and nc <= 255 and nd <= 255 then
			return _byte(0x01)
				.. _byte(na) .. _byte(nb) .. _byte(nc) .. _byte(nd)
				.. _byte(math.floor(port / 256) % 256) .. _byte(port % 256)
		end
	end

	if #host > 0 and #host <= 255 then
		return _byte(0x03) .. _byte(#host) .. host
			.. _byte(math.floor(port / 256) % 256) .. _byte(port % 256)
	end

	return _byte(0x01)
		.. _byte(0) .. _byte(0) .. _byte(0) .. _byte(0)
		.. _byte(0) .. _byte(0)
end


-- @brief 发送 CONNECT 应答
-- @param [in]  sock[any]
-- @param [in]  rep[number]	0 成功; 其它为 RFC1928 REP
-- @param [in]  bnd[table|nil]	真实绑定 `{ host, port }`; 缺省 0.0.0.0:0
-- @return [boolean] ok
-- @return [string|nil] err
local function _reply_impl(sock, rep, bnd)
	if sock == nil or type(sock.send) ~= "function" then
		return false, "need sock"
	end

	rep = tonumber(rep) or 0x01
	local msg = nil
	if W ~= nil and type(W.encode_reply_bnd) == "function" then
		msg = W.encode_reply_bnd(rep, bnd)
	elseif W ~= nil and bnd == nil and type(W.encode_reply) == "function" then
		msg = W.encode_reply(rep)
	else
		msg = _byte(0x05) .. _byte(rep) .. _byte(0x00) .. _encode_bnd(bnd)
	end

	if type(msg) ~= "string" then
		return false, "reply encode fail"
	end

	local ok = kproxy_io.send(sock, msg)
	if not ok then
		return false, "reply send fail"
	end

	return true
end


-- @brief 发送 CONNECT 应答 (失败形状收口)
-- @return [boolean|nil] ok
-- @return [number|nil] code
-- @return [string|nil] errstr
M.reply = function (sock, rep, bnd)
	local ok, err = _reply_impl(sock, rep, bnd)
	if ok then
		return true
	end

	return nil, ((("number" == type(err)) and err) or base.IO), err
end


return M

-- end
