--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbmnp init.lua
-- @note   MNP 协议脚本封装; 内部使用 kmnp
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local mnpclienter = require("klbcore.klbmnp.client.mnpclienter")
local mnplistener = require("klbcore.klbmnp.serve.mnplistener")


local klbmnp = {}


--------------------------------------------------------------------------------------------
-- 客户端

-- @brief 新建一个客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
klbmnp.new_client = function (cfg)
	return mnpclienter.new(cfg)
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @return [table]	客户端模块
klbmnp.connect = function (host, port)
	local client = mnpclienter.new()

	client:connect(host, port)

	return client
end


--------------------------------------------------------------------------------------------
-- 服务端

-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			监听模块配置
-- @return [table]	监听模块
klbmnp.new_listen = function (cfg)
	return mnplistener.new(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置
-- @return [table]	监听模块
klbmnp.listen = function (port, cfg)
	-- 新建
	local l = mnplistener.new(cfg)

	-- 打开端口
	l:open(port)

	-- 返回 监听模块
	return l
end


return klbmnp
