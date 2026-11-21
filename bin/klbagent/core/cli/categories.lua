--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   categories.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua categories — 工具分类; catalog→kv 人读
-- @note   alua categories [--json]
-- @history 修改历史
--  \n [2026-10] 创建文件 (自 cli/init 拆出)
--]]
local catalog = require("klbagent.core.catalog")
local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")


local M = {}


M.help = function ()
	print("Usage: alua categories [--json]")
	fmt.print_kv("categories", "tool category ids")
	fmt.print_kv("format", "--json|json|-j = JSON(AI)")
end


local function _print_text(cats)
	if type(cats) ~= "table" then
		cats = {}
	end

	fmt.print_kv("categories", tostring(#cats))
	for i = 1, #cats do
		local c = cats[i]
		local id = c
		local profile = ""
		if type(c) == "table" then
			id = c.id or c.name or "?"
			profile = c.min_profile or ""
		end

		print(string.format("  %-12s profile=%s", tostring(id), tostring(profile)))
	end
end


-- @brief CLI 入口
-- @param [in] ...		[--json]
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local want_help, want_json = fmt.take_cli_flags({ ... })
	if want_help then
		M.help()
		return true, codes.OK
	end

	local obj = {
		categories = catalog.categories(),
	}
	if want_json then
		fmt.print_json(obj)
		return true, codes.OK
	end

	_print_text(obj.categories)
	return true, codes.OK
end


return M
