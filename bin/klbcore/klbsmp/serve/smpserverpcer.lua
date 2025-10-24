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
local kurl = require("kurl")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现



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



-- @brief 发送文本数据
function smpserverpc:post(...)
	return self._serve:post(...)
end

-- @brief 接收数据
function smpserverpc:co_recv()
	return self._serve:co_recv()
end



--------------------------------------------------------------------------------------------
-- smpserverpcer

local smpserverpcer = {}


-- @brief 新建一个 SMP - RPC 服务端
smpserverpcer.new_rpc = function (conn, cfg)
	local obj = {
		_serve = conn,						-- C/C++ 提供的客户端连接
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
