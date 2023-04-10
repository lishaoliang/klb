--[[
-- Copyright (c) 2022, GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  krpcex.lua
-- @brief krpc extented
--]]
local krpc = require("krpc")


local krpcex = {}


-- @brief 调用远程模块(网络)提供的方法
-- @param [in]		ip[string]			ip
-- @param [in]		port[number(int)]	端口
-- @param [in]		...[任意类型]		参数数据
-- @return [...]	模块回复的数据
-- @note 仅在协程中使用
function krpcex.call(ip, port, ...)	
	local rpc = krpc.new(ip, port)
	
	return (function (...)
		rpc:close()
		return ...
	end)(rpc:co_call(...))	
end


-- @brief Post调用远程模块(网络)提供的方法
-- @param [in]		ip[string]			ip
-- @param [in]		port[number(int)]	端口
-- @param [in]		...[任意类型]		参数数据
-- @return [...]	模块回复的数据
function krpcex.post(ip, port, ...)	
	local rpc = krpc.new(ip, port)
	
	return (function (...)
		rpc:close()
		return ...
	end)(rpc:post(...))
end


return krpcex
