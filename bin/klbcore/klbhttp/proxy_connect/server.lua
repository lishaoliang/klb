--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   server.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP 代理入口 (CONNECT + 明文绝对 URI 转发; klbhttp)
-- @note   serve_conn 解析目标; CONNECT 回 200 后中继; forward 改写为 origin-form
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 实现 CONNECT 解析与应答; N1-1 407 带 Proxy-Authenticate; 明文 HTTP 绝对 URI 转发; GET Upgrade websocket → kind=websocket (入口分流); 自 proxy 迁入 klbhttp.proxy_connect
--]]


local kproxy_io = require("klbcore.klbproxy.io")
local http_status = require("klbcore.klbhttp.status")

local M = {}


local function _recv_hdr(sock, prefix, max_n)
	local buf = prefix or ""
	max_n = tonumber(max_n) or 8192

	while true do
		local pos = string.find(buf, "\r\n\r\n", 1, true)
		if pos ~= nil then
			return string.sub(buf, 1, pos + 3), string.sub(buf, pos + 4)
		end

		if #buf >= max_n then
			return nil, "hdr too large"
		end

		local chunk, err = kproxy_io.recv(sock, math.min(1024, max_n - #buf))
		if type(chunk) ~= "string" or #chunk == 0 then
			return nil, err or "eof"
		end

		buf = buf .. chunk
	end
end


local function _parse_authority(authority, default_port)
	if type(authority) ~= "string" or authority == "" then
		return nil, nil, "bad authority"
	end

	default_port = tonumber(default_port) or 443

	-- strip optional userinfo
	local at = string.find(authority, "@", 1, true)
	if at ~= nil then
		authority = string.sub(authority, at + 1)
	end

	-- [IPv6]:port
	if string.sub(authority, 1, 1) == "[" then
		local close = string.find(authority, "]", 2, true)
		if close == nil then
			return nil, nil, "bad ipv6"
		end

		local host = string.sub(authority, 2, close - 1)
		local rest = string.sub(authority, close + 1)
		if rest == "" then
			return host, default_port, nil
		end

		local port = string.match(rest, "^:(%d+)$")
		if port == nil then
			return nil, nil, "bad ipv6 port"
		end

		return host, tonumber(port), nil
	end

	local host, port_s = string.match(authority, "^([^:]+):(%d+)$")
	if host ~= nil then
		return host, tonumber(port_s), nil
	end

	-- host only
	if string.find(authority, ":", 1, true) == nil then
		return authority, default_port, nil
	end

	return nil, nil, "bad authority"
end


-- @return host, port, path, err
local function _parse_absolute_url(url)
	if type(url) ~= "string" then
		return nil, nil, nil, "bad url"
	end

	local scheme, rest = string.match(url, "^(%w+)://(.+)$")
	if scheme == nil then
		return nil, nil, nil, "not absolute"
	end

	scheme = string.lower(scheme)
	if scheme == "https" then
		return nil, nil, nil, "https absolute use CONNECT"
	end
	if scheme ~= "http" then
		return nil, nil, nil, "unsupported scheme"
	end

	local authority, path
	local slash = string.find(rest, "/", 1, true)
	if slash == nil then
		authority = rest
		path = "/"
	else
		authority = string.sub(rest, 1, slash - 1)
		path = string.sub(rest, slash)
		if path == "" then
			path = "/"
		end
	end

	local host, port, err = _parse_authority(authority, 80)
	if host == nil then
		return nil, nil, nil, err or "bad authority"
	end

	return host, port, path, nil
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


local function _b64_decode(s)
	-- minimal base64 for Proxy-Authorization Basic (ASCII)
	local b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
	s = string.gsub(s, "[^" .. b .. "=]", "")
	local out = {}
	local i = 1
	while i <= #s do
		local function idx(c)
			if c == nil or c == "=" then
				return nil
			end
			return (string.find(b, c, 1, true) or 0) - 1
		end

		local a1 = idx(string.sub(s, i, i))
		local a2 = idx(string.sub(s, i + 1, i + 1))
		local a3 = idx(string.sub(s, i + 2, i + 2))
		local a4 = idx(string.sub(s, i + 3, i + 3))
		if a1 == nil or a2 == nil then
			break
		end

		local n = a1 * 262144 + a2 * 4096 + (a3 or 0) * 64 + (a4 or 0)
		out[#out + 1] = string.char(math.floor(n / 65536) % 256)
		if a3 ~= nil then
			out[#out + 1] = string.char(math.floor(n / 256) % 256)
		end
		if a4 ~= nil then
			out[#out + 1] = string.char(n % 256)
		end
		i = i + 4
	end

	return table.concat(out)
end


local function _hdr_connection_keepalive(hdr)
	for line in string.gmatch(hdr, "[^\r\n]+") do
		local k, v = string.match(line, "^([^:]+):%s*(.*)$")
		if k ~= nil then
			local lk = string.lower(k)
			if lk == "proxy-connection" or lk == "connection" then
				if string.find(string.lower(v), "keep-alive", 1, true) then
					return true
				end
			end
		end
	end
	return false
end


local function _check_proxy_auth(hdr, opts)
	local mode = _auth_mode(opts)
	if mode == "none" or mode == "" then
		return true
	end

	if mode == "basic" or mode == "user" or mode == "password" then
		mode = "userpass"
	end

	local auth_line = nil
	for line in string.gmatch(hdr, "[^\r\n]+") do
		local k, v = string.match(line, "^([^:]+):%s*(.*)$")
		if k ~= nil and string.lower(k) == "proxy-authorization" then
			auth_line = v
			break
		end
	end

	if auth_line == nil then
		return false, "auth required"
	end

	local scheme, payload = string.match(auth_line, "^(%S+)%s+(%S+)$")
	if scheme == nil then
		return false, "bad auth scheme"
	end
	scheme = string.lower(scheme)

	local auth = nil
	if type(opts) == "table" then
		auth = opts.auth
	end

	-- 优先走业务 acl (多用户 / bearer); 无则本地回退
	do
		local ok_a, acl = pcall(require, "klbagent.packages.proxy.acl")
		if ok_a and type(acl) == "table" and type(acl.check_credentials) == "function" then
			if scheme == "bearer" then
				return acl.check_credentials(auth, nil, nil, payload)
			end
			if scheme == "basic" then
				local decoded = _b64_decode(payload)
				local user, pass = string.match(decoded, "^(.-):(.*)$")
				if user == nil then
					return false, "bad auth payload"
				end
				return acl.check_credentials(auth, user, pass, nil)
			end
			return false, "bad auth scheme"
		end
	end

	if mode == "bearer" or mode == "token" then
		if scheme ~= "bearer" then
			return false, "bad auth scheme"
		end
		local expect = ""
		if type(auth) == "table" then
			expect = tostring(auth.token or "")
		end
		if expect ~= "" and payload == expect then
			return true
		end
		return false, "auth rejected"
	end

	if mode ~= "userpass" then
		return false, "unsupported auth"
	end

	if scheme ~= "basic" then
		return false, "bad auth scheme"
	end

	local decoded = _b64_decode(payload)
	local user, pass = string.match(decoded, "^(.-):(.*)$")
	if user == nil then
		return false, "bad auth payload"
	end

	if type(auth) == "table" and type(auth.users) == "table" then
		local users = auth.users
		if #users > 0 then
			for i = 1, #users do
				local u = users[i]
				if type(u) == "table"
					and tostring(u.user or "") == user
					and tostring(u.pass or "") == pass then
					return true
				end
			end
		elseif users[user] ~= nil and tostring(users[user]) == pass then
			return true
		end
		return false, "auth rejected"
	end

	local expect_user = ""
	local expect_pass = ""
	if type(auth) == "table" then
		expect_user = tostring(auth.user or "")
		expect_pass = tostring(auth.pass or "")
	end

	if user == expect_user and pass == expect_pass then
		return true
	end

	return false, "auth rejected"
end


-- @brief 将代理请求头改写为发往源站的 origin-form
-- @param [in]  method[string]
-- @param [in]  path[string]
-- @param [in]  ver[string]
-- @param [in]  hdr[string]		含请求行的完整头
-- @param [in]  host[string]
-- @param [in]  port[number]
-- @return [string] origin request bytes (headers only, 无 body)
M.build_origin_request = function (method, path, ver, hdr, host, port)
	local lines = {
		string.format("%s %s %s", method, path, ver),
	}

	local has_host = false
	local first = true
	for line in string.gmatch(hdr, "[^\r\n]+") do
		if first then
			first = false
			-- skip request line
		else
			local k, v = string.match(line, "^([^:]+):%s*(.*)$")
			if k ~= nil then
				local lk = string.lower(k)
				if lk == "proxy-authorization" or lk == "proxy-connection" then
					-- drop
				elseif lk == "connection" then
					-- force close below
				elseif lk == "host" then
					has_host = true
					lines[#lines + 1] = line
				else
					lines[#lines + 1] = line
				end
			end
		end
	end

	if not has_host then
		if tonumber(port) == 80 then
			lines[#lines + 1] = "Host: " .. tostring(host)
		else
			lines[#lines + 1] = string.format("Host: %s:%d", tostring(host), tonumber(port))
		end
	end

	-- 源站侧恒 Connection: close, 便于泵至 EOF; 客户端 keep-alive 由 entry 循环复用
	lines[#lines + 1] = "Connection: close"
	lines[#lines + 1] = ""
	return table.concat(lines, "\r\n") .. "\r\n"
end


-- @brief 处理 HTTP 代理请求 (CONNECT 或明文绝对 URI)
-- @param [in]  sock[any]		须有 send/recv
-- @param [in]  opts[table|nil]	`{ auth, prefix[string] }`
-- @return [boolean] ok
-- @return [table|string] req 或 err
--   CONNECT: `{ kind="connect", host, port, http_ver }`
--   forward: `{ kind="forward", host, port, method, path, http_ver, origin_hdr, body_prefix }`
M.serve_conn = function (sock, opts)
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

	local hdr, body_or_err = _recv_hdr(sock, prefix, 8192)
	if hdr == nil then
		return false, body_or_err or "hdr fail"
	end
	local body_prefix = body_or_err or ""

	local first = string.match(hdr, "^([^\r\n]+)")
	if first == nil then
		return false, "empty request"
	end

	local method, target, ver = string.match(first, "^(%S+)%s+(%S+)%s+(HTTP/%d%.%d)$")
	if method == nil then
		return false, "bad request line"
	end

	method = string.upper(method)

	-- N5: WebSocket 升级; 细节在 klbcore.klbws.is_upgrade
	do
		local ok_ws, klbws = pcall(require, "klbcore.klbws")
		if ok_ws and type(klbws) == "table" and type(klbws.is_upgrade) == "function" then
			local is_up, info = klbws.is_upgrade(hdr)
			if is_up and type(info) == "table" then
				local token = ""
				if type(info.headers) == "table" and type(info.headers["x-alua-token"]) == "string" then
					token = info.headers["x-alua-token"]
				end

				return true, {
					kind = "websocket",
					path = info.path,
					key = info.key,
					token = token,
					http_ver = ver,
					hdr = hdr,
					body_prefix = body_prefix,
				}
			end
		end
	end

	local ok_auth, err_auth = _check_proxy_auth(hdr, opts)
	if not ok_auth then
		local challenges = nil
		if type(opts) == "table" and type(opts.auth) == "table" then
			local m = string.lower(tostring(opts.auth.mode or ""))
			if m == "bearer" or m == "token" then
				challenges = { 'Bearer realm="alua-proxy"' }
			elseif m == "userpass" or m == "basic" then
				challenges = { 'Basic realm="alua-proxy"' }
			else
				challenges = {
					'Basic realm="alua-proxy"',
					'Bearer realm="alua-proxy"',
				}
			end
		end
		M.reply(sock, http_status.PROXY_AUTH_REQUIRED, "Proxy Authentication Required", {
			authenticate = challenges,
		})
		return false, err_auth or "auth fail"
	end

	local keep_alive = _hdr_connection_keepalive(hdr)

	if method == "CONNECT" then
		local host, port, err_a = _parse_authority(target, 443)
		if host == nil then
			M.reply(sock, http_status.BAD_REQUEST, "Bad Request")
			return false, err_a or "bad target"
		end

		return true, {
			kind = "connect",
			host = host,
			port = port,
			http_ver = ver,
			keep_alive = false,
		}
	end

	-- 明文绝对 URI (GET/POST/HEAD/…)
	local host, port, path, err_u = _parse_absolute_url(target)
	if host == nil then
		if err_u == "https absolute use CONNECT" then
			M.reply(sock, http_status.NOT_IMPLEMENTED, "Not Implemented")
			return false, err_u
		end
		if err_u == "not absolute" then
			M.reply(sock, http_status.BAD_REQUEST, "Bad Request")
			return false, "need absolute-form uri"
		end
		M.reply(sock, http_status.BAD_REQUEST, "Bad Request")
		return false, err_u or "bad url"
	end

	local origin_hdr = M.build_origin_request(method, path, ver, hdr, host, port)
	return true, {
		kind = "forward",
		host = host,
		port = port,
		method = method,
		path = path,
		http_ver = ver,
		origin_hdr = origin_hdr,
		body_prefix = body_prefix,
		keep_alive = keep_alive,
	}
end


-- @brief 发送 CONNECT / 错误应答
-- @param [in]  sock[any]
-- @param [in]  code[number]	默认 200
-- @param [in]  reason[string|nil]
-- @return [boolean] ok
-- @return [string|nil] err
M.reply = function (sock, code, reason, opts)
	if sock == nil or type(sock.send) ~= "function" then
		return false, "need sock"
	end

	code = tonumber(code) or http_status.OK
	if type(reason) ~= "string" or reason == "" then
		if code == http_status.OK then
			reason = "Connection Established"
		else
			reason = "Error"
		end
	end

	local lines = {
		string.format("HTTP/1.1 %d %s", code, reason),
		"Proxy-Agent: alua-proxy",
	}
	if code == http_status.PROXY_AUTH_REQUIRED then
		local auths = nil
		if type(opts) == "table" and type(opts.authenticate) == "table" then
			auths = opts.authenticate
		end
		if auths == nil or #auths == 0 then
			auths = { 'Basic realm="alua-proxy"' }
		end
		for i = 1, #auths do
			lines[#lines + 1] = "Proxy-Authenticate: " .. tostring(auths[i])
		end
	end
	lines[#lines + 1] = ""
	local msg = table.concat(lines, "\r\n") .. "\r\n"
	local ok = kproxy_io.send(sock, msg)
	if not ok then
		return false, "reply send fail"
	end

	return true
end


return M

-- end
