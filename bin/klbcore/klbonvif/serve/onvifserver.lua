--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   onvifserver.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ONVIF 服务端会话
-- @note   经 httpserver 收 HTTP POST, 分发 Device/Media SOAP; 须在 kco 协程内
-- @history 修改历史
--		[2026-09] 创建文件; 补充 SOAP 主路径
--]]
local kco = require("kco")
local soap_serve = require("klbcore.klbonvif.serve.soap_serve")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}

local DEFAULT_MAX_BODY = 4 * 1024 * 1024	-- 请求体上限 (字节)
local DEFAULT_READ_TIMEOUT_MS = 15000		-- 单次读超时


--------------------------------------------------------------------------------------------
-- 内部


-- @brief 请求是否要求保持连接 (HTTP/1.1 默认持久; HTTP/1.0 须显式 keep-alive)
local function req_keep_alive(head)
	local major, minor = string.match(head, "^HTTP/(%d+)%.(%d+)")
	if nil == major then
		return false
	end

	for line in string.gmatch(head, "[^\r\n]+") do
		local v = string.match(line, "^[Cc][Oo][Nn][Nn][Ee][Cc][Tt][Ii][Oo][Nn]:%s*(.+)$")
		if v then
			return nil ~= string.find(string.lower(v), "keep%-alive")
		end
	end

	return "1" == major and "1" == minor
end


-- @brief 等写缓冲排空后再断连, 避免对端收不全
local wait_wbuf_empty = function (http)
	if nil == http or nil == http.co_recv then
		return
	end

	while true do
		local msg, head = http:co_recv()
		if "error" == msg then
			local code = tonumber(head) or 0
			if 70 == code then
				return
			end
			if 63 ~= code then
				return
			end
		elseif "exit" == msg then
			return
		elseif "text" == msg then
			-- keep waiting flush
		else
			return
		end
	end
end


--------------------------------------------------------------------------------------------
-- onvifserve

local onvifserve = {}


-- @brief 断开会话
function onvifserve:disconnect()
	if nil ~= self._http and self._http.disconnect then
		self._http:disconnect()
	end

	self._http = nil
end


-- @brief 处理一次 SOAP 请求 (协程)
-- @return msg[string]					"ok" / "error" / "exit" / "ignore"
-- @return detail[string|number]		操作名或错误信息
-- @note 须在 kco 协程内
function onvifserve:co_once()
	local http = self._http
	if nil == http then
		return "error", "no conn"
	end

	local cfg = self._cfg or {}

	-- 限额: 请求体上限 + 读超时 (每次收包前重设)
	if http.set_max_body then
		local max_body = tonumber(cfg.max_body) or DEFAULT_MAX_BODY
		if 0 >= max_body then
			max_body = DEFAULT_MAX_BODY
		end

		http:set_max_body(max_body)
	end

	if http.set_timeout then
		local read_ms = tonumber(cfg.read_timeout_ms) or DEFAULT_READ_TIMEOUT_MS
		if 0 > read_ms then
			read_ms = DEFAULT_READ_TIMEOUT_MS
		end

		http:set_timeout(read_ms)
	end

	local msg, head, body = http:co_recv()

	if "text" ~= msg then
		return msg, head
	end

	head = head or ""
	body = body or ""

	local method = string.match(head, "^(%S+)") or ""
	method = string.upper(method)

	if "POST" ~= method then
		local rsp = ""
		local h = soap_serve.pack_http_head(405, rsp)
		http:send(h, rsp)
		wait_wbuf_empty(http)
		kco.co_sleep(50)
		return "ignore", "method " .. method
	end

	-- 解析请求头 (供 SOAPAction / Content-Type action=)
	local headers = {}
	for line in string.gmatch(head, "[^\r\n]+") do
		local k, v = string.match(line, "^([^:]+):%s*(.+)$")
		if k then
			headers[string.lower(k)] = v
			headers[k] = v
		end
	end

	-- 对端要求持久连接且未关闭开关时, 回应同持
	local keep = req_keep_alive(head) and false ~= cfg.keep_alive

	local soap, action, ok, status = soap_serve.dispatch(body, self._cfg, headers)
	local code = status or (ok and 200 or 500)

	local h = soap_serve.pack_http_head(code, soap, keep)
	local rc = http:send(h, soap)
	if 0 ~= rc then
		return "error", "send failed"
	end

	wait_wbuf_empty(http)
	if not keep then
		kco.co_sleep(50)
	end

	E.last_action = action
	return "ok", action or "unknown"
end


--------------------------------------------------------------------------------------------
-- onvifserver

local onvifserver = {}


-- @brief 新建一个会话模块
-- @param [in]      conn				httpserver 连接 (klbhttp.co_accept 产物)
-- @param [in]      cfg[table]			[可选] device / xaddrs / profiles
-- @return [table]	会话模块
onvifserver.new = function (conn, cfg)
	local obj = {
		_http = conn,
		_cfg = cfg,
	}

	setmetatable(obj, {
		__index = onvifserve,
	})

	return obj
end


return onvifserver
