--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   httpclienter.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP 客户端
-- @note   HTTP 协议; 内部使用 khttp
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local khttp = require("khttp")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现



--------------------------------------------------------------------------------------------
-- httpclient

local httpclient = {}


-- @brief 断开连接
function httpclient:disconnect()
	if self._client then
		self._client:disconnect()
		self._client = nil
	end
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] 连接选项; 见内联 opts 注释
-- @return [number]	0 成功; 非0 失败
function httpclient:connect(host, port, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	-- }
	local client = khttp.connect(tostring(host), tonumber(port), opts)
	if not client then
		return 1 -- 连接失败
	end

	-- 更新
	self._client = client

	return 0
end


-- @brief 发送 HTTP 文本
-- @param [in]      head[string]		HTTP 头
-- @param [in]      body[string]		[可选] HTTP 体
-- @return [number]	0 成功; 非0 失败
-- @note 须在 kco 协程内配合 co_recv
function httpclient:send(head, body)
	if not self._client then
		return 1
	end

	return self._client:send(head, body)
end


-- @brief 发送 HTTP 头 + 本地文件体
-- @param [in]      head[string]		HTTP 头 (含 Content-Length)
-- @param [in]      path[string]		本地文件路径
-- @param [in]      opts[table]			[可选] offset/length
-- @return [number]	0 成功; 非0 失败
-- @note 须在 kco 协程内配合 co_recv
function httpclient:send_file(head, path, opts)
	if not self._client then
		return 1
	end

	return self._client:send_file(head, path, opts)
end


-- @brief 接收 HTTP 文本
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body
-- @note 须在 kco 协程内
function httpclient:co_recv()
	if not self._client then
		return "error", 1
	end

	return self._client:co_recv()
end


-- @brief 接收至 HTTP 文本; 跳过连接成功/写缓存空
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body
-- @note 须在 kco 协程内; 忽略 KLB_SOCKET_CONNECT(63) 与 KLB_NETCODE_WBUF_EMPTY(70)
function httpclient:co_recv_text()
	while true do
		local msg, head, body = self:co_recv()
		if "text" == msg then
			return "text", head or "", body or ""
		elseif "exit" == msg then
			return "exit", "", ""
		elseif "error" == msg then
			local code = tonumber(head) or 1
			if 63 ~= code and 70 ~= code then
				return "error", code, ""
			end
		else
			return "error", 1, ""
		end
	end
end


--------------------------------------------------------------------------------------------
-- httpclienter

local httpclienter = {}


-- @brief 新建一个 HTTP 客户端
httpclienter.new = function (cfg)
	local obj = {
		_client = nil,						-- C 提供的客户端连接
	}

	setmetatable(obj, {
		__index = httpclient,
		__tostring = function(self)
			return tostring(self._client)
		end
	})

	return obj
end


return httpclienter
