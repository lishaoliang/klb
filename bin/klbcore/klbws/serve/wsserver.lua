--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   wsserver.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  WebSocket 服务端
-- @note   WebSocket 协议; 内部使用 kws
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kws = require("kws") -- 确保 kws userdata 元表已加载


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现



--------------------------------------------------------------------------------------------
-- wsserve

local wsserve = {}


-- @brief 断开连接
function wsserve:disconnect()
	if self._serve then
		self._serve:disconnect()
		self._serve = nil
	end
end


-- @brief 发送文本帧
-- @param [in]      text[string]		文本
-- @return [number]	0 成功; 非0 失败
function wsserve:send_text(text)
	if not self._serve then
		return 1
	end

	return self._serve:send_text(text)
end


-- @brief 发送二进制帧
-- @param [in]      bin[string]			二进制
-- @return [number]	0 成功; 非0 失败
function wsserve:send_binary(bin)
	if not self._serve then
		return 1
	end

	return self._serve:send_binary(bin)
end


-- @brief 接收帧
-- @return msg[string]					"text" / "http" / "binary" / "error" / "exit"
-- @return head[string|number]			text/http 时为 head; error 时为 code
-- @return body[string]					text/http/binary 载荷
-- @note 须在 kco 协程内
function wsserve:co_recv()
	if not self._serve then
		return "error", 1
	end

	return self._serve:co_recv()
end


--------------------------------------------------------------------------------------------
-- wsserver

local wsserver = {}


-- @brief 新建一个 WebSocket 服务端
wsserver.new = function (conn, cfg)
	local obj = {
		_serve = conn,						-- C 提供的服务连接
	}

	setmetatable(obj, {
		__index = wsserve,
		__tostring = function(self)
			return tostring(self._serve)
		end
	})

	return obj
end


return wsserver
