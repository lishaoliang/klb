--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbws init.lua
-- @note   WebSocket 协议脚本封装; 内部使用 kws
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local wsclienter = require("klbcore.klbws.client.wsclienter")
local wslistener = require("klbcore.klbws.serve.wslistener")


local klbws = {}


--------------------------------------------------------------------------------------------
-- 客户端

-- @brief 新建一个客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
klbws.new_client = function (cfg)
	return wsclienter.new(cfg)
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      path[string]		[可选] 握手路径, 默认 `/`; 也可为 opts table
-- @param [in]      opts[table]			[可选] 连接选项; 见内联 opts 注释
-- @return [table]	客户端模块
klbws.connect = function (host, port, path, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	-- }
	local client = wsclienter.new()

	client:connect(host, port, path, opts)

	return client
end


--------------------------------------------------------------------------------------------
-- 服务端

-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			监听模块配置
-- @return [table]	监听模块
klbws.new_listen = function (cfg)
	return wslistener.new(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置; 见内联 opts 注释
-- @return [table]	监听模块
klbws.listen = function (port, cfg)
	-- cfg = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM
	--   key[string]			tls 时必填, 私钥 PEM
	-- }
	-- 新建
	local l = wslistener.new(cfg)

	-- 打开端口
	l:open(port)

	-- 返回 监听模块
	return l
end


return klbws
