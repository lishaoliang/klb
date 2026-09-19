--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   mnpserver.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  MNP 服务端
-- @note   MNP 协议; 内部使用 kmnp
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kmnp = require("kmnp") -- 确保 kmnp userdata 元表已加载


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现



--------------------------------------------------------------------------------------------
-- mnpserve

local mnpserve = {}


-- @brief 断开连接
function mnpserve:disconnect()
	if self._serve then
		self._serve:disconnect()
		self._serve = nil
	end
end


-- @brief 发送 TEXT
-- @param [in]      head[string]		MNP text 头
-- @param [in]      body[string]		[可选] MNP text 体
-- @return [number]	0 成功; 非0 失败
function mnpserve:send_text(head, body)
	if not self._serve then
		return 1
	end

	return self._serve:send_text(head, body)
end


-- @brief 发送 BINARY
-- @param [in]      head[string]		MNP binary 头
-- @param [in]      body[string]		[可选] MNP binary 体
-- @return [number]	0 成功; 非0 失败
function mnpserve:send_binary(head, body)
	if not self._serve then
		return 1
	end

	return self._serve:send_binary(head, body)
end


-- @brief 接收 MNP 包
-- @return msg[string]					"text" / "binary" / "media" / "error" / "exit"
-- @return head[string|number|userdata]	text/binary 时为 head; media 时为 buf; error 时为 code
-- @return body[string]					text/binary 时为 body
-- @note 须在 kco 协程内; media 须随后 free_media
function mnpserve:co_recv()
	if not self._serve then
		return "error", 1
	end

	return self._serve:co_recv()
end


-- @brief 释放 media 包
-- @param [in]      media[userdata]		co_recv 返回的 media buf
-- @return 无
function mnpserve:free_media(media)
	if not self._serve then
		return
	end

	self._serve:free_media(media)
end


--------------------------------------------------------------------------------------------
-- mnpserver

local mnpserver = {}


-- @brief 新建一个 MNP 服务端
mnpserver.new = function (conn, cfg)
	local obj = {
		_serve = conn,						-- C 提供的服务连接
	}

	setmetatable(obj, {
		__index = mnpserve,
		__tostring = function(self)
			return tostring(self._serve)
		end
	})

	return obj
end


return mnpserver
