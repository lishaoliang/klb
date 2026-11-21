--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  runtime.* 公共辅助
-- @note   环境变量名校验
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local M = {}


function M.name_ok(name)
	if type(name) ~= "string" or name == "" then
		return false
	end

	if string.find(name, "[\r\n\0=]") ~= nil then
		return false
	end

	return true
end


return M

-- end
