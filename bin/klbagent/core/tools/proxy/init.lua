--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy.* 工具入口 (桩)
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); proxy.use 临时切链; proxy.bench 全链路测速
--]]


local status = require("klbagent.core.tools.proxy.status")
local use = require("klbagent.core.tools.proxy.use")
local bench = require("klbagent.core.tools.proxy.bench")


local M = {}


local MODULES = {
	status,
	use,
	bench,
}


-- @brief 注册 proxy 工具
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	for i = 1, #MODULES do
		local ok, err = MODULES[i].register(catalog, dispatch)
		if not ok then
			return false, err
		end
	end

	return true
end


return M

-- end
