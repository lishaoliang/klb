--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   httpclient_co_get.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP 客户端一次性 GET
-- @note   须在 kco 协程内; 内部使用 httpclienter
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kurl = require("kurl")
local stringex = require("klbcore.util.stringex")
local httpclienter = require("klbcore.klbhttp.client.httpclienter")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现

-- @brief 组装 GET 请求目标 (path + query)
local function pack_request_target(u)
	local path = u['path'] or '/'
	if '' == path then
		path = '/'
	end

	local query = u['query']
	if query and '' ~= query then
		return path .. '?' .. query
	end

	return path
end


-- @brief Host 头; 默认 80/443 不带端口
local function pack_host(host, port)
	local p = tostring(port)
	if '80' == p or '443' == p then
		return host
	end

	return host .. ':' .. p
end


-- @brief 组装 GET 请求头
local function pack_get_head(host, port, target)
	local t = {}

	table.insert(t, string.format('GET %s HTTP/1.1\r\n', target))
	table.insert(t, string.format('Host: %s\r\n', pack_host(host, port)))
	table.insert(t, 'User-Agent: KLB\r\n')
	table.insert(t, 'Connection: close\r\n')
	table.insert(t, '\r\n')

	return table.concat(t)
end


--------------------------------------------------------------------------------------------
-- httpclient_co_get

local httpclient_co_get = {}


-- @brief 协程内发起一次 HTTP GET
-- @param [in]      url[string]			目标 URL; eg. `http://127.0.0.1/` 或 `https://host/path`
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; 失败时为 ""
-- @note 须在 kco 协程内; https 走 TLS (默认端口 443), http 明文 (默认端口 80)
httpclient_co_get.co_get = function (url)
	-- eg. http://username:password@127.0.0.1:8080/test/test.aspx?name=sviergn&x=true#stuff
	local u = kurl.parse(tostring(url))

	local schema = u['schema'] or 'http'
	local host = u['host'] or ''
	local tls = false

	if stringex.cmp_ignore_case('https', schema) then
		tls = true
	end

	local port = '80'
	if tls then
		port = '443'
	end

	if u['port'] then
		port = u['port']
	end

	local httpclient = httpclienter.new()

	local function finish(msg, head, body)
		httpclient:disconnect()
		return msg, head, body
	end

	-- step1. 连接; https 使用 TLS
	local rc = httpclient:connect(host, tonumber(port), { tls = tls })
	if 0 ~= rc then
		return finish('error', rc, '')
	end

	-- step2. 发送 GET
	rc = httpclient:send(pack_get_head(host, port, pack_request_target(u)))
	if 0 ~= rc then
		return finish('error', rc, '')
	end

	-- step3. 接收回应; 跳过 connect/wbuf 状态码
	local msg, head, body = httpclient:co_recv_text()
	if 'text' == msg then
		return finish('text', head or '', body or '')
	elseif 'exit' == msg then
		return finish('exit', '', '')
	end

	return finish('error', head or 1, '')
end


return httpclient_co_get
