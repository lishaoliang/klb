--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  SOCKS5 出站客户端 (RFC1928/1929; wire 优先 ksocks5)
-- @note   主路径 CONNECT; 须在有 kproxy 的宿主内调用
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); kproxy_io co_*; wire 下沉 ksocks5; probe 探活 API
--]]


local kproxy_io = require("klbcore.klbproxy.io")
local errcode = require("klbcore.klbsocks5.errcode")


local M = {}


local function _load_wire()
	local ok, mod = pcall(require, "ksocks5")
	if ok and type(mod) == "table" and type(mod.encode_greeting) == "function" then
		return mod
	end
	return nil
end


local W = _load_wire()


local function _byte(n)
	return string.char(n % 256)
end


local function _u16be(n)
	n = tonumber(n) or 0
	if n < 0 then
		n = 0
	end
	if n > 65535 then
		n = 65535
	end
	return string.char(math.floor(n / 256) % 256, n % 256)
end


local function _is_ipv4(host)
	local a, b, c, d = string.match(host, "^(%d+)%.(%d+)%.(%d+)%.(%d+)$")
	if a == nil then
		return false
	end

	a = tonumber(a)
	b = tonumber(b)
	c = tonumber(c)
	d = tonumber(d)
	if a == nil or b == nil or c == nil or d == nil then
		return false
	end

	if a > 255 or b > 255 or c > 255 or d > 255 then
		return false
	end

	return true, a, b, c, d
end


local function _recv_exact(tcp, n)
	local parts = {}
	local got = 0

	while got < n do
		local chunk, err = kproxy_io.recv(tcp, n - got)
		if type(chunk) ~= "string" or #chunk == 0 then
			return nil, err or "eof"
		end

		parts[#parts + 1] = chunk
		got = got + #chunk
	end

	return table.concat(parts)
end


local function _close_tcp(tcp)
	if tcp ~= nil and type(tcp.close) == "function" then
		pcall(function ()
			tcp:close()
		end)
	end
end


local function _build_dest_addr(dest_host, dest_port)
	local ok_ip, a, b, c, d = _is_ipv4(dest_host)
	if ok_ip then
		return _byte(0x01) .. _byte(a) .. _byte(b) .. _byte(c) .. _byte(d) .. _u16be(dest_port)
	end

	if #dest_host > 255 then
		return nil, "dest_host too long"
	end

	return _byte(0x03) .. _byte(#dest_host) .. dest_host .. _u16be(dest_port)
end


local function _auth_userpass(tcp, auth)
	local user = tostring(auth.user or "")
	local pass = tostring(auth.pass or "")
	local req = nil

	if W ~= nil then
		req = W.encode_userpass(user, pass)
		if type(req) ~= "string" then
			return false, "auth encode fail"
		end
	else
		if #user > 255 or #pass > 255 then
			return false, "auth too long"
		end
		req = _byte(0x01) .. _byte(#user) .. user .. _byte(#pass) .. pass
	end

	local ok_s, err_s = kproxy_io.send(tcp, req)
	if not ok_s then
		return false, err_s or "auth send fail"
	end

	local rsp, err_r = _recv_exact(tcp, 2)
	if rsp == nil then
		return false, err_r or "auth recv fail"
	end

	if W ~= nil then
		local status, err_p = W.parse_userpass_status(rsp)
		if status == nil then
			return false, err_p or "bad auth ver"
		end
		if status ~= 0x00 then
			return false, "auth rejected"
		end
		return true
	end

	if string.byte(rsp, 1) ~= 0x01 then
		return false, "bad auth ver"
	end

	if string.byte(rsp, 2) ~= 0x00 then
		return false, "auth rejected"
	end

	return true
end


local function _read_bnd(tcp, atyp)
	if atyp == 0x01 then
		return _recv_exact(tcp, 4 + 2)
	end

	if atyp == 0x03 then
		local len_b, err_l = _recv_exact(tcp, 1)
		if len_b == nil then
			return nil, err_l
		end

		local n = string.byte(len_b, 1)
		return _recv_exact(tcp, n + 2)
	end

	if atyp == 0x04 then
		return _recv_exact(tcp, 16 + 2)
	end

	return nil, "bad atyp"
end


-- @brief 经 SOCKS5 代理连接目标
-- @param [in]  proxy_host[string]
-- @param [in]  proxy_port[number]
-- @param [in]  dest_host[string]
-- @param [in]  dest_port[number]
-- @param [in]  auth[table|nil]	[可选] `{ user, pass }`
-- @param [in]  opts[table|nil]	[可选] `{ timeout_ms[number], tcp[any] }` tcp=已连到本跳代理的套接字(多跳)
-- @return [boolean] ok
-- @return [table|string] conn 或 err
-- @note conn: `{ kind, close(), send(), recv(), tcp }`
local function _connect_impl(proxy_host, proxy_port, dest_host, dest_port, auth, opts)
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

	if auth ~= nil and type(auth) ~= "table" then
		return false, "bad auth"
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	local timeout_ms = 15000
	local reuse_tcp = nil
	if opts ~= nil then
		if opts.timeout_ms ~= nil then
			timeout_ms = tonumber(opts.timeout_ms) or timeout_ms
		end
		if opts.tcp ~= nil then
			reuse_tcp = opts.tcp
		end
	end

	-- step1. TCP 连代理 (或多跳复用已有隧道)
	local tcp = reuse_tcp
	if tcp == nil then
		local err_c = nil
		tcp, err_c = kproxy_io.tcp_connect(proxy_host, tonumber(proxy_port), timeout_ms, opts)
		if tcp == nil then
			return false, err_c or "connect fail"
		end
	elseif type(tcp.send) ~= "function" or type(tcp.recv) ~= "function" then
		return false, "bad opts.tcp"
	end

	local function fail(msg)
		_close_tcp(tcp)
		return false, msg
	end

	-- step2. greeting
	local methods = nil
	if W ~= nil then
		local with_auth = (auth ~= nil and (auth.user ~= nil or auth.pass ~= nil))
		methods = W.encode_greeting(with_auth)
		if type(methods) ~= "string" then
			return fail("greeting encode fail")
		end
	else
		if auth ~= nil and (auth.user ~= nil or auth.pass ~= nil) then
			methods = _byte(0x05) .. _byte(0x02) .. _byte(0x00) .. _byte(0x02)
		else
			methods = _byte(0x05) .. _byte(0x01) .. _byte(0x00)
		end
	end

	local ok_s, err_s = kproxy_io.send(tcp, methods)
	if not ok_s then
		return fail(err_s or "greeting send fail")
	end

	local greet, err_g = _recv_exact(tcp, 2)
	if greet == nil then
		return fail(err_g or "greeting recv fail")
	end

	local method = nil
	if W ~= nil then
		method = W.parse_method_select(greet)
		if method == nil then
			return fail("bad socks ver")
		end
	else
		if string.byte(greet, 1) ~= 0x05 then
			return fail("bad socks ver")
		end
		method = string.byte(greet, 2)
	end

	if method == 0xFF then
		return fail("no acceptable method")
	end

	-- step3. 可选 user/pass
	if method == 0x02 then
		if auth == nil then
			return fail("auth required")
		end

		local ok_a, err_a = _auth_userpass(tcp, auth)
		if not ok_a then
			return fail(err_a or "auth fail")
		end
	elseif method ~= 0x00 then
		return fail("unsupported method")
	end

	-- step4. CONNECT 请求
	local req = nil
	if W ~= nil then
		req = W.encode_connect(dest_host, tonumber(dest_port))
		if type(req) ~= "string" then
			return fail("bad dest")
		end
	else
		local addr, err_addr = _build_dest_addr(dest_host, tonumber(dest_port))
		if addr == nil then
			return fail(err_addr or "bad dest")
		end
		req = _byte(0x05) .. _byte(0x01) .. _byte(0x00) .. addr
	end

	ok_s, err_s = kproxy_io.send(tcp, req)
	if not ok_s then
		return fail(err_s or "connect req send fail")
	end

	local hdr, err_h = _recv_exact(tcp, 4)
	if hdr == nil then
		return fail(err_h or "connect rsp fail")
	end

	if string.byte(hdr, 1) ~= 0x05 then
		return fail("bad connect ver")
	end

	local rep = string.byte(hdr, 2)
	if rep ~= 0x00 then
		return fail(string.format("connect refused rep=%d", rep))
	end

	local atyp = string.byte(hdr, 4)
	local bnd, err_b = _read_bnd(tcp, atyp)
	if bnd == nil then
		return fail(err_b or "bnd read fail")
	end

	local conn = {
		kind = "socks5",
		proxy_host = proxy_host,
		proxy_port = tonumber(proxy_port),
		dest_host = dest_host,
		dest_port = tonumber(dest_port),
		tcp = tcp,
	}

	conn.close = function ()
		_close_tcp(tcp)
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

	return true, conn
end


-- @brief 经 SOCKS5 代理连接目标 (失败形状收口)
-- @return [boolean|nil] ok
-- @return [table|number] conn 或错误码
-- @return [string|nil] errstr
M.connect = function (proxy_host, proxy_port, dest_host, dest_port, auth, opts)
	local ok, conn = _connect_impl(proxy_host, proxy_port, dest_host, dest_port, auth, opts)
	if ok then
		return true, conn
	end

	return nil, errcode.of(conn), conn
end


local function _probe_recv_exact(tcp, n)
	local parts = {}
	local got = 0
	while got < n do
		local chunk, err = kproxy_io.recv(tcp, n - got)
		if type(chunk) ~= "string" or #chunk == 0 then
			return nil, err or "eof"
		end
		parts[#parts + 1] = chunk
		got = got + #chunk
	end
	return table.concat(parts)
end


local function _probe_bnd(tcp, atyp)
	if atyp == 0x01 then
		return _probe_recv_exact(tcp, 4 + 2)
	end
	if atyp == 0x03 then
		local len_b, err_l = _probe_recv_exact(tcp, 1)
		if len_b == nil then
			return nil, err_l
		end
		return _probe_recv_exact(tcp, string.byte(len_b, 1) + 2)
	end
	if atyp == 0x04 then
		return _probe_recv_exact(tcp, 16 + 2)
	end
	return nil, "bad atyp"
end


-- @brief 已连 TCP 上 SOCKS5 探活 (问候; 可选 CONNECT 校验 REP)
-- @param [in]  tcp[any]
-- @param [in]  probe_target[table|nil]	`{ host, port }`
-- @return [boolean] ok
-- @return [string|nil] err
local function _probe_impl(tcp, probe_target)
	if tcp == nil or type(tcp.send) ~= "function" then
		return false, "need tcp"
	end

	local greet
	if W ~= nil and type(W.encode_greeting) == "function" then
		greet = W.encode_greeting(false)
	end
	if type(greet) ~= "string" or greet == "" then
		greet = string.char(0x05, 0x01, 0x00)
	end

	local ok_s = kproxy_io.send(tcp, greet)
	if not ok_s then
		return false, "greeting send fail"
	end

	local rsp, err = _probe_recv_exact(tcp, 2)
	if rsp == nil then
		return false, err or "greeting recv fail"
	end

	if string.byte(rsp, 1) ~= 0x05 then
		return false, "bad socks ver"
	end

	if string.byte(rsp, 2) == 0xFF then
		return false, "no acceptable method"
	end

	if type(probe_target) ~= "table"
		or type(probe_target.host) ~= "string"
		or probe_target.host == ""
		or tonumber(probe_target.port) == nil then
		return true
	end

	local host = probe_target.host
	local port = tonumber(probe_target.port)
	local a, b, c, d = string.match(host, "^(%d+)%.(%d+)%.(%d+)%.(%d+)$")
	local addr
	if a ~= nil then
		addr = string.char(0x01, tonumber(a), tonumber(b), tonumber(c), tonumber(d))
			.. string.char(math.floor(port / 256) % 256, port % 256)
	else
		if #host > 255 then
			return false, "probe host too long"
		end
		addr = string.char(0x03, #host) .. host
			.. string.char(math.floor(port / 256) % 256, port % 256)
	end

	local req = string.char(0x05, 0x01, 0x00) .. addr
	ok_s = kproxy_io.send(tcp, req)
	if not ok_s then
		return false, "connect send fail"
	end

	local hdr, err_h = _probe_recv_exact(tcp, 4)
	if hdr == nil then
		return false, err_h or "connect rsp fail"
	end

	if string.byte(hdr, 1) ~= 0x05 then
		return false, "bad socks connect ver"
	end

	local rep = string.byte(hdr, 2)
	if rep ~= 0x00 then
		return false, string.format("connect refused rep=0x%02X", rep)
	end

	local bnd, err_b = _probe_bnd(tcp, string.byte(hdr, 4))
	if bnd == nil then
		return false, err_b or "bnd fail"
	end

	return true
end


-- @brief 已连 TCP 上 SOCKS5 探活 (失败形状收口)
-- @return [boolean|nil] ok
-- @return [number|nil] code
-- @return [string|nil] errstr
M.probe = function (tcp, probe_target)
	local ok, err = _probe_impl(tcp, probe_target)
	if ok then
		return true
	end

	return nil, errcode.of(err), err
end


return M

-- end
