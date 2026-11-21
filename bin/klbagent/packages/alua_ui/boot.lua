--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   boot.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  g2deapp 入口: 挂路径后跑 alua_ui 宿主
-- @note   g2deapp <path>/boot.lua; 由 alua ui 在无进程内 kg2de.klbui 时拉起
-- @history 修改历史
--  \n [2026-10] 创建文件 (M1)
--]]
local kenv = require("kenv")


local base = kenv.base_path()

package.path = base .. "?.lua"
	.. ";" .. base .. "?/init.lua"
	.. ";" .. base .. "klbcore/?.lua"
	.. ";" .. base .. "klbcore/?/init.lua"
	.. ";" .. base .. "klbagent/?.lua"
	.. ";" .. base .. "klbagent/?/init.lua"
	.. ";" .. package.path


local main = require("klbagent.packages.alua_ui.ui.main")
local ok, err = main.run({
	title = "alua-ui",
})

if not ok then
	io.stderr:write("alua_ui boot fail: " .. tostring(err) .. "\n")
	os.exit(1)
end

os.exit(0)

-- end
