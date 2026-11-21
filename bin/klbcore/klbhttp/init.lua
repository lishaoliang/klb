--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbhttp: HTTP 协议脚本封装 (对外入口)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   require("klbcore.klbhttp"); 契约见本文件导出; 内部使用 khttp
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


local klbhttp = {}


--------------------------------------------------------------------------------------------
-- 正向代理 CONNECT (脚本库; 供 alua proxy / 探活)
-- @stability stable
-- @note HTTP 状态码真源: require("klbcore.klbcode").net.HTTP_* (或 klbcode.netcode)

klbhttp.proxy_connect = {
	client = proxy_connect_client,
	server = proxy_connect_server,
}


--------------------------------------------------------------------------------------------
-- 客户端

-- @brief 新建一个客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
-- @stability stable
klbhttp.new_client = function (cfg)
	return httpclienter.new(cfg)
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] 连接选项; 见内联 opts 注释
-- @return [table]	客户端模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e; 描述串用 knetcode.strerror(code)
-- @stability stable
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
-- @stability stable
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
-- @stability stable
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
-- @stability stable
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
-- @stability stable
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
-- @stability stable
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
-- @stability stable
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
-- @stability stable
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
-- @stability stable
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
-- @stability stable
klbhttp.new_listen = function (cfg)
	return httplistener.new(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置; 见内联 opts 注释
-- @return [table]	监听模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e; 描述串用 knetcode.strerror(code)
-- @stability stable
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
-- @stability stable
klbhttp.new_client_rpc = function (cfg)
	return httpclientrpcer.new_rpc(cfg)
end

-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] `{ tls }`
-- @return [table]	客户端模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e; 描述串用 knetcode.strerror(code)
-- @stability stable
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
-- @stability stable
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
-- @stability stable
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
-- @stability stable
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
-- @stability stable
klbhttp.new_listen_rpc = function (cfg)
	return httplistener.new_rpc(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置; tls/plain/cert/key/rpctype
-- @return [table]	监听模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e; 描述串用 knetcode.strerror(code)
-- @stability stable
klbhttp.listen_rpc = function (port, cfg)
	local l = httplistener.new_rpc(cfg)

	if not l:open(port) then
		return nil, l._last_code
	end

	return l
end


--------------------------------------------------------------------------------------------
-- 一些常量
-- @stability stable

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

-- end
