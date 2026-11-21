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


-- @brief 对端地址 "ip:port"
-- @return peer[string]					空串表示未知
function httpserve:peer()
	if not self._serve or not self._serve.peer then
		return ""
	end

	return self._serve:peer()
end


-- @brief 设置读/写/idle 超时(毫秒; 0=关闭)
-- @param [in]      read_or_opts[number|table]	读超时, 或 `{read=,write=,idle=}`
-- @param [in]      write_ms[number]			[可选] 写超时
-- @param [in]      idle_ms[number]				[可选] 空闲超时
-- @return [number]	0 成功; 非0 为 klb_netcode_e
function httpserve:set_timeout(read_or_opts, write_ms, idle_ms)
	if not self._serve or not self._serve.set_timeout then
		return 1
	end

	return self._serve:set_timeout(read_or_opts, write_ms, idle_ms)
end


-- @brief 设置请求体上限(字节; 0=不限制)
-- @param [in]      max_body[number]			上限
-- @return [number]	0 成功; 非0 为 klb_netcode_e
function httpserve:set_max_body(max_body)
	if not self._serve or not self._serve.set_max_body then
		return 1
	end

	return self._serve:set_max_body(max_body)
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


-- @brief 在 101 已写完后, 把本连接交给 kws
-- @return [userdata]	kws 服务连接; 失败为 nil
-- @return [number]		失败时 klb_netcode_e; 描述串用 knetcode.strerror(code)
-- @note 成功后本对象不再持有 HTTP 连接
function httpserve:upgrade()
	if not self._serve or not self._serve.upgrade then
		return nil
	end

	local conn, code = self._serve:upgrade()
	if not conn then
		return nil, code
	end

	self._serve = nil
	return conn
end


-- @brief 接收 HTTP 文本
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; error 时无第三返回
-- @note 须在 kco 协程内; error 描述串用 knetcode.strerror(code)
function httpserve:co_recv()
	if not self._serve then
		return "error", 1
	end

	-- 流水线: 等写缓冲期间收到的后续请求先由栈存队列吐出
	local stash = self._stash
	if stash and 0 < #stash then
		local item = table.remove(stash, 1)
		return "text", item.head, item.body
	end

	return self._serve:co_recv()
end


-- @brief 直读网络事件(不经栈存队列; 供等写缓冲使用)
-- @return msg[string]					"text" / "error" / "exit"
-- @return head[string|number]			text 时为 head; error 时为 code
-- @return body[string]					text 时为 body; error 时无第三返回
function httpserve:co_recv_net()
	if not self._serve then
		return "error", 1
	end

	return self._serve:co_recv()
end


-- @brief 栈存等写缓冲期间收到的后续请求 (流水线保序)
-- @param [in]      head[string]		HTTP 头
-- @param [in]      body[string]		HTTP 体
function httpserve:stash_recv(head, body)
	local stash = self._stash
	if not stash then
		stash = {}
		self._stash = stash
	end

	stash[#stash + 1] = { head = head or "", body = body or "" }
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
