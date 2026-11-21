--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP CONNECT 出站客户端 (klbhttp)
-- @note   经 HTTP 代理 CONNECT 到目标; 隧道后由调用方读写
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 实现 CONNECT 出站; kproxy_io co_*; 自 proxy 迁入 klbhttp.proxy_connect
--]]


local kproxy_io = require("klbcore.klbproxy.io")


local M = {}


local function _recv_until(tcp, mark, max_n)
	local buf = ""
	max_n = tonumber(max_n) or 8192

	while true do
		local pos = string.find(buf, mark, 1, true)
		if pos ~= nil then
			return string.sub(buf, 1, pos + #mark - 1), string.sub(buf, pos + #mark)
		end

		if #buf >= max_n then
			return nil, "rsp too large"
		end

		local chunk, err = kproxy_io.recv(tcp, math.min(1024, max_n - #buf))
		if type(chunk) ~= "string" or #chunk == 0 then
			return nil, err or "eof"
		end

		buf = buf .. chunk
	end
end


local function _close_tcp(tcp)
	if tcp ~= nil and type(tcp.close) == "function" then
		pcall(function ()
			tcp:close()
		end)
	end
end


local function _b64_encode(s)
	local b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
	local out = {}
	local i = 1
	while i <= #s do
		local c1 = string.byte(s, i) or 0
		local c2 = string.byte(s, i + 1)
		local c3 = string.byte(s, i + 2)
		local n = c1 * 65536 + (c2 or 0) * 256 + (c3 or 0)
		local n1 = math.floor(n / 262144) % 64
		local n2 = math.floor(n / 4096) % 64
		local n3 = math.floor(n / 64) % 64
		local n4 = n % 64
		out[#out + 1] = string.sub(b, n1 + 1, n1 + 1)
		out[#out + 1] = string.sub(b, n2 + 1, n2 + 1)
		if c2 == nil then
			out[#out + 1] = "="
			out[#out + 1] = "="
		elseif c3 == nil then
			out[#out + 1] = string.sub(b, n3 + 1, n3 + 1)
			out[#out + 1] = "="
		else
			out[#out + 1] = string.sub(b, n3 + 1, n3 + 1)
			out[#out + 1] = string.sub(b, n4 + 1, n4 + 1)
		end
		i = i + 3
	end

	return table.concat(out)
end


-- @brief 经 HTTP 代理 CONNECT 到目标
-- @param [in]  proxy_host[string]
-- @param [in]  proxy_port[number]
-- @param [in]  dest_host[string]
-- @param [in]  dest_port[number]
-- @param [in]  opts[table|nil]	`{ timeout_ms, auth={user,pass}, proxy_tls, tcp }` tcp=多跳复用
-- @return [boolean] ok
-- @return [table|string] conn 或 err
-- @note conn: `{ kind, close(), send(), recv(), tcp }`
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

	local timeout_ms = 15000
	local auth = nil
	local reuse_tcp = nil
	if opts ~= nil then
		if opts.timeout_ms ~= nil then
			timeout_ms = tonumber(opts.timeout_ms) or timeout_ms
		end
		if type(opts.auth) == "table" then
			auth = opts.auth
		end
		if opts.tcp ~= nil then
			reuse_tcp = opts.tcp
		end
	end

	-- step1. TCP 连代理 (或多跳复用已有隧道; 代理侧 TLS 后置)
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

	local authority = string.format("%s:%d", dest_host, tonumber(dest_port))
	local lines = {
		string.format("CONNECT %s HTTP/1.1", authority),
		string.format("Host: %s", authority),
		"Proxy-Connection: Keep-Alive",
	}

	if auth ~= nil and (auth.user ~= nil or auth.pass ~= nil) then
		local user = tostring(auth.user or "")
		local pass = tostring(auth.pass or "")
		local token = _b64_encode(user .. ":" .. pass)
		lines[#lines + 1] = "Proxy-Authorization: Basic " .. token
	end

	lines[#lines + 1] = ""
	lines[#lines + 1] = ""
	local req = table.concat(lines, "\r\n")

	local ok_s, err_s = kproxy_io.send(tcp, req)
	if not ok_s then
		return fail(err_s or "connect send fail")
	end

	local rsp, err_r = _recv_until(tcp, "\r\n\r\n", 8192)
	if rsp == nil then
		return fail(err_r or "connect rsp fail")
	end

	local code = string.match(rsp, "^HTTP/%d%.%d%s+(%d%d%d)")
	if code ~= "200" then
		return fail(string.format("connect refused code=%s", tostring(code)))
	end

	local conn = {
		kind = "http-connect",
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


-- @brief 已连 TCP 上 HTTP CONNECT 探活
-- @param [in]  tcp[any]
-- @param [in]  probe_target[table|nil]	`{ host, port }`; 缺则仅视为 TCP 可达
-- @return [boolean] ok
-- @return [string|nil] err
M.probe = function (tcp, probe_target)
	if tcp == nil or type(tcp.send) ~= "function" then
		return false, "need tcp"
	end

	if type(probe_target) ~= "table"
		or type(probe_target.host) ~= "string"
		or probe_target.host == ""
		or tonumber(probe_target.port) == nil then
		return true
	end

	local authority = string.format("%s:%d", probe_target.host, tonumber(probe_target.port))
	local req = string.format(
		"CONNECT %s HTTP/1.1\r\nHost: %s\r\nProxy-Connection: Keep-Alive\r\n\r\n",
		authority,
		authority
	)
	local ok_s = kproxy_io.send(tcp, req)
	if not ok_s then
		return false, "connect send fail"
	end

	local rsp, err_r = _recv_until(tcp, "\r\n\r\n", 4096)
	if rsp == nil then
		return false, err_r or "connect rsp fail"
	end

	local code = string.match(rsp, "^HTTP/%d%.%d%s+(%d%d%d)")
	if code == nil then
		return false, "bad connect rsp"
	end

	local n = tonumber(code)
	if n == nil or n < 200 or n >= 300 then
		return false, string.format("connect refused code=%s", code)
	end

	return true
end


return M

-- end
