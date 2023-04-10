--[[
-- Copyright (c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  tableex.lua
-- @brief table extented
--]]
local string = require("string")

local tableex = {}


-- @brief 判定table是否为数组
-- @param [in]		t[table,nil]		table对象
-- @return [boolean]	true.是数组; false.不是数组
tableex.is_array = function(t)
	if 'table' ~= type(t) then 
		return false
	end
	
	local len = #t
	for k, v in pairs(t) do
		if 'number' ~= type(k) then
			return false  
		end
		
		if len < k then
			return false
		end
	end
	
	return true
end


-- @brief 判定table是否为空
-- @param [in]		t[table,nil,{}]		table对象
-- @return [boolean]	true.为空; false.不为空
tableex.is_empty = function(t)
	if 'table' ~= type(t) then
		return true
	end

	if nil ~= next(t) then
		return false
	end
	
	return true
end


-- @brief 判定table不为空
-- @param [in]		t[table,nil,{}]		table对象
-- @return [boolean]	true.不为空; false.为空
tableex.is_not_empty = function(t)
	if 'table' == type(t) then
		if nil ~= next(t) then
			return true
		end
	end
	
	return false
end


-- @brief 复制一个table,只复制:number, string, boolean
-- @param [in]		src [table]		原始table
-- @return [table]	被复制的新table
-- @note 只处理 [number, string, boolean]
tableex.copy = function(src)
	if nil == src then
		return {}
	end

	local dst = {}
	for k, v in pairs(src) do
		if 'table' == type(src[k]) then
			dst[k] = tableex.copy(v)
		elseif 'number' == type(src[k]) or 'string' == type(src[k]) or 'boolean' == type(src[k]) then
			dst[k] = v
		else
			
		end
	end
	
	return dst
end

return tableex
