--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  fs.* 工具入口
-- @note   stat/list/read/write/mkdir/remove/copy/move/grep 见同目录子模块
-- @history 修改历史
--  \n [2026-09] 创建文件; 补充 list/read/write/mkdir/remove/copy/move/grep
--  \n [2026-10] 按子命令拆分子模块
--]]


local stat_list = require("klbagent.core.tools.fs.stat_list")
local read_write = require("klbagent.core.tools.fs.read_write")
local tree_ops = require("klbagent.core.tools.fs.tree_ops")
local grep = require("klbagent.core.tools.fs.grep")


local M = {}


local MODULES = {
	stat_list,
	read_write,
	tree_ops,
	grep,
}


-- @brief 注册 fs 工具
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
