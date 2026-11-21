--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_ui 可选包入口 (人用运维台; 本机/远程)
-- @note   alua ui|gui (= pkg run alua_ui); 窗体宿主为 kg2de.klbui / g2deapp
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); run 拉起 ui.main; selfcheck 里程碑
--]]
local codes = require("klbagent.code")


local M = {}


-- @brief 包自检
-- @return [table]
M.selfcheck = function ()
	local ok_ui = false
	local ok_require, mod = pcall(require, "kg2de.klbui")

	if ok_require and type(mod) == "table" and type(mod.open) == "function" then
		ok_ui = true
	end

	return {
		ok = true,
		stub = false,
		package = "alua_ui",
		kg2de_klbui = ok_ui,
		milestones = { "M1", "M2", "M3", "M4", "M5", "M6" },
	}
end


-- @brief CLI 入口
-- @param [in] argv[table]
-- @param [in] man[table]
-- @return ok[boolean]
-- @return code[number]
-- @return message[string|nil]
M.entry = function (argv, _man)
	local main = require("klbagent.packages.alua_ui.ui.main")
	local ok, err = main.run({
		argv = argv,
		title = "alua-ui",
	})

	if ok then
		return true, codes.OK, nil
	end

	return false, codes.alua_ui.TOOL_FAILED, tostring(err or "alua_ui run fail")
end


return M

-- end
