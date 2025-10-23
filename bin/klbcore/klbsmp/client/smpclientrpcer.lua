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
local kurl = require("kurl")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现



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
function smpclientrpc:send_text()
	
end

-- @brief 发送二进制数据
function smpclientrpc:send_binary()
	
end

-- @brief 发送媒体数据
function smpclientrpc:send_media()
	
end

-- @brief 接收文本数据
function smpclientrpc:co_recv_text()
	
end

-- @brief 接收二进制数据
function smpclientrpc:co_recv_binary()
	
end

-- @brief 接收媒体数据
function smpclientrpc:co_recv_media()
	
end


--------------------------------------------------------------------------------------------
-- smpclientrpcer

local smpclientrpcer = {}


-- @brief 新建一个 SMP-RPC 客户端
smpclientrpcer.new_rpc = function (cfg)
	local obj = {
		_client = nil,						-- C/C++ 提供的客户端连接
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
