--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   smpserverpcer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  SMP 服务端
-- @note   SMP协议
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
	local serve = self._serve
	local rpctype = self.rpctype
	
	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)
		
		return serve:post(rpctype, s)
	end
	
	-- CONST_RPC_LUA
	return serve:post(rpctype, ...)
end

-- @brief NOTIFY-RPC
local NotifyRpc = function (self, ...)
	local serve = self._serve
	local rpctype = self.rpctype
	
	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)
		
		return serve:notify(rpctype, s)
	end
	
	-- CONST_RPC_LUA
	return serve:notify(rpctype, ...)
end

-- @brief RESPONSE-RPC
local ResponseRpc = function (self, ...)
	local serve = self._serve
	local rpctype = self.rpctype
	
	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)
		
		return serve:response(rpctype, s)
	end
	
	-- CONST_RPC_LUA
	return serve:response(rpctype, ...)
end


--------------------------------------------------------------------------------------------
-- smpserverpc

local smpserverpc = {}


-- @brief 断开连接
function smpserverpc:disconnect()
	if self._serve then
		self._serve:disconnect()
		self._serve = nil
	end	
end


-- @brief post - RPC 数据
function smpserverpc:post(...)
	return PostRpc(self, ...)
end


-- @brief notify - RPC 数据
function smpserverpc:notify(...)
	return NotifyRpc(self, ...)
end

-- @brief response - RPC 数据
function smpserverpc:response(...)
	return ResponseRpc(self, ...)
end

-- @brief 接收数据
function smpserverpc:co_recv()
	return self._serve:co_recv()
end

-- @brief 当前状态
function smpserverpc:status()
	return self._serve:status()
end


--------------------------------------------------------------------------------------------
-- smpserverpcer

local smpserverpcer = {}


-- @brief 新建一个 SMP - RPC 服务端
smpserverpcer.new_rpc = function (conn, cfg)
	local obj = {
		_serve = conn,						-- C/C++ 提供的客户端连接
		
		rpctype = CONST_RPC_LUA,			-- RPC 数据组织方式
	}
	
	setmetatable(obj, {
		__index = smpserverpc,
		__tostring = function(self)
			return tostring(self._serve)
		end
	})
	
	return obj
end


return smpserverpcer
