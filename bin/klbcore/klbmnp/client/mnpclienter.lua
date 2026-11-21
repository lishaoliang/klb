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
local errcode = require("klbcore.klbmnp.errcode")


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
-- @param [in]      opts[table|nil]		[可选] `{ tls[boolean], crypto[table] }`
-- \n crypto: `{ format[string|number], key_size[number], key[string|nil], rekey_after[number|nil] }`
-- \n 首配 crypto 时连接后首包发 CRYPTO_PSK; key 省略则按 key_size 随机
-- @return [number]	0 成功; 非0 为 klb_netcode_e
function mnpclient:connect(host, port, opts)
	local client, code = kmnp.connect(tostring(host), tonumber(port), opts)
	if not client then
		self._last_code = code or 1
		self._last_error = errcode.str(code or errcode.E_UNKNOWN)
		return self._last_code
	end

	-- 更新
	self._client = client
	self._last_code = 0
	self._last_error = nil

	return 0
end


-- @brief 最近一次错误码 / 描述
-- @return [number|nil] code
-- @return [string|nil] err
function mnpclient:last_error_of()
	return self._last_code, self._last_error
end

-- @brief 是否已连接
-- @return ok[boolean]
function mnpclient:is_open()
	return nil ~= self._client
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
-- @return head[string|number|userdata]	text/binary 时为 head; media 时为 buf; error 时为 code; 其余为 ""
-- @return body[string]					text/binary/exit 时为 body; media/error 时为 ""
-- @note 须在 kco 协程内; 恒 3 值; media 须随后 free_media
function mnpclient:co_recv()
	if not self._client then
		return "error", 1, ""
	end

	local msg, head, body = self._client:co_recv()
	if "error" == msg then
		self._last_code = head or 1
		self._last_error = errcode.str(errcode.E_IO)
		return "error", self._last_code, self._last_error
	end

	if "media" == msg then
		return "media", head, ""
	end

	return msg, head or "", body or ""
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
-- @param [in]      cfg[table]			[可选] 预留配置表; 现行无字段
mnpclienter.new = function (cfg)
	local obj = {
		_client = nil,						-- C 提供的客户端连接
		_cfg = cfg,							-- 预留; 现行无消费字段
		_last_code = 0,						-- 最近一次 connect 失败码
		_last_error = nil,					-- 最近一次错误描述
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
