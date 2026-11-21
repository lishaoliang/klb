--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   soap_serve.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ONVIF 服务端 SOAP 分发与应答组装
-- @note   cfg.auth `{ user, password }` 存在时校验 UsernameToken (Digest / PasswordText); 缺省不鉴权
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local soap_util = require("klbcore.klbonvif.client.soap_util")


local soap_serve = {}


--------------------------------------------------------------------------------------------
-- 内部


local KNOWN_ACTIONS = {
	"GetDeviceInformation",
	"GetCapabilities",
	"GetServices",
	"GetProfiles",
	"GetStreamUri",
}


-- @brief 从 Action URI / SOAPAction 串取末段操作名
local action_tail = function (s)
	if "string" ~= type(s) or "" == s then
		return nil
	end

	s = string.gsub(s, '^%s*"?', "")
	s = string.gsub(s, '"?%s*$', "")
	local name = string.match(s, "([%w]+)$")
	return name
end


local detect_action_from_headers = function (headers)
	if "table" ~= type(headers) then
		return nil
	end

	local soap_action = headers["soapaction"] or headers["SOAPAction"]
	local name = action_tail(soap_action)
	if name then
		return name
	end

	local ctype = headers["content-type"] or headers["Content-Type"] or ""
	local act = string.match(ctype, "[Aa]ction%s*=%s*\"?([^\";%s]+)")
	return action_tail(act)
end


local detect_action = function (body)
	if "string" ~= type(body) then
		return nil
	end

	-- 优先 wsa:Action
	local wsa = string.match(body, "[Aa]ction[^>]*>([^<]+)<")
	local name = action_tail(wsa)
	if name then
		return name
	end

	for i = 1, #KNOWN_ACTIONS do
		local a = KNOWN_ACTIONS[i]
		if nil ~= string.find(body, a, 1, true) then
			return a
		end
	end

	return nil
end


local wrap_body = function (inner)
	return table.concat({
		'<?xml version="1.0" encoding="UTF-8"?>',
		'<s:Envelope xmlns:s="http://www.w3.org/2003/05/soap-envelope"',
		' xmlns:tds="http://www.onvif.org/ver10/device/wsdl"',
		' xmlns:trt="http://www.onvif.org/ver10/media/wsdl"',
		' xmlns:tt="http://www.onvif.org/ver10/schema">',
		"  <s:Header/>",
		"  <s:Body>",
		inner or "",
		"  </s:Body>",
		"</s:Envelope>",
	})
end


-- @brief 组装 SOAP Fault
-- @param [in]  reason[string|nil]		原因文本
-- @param [in]  subcode[string|nil]		Subcode (如 ter:ActionNotSupported)
local build_fault = function (reason, subcode)
	local code = { "<s:Code><s:Value>s:Sender</s:Value>" }
	if "string" == type(subcode) and "" ~= subcode then
		code[#code + 1] = "<s:Subcode><s:Value>" .. soap_util.escape(subcode) .. "</s:Value></s:Subcode>"
	end
	code[#code + 1] = "</s:Code>"

	return wrap_body(table.concat({
		'<s:Fault>',
		table.concat(code),
		'<s:Reason><s:Text xml:lang="en">', soap_util.escape(reason or "fault"), "</s:Text></s:Reason>",
		"</s:Fault>",
	}))
end


local device_info = function (cfg)
	local d = (cfg and cfg.device) or {}
	return {
		manufacturer = d.manufacturer or "klb",
		model = d.model or "onvif",
		firmware_version = d.firmware_version or "0.1",
		serial_number = d.serial_number or "0",
		hardware_id = d.hardware_id or "0",
	}
end


local xaddrs = function (cfg)
	local x = (cfg and cfg.xaddrs) or {}
	return {
		device = x.device or "http://127.0.0.1/onvif/device_service",
		media = x.media or x.device or "http://127.0.0.1/onvif/media_service",
	}
end


local profiles = function (cfg)
	local p = cfg and cfg.profiles
	if "table" == type(p) and 0 < #p then
		return p
	end

	return {
		{
			token = "Profile_1",
			name = "MainStream",
			stream_uri = "rtsp://127.0.0.1:554/stream1",
		},
	}
end


--------------------------------------------------------------------------------------------
-- 应答体


local rsp_get_device_information = function (cfg)
	local d = device_info(cfg)
	return wrap_body(table.concat({
		"<tds:GetDeviceInformationResponse>",
		"<tds:Manufacturer>", soap_util.escape(d.manufacturer), "</tds:Manufacturer>",
		"<tds:Model>", soap_util.escape(d.model), "</tds:Model>",
		"<tds:FirmwareVersion>", soap_util.escape(d.firmware_version), "</tds:FirmwareVersion>",
		"<tds:SerialNumber>", soap_util.escape(d.serial_number), "</tds:SerialNumber>",
		"<tds:HardwareId>", soap_util.escape(d.hardware_id), "</tds:HardwareId>",
		"</tds:GetDeviceInformationResponse>",
	}))
end


local rsp_get_capabilities = function (cfg)
	local x = xaddrs(cfg)
	return wrap_body(table.concat({
		"<tds:GetCapabilitiesResponse>",
		"<tds:Capabilities>",
		"<tt:Device>",
		"<tt:XAddr>", soap_util.escape(x.device), "</tt:XAddr>",
		"</tt:Device>",
		"<tt:Media>",
		"<tt:XAddr>", soap_util.escape(x.media), "</tt:XAddr>",
		"<tt:StreamingCapabilities>",
		"<tt:RTPMulticast>false</tt:RTPMulticast>",
		"<tt:RTP_TCP>true</tt:RTP_TCP>",
		"<tt:RTP_RTSP_TCP>true</tt:RTP_RTSP_TCP>",
		"</tt:StreamingCapabilities>",
		"</tt:Media>",
		"</tds:Capabilities>",
		"</tds:GetCapabilitiesResponse>",
	}))
end


local rsp_get_services = function (cfg)
	local x = xaddrs(cfg)
	return wrap_body(table.concat({
		"<tds:GetServicesResponse>",
		"<tds:Service>",
		"<tds:Namespace>http://www.onvif.org/ver10/device/wsdl</tds:Namespace>",
		"<tds:XAddr>", soap_util.escape(x.device), "</tds:XAddr>",
		"<tds:Version><tt:Major>2</tt:Major><tt:Minor>0</tt:Minor></tds:Version>",
		"</tds:Service>",
		"<tds:Service>",
		"<tds:Namespace>http://www.onvif.org/ver10/media/wsdl</tds:Namespace>",
		"<tds:XAddr>", soap_util.escape(x.media), "</tds:XAddr>",
		"<tds:Version><tt:Major>2</tt:Major><tt:Minor>0</tt:Minor></tds:Version>",
		"</tds:Service>",
		"</tds:GetServicesResponse>",
	}))
end


local rsp_get_profiles = function (cfg)
	local list = profiles(cfg)
	local parts = { "<trt:GetProfilesResponse>" }

	for i = 1, #list do
		local p = list[i]
		parts[#parts + 1] = '<trt:Profiles token="' .. soap_util.escape(p.token or ("P" .. i)) .. '">'
		parts[#parts + 1] = "<tt:Name>" .. soap_util.escape(p.name or p.token or ("P" .. i)) .. "</tt:Name>"
		parts[#parts + 1] = "</trt:Profiles>"
	end

	parts[#parts + 1] = "</trt:GetProfilesResponse>"
	return wrap_body(table.concat(parts))
end


local rsp_get_stream_uri = function (cfg, body)
	local token = soap_util.find_tag_text(body, "ProfileToken") or ""
	local list = profiles(cfg)
	local uri = nil

	for i = 1, #list do
		if list[i].token == token then
			uri = list[i].stream_uri
			break
		end
	end

	if nil == uri then
		uri = list[1] and list[1].stream_uri
	end

	if nil == uri or "" == uri then
		return build_fault("unknown ProfileToken", "ter:InvalidArgVal"), false
	end

	return wrap_body(table.concat({
		"<trt:GetStreamUriResponse>",
		"<trt:MediaUri>",
		"<tt:Uri>", soap_util.escape(uri), "</tt:Uri>",
		"<tt:InvalidAfterConnect>false</tt:InvalidAfterConnect>",
		"<tt:InvalidAfterReboot>false</tt:InvalidAfterReboot>",
		"<tt:Timeout>PT60S</tt:Timeout>",
		"</trt:MediaUri>",
		"</trt:GetStreamUriResponse>",
	})), true
end


--------------------------------------------------------------------------------------------
-- 对外


-- @brief 分发 SOAP 请求
-- @param [in]      body[string]		请求体
-- @param [in]      cfg[table]			服务配置
-- @param [in]      headers[table]		[可选] 小写/原样头表; 优先 SOAPAction / Content-Type action=
-- @return soap[string]					应答 SOAP
-- @return action[string|nil]			识别到的操作名
-- @return ok[boolean]					true 已处理; false Fault
-- @return http_code[number|nil]			非 nil 时覆盖 HTTP 状态码 (鉴权失败 401)
-- @note cfg.auth `{ user, password }` 存在时校验 UsernameToken; 缺省不鉴权
soap_serve.dispatch = function (body, cfg, headers)
	local auth_ok, auth_msg = soap_util.verify_security(body, cfg and cfg.auth)
	if not auth_ok then
		return build_fault(auth_msg or "not authorized", "ter:NotAuthorized"), nil, false, 401
	end

	local action = detect_action_from_headers(headers)
	if not action then
		action = detect_action(body)
	end

	if "GetDeviceInformation" == action then
		return rsp_get_device_information(cfg), action, true
	elseif "GetCapabilities" == action then
		return rsp_get_capabilities(cfg), action, true
	elseif "GetServices" == action then
		return rsp_get_services(cfg), action, true
	elseif "GetProfiles" == action then
		return rsp_get_profiles(cfg), action, true
	elseif "GetStreamUri" == action then
		local soap, ok = rsp_get_stream_uri(cfg, body)
		return soap, action, ok
	end

	return build_fault("unsupported action", "ter:ActionNotSupported"), nil, false
end


-- @brief 组装 HTTP 响应头
-- @param [in]      status_code[number]	200 / 400 / 405 / 500
-- @param [in]      body[string]
-- @param [in]      keep_alive[boolean]	[可选] true 保持连接; 默认关闭
-- @return [string]
soap_serve.pack_http_head = function (status_code, body, keep_alive)
	local status = "HTTP/1.1 200 OK"
	if 405 == status_code then
		status = "HTTP/1.1 405 Method Not Allowed"
	elseif 400 == status_code then
		status = "HTTP/1.1 400 Bad Request"
	elseif 401 == status_code then
		status = "HTTP/1.1 401 Unauthorized"
	elseif 500 == status_code then
		status = "HTTP/1.1 500 Internal Server Error"
	elseif 200 ~= status_code then
		status = "HTTP/1.1 " .. tostring(status_code)
	end

	local len = 0
	if "string" == type(body) then
		len = #body
	end

	local conn = "Connection: close\r\n"
	if true == keep_alive then
		conn = "Connection: keep-alive\r\n"
	end

	return table.concat({
		status, "\r\n",
		"Content-Type: application/soap+xml; charset=utf-8\r\n",
		string.format("Content-Length: %d\r\n", len),
		conn,
		"\r\n",
	})
end


soap_serve.detect_action = detect_action


return soap_serve
