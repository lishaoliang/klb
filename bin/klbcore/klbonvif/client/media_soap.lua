--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   media_soap.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ONVIF Media 服务 SOAP 体与解析
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local soap_util = require("klbcore.klbonvif.client.soap_util")


local media_soap = {}


media_soap.ACTION_GET_PROFILES = "http://www.onvif.org/ver10/media/wsdl/GetProfiles"
media_soap.ACTION_GET_STREAM_URI = "http://www.onvif.org/ver10/media/wsdl/GetStreamUri"


media_soap.build_get_profiles = function ()
	return "<trt:GetProfiles />"
end


-- @brief 组装 GetStreamUri
-- @param [in]      profile_token[string]
-- @param [in]      opts[table]			[可选] stream / protocol
media_soap.build_get_stream_uri = function (profile_token, opts)
	opts = opts or {}
	local stream = opts.stream or "RTP-Unicast"
	local protocol = opts.protocol or "RTSP"

	return table.concat({
		"<trt:GetStreamUri>",
		"<trt:StreamSetup>",
		"<tt:Stream>", soap_util.escape(stream), "</tt:Stream>",
		"<tt:Transport><tt:Protocol>", soap_util.escape(protocol), "</tt:Protocol></tt:Transport>",
		"</trt:StreamSetup>",
		"<trt:ProfileToken>", soap_util.escape(profile_token), "</trt:ProfileToken>",
		"</trt:GetStreamUri>",
	})
end


-- @brief 解析 GetProfilesResponse
-- @return [table]	{ {token, name}, ... }
media_soap.parse_profiles = function (xml)
	local list = {}
	if "string" ~= type(xml) then
		return list
	end

	-- Profiles often: <trt:Profiles token="xxx">...</trt:Profiles>
	for attrs, inner in string.gmatch(xml, "<[%w%-]*:?Profiles([^>]*)>(.-)</[%w%-]*:?Profiles>") do
		local token = string.match(attrs, 'token%s*=%s*"([^"]+)"')
		if nil == token then
			token = soap_util.find_tag_text(inner, "token")
		end

		local name = soap_util.find_tag_text(inner, "Name")
		if nil ~= token and "" ~= token then
			list[#list + 1] = {
				token = token,
				name = name,
			}
		end
	end

	return list
end


-- @brief 解析 GetStreamUriResponse
media_soap.parse_stream_uri = function (xml)
	if "string" ~= type(xml) then
		return nil
	end

	local uri = soap_util.find_tag_text(xml, "Uri")
	if nil == uri or "" == uri then
		return nil
	end

	return {
		uri = uri,
		xml = xml,
	}
end


return media_soap
