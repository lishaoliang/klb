--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   smpserver.lua
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
-- smpserve

local smpserve = {}


-- @brief 断开连接
function smpserve:disconnect()
	if self._serve then
		self._serve:disconnect()
		self._serve = nil
	end	
end


-- @brief 发送文本数据
function smpserve:send_text()
	
end

-- @brief 发送二进制数据
function smpserve:send_binary()
	
end

-- @brief 发送媒体数据
function smpserve:send_media()
	
end

-- @brief 接收文本数据
function smpserve:co_recv_text()
	
end

-- @brief 接收二进制数据
function smpserve:co_recv_binary()
	
end

-- @brief 接收媒体数据
function smpserve:co_recv_media()
	
end


--------------------------------------------------------------------------------------------
-- smpserver

local smpserver = {}


-- @brief 新建一个 SMP 服务端
smpserver.new = function (conn, cfg)
	local obj = {
		_serve = conn,						-- C/C++ 提供的客户端连接
	}
	
	setmetatable(obj, {
		__index = smpserve,
		__tostring = function(self)
			return tostring(self._serve)
		end
	})
	
	return obj
end


return smpserver
