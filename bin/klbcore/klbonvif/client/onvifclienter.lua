--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   onvifclienter.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ONVIF 客户端
-- @note   SOAP 经 klbhttp.co_post; 须在 kco 协程内调用 co_* / call
-- @history 修改历史
--		[2026-09] 创建文件; 补充 Device/Media 主路径
--]]
local klbhttp = require("klbcore.klbhttp")
local kurl = require("kurl")
local errcode = require("klbcore.klbonvif.errcode")
local soap_util = require("klbcore.klbonvif.client.soap_util")
local device_soap = require("klbcore.klbonvif.client.device_soap")
local media_soap = require("klbcore.klbonvif.client.media_soap")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- 内部


local auth_opts = function (self, opts)
	opts = opts or {}
	local user = opts.user
	local password = opts.password
	local mode = opts.mode

	if "string" ~= type(user) or "" == user then
		local o = self._opts or {}
		user = o.user
		password = o.password
		mode = mode or o.mode
	end

	return {
		user = user,
		password = password,
		mode = mode,
	}
end


-- @brief 401 + Digest 质询时重发一次 POST
-- @param [in]      keep_alive[boolean]	[可选] true 复用进程内 HTTP 连接
-- @return msg, head, body
local function post_with_digest(url, soap, content_type, auth, keep_alive)
	local post_opts = {
		content_type = content_type,
		keep_alive = true == keep_alive,
	}

	local msg, head, body = klbhttp.co_post(url, soap, post_opts)
	if "text" ~= msg or "string" ~= type(head) then
		return msg, head, body
	end

	if nil == string.match(head, "^HTTP/%d%.%d%s+401") then
		return msg, head, body
	end

	local user = auth.user
	if "string" ~= type(user) or "" == user then
		return msg, head, body
	end

	local challenge = soap_util.parse_digest_challenge(head)
	if nil == challenge then
		return msg, head, body
	end

	local u = kurl.parse(url)
	local uri = u["path"] or "/"
	if "string" == type(u["query"]) and "" ~= u["query"] then
		uri = uri .. "?" .. u["query"]
	end

	local authz = soap_util.digest_authorization("POST", uri, user, tostring(auth.password or ""), challenge)
	if nil == authz then
		return msg, head, body
	end

	return klbhttp.co_post(url, soap, {
		content_type = content_type,
		headers = { Authorization = authz },
		keep_alive = true == keep_alive,
	})
end


--------------------------------------------------------------------------------------------
-- onvifclient

local onvifclient = {}


-- @brief 关闭客户端
function onvifclient:close()
	self._closed = true
	self._xaddr = nil
	self._media_xaddr = nil
end


-- @brief 设置设备服务地址与凭证
-- @param [in]      xaddr[string]		Device/Media 等服务 XAddr (URL)
-- @param [in]      opts[table]			[可选] user / password / tls
-- @return ok[boolean]					true 成功
-- @return [number|nil] code			失败错误码
-- @return [string|nil] err				失败描述
function onvifclient:connect(xaddr, opts)
	-- opts = {
	--   user[string]			[可选] 用户名
	--   password[string]		[可选] 密码
	--   tls[boolean]			[可选] 是否 TLS, 默认按 URL
	-- }
	if "string" ~= type(xaddr) or "" == xaddr then
		return nil, errcode.E_ARG, "invalid xaddr"
	end

	self._xaddr = xaddr
	self._opts = opts or {}
	self._media_xaddr = nil
	self._closed = false
	self._last_fault = nil
	return true
end


-- @brief 发起一次 SOAP 调用 (协程)
-- @param [in]      action[string]		SOAP Action URI
-- @param [in]      body_inner[string]	Body 内层 XML
-- @param [in]      opts[table]			[可选] xaddr / user / password / keep_alive
-- @return rsp[string|nil]				成功时响应 XML
-- @return [number|nil] code			失败错误码
-- @return [string|nil] err				失败描述
-- @note 须在 kco 协程内; Fault 结构化存于 self._last_fault
function onvifclient:call(action, body_inner, opts)
	opts = opts or {}

	if "string" ~= type(action) or "" == action then
		return nil, errcode.E_ARG, "invalid action"
	end

	if self._closed then
		return nil, errcode.E_CLOSED, "closed"
	end

	local url = opts.xaddr or self._xaddr
	if "string" ~= type(url) or "" == url then
		return nil, errcode.E_ARG, "no xaddr"
	end

	local auth = auth_opts(self, opts)
	local soap = soap_util.build_envelope(body_inner or "", auth)

	-- SOAP 1.2: action 放 Content-Type; 不再混用 SOAP 1.1 SOAPAction 头
	-- 401 + Digest 质询时自动重发一次
	-- 默认复用 HTTP 连接; opts.keep_alive=false / 构造时 keep_alive=false 关闭
	local keep_alive = true
	local o = self._opts or {}
	if nil ~= opts.keep_alive then
		keep_alive = true == opts.keep_alive
	elseif nil ~= o.keep_alive then
		keep_alive = true == o.keep_alive
	end

	local content_type = string.format('application/soap+xml; charset=utf-8; action="%s"', action)
	local msg, head, body = post_with_digest(url, soap, content_type, auth, keep_alive)

	if "text" ~= msg then
		E.last_err = "http " .. tostring(msg) .. " " .. tostring(head)
		return nil, errcode.of(E.last_err), E.last_err
	end

	local fault = soap_util.fault_of(body)
	if nil ~= fault then
		self._last_fault = fault
		local reason = fault.reason
		if "" == reason then
			reason = fault.subcode
		end
		if "" == reason then
			reason = "soap fault"
		end
		return nil, errcode.E_PEER, reason
	end

	self._last_fault = nil
	return body
end


-- @brief GetDeviceInformation (协程)
-- @return info[table|nil], err[string|nil]
function onvifclient:co_get_device_information(opts)
	local xml, code, err = self:call(device_soap.ACTION_GET_DEVICE_INFORMATION,
		device_soap.build_get_device_information(), opts)
	if nil == xml then
		return nil, code, err
	end

	return device_soap.parse_device_information(xml)
end


-- @brief GetCapabilities (协程); 成功时缓存 Media XAddr
-- @return xml[string|nil], err[string|nil]
function onvifclient:co_get_capabilities(opts)
	opts = opts or {}
	local xml, code, err = self:call(device_soap.ACTION_GET_CAPABILITIES,
		device_soap.build_get_capabilities(opts.category), opts)
	if nil == xml then
		return nil, code, err
	end

	local media_xaddr = device_soap.parse_media_xaddr(xml)
	if nil ~= media_xaddr and "" ~= media_xaddr then
		self._media_xaddr = media_xaddr
	end

	return xml
end


-- @brief GetServices (协程)
-- @return list[table|nil], err[string|nil]
function onvifclient:co_get_services(opts)
	opts = opts or {}
	local xml, code, err = self:call(device_soap.ACTION_GET_SERVICES,
		device_soap.build_get_services(opts.include_capability), opts)
	if nil == xml then
		return nil, code, err
	end

	return device_soap.parse_services(xml)
end


-- @brief GetProfiles (协程); 优先使用 Media XAddr
-- @return list[table|nil], err[string|nil]
function onvifclient:co_get_profiles(opts)
	opts = opts or {}
	local call_opts = {}
	for k, v in pairs(opts) do
		call_opts[k] = v
	end

	if "string" ~= type(call_opts.xaddr) or "" == call_opts.xaddr then
		call_opts.xaddr = self._media_xaddr or self._xaddr
	end

	local xml, code, err = self:call(media_soap.ACTION_GET_PROFILES,
		media_soap.build_get_profiles(), call_opts)
	if nil == xml then
		return nil, code, err
	end

	return media_soap.parse_profiles(xml)
end


-- @brief GetStreamUri (协程)
-- @param [in]      profile_token[string]
-- @param [in]      opts[table]			[可选] stream / protocol / xaddr
-- @return info[table|nil], err[string|nil]		info.uri 为 RTSP URL
function onvifclient:co_get_stream_uri(profile_token, opts)
	if "string" ~= type(profile_token) or "" == profile_token then
		return nil, errcode.E_ARG, "invalid profile_token"
	end

	opts = opts or {}
	local call_opts = {}
	for k, v in pairs(opts) do
		call_opts[k] = v
	end

	if "string" ~= type(call_opts.xaddr) or "" == call_opts.xaddr then
		call_opts.xaddr = self._media_xaddr or self._xaddr
	end

	local xml, code, err = self:call(media_soap.ACTION_GET_STREAM_URI,
		media_soap.build_get_stream_uri(profile_token, opts), call_opts)
	if nil == xml then
		return nil, code, err
	end

	local info = media_soap.parse_stream_uri(xml)
	if nil == info then
		return nil, errcode.E_NOENT, "no Uri"
	end

	return info
end


--------------------------------------------------------------------------------------------
-- onvifclienter

local onvifclienter = {}


-- @brief 新建一个客户端模块
-- @param [in]      cfg[table]			[可选] 客户端配置
-- @return [table]	客户端模块
onvifclienter.new = function (cfg)
	local obj = {
		_cfg = cfg,
		_opts = cfg,
		_xaddr = nil,
		_media_xaddr = nil,
		_closed = false,
	}

	setmetatable(obj, {
		__index = onvifclient,
	})

	return obj
end


return onvifclienter
