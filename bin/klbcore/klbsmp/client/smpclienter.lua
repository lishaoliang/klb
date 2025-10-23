--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   smpclienter.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  SMP 客户端
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
-- smpclient

local smpclient = {}


-- @brief 断开连接
function smpclient:disconnect()
	if self._client then
		self._client:disconnect()
		self._client = nil
	end	
end


-- @brief 连接到目标
function smpclient:connect(host, port)
	
	local client = ksmp.connect(tostring(host), tonumber(port))
	if not client then
		return 1 -- 连接失败
	end
	
	-- 更新
	self._client = client
	
	return 0
end


-- @brief 发送文本数据
function smpclient:send_text()
	
end

-- @brief 发送二进制数据
function smpclient:send_binary()
	
end

-- @brief 发送媒体数据
function smpclient:send_media()
	
end

-- @brief 接收文本数据
function smpclient:co_recv_text()
	
end

-- @brief 接收二进制数据
function smpclient:co_recv_binary()
	
end

-- @brief 接收媒体数据
function smpclient:co_recv_media()
	
end


--------------------------------------------------------------------------------------------
-- smpclienter

local smpclienter = {}


-- @brief 新建一个 SMP 客户端
smpclienter.new = function (cfg)
	local obj = {
		_client = nil,						-- C/C++ 提供的客户端连接
	}
	
	setmetatable(obj, {
		__index = smpclient,
		__tostring = function(self)
			return tostring(self._client)
		end
	})
	
	return obj
end


return smpclienter
