--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   proxy.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  svn_https HTTP(S) 反向代理 (WebDAV 透传) [已废止]
-- @note   init.lua 不再路由至此; 保留文件仅供对照. 须在 kco 协程内;
--   \n 改写 Destination/Location scheme+host; edge 优先 ALUA_SVN_EDGE_HOST
-- @history 修改历史
--  \n [2026-09] 创建文件
--]]
local httpclienter = require("klbcore.klbhttp.client.httpclienter")


local M = {}


-- hop-by-hop: 不转发; Authorization / WWW-Authenticate 故意不在此列 (鉴权透传)
local HOP = {
	["connection"] = true,
	["keep-alive"] = true,
	["proxy-authenticate"] = true,
	["proxy-authorization"] = true,
	["te"] = true,
	["trailers"] = true,
	["transfer-encoding"] = true,
	["upgrade"] = true,
	["content-length"] = true,
}


-- 上游连接空闲池: key -> { client, ... }; 协程间借还, 同时仅一请求占用一条连接
local _up_free = {}


-- @brief 解析 upstream URL
-- @return info[table|nil] { scheme, host, port, path_prefix, tls }
M.parse_upstream = function (url)
	if type(url) ~= "string" or url == "" then
		return nil
	end

	local scheme, rest = string.match(url, "^(https?)://(.+)$")
	if scheme == nil then
		return nil
	end

	scheme = string.lower(scheme)
	local hostport, path = string.match(rest, "^([^/]+)(/.*)$")
	if hostport == nil then
		hostport = rest
		path = ""
	end

	local host, port_s = string.match(hostport, "^([^:]+):(%d+)$")
	if host == nil then
		host = hostport
		if scheme == "https" then
			port_s = "443"
		else
			port_s = "80"
		end
	end

	local port = tonumber(port_s)
	if type(host) ~= "string" or host == "" or port == nil then
		return nil
	end

	if path == "/" then
		path = ""
	end

	return {
		scheme = scheme,
		host = host,
		port = port,
		path_prefix = path or "",
		tls = (scheme == "https"),
	}
end


-- @brief 解析 host[:port] 字面量
-- @return host[string|nil], port[number|nil]
local function _split_hostport(s, default_port)
	if type(s) ~= "string" or s == "" then
		return nil, nil
	end

	-- 去掉可能的方括号 IPv6 简写暂不支持, 仅 host 或 host:port
	local host, port_s = string.match(s, "^([^:]+):(%d+)$")
	if host ~= nil then
		return host, tonumber(port_s)
	end

	return s, default_port
end


-- @brief 从请求/环境解析边缘对外 host (改写 Destination/Location)
-- @param [in] req[table|nil]
-- @param [in] listen_port[number]
-- @param [in] tls[boolean]
-- @return edge[table] { host, port, tls }
M.resolve_edge = function (req, listen_port, tls)
	local port = tonumber(listen_port) or 443
	local host = "127.0.0.1"
	local use_tls = (tls ~= false)

	local env_h = os.getenv("ALUA_SVN_EDGE_HOST")
	if type(env_h) == "string" and env_h ~= "" then
		local h, p = _split_hostport(env_h, port)
		if h ~= nil then
			host = h
			if p ~= nil then
				port = p
			end
		end
	elseif type(req) == "table" and type(req.headers) == "table" then
		local hh = req.headers["host"]
		if type(hh) == "string" and hh ~= "" then
			local h, p = _split_hostport(hh, port)
			if h ~= nil then
				host = h
				if p ~= nil then
					port = p
				end
			end
		end
	end

	return {
		host = host,
		port = port,
		tls = use_tls,
	}
end


local function _header_ok(s)
	if type(s) ~= "string" or s == "" then
		return false
	end

	if string.find(s, "\r", 1, true) ~= nil or string.find(s, "\n", 1, true) ~= nil then
		return false
	end

	return true
end


-- 改写绝对 URL 的 scheme://host[:port] 为 upstream 对外可见侧 / 或后端
local function _rewrite_abs_url(url, from_host, from_port, from_tls, to_host, to_port, to_tls)
	if type(url) ~= "string" or url == "" then
		return url
	end

	local sch, rest = string.match(url, "^(https?)://(.+)$")
	if sch == nil then
		return url
	end

	local hp, path = string.match(rest, "^([^/]+)(.*)$")
	if hp == nil then
		return url
	end

	local h, p = string.match(hp, "^([^:]+):(%d+)$")
	local port
	if h == nil then
		h = hp
		if string.lower(sch) == "https" then
			port = 443
		else
			port = 80
		end
	else
		port = tonumber(p)
	end

	if string.lower(h) ~= string.lower(from_host) then
		return url
	end

	if port ~= from_port then
		-- 默认端口省略时 from_port 可能是 443/80
		local def = from_tls and 443 or 80
		if not (port == def and from_port == def) then
			return url
		end
	end

	local out_sch = to_tls and "https" or "http"
	local out_hp = to_host
	local def_to = to_tls and 443 or 80
	if to_port ~= def_to then
		out_hp = to_host .. ":" .. tostring(to_port)
	end

	return out_sch .. "://" .. out_hp .. (path or "")
end


local function _parse_status(head)
	local code = tonumber(string.match(head or "", "^HTTP/%d%.%d%s+(%d+)"))
	return code or 502
end


local function _parse_resp_headers(head)
	local hdrs = {}
	local list = {}
	if type(head) ~= "string" then
		return hdrs, list
	end

	local first = true
	for line in string.gmatch(head, "([^\r\n]+)") do
		if first then
			first = false
		else
			local name, value = string.match(line, "^([^:]+):%s*(.*)$")
			if name ~= nil then
				local low = string.lower(name)
				hdrs[low] = value
				list[#list + 1] = { name, value }
			end
		end
	end

	return hdrs, list
end


local function _conn_wants_close(hdrs, req_close)
	if req_close then
		return true
	end

	if type(hdrs) ~= "table" then
		return true
	end

	local c = hdrs["connection"]
	if type(c) == "string" and string.find(string.lower(c), "close", 1, true) ~= nil then
		return true
	end

	return false
end


local function _up_key(up)
	return string.format("%s:%d:%s", up.host, up.port, up.tls and "1" or "0")
end


local function _borrow_up(up, want_ka)
	local key = _up_key(up)
	if want_ka then
		local list = _up_free[key]
		if type(list) == "table" and #list > 0 then
			local c = table.remove(list)
			if c ~= nil then
				return c, true
			end
		end
	end

	local client = httpclienter.new()
	local rc = client:connect(up.host, up.port, { tls = up.tls })
	if rc ~= 0 then
		client:disconnect()
		return nil, false
	end

	return client, false
end


local function _release_up(up, client, reuse)
	if client == nil then
		return
	end

	if not reuse then
		client:disconnect()
		return
	end

	local key = _up_key(up)
	local list = _up_free[key]
	if type(list) ~= "table" then
		list = {}
		_up_free[key] = list
	end

	-- 限制池大小, 避免空闲连接堆积
	if #list >= 8 then
		client:disconnect()
		return
	end

	list[#list + 1] = client
end


local function _pack_req_head(method, target, host, port, headers, body_len, keep_alive)
	local t = {}
	t[#t + 1] = string.format("%s %s HTTP/1.1\r\n", method, target)
	local hp = host
	if port ~= 80 and port ~= 443 then
		hp = host .. ":" .. tostring(port)
	end

	t[#t + 1] = "Host: " .. hp .. "\r\n"
	if keep_alive then
		t[#t + 1] = "Connection: keep-alive\r\n"
	else
		t[#t + 1] = "Connection: close\r\n"
	end

	t[#t + 1] = string.format("Content-Length: %d\r\n", body_len or 0)

	if type(headers) == "table" then
		for name, value in pairs(headers) do
			local low = string.lower(tostring(name))
			if not HOP[low] and low ~= "host" then
				if _header_ok(tostring(name)) and _header_ok(tostring(value)) then
					t[#t + 1] = string.format("%s: %s\r\n", name, value)
				end
			end
		end
	end

	t[#t + 1] = "\r\n"
	return table.concat(t)
end


local function _upstream_ka_enabled()
	-- 默认关闭: khttp 客户端复用上游连接在部分后端上会挂死; 显式 1/true 才开
	local v = os.getenv("ALUA_SVN_UPSTREAM_KEEPALIVE")
	if v == nil or v == "" then
		return false
	end

	return v == "1" or string.lower(v) == "true" or string.lower(v) == "on"
end


-- @brief 转发当前请求到 upstream
-- @param [in] req[table] klbweb req
-- @param [in] res[table] klbweb res
-- @param [in] up[table]  parse_upstream 结果
-- @param [in] edge[table|nil] { host, port, tls }; nil 则按请求 Host 解析
-- @param [in] listen_port[number|nil]  edge 缺省时的监听口
-- @return 无
M.forward = function (req, res, up, edge, listen_port)
	local path = req.path or "/"
	local qs = req.query_string or ""
	local target = up.path_prefix .. path
	if target == "" then
		target = "/"
	end

	if qs ~= "" then
		target = target .. "?" .. qs
	end

	local headers = {}
	if type(req.headers) == "table" then
		for k, v in pairs(req.headers) do
			headers[k] = v
		end
	end

	if edge == nil then
		edge = M.resolve_edge(req, listen_port or 443, true)
	end

	-- Destination: 客户端 https 边缘 -> 后端 http(s)
	local dest = headers["destination"]
	if type(dest) == "string" and dest ~= "" and edge ~= nil then
		headers["destination"] = _rewrite_abs_url(
			dest,
			edge.host, edge.port, edge.tls,
			up.host, up.port, up.tls)
	end

	local body = req.body or ""
	local has_auth = (type(headers["authorization"]) == "string" and headers["authorization"] ~= "")
	local want_ka = _upstream_ka_enabled()

	if os.getenv("ALUA_SVN_PROXY_DEBUG") == "1" then
		local line1 = string.match(req.head or "", "^([^\r\n]+)") or ""
		io.stderr:write(string.format(
			"svn_https proxy: method=%s line1=[%s] path=%s body=%d edge=%s:%d auth=%s ka=%s\n",
			tostring(req.method), line1, tostring(target), #body,
			tostring(edge.host), tonumber(edge.port) or 0,
			has_auth and "1" or "0", want_ka and "1" or "0"))
	end

	local head = _pack_req_head(
		string.upper(req.method or "GET"),
		target,
		up.host,
		up.port,
		headers,
		#body,
		want_ka)

	local client, from_pool = _borrow_up(up, want_ka)
	if client == nil then
		res:co_status(502, "text/plain", "upstream connect fail")
		return
	end

	local rc = client:send(head, body)
	if rc ~= 0 then
		_release_up(up, client, false)
		-- 池连接可能已失效, 再试一次新连接
		if from_pool then
			client, from_pool = _borrow_up(up, false)
			if client == nil then
				res:co_status(502, "text/plain", "upstream connect fail")
				return
			end

			rc = client:send(head, body)
		end

		if rc ~= 0 then
			_release_up(up, client, false)
			res:co_status(502, "text/plain", "upstream send fail")
			return
		end
	end

	local msg, up_head, up_body = client:co_recv_text()
	if msg ~= "text" then
		_release_up(up, client, false)
		res:co_status(502, "text/plain", "upstream recv fail")
		return
	end

	local code = _parse_status(up_head)
	local hdrs, list = _parse_resp_headers(up_head)
	local reuse = want_ka and (not _conn_wants_close(hdrs, false))
	_release_up(up, client, reuse)

	for i = 1, #list do
		local name = list[i][1]
		local value = list[i][2]
		local low = string.lower(name)
		if not HOP[low] then
			if (low == "location" or low == "content-location") and edge ~= nil then
				value = _rewrite_abs_url(
					value,
					up.host, up.port, up.tls,
					edge.host, edge.port, edge.tls)
			end

			res:header(name, value)
		end
	end

	local ctype = hdrs["content-type"] or "application/octet-stream"
	if req.method == "HEAD" then
		res:co_status(code, ctype, "")
	else
		res:co_status(code, ctype, up_body or "")
	end
end


return M

-- end
