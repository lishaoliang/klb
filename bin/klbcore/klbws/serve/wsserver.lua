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
local kco = require("kco")
local ktime = require("ktime")
local errcode = require("klbcore.klbws.errcode")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}

local FLUSH_POLL_MS = 1
local FLUSH_TIMEOUT_MS = 5000


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


-- @brief 写缓存未发送字节数 (背压观测)
-- @return [number]	待发送字节数; 无连接时为 0
function wsserve:buffered_amount()
	if not self._serve then
		return 0
	end

	return self._serve:buffered_amount()
end


-- @brief 等待写缓存排空 (协程)
-- @param [in]      timeout_ms[number]	[可选] 超时毫秒, 默认 5000; 0 不限
-- @return [number]	0 已排空; 1 超时; 2 未连接
-- @note 须在 kco 协程内; 以 1ms 步长让出轮询, 不阻塞其它协程
function wsserve:co_flush(timeout_ms)
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


-- @brief 发送文本帧并等待排空 (协程)
-- @param [in]      text[string]		文本
-- @param [in]      timeout_ms[number]	[可选] 排空超时毫秒, 默认 5000; 0 不限
-- @return [number]	0 成功; 非0 失败 (发送错误码 / 1 超时 / 2 未连接)
function wsserve:co_send_text(text, timeout_ms)
	local rc = self:send_text(text)
	if 0 ~= rc then
		return rc
	end

	return self:co_flush(timeout_ms)
end


-- @brief 发送二进制帧并等待排空 (协程)
-- @param [in]      bin[string]			二进制
-- @param [in]      timeout_ms[number]	[可选] 排空超时毫秒, 默认 5000; 0 不限
-- @return [number]	0 成功; 非0 失败 (发送错误码 / 1 超时 / 2 未连接)
function wsserve:co_send_binary(bin, timeout_ms)
	local rc = self:send_binary(bin)
	if 0 ~= rc then
		return rc
	end

	return self:co_flush(timeout_ms)
end


-- @brief 发送 Ping
-- @param [in]      data[string]		[可选] 载荷, 不超过 125 字节
-- @return [number]	0 成功; 非0 失败
function wsserve:send_ping(data)
	if not self._serve then
		return 1
	end

	return self._serve:send_ping(data)
end


-- @brief 发送 Close
-- @param [in]      code[number]		[可选] 关闭码, 默认 1000
-- @param [in]      reason[string]		[可选] 原因, UTF-8, 不超过 123 字节
-- @return [number]	0 成功; 非0 失败
function wsserve:send_close(code, reason)
	if not self._serve then
		return 1
	end

	return self._serve:send_close(code, reason)
end


-- @brief 对端地址
-- @return [string]	"ip:port"; 无连接时为空串
function wsserve:peer()
	if not self._serve then
		return ""
	end

	return self._serve:peer()
end


-- @brief 接收帧
-- @return msg[string]					"open" / "text" / "binary" / "pong" / "close" / "error" / "exit"
-- @return head[string|number]			close 时为关闭码; error 时为 code; 其余为空串
-- @return body[string]					text/binary/pong 载荷, 或 close 原因
-- @note 须在 kco 协程内; "open" 表示握手完成
function wsserve:co_recv()
	if not self._serve then
		return "error", self.last_code or 1, self.last_error
	end

	local msg, head, body = self._serve:co_recv()
	if "http" == msg then
		self:disconnect()
		self.last_code = 1
		self.last_error = "bad handshake"
		return "error", self.last_code, self.last_error
	end

	if "error" == msg or "exit" == msg then
		self.last_code = head
		self.last_error = errcode.str(errcode.E_IO)
	elseif "close" == msg then
		self.last_code = head
		self.last_error = tostring(body or "")
	end

	return msg, head, body
end


-- @brief 最近一次错误码 / 描述
-- @return [number|nil] code
-- @return [string|nil] err
function wsserve:last_error_of()
	return self.last_code, self.last_error
end


--------------------------------------------------------------------------------------------
-- wsserver

local wsserver = {}


-- @brief 新建一个 WebSocket 服务端
wsserver.new = function (conn, cfg)
	local obj = {
		_serve = conn,						-- C 提供的服务连接
		last_code = 0,						-- 最近一次错误码
		last_error = nil,					-- 最近一次错误描述
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
