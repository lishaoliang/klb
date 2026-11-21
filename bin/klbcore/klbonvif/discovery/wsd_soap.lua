--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   wsd_soap.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  WS-Discovery Probe / ProbeMatches SOAP 组装与解析
-- @note   载荷在 Lua 层; C 仅做组播收发
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local krand = require("krand")


local wsd_soap = {}


--------------------------------------------------------------------------------------------
-- 内部


local escape_xml = function (s)
	if "string" ~= type(s) then
		return ""
	end

	s = string.gsub(s, "&", "&amp;")
	s = string.gsub(s, "<", "&lt;")
	s = string.gsub(s, ">", "&gt;")
	s = string.gsub(s, "\"", "&quot;")
	return s
end


local new_message_id = function ()
	return "uuid:" .. krand.rand_string(8) .. "-" .. krand.rand_string(4)
		.. "-" .. krand.rand_string(4) .. "-" .. krand.rand_string(4)
		.. "-" .. krand.rand_string(12)
end


local find_tag_text = function (xml, tag)
	if "string" ~= type(xml) or "string" ~= type(tag) then
		return nil
	end

	-- <prefix:Tag ...>text</prefix:Tag> or <Tag>text</Tag>
	local pat = "<[%w%-]*:?" .. tag .. "[^>]*>([^<]*)</[%w%-]*:?" .. tag .. ">"
	return string.match(xml, pat)
end


local find_relates_to = function (xml)
	return find_tag_text(xml, "MessageID")
end


--------------------------------------------------------------------------------------------
-- 组装


-- @brief 组装 Probe SOAP
-- @param [in]      opts[table]			[可选] types / scopes / message_id
-- @return [string]	SOAP XML
wsd_soap.build_probe = function (opts)
	opts = opts or {}

	local message_id = opts.message_id
	if "string" ~= type(message_id) or "" == message_id then
		message_id = new_message_id()
	end

	local types = opts.types
	if "string" ~= type(types) or "" == types then
		types = "dn:NetworkVideoTransmitter"
	end

	local scopes_xml = ""
	if "string" == type(opts.scopes) and "" ~= opts.scopes then
		scopes_xml = "\n      <d:Scopes>" .. escape_xml(opts.scopes) .. "</d:Scopes>"
	end

	return table.concat({
		'<?xml version="1.0" encoding="UTF-8"?>',
		'<s:Envelope xmlns:s="http://www.w3.org/2003/05/soap-envelope"',
		' xmlns:a="http://schemas.xmlsoap.org/ws/2004/08/addressing"',
		' xmlns:d="http://schemas.xmlsoap.org/ws/2005/04/discovery"',
		' xmlns:dn="http://www.onvif.org/ver10/network/wsdl">',
		"  <s:Header>",
		'    <a:Action s:mustUnderstand="1">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</a:Action>',
		"    <a:MessageID>", escape_xml(message_id), "</a:MessageID>",
		'    <a:To s:mustUnderstand="1">urn:schemas-xmlsoap-org:ws:2005:04:discovery</a:To>',
		"  </s:Header>",
		"  <s:Body>",
		"    <d:Probe>",
		"      <d:Types>", escape_xml(types), "</d:Types>",
		scopes_xml,
		"    </d:Probe>",
		"  </s:Body>",
		"</s:Envelope>",
	})
end


-- @brief 组装 ProbeMatches SOAP
-- @param [in]      cfg[table]			xaddr / types / scopes / endpoint / relates_to
-- @return [string]	SOAP XML
wsd_soap.build_probe_matches = function (cfg)
	cfg = cfg or {}

	local xaddr = cfg.xaddr or cfg.XAddrs or ""
	local types = cfg.types or "dn:NetworkVideoTransmitter"
	local scopes = cfg.scopes or "onvif://www.onvif.org/Profile/Streaming"
	local endpoint = cfg.endpoint
	if "string" ~= type(endpoint) or "" == endpoint then
		endpoint = new_message_id()
	end

	local message_id = cfg.message_id
	if "string" ~= type(message_id) or "" == message_id then
		message_id = new_message_id()
	end

	local relates_to = cfg.relates_to or ""
	local relates_xml = ""
	if "string" == type(relates_to) and "" ~= relates_to then
		relates_xml = "\n    <a:RelatesTo>" .. escape_xml(relates_to) .. "</a:RelatesTo>"
	end

	return table.concat({
		'<?xml version="1.0" encoding="UTF-8"?>',
		'<s:Envelope xmlns:s="http://www.w3.org/2003/05/soap-envelope"',
		' xmlns:a="http://schemas.xmlsoap.org/ws/2004/08/addressing"',
		' xmlns:d="http://schemas.xmlsoap.org/ws/2005/04/discovery"',
		' xmlns:dn="http://www.onvif.org/ver10/network/wsdl">',
		"  <s:Header>",
		'    <a:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches</a:Action>',
		"    <a:MessageID>", escape_xml(message_id), "</a:MessageID>",
		relates_xml,
		'    <a:To>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</a:To>',
		"  </s:Header>",
		"  <s:Body>",
		"    <d:ProbeMatches>",
		"      <d:ProbeMatch>",
		"        <a:EndpointReference><a:Address>", escape_xml(endpoint), "</a:Address></a:EndpointReference>",
		"        <d:Types>", escape_xml(types), "</d:Types>",
		"        <d:Scopes>", escape_xml(scopes), "</d:Scopes>",
		"        <d:XAddrs>", escape_xml(xaddr), "</d:XAddrs>",
		"        <d:MetadataVersion>1</d:MetadataVersion>",
		"      </d:ProbeMatch>",
		"    </d:ProbeMatches>",
		"  </s:Body>",
		"</s:Envelope>",
	})
end


--------------------------------------------------------------------------------------------
-- 解析


-- @brief 是否为 Probe 请求
-- @param [in]      xml[string]			SOAP XML
-- @return [boolean]
wsd_soap.is_probe = function (xml)
	if "string" ~= type(xml) then
		return false
	end

	if nil ~= string.find(xml, "discovery/Probe", 1, true)
		and nil == string.find(xml, "discovery/ProbeMatches", 1, true) then
		return true
	end

	if nil ~= string.find(xml, ":Probe>", 1, true)
		or nil ~= string.find(xml, "<Probe>", 1, true) then
		if nil == string.find(xml, "ProbeMatches", 1, true) then
			return true
		end
	end

	return false
end


-- @brief 解析 Probe 中的 MessageID (供 RelatesTo)
-- @param [in]      xml[string]
-- @return [string|nil]
wsd_soap.probe_message_id = function (xml)
	return find_relates_to(xml)
end


-- @brief 解析对端 "ip:port"
-- @param [in]      peer[string]
-- @return ip[string|nil], port[number|nil]
wsd_soap.parse_peer = function (peer)
	if "string" ~= type(peer) or "" == peer then
		return nil, nil
	end

	local ip, port = string.match(peer, "^([%d%.]+):(%d+)$")
	if nil == ip then
		return nil, nil
	end

	return ip, tonumber(port)
end


-- @brief 解析 ProbeMatches
-- @param [in]      xml[string]			SOAP XML
-- @param [in]      peer[string]		[可选] 对端 "ip:port"
-- @return [table|nil]	{ peer, xaddrs, types, scopes, endpoint, xml }
wsd_soap.parse_probe_matches = function (xml, peer)
	if "string" ~= type(xml) or "" == xml then
		return nil
	end

	if nil == string.find(xml, "ProbeMatch", 1, true) then
		return nil
	end

	local xaddrs_raw = find_tag_text(xml, "XAddrs") or ""
	local xaddrs = {}
	for addr in string.gmatch(xaddrs_raw, "%S+") do
		xaddrs[#xaddrs + 1] = addr
	end

	local item = {
		peer = peer,
		xaddrs = xaddrs,
		xaddr = xaddrs[1],
		types = find_tag_text(xml, "Types"),
		scopes = find_tag_text(xml, "Scopes"),
		endpoint = find_tag_text(xml, "Address"),
		xml = xml,
	}

	return item
end


return wsd_soap
