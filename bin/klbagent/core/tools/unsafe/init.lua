--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  unsafe.* 工具入口
-- @note   status / run_file / exec / process_* 见同目录子模块
-- @history 修改历史
--  \n [2026-09] 创建文件; 补充 unsafe.exec; 补充 process_list / process_kill
--  \n [2026-10] 按子命令拆分子模块
--]]


local status = require("klbagent.core.tools.unsafe.status")
local run_file = require("klbagent.core.tools.unsafe.run_file")
local exec = require("klbagent.core.tools.unsafe.exec")
local process = require("klbagent.core.tools.unsafe.process")


local M = {}


local MODULES = {
	status,
	run_file,
	exec,
	process,
}


-- @brief 注册 unsafe 工具
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
