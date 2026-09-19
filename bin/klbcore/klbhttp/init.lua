--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbhttp init.lua
-- @note   HTTP 协议脚本封装; 内部使用 khttp
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local httpclienter = require("klbcore.klbhttp.client.httpclienter")
local httpclient_co_get = require("klbcore.klbhttp.client.httpclient_co_get")
local httpclient_co_post = require("klbcore.klbhttp.client.httpclient_co_post")
local httplistener = require("klbcore.klbhttp.serve.httplistener")


local klbhttp = {}


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
-- @return [table]	客户端模块
klbhttp.connect = function (host, port, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	-- }
	local client = httpclienter.new()

	client:connect(host, port, opts)

	return client
end


-- @brief 协程内发起一次 HTTP GET
-- @param [in]      url[string]			目标 URL; eg. `http://127.0.0.1/` 或 `https://host/path`
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; 失败时为 ""
-- @note 须在 kco 协程内; https 走 TLS (默认端口 443), http 明文 (默认端口 80)
klbhttp.co_get = function (url)
	return httpclient_co_get.co_get(url)
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
	-- }
	return httpclient_co_post.co_post(url, data, opts)
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
-- @return [table]	监听模块; PEM 无法解析 / tls 证书无效等失败时为 nil
klbhttp.listen = function (port, cfg)
	-- cfg = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM 原文或文件路径
	--   key[string]			tls 时必填, 私钥 PEM 原文或文件路径
	-- }
	-- 新建
	local l = httplistener.new(cfg)

	-- 打开端口
	if not l:open(port) then
		return nil
	end

	-- 返回 监听模块
	return l
end


return klbhttp
