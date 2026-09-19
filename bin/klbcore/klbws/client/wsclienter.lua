--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   wsclienter.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  WebSocket 客户端
-- @note   WebSocket 协议; 内部使用 kws
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kws = require("kws")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现



--------------------------------------------------------------------------------------------
-- wsclient

local wsclient = {}


-- @brief 断开连接
function wsclient:disconnect()
	if self._client then
		self._client:disconnect()
		self._client = nil
	end
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      path[string]		[可选] 握手路径, 默认 `/`
-- @return [number]	0 成功; 非0 失败
function wsclient:connect(host, port, path)
	local ws_path = "/"
	if path then
		ws_path = tostring(path)
	end

	local client = kws.connect(tostring(host), tonumber(port), ws_path)
	if not client then
		return 1 -- 连接失败
	end

	-- 更新
	self._client = client

	return 0
end


-- @brief 发送文本帧
-- @param [in]      text[string]		文本
-- @return [number]	0 成功; 非0 失败
function wsclient:send_text(text)
	if not self._client then
		return 1
	end

	return self._client:send_text(text)
end


-- @brief 发送二进制帧
-- @param [in]      bin[string]			二进制
-- @return [number]	0 成功; 非0 失败
function wsclient:send_binary(bin)
	if not self._client then
		return 1
	end

	return self._client:send_binary(bin)
end


-- @brief 接收帧
-- @return msg[string]					"text" / "http" / "binary" / "error" / "exit"
-- @return head[string|number]			text/http 时为 head; error 时为 code
-- @return body[string]					text/http/binary 载荷
-- @note 须在 kco 协程内
function wsclient:co_recv()
	if not self._client then
		return "error", 1
	end

	return self._client:co_recv()
end


--------------------------------------------------------------------------------------------
-- wsclienter

local wsclienter = {}


-- @brief 新建一个 WebSocket 客户端
wsclienter.new = function (cfg)
	local obj = {
		_client = nil,						-- C 提供的客户端连接
	}

	setmetatable(obj, {
		__index = wsclient,
		__tostring = function(self)
			return tostring(self._client)
		end
	})

	return obj
end


return wsclienter
