--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   wsserverpcer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  WebSocket 服务端
-- @note   WebSocket 协议 - RPC
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local cjson = require("cjson.safe")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


local CONST_RPC_LUA		= 0x10		-- KLB_MNP_RPC_LUA
local CONST_RPC_JSON	= 0x11		-- KLB_MNP_RPC_JSON


-- @brief 公开常量是字符串, 线上类型是整数
local RpcType = function (v)
	if CONST_RPC_JSON == v or 'JSON' == v then
		return CONST_RPC_JSON
	end

	return CONST_RPC_LUA
end


-- @brief JSON 数组还原成多个返回值
local DecodeJsonArgs = function (s)
	if 'string' ~= type(s) then
		return s
	end

	local args = cjson.decode(s)
	if 'table' ~= type(args) then
		return args
	end

	return table.unpack(args)
end


--------------------------------------------------------------------------------------------
-- 内部实现

-- @brief POST-RPC
local PostRpc = function (self, ...)
	local serve = self._serve
	if not serve then
		return 1
	end

	local rpctype = RpcType(self.rpctype)

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
	if not serve then
		return 1
	end

	local rpctype = RpcType(self.rpctype)

	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)

		return serve:notify(rpctype, s)
	end

	-- CONST_RPC_LUA
	return serve:notify(rpctype, ...)
end

-- @brief RESPONSE-RPC
local ResponseRpc = function (self, sequence, ...)
	local serve = self._serve
	if not serve then
		return 1
	end

	local rpctype = RpcType(self.rpctype)

	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)

		return serve:response(rpctype, sequence, s)
	end

	-- CONST_RPC_LUA
	return serve:response(rpctype, sequence, ...)
end


--------------------------------------------------------------------------------------------
-- wsserverpc

local wsserverpc = {}


-- @brief 断开连接
function wsserverpc:disconnect()
	if self._serve then
		self._serve:disconnect()
		self._serve = nil
	end
end


-- @brief post - RPC 数据
function wsserverpc:post(...)
	return PostRpc(self, ...)
end


-- @brief notify - RPC 数据
function wsserverpc:notify(...)
	return NotifyRpc(self, ...)
end

-- @brief response - RPC 数据
function wsserverpc:response(sequence, ...)
	return ResponseRpc(self, sequence, ...)
end

-- @brief 接收数据
function wsserverpc:co_recv()
	if not self._serve then
		return
	end

	if CONST_RPC_JSON == RpcType(self.rpctype) then
		return DecodeJsonArgs(self._serve:co_recv())
	end

	return self._serve:co_recv()
end

-- @brief 当前状态
function wsserverpc:status()
	return self._serve:status()
end


--------------------------------------------------------------------------------------------
-- wsserverpcer

local wsserverpcer = {}


-- @brief 新建一个 WS - RPC 服务端
wsserverpcer.new_rpc = function (conn, cfg)
	local rpctype = CONST_RPC_LUA
	if cfg and cfg.rpctype then
		rpctype = RpcType(cfg.rpctype)
	end

	local obj = {
		_serve = conn,						-- C 提供的服务连接

		rpctype = rpctype,					-- RPC 数据组织方式
	}

	setmetatable(obj, {
		__index = wsserverpc,
		__tostring = function(self)
			return tostring(self._serve)
		end
	})

	return obj
end


return wsserverpcer
