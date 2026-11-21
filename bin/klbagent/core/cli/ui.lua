--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   ui.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua ui|gui — 人用运维 UI (alua_ui 包)
-- @note   非 JSON 查询; 失败时 JSON envelope
-- @history 修改历史
--  \n [2026-10] 创建文件 (自 cli/init 拆出)
--]]
local packages = require("klbagent.packages")
local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")


local M = {}


M.help = function ()
	print("Usage: alua ui [args...]")
	print("       alua gui [args...]")
	fmt.print_kv("ui", "human ops UI (package alua_ui; alias gui)")
end


-- @brief CLI 入口
-- @param [in] ...		传给 alua_ui 的参数
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local want_help, _, rest = fmt.take_cli_flags({ ... })
	if want_help then
		M.help()
		return true, codes.OK
	end

	local argv = rest
	local ok, code, msg = packages.entry("alua_ui", argv)
	if ok then
		if type(msg) == "string" and msg ~= "" then
			print(msg)
		end
		return true, codes.OK
	end

	local c = code
	if type(c) ~= "number" then
		c = codes.cli.TOOL_FAILED
	end

	fmt.print_json(codes.make_error(c, msg or "alua_ui run fail"))
	return false, c
end


return M
