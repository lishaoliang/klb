--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbhttp init.lua
-- @note   HTTP 协议脚本封装; 内部使用 khttp
-- @history 修改历史
--		[2026-09] 创建文件
--		[2026-10] proxy_connect / is_request_byte (proxy L0-2/3)
--]]
local knetcode = require("knetcode")
local httpclienter = require("klbcore.klbhttp.client.httpclienter")
local httpclientrpcer = require("klbcore.klbhttp.client.httpclientrpcer")
local httpclient_get = require("klbcore.klbhttp.client.httpclient_get")
local httpclient_post = require("klbcore.klbhttp.client.httpclient_post")
local httplistener = require("klbcore.klbhttp.serve.httplistener")
local proxy_connect_client = require("klbcore.klbhttp.proxy_connect.client")
local proxy_connect_server = require("klbcore.klbhttp.proxy_connect.server")
local http_status = require("klbcore.klbhttp.status")


local klbhttp = {}


--------------------------------------------------------------------------------------------
-- HTTP 状态码 (供其他模块; 真源 status.lua)

-- 1xx
klbhttp.STATUS_CONTINUE = http_status.CONTINUE
klbhttp.STATUS_SWITCHING_PROTOCOLS = http_status.SWITCHING_PROTOCOLS

-- 2xx
klbhttp.STATUS_OK = http_status.OK
klbhttp.STATUS_CREATED = http_status.CREATED
klbhttp.STATUS_ACCEPTED = http_status.ACCEPTED
klbhttp.STATUS_NO_CONTENT = http_status.NO_CONTENT
klbhttp.STATUS_PARTIAL_CONTENT = http_status.PARTIAL_CONTENT

-- 3xx
klbhttp.STATUS_MOVED_PERMANENTLY = http_status.MOVED_PERMANENTLY
klbhttp.STATUS_FOUND = http_status.FOUND
klbhttp.STATUS_SEE_OTHER = http_status.SEE_OTHER
klbhttp.STATUS_NOT_MODIFIED = http_status.NOT_MODIFIED
klbhttp.STATUS_TEMPORARY_REDIRECT = http_status.TEMPORARY_REDIRECT
klbhttp.STATUS_PERMANENT_REDIRECT = http_status.PERMANENT_REDIRECT

-- 4xx
klbhttp.STATUS_BAD_REQUEST = http_status.BAD_REQUEST
klbhttp.STATUS_UNAUTHORIZED = http_status.UNAUTHORIZED
klbhttp.STATUS_FORBIDDEN = http_status.FORBIDDEN
klbhttp.STATUS_NOT_FOUND = http_status.NOT_FOUND
klbhttp.STATUS_METHOD_NOT_ALLOWED = http_status.METHOD_NOT_ALLOWED
klbhttp.STATUS_PROXY_AUTH_REQUIRED = http_status.PROXY_AUTH_REQUIRED
klbhttp.STATUS_REQUEST_TIMEOUT = http_status.REQUEST_TIMEOUT
klbhttp.STATUS_CONFLICT = http_status.CONFLICT
klbhttp.STATUS_GONE = http_status.GONE
klbhttp.STATUS_CONTENT_TOO_LARGE = http_status.CONTENT_TOO_LARGE
klbhttp.STATUS_UNSUPPORTED_MEDIA_TYPE = http_status.UNSUPPORTED_MEDIA_TYPE
klbhttp.STATUS_RANGE_NOT_SATISFIABLE = http_status.RANGE_NOT_SATISFIABLE
klbhttp.STATUS_TOO_MANY_REQUESTS = http_status.TOO_MANY_REQUESTS

-- 5xx
klbhttp.STATUS_INTERNAL_SERVER_ERROR = http_status.INTERNAL_SERVER_ERROR
klbhttp.STATUS_NOT_IMPLEMENTED = http_status.NOT_IMPLEMENTED
klbhttp.STATUS_BAD_GATEWAY = http_status.BAD_GATEWAY
klbhttp.STATUS_SERVICE_UNAVAILABLE = http_status.SERVICE_UNAVAILABLE
klbhttp.STATUS_GATEWAY_TIMEOUT = http_status.GATEWAY_TIMEOUT

klbhttp.status = http_status


--------------------------------------------------------------------------------------------
-- 正向代理 CONNECT (脚本库; 供 alua proxy / 探活)

klbhttp.proxy_connect = {
	client = proxy_connect_client,
	server = proxy_connect_server,
}


--------------------------------------------------------------------------------------------
-- 客户端

-- @brief 新建一个客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
klbhttp.new_client = function (cfg)
	return httpclienter.new(cfg)
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] 连接选项; 见内联 opts 注释
-- @return [table]	客户端模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e; 描述串用 knetcode.strerror(code)
klbhttp.connect = function (host, port, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	-- }
	local client = httpclienter.new()
	local rc = client:connect(host, port, opts)
	if 0 ~= rc then
		return nil, client._last_code or rc
	end

	return client
end


-- @brief 协程内发起一次 HTTP GET
-- @param [in]      url[string]			目标 URL; eg. `http://127.0.0.1/` 或 `https://host/path`
-- @param [in]      opts[table]			[可选] 见内联 opts 注释
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; 失败时为 ""
-- @note 须在 kco 协程内; https 走 TLS (默认端口 443), http 明文 (默认端口 80)
klbhttp.co_get = function (url, opts)
	-- opts = {
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	--   proxy[table]			[可选] 正向代理 `{ host, port, tls }`; 省略则直连
	-- }
	return httpclient_get.co_get(url, opts)
end


-- @brief 协程内发起一次 HTTP POST
-- @param [in]      url[string]			目标 URL; eg. `http://127.0.0.1/` 或 `https://host/path`
-- @param [in]      data[string]		[可选] POST 体; 非 string 时按空串
-- @param [in]      opts[table]			[可选] 见内联 opts 注释
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; 失败时为 ""
-- @note 须在 kco 协程内; https 走 TLS (默认端口 443), http 明文 (默认端口 80)
klbhttp.co_post = function (url, data, opts)
	-- opts = {
	--   content_type[string]		[可选] Content-Type, 默认 `application/octet-stream`
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	--   proxy[table]			[可选] 正向代理 `{ host, port, tls }`; 省略则直连
	-- }
	return httpclient_post.co_post(url, data, opts)
end

--------------------------------------------------------------------------------------------
-- form

-- @brief 协程内 GET, 将 form 表写入 URL query
-- @param [in]      url[string]			目标 URL
-- @param [in]      form[table]			一层键值; 值为数组时同名多值
-- @param [in]      opts[table]			[可选] 见内联 opts 注释
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; 失败时为 ""
-- @note 须在 kco 协程内. 服务端读 req.query. 非法字段不发请求
klbhttp.co_get_form = function (url, form, opts)
	-- opts = {
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	-- }
	return httpclient_get.co_get_form(url, form, opts)
end


-- @brief 协程内 POST, 将 form 表写成 urlencoded 体
-- @param [in]      url[string]			目标 URL
-- @param [in]      form[table]			一层键值; 值为数组时同名多值
-- @param [in]      opts[table]			[可选] 见内联 opts 注释
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; 失败时为 ""
-- @note 须在 kco 协程内. Content-Type 默认 application/x-www-form-urlencoded. 服务端读 req.form
klbhttp.co_post_form = function (url, form, opts)
	-- opts = {
	--   content_type[string]		[可选] Content-Type, 默认 `application/x-www-form-urlencoded`
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	-- }
	return httpclient_post.co_post_form(url, form, opts)
end

--------------------------------------------------------------------------------------------
-- json

-- @brief 协程内 GET, 并将回应体解码为 JSON table
-- @param [in]      url[string]			目标 URL
-- @param [in]      opts[table]			[可选] 见内联 opts 注释
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					原始回应体; 失败时为 ""
-- @return obj[table]					解码结果; 失败为 {}
-- @return err[string]					解码失败时为 "invalid json"; 否则 ""
-- @note 须在 kco 协程内. 传输失败时 err 为 ""
klbhttp.co_get_json = function (url, opts)
	-- opts = {
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	-- }
	return httpclient_get.co_get_json(url, opts)
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
-- @note 须在 kco 协程内. Content-Type 默认 application/json
klbhttp.co_post_json = function (url, obj, opts)
	-- opts = {
	--   content_type[string]		[可选] Content-Type, 默认 `application/json`
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	-- }
	return httpclient_post.co_post_json(url, obj, opts)
end


--------------------------------------------------------------------------------------------
-- xml

-- @brief 协程内 GET, 并将回应体解码为 XML table
-- @param [in]      url[string]			目标 URL
-- @param [in]      opts[table]			[可选] 见内联 opts 注释
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					原始回应体; 失败时为 ""
-- @return obj[table]					解码结果; 失败为 {}
-- @return err[string]					解码失败时为 "invalid xml"; 否则 ""
-- @note 须在 kco 协程内. 仅适合近似 JSON 的 XML
klbhttp.co_get_xml = function (url, opts)
	-- opts = {
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	-- }
	return httpclient_get.co_get_xml(url, opts)
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
klbhttp.co_post_xml = function (url, obj, opts)
	-- opts = {
	--   content_type[string]		[可选] Content-Type, 默认 `application/xml`
	--   headers[table]			[可选] 追加请求头; 键为头名, 值为 string; 省略则不追加
	-- }
	return httpclient_post.co_post_xml(url, obj, opts)
end


--------------------------------------------------------------------------------------------
-- 服务端

-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			监听模块配置
-- @return [table]	监听模块
klbhttp.new_listen = function (cfg)
	return httplistener.new(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置; 见内联 opts 注释
-- @return [table]	监听模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e; 描述串用 knetcode.strerror(code)
klbhttp.listen = function (port, cfg)
	-- cfg = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   plain[boolean]			[可选] tls 时是否同时收明文 HTTP, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM 原文或文件路径
	--   key[string]			tls 时必填, 私钥 PEM 原文或文件路径
	-- }
	-- @note 同端口 HTTP+HTTPS: `tls=true, plain=true` 一次 listen
	local l = httplistener.new(cfg)

	if not l:open(port) then
		return nil, l._last_code
	end

	return l
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC

-- @brief 新建一个RPC客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
klbhttp.new_client_rpc = function (cfg)
	return httpclientrpcer.new_rpc(cfg)
end

-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] `{ tls }`
-- @return [table]	客户端模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e; 描述串用 knetcode.strerror(code)
klbhttp.connect_rpc = function (host, port, opts)
	local client = httpclientrpcer.new_rpc()
	local rc = client:connect_rpc(host, port, opts)
	if 0 ~= rc then
		return nil, client._last_code or rc
	end

	return client
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC 一次性接口

-- @brief 发起一次 POST-RPC
-- @param [in]      url[string]			目标, eg. 'httprpc://127.0.0.1:3458'
-- @return [string]						成功为 'success'
-- @return [number]						失败时 code (首值为 nil)
-- @return [string]						失败时 knetcode.strerror(code)
klbhttp.co_post_rpc = function (url, ...)
	local r = httpclientrpcer.new_rpc()
	local rc = r:connect(url)
	if 0 ~= rc then
		return nil, rc, knetcode.strerror(rc)
	end

	local post_rc = r:post(...)
	r:co_wait()
	r:disconnect()
	if 0 ~= post_rc then
		return nil, post_rc, knetcode.strerror(post_rc)
	end
	return 'success'
end

-- @brief 发起一次 NOTIFY-RPC
-- @param [in]      url[string]			目标, eg. 'httprpc://127.0.0.1:3458'
-- @return [string]						成功为 'success'
-- @return [number]						失败时 code (首值为 nil)
-- @return [string]						失败时 knetcode.strerror(code)
klbhttp.co_notify_rpc = function (url, ...)
	local r = httpclientrpcer.new_rpc()
	local rc = r:connect(url)
	if 0 ~= rc then
		return nil, rc, knetcode.strerror(rc)
	end

	local notify_rc = r:notify(...)
	r:co_wait()
	r:disconnect()
	if 0 ~= notify_rc then
		return nil, notify_rc, knetcode.strerror(notify_rc)
	end
	return 'success'
end

-- @brief 发起一次 CALL-RPC
-- @param [in]      url[string]			目标, eg. 'httprpc://127.0.0.1:3458'
-- @return [string]						成功为 'success'
-- @return [...]						成功时回应数据; 失败时 code, errstr (首值为 nil)
-- @note 须在 kco 协程内
klbhttp.co_call_rpc = function (url, ...)
	local ReturnCoCall = function (r, ...)
		r:disconnect()
		return 'success', ...
	end

	local r = httpclientrpcer.new_rpc()
	local rc = r:connect(url)
	if 0 ~= rc then
		return nil, rc, knetcode.strerror(rc)
	end

	return ReturnCoCall(r, r:co_call(...))
end

--------------------------------------------------------------------------------------------
-- 服务端 RPC


-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			监听模块配置
-- @return [table]	监听模块
klbhttp.new_listen_rpc = function (cfg)
	return httplistener.new_rpc(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置; tls/plain/cert/key/rpctype
-- @return [table]	监听模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e; 描述串用 knetcode.strerror(code)
klbhttp.listen_rpc = function (port, cfg)
	local l = httplistener.new_rpc(cfg)

	if not l:open(port) then
		return nil, l._last_code
	end

	return l
end


--------------------------------------------------------------------------------------------
-- 一些常量

-------------------------------------------------------
-- 数据组织方式
klbhttp.RPC_LUA			= 'LUA'
klbhttp.RPC_JSON		= 'JSON'


-------------------------------------------------------
-- RPC 方法
klbhttp.RPC_POST 		= 'POST'
klbhttp.RPC_NOTIFY 		= 'NOTIFY'
klbhttp.RPC_REQUEST 	= 'REQUEST'
klbhttp.RPC_RESPONSE 	= 'RESPONSE'


return klbhttp
