--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   docs.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua docs — docs.list/read; 结果→kv/正文 人读
-- @note   alua docs [path] [--json]
-- @history 修改历史
--  \n [2026-10] 创建文件 (自 cli/init 拆出)
--]]
local fmt = require("klbagent.core.cli.fmt")
local codes = require("klbagent.code")


local M = {}


M.help = function ()
	print("Usage: alua docs [path] [--json]")
	fmt.print_kv("docs", "no path = list; path = read markdown")
	fmt.print_kv("format", "--json|json|-j = JSON(AI)")
end


local function _print_list_text(result)
	local files = type(result) == "table" and result.files or {}
	fmt.print_kv("docs", tostring(#files))
	for i = 1, #files do
		print("  " .. tostring(files[i]))
	end
end


local function _print_read_text(result)
	if type(result) ~= "table" then
		print(tostring(result))
		return
	end

	local text = result.text
	if type(text) ~= "string" then
		text = ""
	end

	print(text)
end


-- @brief CLI 入口
-- @param [in] ...		[path] [--json]
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local want_help, want_json, rest = fmt.take_cli_flags({ ... })
	if want_help then
		M.help()
		return true, codes.OK
	end

	local path = rest[1]
	if type(path) ~= "string" or path == "" then
		return fmt.emit_tool("docs.list", {}, want_json, _print_list_text)
	end

	return fmt.emit_tool("docs.read", { path = path }, want_json, _print_read_text)
end


return M
