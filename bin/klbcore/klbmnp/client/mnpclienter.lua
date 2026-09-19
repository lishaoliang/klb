--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   mnpclienter.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  MNP 客户端
-- @note   MNP 协议; 内部使用 kmnp
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kmnp = require("kmnp")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现



--------------------------------------------------------------------------------------------
-- mnpclient

local mnpclient = {}


-- @brief 断开连接
function mnpclient:disconnect()
	if self._client then
		self._client:disconnect()
		self._client = nil
	end
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @return [number]	0 成功; 非0 失败
function mnpclient:connect(host, port)
	local client = kmnp.connect(tostring(host), tonumber(port))
	if not client then
		return 1 -- 连接失败
	end

	-- 更新
	self._client = client

	return 0
end


-- @brief 发送 TEXT
-- @param [in]      head[string]		MNP text 头
-- @param [in]      body[string]		[可选] MNP text 体
-- @return [number]	0 成功; 非0 失败
-- @note 须在 kco 协程内配合 co_recv
function mnpclient:send_text(head, body)
	if not self._client then
		return 1
	end

	return self._client:send_text(head, body)
end


-- @brief 发送 BINARY
-- @param [in]      head[string]		MNP binary 头
-- @param [in]      body[string]		[可选] MNP binary 体
-- @return [number]	0 成功; 非0 失败
-- @note 须在 kco 协程内配合 co_recv
function mnpclient:send_binary(head, body)
	if not self._client then
		return 1
	end

	return self._client:send_binary(head, body)
end


-- @brief 接收 MNP 包
-- @return msg[string]					"text" / "binary" / "media" / "error" / "exit"
-- @return head[string|number|userdata]	text/binary 时为 head; media 时为 buf; error 时为 code
-- @return body[string]					text/binary 时为 body
-- @note 须在 kco 协程内; media 须随后 free_media
function mnpclient:co_recv()
	if not self._client then
		return "error", 1
	end

	return self._client:co_recv()
end


-- @brief 释放 media 包
-- @param [in]      media[userdata]		co_recv 返回的 media buf
-- @return 无
function mnpclient:free_media(media)
	if not self._client then
		return
	end

	self._client:free_media(media)
end


--------------------------------------------------------------------------------------------
-- mnpclienter

local mnpclienter = {}


-- @brief 新建一个 MNP 客户端
mnpclienter.new = function (cfg)
	local obj = {
		_client = nil,						-- C 提供的客户端连接
	}

	setmetatable(obj, {
		__index = mnpclient,
		__tostring = function(self)
			return tostring(self._client)
		end
	})

	return obj
end


return mnpclienter
