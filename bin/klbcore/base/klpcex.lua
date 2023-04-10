--[[
-- Copyright (c) 2022, GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  klpcex.lua
-- @brief klpc extented
--]]
local klpc = require("klpc")


local klpcex = {}



-- @brief 调用本地模块提供的方法(可跨线程Lua环境)
-- @param [in]		mo_name[string]		模块名称
-- @param [in]		...[任意类型]		参数数据
-- @return [...]	模块回复的数据
-- @note 仅在协程中使用
function klpcex.call(mo_name, ...)
	local lpc = klpc.new()
	
	return (function (...)
		lpc:close()
		return ...
	end)(lpc:co_call(mo_name, ...))
end


return klpcex
