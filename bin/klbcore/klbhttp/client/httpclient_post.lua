--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   httpclient_post.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP 客户端一次性 POST
-- @note   须在 kco 协程内; 内部使用 httpclienter
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kurl = require("kurl")
local stringex = require("klbcore.util.stringex")
local cjson = require("cjson.safe")
local xmlparser = require("klbcore.util.xmlparser")
local xml = require("klbcore.util.LuaXml")
local httpclienter = require("klbcore.klbhttp.client.httpclienter")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现

-- @brief 组装请求目标 (path + query)
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


-- @brief 头名或头值可写入请求头
-- @note 须为非空 string, 且不含 CR/LF
local function header_text_ok(s)
	if "string" ~= type(s) or "" == s then
		return false
	end

	if nil ~= string.find(s, "\r", 1, true) or nil ~= string.find(s, "\n", 1, true) then
		return false
	end

	return true
end


-- @brief 这些头由本函数写, opts.headers 里同名项丢弃
local function is_owned_header(key)
	if "host" == key then
		return true
	elseif "content-length" == key then
		return true
	elseif "content-type" == key then
		return true
	end

	return false
end


-- @brief 收集可写入的 opts.headers 键, 按头名排序
local function collect_header_names(opts)
	local names = {}
	if "table" ~= type(opts) or "table" ~= type(opts.headers) then
		return names
	end

	for name, value in pairs(opts.headers) do
		if header_text_ok(name) and header_text_ok(value) then
			names[#names + 1] = name
		end
	end

	table.sort(names)
	return names
end


-- @brief User-Agent; headers 中有则替换, 默认 `KLB`
local function pick_user_agent(names, opts)
	local headers = opts and opts.headers
	for i = 1, #names do
		local name = names[i]
		if "user-agent" == string.lower(name) then
			return headers[name]
		end
	end

	return "KLB"
end


-- @brief 追加调用方请求头; 跳过 User-Agent 与本函数拥有的头
local function append_caller_headers(t, names, opts)
	local headers = opts.headers
	for i = 1, #names do
		local name = names[i]
		local key = string.lower(name)
		if "user-agent" ~= key and not is_owned_header(key) then
			table.insert(t, string.format("%s: %s\r\n", name, headers[name]))
		end
	end
end


-- @brief 组装 POST 请求头
local function pack_post_head(host, port, target, body, opts)
	local content_type = 'application/octet-stream'
	if opts and 'string' == type(opts.content_type) and '' ~= opts.content_type then
		content_type = opts.content_type
	end

	local names = collect_header_names(opts)
	local has_conn = false
	for i = 1, #names do
		if "connection" == string.lower(names[i]) then
			has_conn = true
		end
	end

	local t = {}

	table.insert(t, string.format('POST %s HTTP/1.1\r\n', target))
	table.insert(t, string.format('Host: %s\r\n', pack_host(host, port)))
	table.insert(t, string.format('User-Agent: %s\r\n', pick_user_agent(names, opts)))
	table.insert(t, string.format('Content-Type: %s\r\n', content_type))
	table.insert(t, string.format('Content-Length: %d\r\n', #body))
	if 0 < #names then
		append_caller_headers(t, names, opts)
	end

	if not has_conn then
		if opts and true == opts.keep_alive then
			table.insert(t, 'Connection: keep-alive\r\n')
		else
			table.insert(t, 'Connection: close\r\n')
		end
	end

	table.insert(t, '\r\n')

	return table.concat(t)
end


--------------------------------------------------------------------------------------------
-- httpclient_post

local httpclient_post = {}


-- @brief 协程内发起一次 HTTP POST
-- @param [in]      url[string]			目标 URL; eg. `http://127.0.0.1/` 或 `https://host/path`
-- @param [in]      data[string]		[可选] POST 体; 非 string 时按空串
-- @param [in]      opts[table]			[可选] 见内联 opts 注释
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; 失败时为 ""
-- @note 须在 kco 协程内; https 走 TLS (默认端口 443), http 明文 (默认端口 80)
-- @note Host, Connection, Content-Type, Content-Length 由本函数写; 调用方给出 Connection 头时以其为准
httpclient_post.co_post = function (url, data, opts)
	-- opts = {
	--   content_type[string]		[可选] Content-Type, 默认 `application/octet-stream`
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	--   proxy[table]			[可选] 正向代理 `{ host, port, tls }`; 省略则直连
	--   auth[table]				[可选] `{ type="basic"|"bearer", user, pass, token }`
	--   token[string]			[可选] Bearer 令牌; opts.auth 优先
	--   cookie[boolean]			[可选] 默认 true; false 关闭 cookie jar
	--   keep_alive[boolean]		[可选] 默认 false; true 时复用进程内空闲连接
	-- }
	-- eg. http://username:password@127.0.0.1:8080/test/test.aspx?name=sviergn&x=true#stuff
	local u = kurl.parse(tostring(url))

	local schema = u['schema'] or 'http'
	local host = u['host'] or ''
	local tls = false

	if stringex.cmp_ignore_case('https', schema) then
		tls = true
	end

	opts = httpclienter.opts_prepare(opts, u, tls)

	local port = '80'
	if tls then
		port = '443'
	end

	if u['port'] then
		port = u['port']
	end

	local body = ''
	if 'string' == type(data) then
		body = data
	end

	local proxy = httpclienter.pick_proxy(opts)
	local keep = httpclienter.keep_alive(opts)
	local key = keep and httpclienter.pool_key(tls, host, port, proxy) or nil
	local httpclient = nil
	local reused = false

	-- step1. 连接; keep-alive 先取空闲连接, 无则新建
	if keep then
		httpclient = httpclienter.pool_take(key)
		reused = nil ~= httpclient
	end

	if not httpclient then
		local code
		httpclient, code = httpclienter.open(host, tonumber(port), tls, proxy)
		if not httpclient then
			return 'error', code, ''
		end
	end

	-- step2. 发送 POST; 经代理的 http 目标改用绝对 URI
	local target = httpclienter.proxy_target(pack_request_target(u), u, proxy, tls)
	local head_req = pack_post_head(host, port, target, body, opts)

	local function exchange()
		local r = httpclient:send(head_req, body)
		if 0 ~= r then
			return 'error', r, ''
		end

		return httpclient:co_recv_text()
	end

	local msg, head, recv_body = exchange()

	-- 复用连接可能已被对端关闭: 重开一次再试
	if 'text' ~= msg and reused then
		httpclient:disconnect()
		reused = false

		local code
		httpclient, code = httpclienter.open(host, tonumber(port), tls, proxy)
		if not httpclient then
			return 'error', code, ''
		end

		msg, head, recv_body = exchange()
	end

	-- step3. 收尾; 可复用则归还连接池, 否则关闭
	if 'text' == msg then
		httpclienter.store_cookies(host, head, opts)
		if keep and httpclienter.resp_keep_alive(head) and httpclienter.pool_put(key, httpclient) then
			return 'text', head or '', recv_body or ''
		end

		httpclient:disconnect()
		return 'text', head or '', recv_body or ''
	end

	httpclient:disconnect()

	if 'exit' == msg then
		return 'exit', '', ''
	end

	return 'error', head or 1, ''
end


-- @brief 未写 content_type 时补上默认值; 不改调用方的 opts
local function opts_with_content_type(opts, content_type)
	local out = {}
	if "table" == type(opts) then
		for k, v in pairs(opts) do
			out[k] = v
		end
	end

	if "string" ~= type(out.content_type) or "" == out.content_type then
		out.content_type = content_type
	end

	return out
end


-- @brief 将回应体解码为 JSON table
-- @return msg, head, body, obj[table], err[string]
local function decode_json_body(msg, head, body)
	if "text" ~= msg then
		if "string" ~= type(body) then
			body = ""
		end

		return msg, head, body, {}, ""
	end

	local obj = cjson.decode(body or "")
	if "table" ~= type(obj) then
		return msg, head or "", body or "", {}, "invalid json"
	end

	return msg, head or "", body or "", obj, ""
end


-- @brief 将回应体解码为 XML table
-- @return msg, head, body, obj[table], err[string]
-- @note 仅适合近似 JSON 的 XML. 非法文本 err 为 "invalid xml"
local function decode_xml_body(msg, head, body)
	if "text" ~= msg then
		if "string" ~= type(body) then
			body = ""
		end

		return msg, head, body, {}, ""
	end

	local ok, node = pcall(xml.eval, body or "")
	if not ok or "table" ~= type(node) then
		return msg, head or "", body or "", {}, "invalid xml"
	end

	return msg, head or "", body or "", xmlparser.to_table(node), ""
end


-- @brief 协程内 POST 一个 JSON table, 并把回应体解码为 table
-- @param [in]      url[string]			目标 URL
-- @param [in]      obj[table]			请求体 table
-- @param [in]      opts[table]			[可选] 见内联 opts 注释
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					原始回应体; 失败时为 ""
-- @return obj[table]					回应解码结果; 失败为 {}
-- @return err[string]					编码或解码失败时为 "invalid json"; 否则 ""
-- @note 须在 kco 协程内. Content-Type 默认 application/json. opts.content_type 非空则沿用
httpclient_post.co_post_json = function (url, obj, opts)
	-- opts = {
	--   content_type[string]		[可选] Content-Type, 默认 `application/json`
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	-- }
	if "table" ~= type(obj) then
		return "error", 1, "", {}, "invalid json"
	end

	local data = cjson.encode(obj)
	if "string" ~= type(data) then
		return "error", 1, "", {}, "invalid json"
	end

	local msg, head, body = httpclient_post.co_post(url, data, opts_with_content_type(opts, "application/json"))
	return decode_json_body(msg, head, body)
end


-- @brief 协程内 POST 一个 XML table, 并把回应体解码为 table
-- @param [in]      url[string]			目标 URL
-- @param [in]      obj[table]			请求体 table
-- @param [in]      opts[table]			[可选] 见内联 opts 注释
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					原始回应体; 失败时为 ""
-- @return obj[table]					回应解码结果; 失败为 {}
-- @return err[string]					编码或解码失败时为 "invalid xml"; 否则 ""
-- @note 须在 kco 协程内. Content-Type 默认 application/xml. 仅适合近似 JSON 的 XML
httpclient_post.co_post_xml = function (url, obj, opts)
	-- opts = {
	--   content_type[string]		[可选] Content-Type, 默认 `application/xml`
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	-- }
	if "table" ~= type(obj) then
		return "error", 1, "", {}, "invalid xml"
	end

	local data = xmlparser.to_xml_str(obj)
	if "string" ~= type(data) or "" == data then
		return "error", 1, "", {}, "invalid xml"
	end

	local msg, head, body = httpclient_post.co_post(url, data, opts_with_content_type(opts, "application/xml"))
	return decode_xml_body(msg, head, body)
end


-- @brief 标量可写入 form
local function form_scalar_ok(v)
	local tv = type(v)
	if "string" == tv or "number" == tv or "boolean" == tv then
		return true
	end

	return false
end


-- @brief 同名多值须为连续数组, 元素为标量
local function form_list_ok(v)
	local n = #v
	local count = 0
	for k, item in pairs(v) do
		count = count + 1
		if "number" ~= type(k) or k < 1 or k > n or k ~= math.floor(k) then
			return false
		end

		if not form_scalar_ok(item) then
			return false
		end
	end

	if count ~= n then
		return false
	end

	return true
end


-- @brief 将 form 表编成 k=v&k2=v2
-- @return [string|nil]	非法字段时为 nil; 空表为 ""
local function pack_form(form)
	if "table" ~= type(form) then
		return nil
	end

	local keys = {}
	for k, v in pairs(form) do
		if nil ~= v then
			if not form_scalar_ok(k) or "" == tostring(k) then
				return nil
			end

			keys[#keys + 1] = k
		end
	end

	table.sort(keys, function (a, b)
		return tostring(a) < tostring(b)
	end)

	local parts = {}
	for i = 1, #keys do
		local k = keys[i]
		local v = form[k]
		if form_scalar_ok(v) then
			parts[#parts + 1] = stringex.url_encode(tostring(k)) .. "=" .. stringex.url_encode(tostring(v))
		elseif "table" == type(v) then
			if not form_list_ok(v) then
				return nil
			end

			for j = 1, #v do
				parts[#parts + 1] = stringex.url_encode(tostring(k)) .. "=" .. stringex.url_encode(tostring(v[j]))
			end
		else
			return nil
		end
	end

	return table.concat(parts, "&")
end


-- @brief 协程内 POST, 将 form 表写成 urlencoded 体
-- @param [in]      url[string]			目标 URL
-- @param [in]      form[table]			一层键值; 值为数组时同名多值
-- @param [in]      opts[table]			[可选] 见内联 opts 注释
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; 失败时为 ""
-- @note 须在 kco 协程内. Content-Type 默认 application/x-www-form-urlencoded. 非法字段不发请求
httpclient_post.co_post_form = function (url, form, opts)
	-- opts = {
	--   content_type[string]		[可选] Content-Type, 默认 `application/x-www-form-urlencoded`
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	-- }
	local data = pack_form(form)
	if nil == data then
		return "error", 1, ""
	end

	return httpclient_post.co_post(url, data, opts_with_content_type(opts, "application/x-www-form-urlencoded"))
end


return httpclient_post
