--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  lpc.* 工具入口
-- @history 修改历史
--  \n [2026-10] 创建文件 (LPC L1)
--]]


local call = require("klbagent.core.tools.lpc.call")


local M = {}


local MODULES = {
	call,
}


M.register = function (catalog, dispatch_reg)
	for i = 1, #MODULES do
		local ok, err = MODULES[i].register(catalog, dispatch_reg)
		if not ok then
			return false, err
		end
	end

	return true
end


return M

-- end
