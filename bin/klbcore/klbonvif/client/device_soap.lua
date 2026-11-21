--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   device_soap.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ONVIF Device 服务 SOAP 体与解析
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local soap_util = require("klbcore.klbonvif.client.soap_util")


local device_soap = {}


device_soap.ACTION_GET_DEVICE_INFORMATION = "http://www.onvif.org/ver10/device/wsdl/GetDeviceInformation"
device_soap.ACTION_GET_CAPABILITIES = "http://www.onvif.org/ver10/device/wsdl/GetCapabilities"
device_soap.ACTION_GET_SERVICES = "http://www.onvif.org/ver10/device/wsdl/GetServices"


device_soap.build_get_device_information = function ()
	return "<tds:GetDeviceInformation />"
end


device_soap.build_get_capabilities = function (category)
	if "string" ~= type(category) or "" == category then
		category = "All"
	end

	return "<tds:GetCapabilities><tds:Category>"
		.. soap_util.escape(category)
		.. "</tds:Category></tds:GetCapabilities>"
end


device_soap.build_get_services = function (include_capability)
	local flag = "false"
	if include_capability then
		flag = "true"
	end

	return "<tds:GetServices><tds:IncludeCapability>"
		.. flag
		.. "</tds:IncludeCapability></tds:GetServices>"
end


-- @brief 解析 GetDeviceInformationResponse
device_soap.parse_device_information = function (xml)
	if "string" ~= type(xml) then
		return nil
	end

	return {
		manufacturer = soap_util.find_tag_text(xml, "Manufacturer"),
		model = soap_util.find_tag_text(xml, "Model"),
		firmware_version = soap_util.find_tag_text(xml, "FirmwareVersion"),
		serial_number = soap_util.find_tag_text(xml, "SerialNumber"),
		hardware_id = soap_util.find_tag_text(xml, "HardwareId"),
		xml = xml,
	}
end


-- @brief 从 Capabilities 中取 Media XAddr
device_soap.parse_media_xaddr = function (xml)
	if "string" ~= type(xml) then
		return nil
	end

	-- <tt:Media> ... <tt:XAddr>url</tt:XAddr>
	local media_block = string.match(xml, "<[%w%-]*:?Media[^>]*>(.-)</[%w%-]*:?Media>")
	if nil ~= media_block then
		local xaddr = soap_util.find_tag_text(media_block, "XAddr")
		if nil ~= xaddr and "" ~= xaddr then
			return xaddr
		end
	end

	-- fallback: first XAddr after Media
	local xaddr = string.match(xml, "Media[^>]*>.-<[%w%-]*:?XAddr[^>]*>([^<]+)</")
	return xaddr
end


-- @brief 解析 GetServicesResponse 中 Namespace -> XAddr
device_soap.parse_services = function (xml)
	local list = {}
	if "string" ~= type(xml) then
		return list
	end

	for block in string.gmatch(xml, "<[%w%-]*:?Service[^>]*>(.-)</[%w%-]*:?Service>") do
		list[#list + 1] = {
			namespace = soap_util.find_tag_text(block, "Namespace"),
			xaddr = soap_util.find_tag_text(block, "XAddr"),
		}
	end

	return list
end


return device_soap
