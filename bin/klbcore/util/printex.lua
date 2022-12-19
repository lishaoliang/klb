--[[
-- Copyright (c) 2022, GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  printex.lua
-- @brief print extented
--]]
local cjson = require("cjson.safe")


local function copy_to_string_table(t)
	local dst = {}
	
	for k, v in pairs(t) do
		if type(v) == 'table' then
			dst[k] = copy_to_string_table(v)
		elseif type(v) == 'string' or type(v) == 'number' or type(v) == 'boolean' then
			dst[k] = v
		else
			dst[k] = tostring(v)
		end
	end
	
	return dst
end

local printex = function (...)
	local arg = {...}	
	local tmp = copy_to_string_table(arg)
	
	print(cjson.encode(tmp))
end

return printex
