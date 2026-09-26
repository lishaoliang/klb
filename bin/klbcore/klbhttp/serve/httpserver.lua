--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   httpserver.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP 服务端
-- @note   HTTP 协议; 内部使用 khttp
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local khttp = require("khttp") -- 确保 khttp userdata 元表已加载


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现



--------------------------------------------------------------------------------------------
-- httpserve

local httpserve = {}


-- @brief 断开连接
function httpserve:disconnect()
	if self._serve then
		self._serve:disconnect()
		self._serve = nil
	end
end


-- @brief 本连接是否为 TLS
-- @return tls[boolean]					true 为 HTTPS; false 为明文 HTTP
-- @note 同端口混用时按连接判定, 不按监听 cfg
function httpserve:tls()
	if not self._serve or not self._serve.tls then
		return false
	end

	return self._serve:tls()
end


-- @brief 发送 HTTP 文本
-- @param [in]      head[string]		HTTP 头
-- @param [in]      body[string]		[可选] HTTP 体
-- @return [number]	0 成功; 非0 失败
function httpserve:send(head, body)
	if not self._serve then
		return 1
	end

	return self._serve:send(head, body)
end


-- @brief 发送 HTTP 头 + 本地文件体
-- @param [in]      head[string]		HTTP 头 (含 Content-Length)
-- @param [in]      path[string]		本地文件路径
-- @param [in]      opts[table]			[可选] offset/length
-- @return [number]	0 成功; 非0 失败
function httpserve:send_file(head, path, opts)
	if not self._serve then
		return 1
	end

	return self._serve:send_file(head, path, opts)
end


-- @brief 接收 HTTP 文本
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body
-- @note 须在 kco 协程内
function httpserve:co_recv()
	if not self._serve then
		return "error", 1
	end

	return self._serve:co_recv()
end


--------------------------------------------------------------------------------------------
-- httpserver

local httpserver = {}


-- @brief 新建一个 HTTP 服务端
httpserver.new = function (conn, cfg)
	local obj = {
		_serve = conn,						-- C 提供的服务连接
	}

	setmetatable(obj, {
		__index = httpserve,
		__tostring = function(self)
			return tostring(self._serve)
		end
	})

	return obj
end


return httpserver
