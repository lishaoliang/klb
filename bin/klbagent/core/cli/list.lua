--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   list.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua list — 工具目录; catalog→kv 人读
-- @note   alua list [category] [--json]
-- @history 修改历史
--  \n [2026-10] 创建文件 (自 cli/init 拆出)
--]]
local catalog = require("klbagent.core.catalog")
local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")


local M = {}


M.help = function ()
	print("Usage: alua list [category] [--json]")
	fmt.print_kv("list", "tool catalog; optional category filter")
	fmt.print_kv("format", "--json|json|-j = JSON(AI)")
end


local function _print_text(tools)
	if type(tools) ~= "table" then
		tools = {}
	end

	fmt.print_kv("tools", tostring(#tools))
	for i = 1, #tools do
		local t = tools[i]
		print(string.format("  %-28s cat=%s  profile=%s",
			tostring(t.name or "?"),
			tostring(t.category or ""),
			tostring(t.min_profile or "")))
	end
end


-- @brief CLI 入口
-- @param [in] ...		[category] [--json]
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local want_help, want_json, rest = fmt.take_cli_flags({ ... })
	if want_help then
		M.help()
		return true, codes.OK
	end

	local category = rest[1]
	local opts = {}
	if type(category) == "string" and category ~= "" then
		if not catalog.is_category(category) then
			fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "bad category"))
			return false, codes.cli.BAD_REQUEST
		end

		opts.category = category
	end

	local obj = {
		tools = catalog.list(opts),
	}
	if want_json then
		fmt.print_json(obj)
		return true, codes.OK
	end

	_print_text(obj.tools)
	return true, codes.OK
end


return M
