--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   httpclienter.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP 客户端
-- @note   HTTP 协议; 内部使用 khttp
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local khttp = require("khttp")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现

local b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"


-- @brief Base64 编码 (URL userinfo -> Authorization: Basic)
local function b64enc(s)
	local t = {}
	local n = string.len(s)
	local i = 1

	while i <= n do
		local remain = n - i + 1
		local a = string.byte(s, i)
		local b = 0
		local c = 0

		if 2 <= remain then
			b = string.byte(s, i + 1)
		end

		if 3 <= remain then
			c = string.byte(s, i + 2)
		end

		local n2 = ((a & 3) << 4) | (b >> 4)
		local n3 = ((b & 15) << 2) | (c >> 6)

		t[#t + 1] = string.sub(b64chars, (a >> 2) + 1, (a >> 2) + 1)
		t[#t + 1] = string.sub(b64chars, n2 + 1, n2 + 1)

		if 1 == remain then
			t[#t + 1] = "="
			t[#t + 1] = "="
		elseif 2 == remain then
			t[#t + 1] = string.sub(b64chars, n3 + 1, n3 + 1)
			t[#t + 1] = "="
		else
			t[#t + 1] = string.sub(b64chars, n3 + 1, n3 + 1)
			t[#t + 1] = string.sub(b64chars, (c & 63) + 1, (c & 63) + 1)
		end

		i = i + 3
	end

	return table.concat(t)
end


-- @brief 已含 Authorization 头 (名大小写不敏感)
local function has_authorization(headers)
	if "table" ~= type(headers) then
		return false
	end

	for name, _ in pairs(headers) do
		if "authorization" == string.lower(tostring(name)) then
			return true
		end
	end

	return false
end


-- @brief 由 opts 生成 Authorization 值; 无则 nil
-- @note 优先级: opts.auth > opts.token (Bearer) > URL userinfo (Basic)
local function auth_value(opts, u)
	if "table" ~= type(opts) then
		opts = {}
	end

	local a = opts.auth
	if "table" == type(a) then
		local atype = string.lower(tostring(a.type or ""))
		if "basic" == atype then
			local user = tostring(a.user or "")
			if "" ~= user then
				return "Basic " .. b64enc(user .. ":" .. tostring(a.pass or ""))
			end
		elseif "bearer" == atype or "" == atype then
			if "string" == type(a.token) and "" ~= a.token then
				return "Bearer " .. a.token
			end
		end
	end

	if "string" == type(opts.token) and "" ~= opts.token then
		return "Bearer " .. opts.token
	end

	local info = u and u["userinfo"]
	if "string" == type(info) and "" ~= info then
		local user, pass = string.match(info, "^(.-):(.*)$")
		if nil == user then
			user = info
			pass = ""
		end

		return "Basic " .. b64enc(user .. ":" .. (pass or ""))
	end

	return nil
end


--------------------------------------------------------------------------------------------
-- cookie jar (进程内; 以主机为键)

local jar = {}


local function now_ms()
	return os.time() * 1000
end


-- @brief 解析一行 Set-Cookie
-- @return [table|nil]	`{ name, value, path, expires, secure }`
local function parse_set_cookie(line)
	local first, rest = string.match(line, "^([^;]*)(.*)$")
	if nil == first then
		return nil
	end

	local name, value = string.match(first, "^%s*([^=;%s]+)%s*=%s*(.-)%s*$")
	if nil == name or "" == name then
		return nil
	end

	local c = {
		name = name,
		value = value or "",
		path = "/",
		expires = nil,
		secure = false,
	}

	for attr in string.gmatch(rest, ";([^;]+)") do
		local k, v = string.match(attr, "^%s*([%w%-]+)%s*=%s*(.-)%s*$")
		local key = string.lower(k or string.match(attr, "^%s*([%w%-]+)") or "")
		if "path" == key and "string" == type(v) and "" ~= v then
			c.path = v
		elseif "max-age" == key then
			local n = tonumber(v)
			if n then
				c.expires = now_ms() + n * 1000
			end
		elseif "secure" == key then
			c.secure = true
		end
	end

	return c
end


-- @brief 收存响应头中的 Set-Cookie; opts.cookie=false 时不存
local function store_cookies(host, head, opts)
	if "table" == type(opts) and false == opts.cookie then
		return
	end

	if "string" ~= type(host) or "" == host or "string" ~= type(head) then
		return
	end

	local bucket = jar[host]
	if not bucket then
		bucket = {}
		jar[host] = bucket
	end

	local now = now_ms()
	for line in string.gmatch(head, "([^\r\n]+)") do
		local value = string.match(line, "^[Ss]et%-[Cc]ookie:%s*(.+)$")
		if value then
			local c = parse_set_cookie(value)
			if c then
				if nil ~= c.expires and c.expires <= now then
					bucket[c.name] = nil
				else
					bucket[c.name] = c
				end
			end
		end
	end
end


-- @brief 生成 Cookie 头值; 无可用 cookie 时 nil
local function cookie_value(host, path, secure)
	local bucket = jar[host]
	if "table" ~= type(bucket) then
		return nil
	end

	path = path or "/"
	local now = now_ms()
	local names = {}

	for name, c in pairs(bucket) do
		if nil ~= c.expires and c.expires <= now then
			bucket[name] = nil
		elseif secure or not c.secure then
			local p = c.path or "/"
			if string.sub(path, 1, #p) == p then
				names[#names + 1] = name
			end
		end
	end

	table.sort(names)

	local parts = {}
	for i = 1, #names do
		local c = bucket[names[i]]
		parts[#parts + 1] = c.name .. "=" .. c.value
	end

	if 0 == #parts then
		return nil
	end

	return table.concat(parts, "; ")
end


--------------------------------------------------------------------------------------------
-- httpclient

local httpclient = {}


-- @brief 断开连接
function httpclient:disconnect()
	if self._client then
		self._client:disconnect()
		self._client = nil
	end
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] 连接选项; 见内联 opts 注释
-- @return [number]	0 成功; 非0 为 klb_netcode_e
-- @note 对象方法保持 0/非0; 模块级 `klbhttp.connect` 为 table|nil,code
-- @note 失败时 self._last_code 为 code; 描述串用 knetcode.strerror(code)
function httpclient:connect(host, port, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	-- }
	local client, code = khttp.connect(tostring(host), tonumber(port), opts)
	if not client then
		self._last_code = code
		return code or 1
	end

	self._last_code = nil
	self._client = client

	return 0
end


-- @brief 经代理连接; https 目标再建 CONNECT 隧道并就地升级 TLS
-- @param [in]      host[string]		目标主机
-- @param [in]      port[number(int)]	目标端口
-- @param [in]      tls[boolean]		目标是否 TLS
-- @param [in]      proxy[table]		代理 `{ host[string], port[number(int)], tls[boolean] }`
-- @return [number]	0 成功; 非0 失败
-- @note http 目标连上代理即可, 请求改用绝对 URI; https 目标须隧道
function httpclient:connect_proxy(host, port, tls, proxy)
	-- step1. 连到代理
	local rc = self:connect(proxy.host, proxy.port, { tls = proxy.tls })
	if 0 ~= rc then
		return rc
	end

	-- http 目标: 直连代理, 稍后用绝对 URI 发请求
	if not tls then
		return 0
	end

	-- step2. https 目标: CONNECT 请求应答按无体收尾
	rc = self:set_connect(true)
	if 0 ~= rc then
		return rc
	end

	local authority = string.format('%s:%d', host, port)
	rc = self:send(string.format('CONNECT %s HTTP/1.1\r\nHost: %s\r\n\r\n', authority, authority))
	if 0 ~= rc then
		return rc
	end

	local msg, head = self:co_recv_text()
	if 'text' ~= msg then
		return 1
	end

	local code = string.match(tostring(head), '^HTTP/%d%.%d%s+(%d%d%d)')
	if '200' ~= code then
		return 1
	end

	-- step3. 隧道已通: 就地升级 TLS, 之后照常发请求
	return self:upgrade_tls()
end


-- @brief 置/清 CONNECT 应答模式 (正向代理 https 隧道)
-- @param [in]      connect[boolean]	true. 下一个响应按 CONNECT 2xx 无体解析; false. 普通响应
-- @return [number]	0 成功; 非0 为 klb_netcode_e
-- @note 发 CONNECT 前置 true; upgrade_tls 成功后由 C 自动复位
function httpclient:set_connect(connect)
	if not self._client then
		return 1
	end

	return self._client:set_connect(connect == true)
end


-- @brief 设置读/写/idle 超时(毫秒; 0=关闭)
-- @param [in]      read_or_opts[number|table]	读超时, 或 `{read=,write=,idle=}`
-- @param [in]      write_ms[number]			[可选] 写超时
-- @param [in]      idle_ms[number]				[可选] 空闲超时
-- @return [number]	0 成功; 非0 为 klb_netcode_e
function httpclient:set_timeout(read_or_opts, write_ms, idle_ms)
	if not self._client or not self._client.set_timeout then
		return 1
	end

	return self._client:set_timeout(read_or_opts, write_ms, idle_ms)
end


-- @brief 设置响应体上限(字节; 0=不限制)
-- @param [in]      max_body[number]			上限
-- @return [number]	0 成功; 非0 为 klb_netcode_e
function httpclient:set_max_body(max_body)
	if not self._client or not self._client.set_max_body then
		return 1
	end

	return self._client:set_max_body(max_body)
end


-- @brief 明文连接就地升级为 TLS (正向代理隧道)
-- @return [number]	0 成功; 非0 为 klb_netcode_e
-- @note 须已发 CONNECT 且读缓存已空; fd 不变, 原 socket 换为 TLS
function httpclient:upgrade_tls()
	if not self._client then
		return 1
	end

	return self._client:upgrade_tls()
end


-- @brief 发送 HTTP 文本
-- @param [in]      head[string]		HTTP 头
-- @param [in]      body[string]		[可选] HTTP 体
-- @return [number]	0 成功; 非0 失败
-- @note 须在 kco 协程内配合 co_recv
function httpclient:send(head, body)
	if not self._client then
		return 1
	end

	return self._client:send(head, body)
end


-- @brief 发送 HTTP 头 + 本地文件体
-- @param [in]      head[string]		HTTP 头 (含 Content-Length)
-- @param [in]      path[string]		本地文件路径
-- @param [in]      opts[table]			[可选] offset/length
-- @return [number]	0 成功; 非0 失败
-- @note 须在 kco 协程内配合 co_recv
function httpclient:send_file(head, path, opts)
	if not self._client then
		return 1
	end

	return self._client:send_file(head, path, opts)
end


-- @brief 接收 HTTP 文本
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; error 时无第三返回
-- @note 须在 kco 协程内; error 描述串用 knetcode.strerror(code)
function httpclient:co_recv()
	if not self._client then
		return "error", 1
	end

	return self._client:co_recv()
end


-- @brief 接收至 HTTP 文本; 跳过连接成功/写缓存空
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; error 时无第三返回
-- @note 须在 kco 协程内; 忽略 KLB_SOCKET_CONNECT(63) 与 KLB_NETCODE_WBUF_EMPTY(67536)
function httpclient:co_recv_text()
	while true do
		local msg, head, body = self:co_recv()
		if "text" == msg then
			return "text", head or "", body or ""
		elseif "exit" == msg then
			return "exit", "", ""
		elseif "error" == msg then
			local code = tonumber(head) or 1
			if 63 ~= code and 67536 ~= code then
				return "error", code
			end
		else
			return "error", 1
		end
	end
end


--------------------------------------------------------------------------------------------
-- httpclienter

local httpclienter = {}


--------------------------------------------------------------------------------------------
-- keep-alive 连接池 (进程内; 按目标分组)
-- 说明: 复用的只是 TCP/TLS 连接, 协议语义仍是一次一请求, 由调用方按需归还

local KEEP_IDLE_MS = 30000			-- 空闲超时: 超过则关闭
local KEEP_POOL_MAX = 64			-- 全局空闲连接上限
local KEEP_PER_KEY_MAX = 4			-- 单 key 空闲连接上限

local pool = {}
local pool_count = 0


-- @brief 连接池键; tls/主机/端口/代理 均相同才可复用
-- @param [in]      tls[boolean]		目标是否 TLS
-- @param [in]      host[string]		目标主机
-- @param [in]      port[string|number]	目标端口
-- @param [in]      proxy[table|nil]	代理表
-- @return [string]	池键
httpclienter.pool_key = function (tls, host, port, proxy)
	local pk = "-"
	if "table" == type(proxy) then
		pk = string.format("%s:%s:%s", tostring(proxy.host), tostring(proxy.port), tostring(proxy.tls))
	end

	return string.format("%s|%s|%s|%s", tls and "tls" or "plain", tostring(host), tostring(port), pk)
end


-- @brief 取一个空闲连接; 无可用或已过期则 nil
-- @param [in]      key[string]		池键
-- @return [table|nil]	已连接的 httpclient
httpclienter.pool_take = function (key)
	local q = pool[key]
	if "table" ~= type(q) then
		return nil
	end

	local now = now_ms()
	while 0 < #q do
		local item = table.remove(q)
		pool_count = pool_count - 1

		if 0 == #q then
			pool[key] = nil
		end

		if 0 < KEEP_IDLE_MS and KEEP_IDLE_MS < now - item.idle then
			item.client:disconnect()
		else
			return item.client
		end
	end

	return nil
end


-- @brief 归还空闲连接; 超上限则直接关闭
-- @param [in]      key[string]		池键
-- @param [in]      client[table]		已连接的 httpclient
-- @return stored[boolean]				true 已入池; false 已关闭
httpclienter.pool_put = function (key, client)
	if nil == client then
		return false
	end

	local q = pool[key]
	if "table" == type(q) and KEEP_PER_KEY_MAX <= #q then
		client:disconnect()
		return false
	end

	if KEEP_POOL_MAX <= pool_count then
		client:disconnect()
		return false
	end

	if "table" ~= type(q) then
		q = {}
		pool[key] = q
	end

	q[#q + 1] = { client = client, idle = now_ms() }
	pool_count = pool_count + 1

	return true
end


-- @brief 关闭全部空闲连接
httpclienter.pool_close = function ()
	for key, q in pairs(pool) do
		while 0 < #q do
			local item = table.remove(q)
			item.client:disconnect()
		end

		pool[key] = nil
	end

	pool_count = 0
end


-- @brief opts.headers 中 connection 头值; 无则 nil
-- @param [in]      opts[table|nil]		调用选项
-- @return [string|nil]				调用方给出的 Connection 值
httpclienter.conn_value = function (opts)
	local headers = "table" == type(opts) and opts.headers or nil
	if "table" ~= type(headers) then
		return nil
	end

	for name, value in pairs(headers) do
		if "connection" == string.lower(tostring(name)) then
			return tostring(value)
		end
	end

	return nil
end


-- @brief 本次请求是否启用持久连接
-- @param [in]      opts[table|nil]		调用选项
-- @return keep[boolean]				true 可复用连接池
-- @note 须 opts.keep_alive=true; 且调用方未用 Connection: close 显式关闭
httpclienter.keep_alive = function (opts)
	if "table" ~= type(opts) or true ~= opts.keep_alive then
		return false
	end

	local cv = httpclienter.conn_value(opts)
	if nil ~= cv and nil == string.find(string.lower(cv), "keep%-alive") then
		return false
	end

	return true
end


-- @brief 回应是否允许保持连接
-- @param [in]      head[string|nil]	HTTP 回应头文本
-- @return keep[boolean]
-- @note 无 Connection 头时按 HTTP/1.1 默认持久
httpclienter.resp_keep_alive = function (head)
	if "string" ~= type(head) then
		return false
	end

	local major, minor = string.match(head, "^HTTP/(%d+)%.(%d+)")
	if nil == major then
		return false
	end

	local def_keep = ("1" == major and "1" == minor)

	for line in string.gmatch(head, "[^\r\n]+") do
		local v = string.match(line, "^[Cc][Oo][Nn][Nn][Ee][Cc][Tt][Ii][Oo][Nn]:%s*(.+)$")
		if v then
			return nil ~= string.find(string.lower(v), "keep%-alive")
		end
	end

	return def_keep
end


-- @brief 新建并连接; 有代理时经代理
-- @param [in]      host[string]		目标主机
-- @param [in]      port[number(int)]	目标端口
-- @param [in]      tls[boolean]		目标是否 TLS
-- @param [in]      proxy[table|nil]	代理表
-- @return client[table|nil]			已连接 httpclient; 失败为 nil
-- @return code[number]					失败时为 klb_netcode_e
httpclienter.open = function (host, port, tls, proxy)
	local client = httpclienter.new()
	local rc

	if proxy then
		rc = client:connect_proxy(host, port, tls, proxy)
	else
		rc = client:connect(host, port, { tls = tls })
	end

	if 0 ~= rc then
		client:disconnect()
		return nil, rc or 1
	end

	return client
end


-- @brief 新建一个 HTTP 客户端
httpclienter.new = function (cfg)
	local obj = {
		_client = nil,						-- C 提供的客户端连接
	}

	setmetatable(obj, {
		__index = httpclient,
		__tostring = function(self)
			return tostring(self._client)
		end
	})

	return obj
end


-- @brief 校验 opts.proxy; 合法时返回规范化代理表
-- @param [in]      opts[table]		调用选项; 读 opts.proxy
-- @return [table|nil]	`{ host[string], port[number(int)], tls[boolean] }`; 无或非法为 nil
-- @note host 非空 string 且 port 在 1..65535 才算合法
httpclienter.pick_proxy = function (opts)
	if 'table' ~= type(opts) then
		return nil
	end

	local p = opts.proxy
	if 'table' ~= type(p) then
		return nil
	end

	if 'string' ~= type(p.host) or '' == p.host then
		return nil
	end

	local port = tonumber(p.port)
	if not port or port < 1 or port > 65535 then
		return nil
	end

	return { host = p.host, port = port, tls = p.tls == true }
end


-- @brief 注入 Authorization 与 Cookie; 调用方已设 Authorization 则不覆盖
-- @param [in]      opts[table|nil]		原 opts
-- @param [in]      u[table]			kurl.parse 结果
-- @param [in]      secure[boolean]		目标是否 TLS (secure cookie 只在 TLS 发)
-- @return [table|nil]					可能带 headers 的 opts
-- @note 认证优先级见 auth_value; cookie 默认启用, opts.cookie=false 关闭
httpclienter.opts_prepare = function (opts, u, secure)
	local headers = "table" == type(opts) and opts.headers or nil
	local auth = nil
	if not has_authorization(headers) then
		auth = auth_value(opts, u)
	end

	local ck = nil
	if not ("table" == type(opts) and false == opts.cookie) then
		ck = cookie_value((u and u["host"]) or "", (u and u["path"]) or "/", secure == true)
	end

	if nil == auth and nil == ck then
		return opts
	end

	local out = {}
	if "table" == type(opts) then
		for k, v in pairs(opts) do
			out[k] = v
		end
	end

	local h = {}
	if "table" == type(out.headers) then
		for k, v in pairs(out.headers) do
			h[k] = v
		end
	end

	if auth then
		h["Authorization"] = auth
	end

	if ck then
		h["Cookie"] = ck
	end

	out.headers = h
	return out
end


-- @brief 收存响应 Set-Cookie (供后续同主机请求复用)
-- @param [in]      host[string]		主机
-- @param [in]      head[string|nil]		HTTP 头文本
-- @param [in]      opts[table|nil]		原 opts; opts.cookie=false 时不存
httpclienter.store_cookies = function (host, head, opts)
	store_cookies(host, head, opts)
end


-- @brief 请求行目标; 经代理的 http 改用绝对 URI, 其余 origin-form
-- @param [in]      target[string]		origin-form 目标 (path + query)
-- @param [in]      u[table]			kurl.parse 结果
-- @param [in]      proxy[table]		代理表; nil 表示直连
-- @param [in]      tls[boolean]		目标是否 TLS
-- @return [string]	用于请求行的目标
-- @note https 已建隧道, 仍用 origin-form
httpclienter.proxy_target = function (target, u, proxy, tls)
	if not proxy or tls then
		return target
	end

	local authority = u['host'] or ''
	if u['port'] then
		authority = authority .. ':' .. tostring(u['port'])
	end

	return string.format('http://%s%s', authority, target)
end


return httpclienter
