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
local kco = require("kco")
local ktime = require("ktime")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}

local FLUSH_POLL_MS = 1
local FLUSH_TIMEOUT_MS = 5000


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


-- @brief 发送 TEXT
-- @param [in]      head[string]		SMP text 头
-- @param [in]      body[string]		[可选] SMP text 体
-- @return [number]	0 成功; 非0 失败
function smpserve:send_text(head, body)
	if not self._serve then
		return 1
	end

	return self._serve:send_text(head, body)
end


-- @brief 发送 BINARY
-- @param [in]      head[string]		SMP binary 头
-- @param [in]      body[string]		[可选] SMP binary 体
-- @return [number]	0 成功; 非0 失败
function smpserve:send_binary(head, body)
	if not self._serve then
		return 1
	end

	return self._serve:send_binary(head, body)
end


-- @brief 写缓存未发送字节数 (背压观测)
-- @return [number]	待发送字节数; 无连接时为 0
function smpserve:buffered_amount()
	if not self._serve then
		return 0
	end

	return self._serve:buffered_amount()
end


-- @brief 等待写缓存排空 (协程)
-- @param [in]      timeout_ms[number]	[可选] 超时毫秒, 默认 5000; 0 不限
-- @return [number]	0 已排空; 1 超时; 2 未连接
-- @note 须在 kco 协程内; 以 1ms 步长让出轮询
function smpserve:co_flush(timeout_ms)
	if not self._serve then
		return 2
	end

	timeout_ms = tonumber(timeout_ms) or FLUSH_TIMEOUT_MS
	local start = ktime.tick_count()

	while true do
		if 0 == self._serve:buffered_amount() then
			return 0
		end

		if 0 < timeout_ms and timeout_ms <= (ktime.tick_count() - start) then
			return 1
		end

		kco.co_sleep(FLUSH_POLL_MS)
	end
end


-- @brief 发送 TEXT 并等待排空 (协程)
-- @param [in]      head[string]		SMP text 头
-- @param [in]      body[string]		[可选] SMP text 体
-- @param [in]      timeout_ms[number]	[可选] 排空超时毫秒, 默认 5000; 0 不限
-- @return [number]	0 成功; 非0 失败 (发送错误码 / 1 超时 / 2 未连接)
function smpserve:co_send_text(head, body, timeout_ms)
	local rc = self:send_text(head, body)
	if 0 ~= rc then
		return rc
	end

	return self:co_flush(timeout_ms)
end


-- @brief 发送 BINARY 并等待排空 (协程)
-- @param [in]      head[string]		SMP binary 头
-- @param [in]      body[string]		[可选] SMP binary 体
-- @param [in]      timeout_ms[number]	[可选] 排空超时毫秒, 默认 5000; 0 不限
-- @return [number]	0 成功; 非0 失败 (发送错误码 / 1 超时 / 2 未连接)
function smpserve:co_send_binary(head, body, timeout_ms)
	local rc = self:send_binary(head, body)
	if 0 ~= rc then
		return rc
	end

	return self:co_flush(timeout_ms)
end


-- @brief 发送 MEDIA
-- @param [in]      media[userdata]	媒体 buf
-- @return [number]	0 成功; 非0 失败
function smpserve:send_media(media)
	if not self._serve then
		return 1
	end

	return self._serve:send_media(media)
end


-- @brief 接收 SMP 包
-- @return msg[string]					"text" / "binary" / "media" / "error" / "exit"
-- @return head[string|number|userdata]	text/binary 时为 head; media 时为 buf; error 时为 code
-- @return body[string]					text/binary 时为 body
-- @note 须在 kco 协程内; media 须随后 free_media
function smpserve:co_recv()
	if not self._serve then
		return "error", 1
	end

	return self._serve:co_recv()
end


-- @brief 释放 media 包
-- @param [in]      media[userdata]		co_recv 返回的 media buf
-- @return 无
function smpserve:free_media(media)
	if not self._serve then
		return
	end

	self._serve:free_media(media)
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
