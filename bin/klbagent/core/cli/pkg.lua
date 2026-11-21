--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   pkg.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua pkg — list/run; list 结构化→kv 人读
-- @note   alua pkg list [--json]; alua pkg run <name> [args...]
-- @history 修改历史
--  \n [2026-10] 创建文件 (自 cli/init 拆出)
--]]
local packages = require("klbagent.packages")
local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")


local M = {}


M.help = function ()
	print("Usage: alua pkg list [--json]")
	print("       alua pkg run <name> [args...]")
	fmt.print_kv("list", "installed optional packages (default=text)")
	fmt.print_kv("run", "daemon|cli package entry")
	fmt.print_kv("format", "--json|json|-j = JSON(AI)")
end


-- @brief packages 清单 → kv
local function _print_list_text(obj)
	local pkgs = obj.packages
	if type(pkgs) ~= "table" then
		pkgs = {}
	end

	fmt.print_kv("packages", tostring(#pkgs))
	for i = 1, #pkgs do
		local p = pkgs[i]
		print(string.format("  %-12s tier=%s  kind=%s  ver=%s",
			tostring(p.name or "?"),
			tostring(p.tier or ""),
			tostring(p.kind or ""),
			tostring(p.version or "")))
	end
end


-- @brief CLI 入口
-- @param [in] ...		list|run …
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local want_help, want_json, rest = fmt.take_cli_flags({ ... })
	if want_help then
		M.help()
		return true, codes.OK
	end

	local sub = rest[1]
	if sub == "list" then
		local obj = {
			packages = packages.list(),
		}
		if want_json then
			fmt.print_json(obj)
		else
			_print_list_text(obj)
		end

		return true, codes.OK
	end

	if sub == "run" then
		local name = rest[2]
		if type(name) ~= "string" or name == "" then
			fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "need package name"))
			return false, codes.cli.BAD_REQUEST
		end

		local run_argv = {}
		for i = 3, #rest do
			run_argv[#run_argv + 1] = rest[i]
		end

		local ok, code, msg = packages.entry(name, run_argv)
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

		fmt.print_json(codes.make_error(c, msg or "package run fail"))
		return false, c
	end

	M.help()
	fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "alua pkg list|run …"))
	return false, codes.cli.BAD_REQUEST
end


return M
