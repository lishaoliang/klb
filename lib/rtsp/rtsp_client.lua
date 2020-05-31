--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief rtsp client �ı�����
-- @author ������
--]]
local table = require("table")
local string = require("string")

local trim = require("base.stringex").trim

local rtsp_client = {}

local parser_frist = function (res, line)
	return true
end

local parser_colon = function (res, line)
	local k, v = string.match(line, '([^:]+):(.*)') -- ƥ�����
	if nil ~= k and nil ~= v then
		k = trim(k)
		v = trim(v)
		
		res[k] = v
		
		return true
	end
	
	return false
end

rtsp_client.parser = function (head, body)
	local res = {
	}
	
	-- head
	local frist = true
	for line in string.gmatch(head, '[^\r\n]+\r\n') do
		if frist then
			parser_frist(res, line)
		else
			parser_colon(res, line)
		end
		
		frist = false
	end
	
	return res
end


return rtsp_client
