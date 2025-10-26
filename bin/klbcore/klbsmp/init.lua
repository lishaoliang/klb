--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbsmp init.lua
-- @note   SMP协议
-- @history 修改历史
--		[2025-10] 添加基础
--]]
local smpclienter =require("klbcore.klbsmp.client.smpclienter")
local smpclientrpcer =require("klbcore.klbsmp.client.smpclientrpcer")
local smplistener = require("klbcore.klbsmp.serve.smplistener")


local klbsmp = {}


--------------------------------------------------------------------------------------------
-- 客户端

-- @brief 新建一个客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
klbsmp.new_client = function (cfg)
	return smpclienter.new(cfg)
end


-- @brief 连接到目标
klbsmp.connect = function (host, port)
	local client = smpclienter.new()
	
	client:connect(host, port)
	
	return client
end


--------------------------------------------------------------------------------------------
-- 服务端

-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			监听模块配置
-- @return [table]	监听模块
klbsmp.new_listen = function (cfg)
	return smplistener.new(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置
-- @return [table]	监听模块
klbsmp.listen = function (port, cfg)
	-- 新建
	local l = smplistener.new(cfg)
	
	-- 打开端口
	l:open(port)
	
	-- 返回 监听模块
	return l
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC

-- @brief 新建一个RPC客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
klbsmp.new_client_rpc = function (cfg)
	return smpclientrpcer.new_rpc(cfg)
end

-- @brief 连接到目标
klbsmp.connect_rpc = function (host, port)
	local client = smpclientrpcer.new_rpc()
	
	client:connect_rpc(host, port)
	
	return client
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC 一次性接口

-- @brief 发起一次 POST-RPC
-- @param [in]      url[string]			目标, eg. 'smprpc://username:password@127.0.0.1:3457'
-- @return [string] 消息
klbsmp.co_post_rpc = function (url, ...)
	local r = smpclientrpcer.new_rpc()	
	r:connect(url)	
	r:post(...)
	r:co_wait()
	r:disconnect()	
	return 'success'
end

-- @brief 发起一次 NOTIFY-RPC
-- @param [in]      url[string]			目标, eg. 'smprpc://username:password@127.0.0.1:3457'
-- @return [string] 消息
klbsmp.co_notify_rpc = function (url, ...)
	local r = smpclientrpcer.new_rpc()
	r:connect(url)	
	r:notify(...)
	r:co_wait()
	r:disconnect()	
	return 'success'
end

-- @brief 发起一次 CALL-RPC
-- @param [in]      url[string]			目标, eg. 'smprpc://username:password@127.0.0.1:3457'
-- @return [string] 消息
--			[...] 回应数据
klbsmp.co_call_rpc = function (url, ...)
	local ReturnCoCall = function (r, ...)
		r:disconnect()
		return 'success', ...
	end
	
	local r = smpclientrpcer.new_rpc()
	r:connect(url)
	
	return ReturnCoCall(r, r:co_call(...))
end

--------------------------------------------------------------------------------------------
-- 服务端 RPC


-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			监听模块配置
-- @return [table]	监听模块
klbsmp.new_listen_rpc = function (cfg)
	return smplistener.new_rpc(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置
-- @return [table]	监听模块
klbsmp.listen_rpc = function (port, cfg)
	-- 新建
	local l = smplistener.new_rpc(cfg)
	
	-- 打开端口
	l:open(port)
	
	-- 返回 监听模块
	return l
end


--------------------------------------------------------------------------------------------
-- 一些常量

-------------------------------------------------------
-- 数据组织方式
klbsmp.RPC_LUA			= 'LUA'
klbsmp.RPC_JSON			= 'JSON'


-------------------------------------------------------
-- RPC 方法
klbsmp.RPC_POST 		= 'POST'
klbsmp.RPC_NOTIFY 		= 'NOTIFY'
klbsmp.RPC_REQUEST 		= 'REQUEST'
klbsmp.RPC_RESPONSE 	= 'RESPONSE'


return klbsmp
