--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   smpclientrpcer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  SMP 客户端
-- @note   SMP协议 - RPC
-- @history 修改历史
--		[2025-10] 添加基础
--]]
local ksmp = require("ksmp")
local cjson = require("cjson.safe")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}

local CONST_RPC_LUA		= 0x10		-- KLB_MNP_RPC_LUA
local CONST_RPC_JSON	= 0x11		-- KLB_MNP_RPC_JSON


--------------------------------------------------------------------------------------------
-- 内部实现


-- @brief POST-RPC
local PostRpc = function (self, ...)
	local client = self._client
	local rpctype = self.rpctype
	
	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)
		
		return client:post(rpctype, s)
	end
	
	-- CONST_RPC_LUA
	return client:post(rpctype, ...)
end

-- @brief NOTIFY-RPC
local NotifyRpc = function (self, ...)
	local client = self._client
	local rpctype = self.rpctype
	
	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)
		
		return client:notify(rpctype, s)
	end
	
	-- CONST_RPC_LUA
	return client:notify(rpctype, ...)
end


-- @brief call
local CoCall = function (self, ...)
	local client = self._client
	local rpctype = self.rpctype

	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)
		
		return client:co_call(rpctype, s)
	end
	
	-- CONST_RPC_LUA
	return client:co_call(rpctype, ...)
end


--------------------------------------------------------------------------------------------
-- smpclientrpc

local smpclientrpc = {}


-- @brief 断开连接
function smpclientrpc:disconnect()
	if self._client then
		self._client:disconnect()
		self._client = nil
	end	
end


-- @brief 连接到目标
function smpclientrpc:connect_rpc(host, port)
	
	local client = ksmp.connect_rpc(tostring(host), tonumber(port))
	if not client then
		return 1 -- 连接失败
	end
	
	-- 更新
	self._client = client
	
	return 0
end


-- @brief 发送文本数据
function smpclientrpc:post(...)
	return PostRpc(self, ...)
end


-- @brief notify - RPC 数据
function smpclientrpc:notify(...)
	return NotifyRpc(self, ...)
end


-- @brief 接收数据
function smpclientrpc:co_recv()
	return self._client:co_recv()
end


-- @brief 调用远程函数
function smpclientrpc:co_call(...)
	return CoCall(self, ...)
end


--------------------------------------------------------------------------------------------
-- smpclientrpcer

local smpclientrpcer = {}


-- @brief 新建一个 SMP-RPC 客户端
smpclientrpcer.new_rpc = function (cfg)
	local obj = {
		_client = nil,						-- C/C++ 提供的客户端连接
		
		rpctype = CONST_RPC_LUA,			-- RPC 数据组织方式
	}
	
	setmetatable(obj, {
		__index = smpclientrpc,
		__tostring = function(self)
			return tostring(self._client)
		end
	})
	
	return obj
end


return smpclientrpcer
