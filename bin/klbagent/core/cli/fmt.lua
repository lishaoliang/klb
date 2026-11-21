--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   fmt.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  CLI 人读/机读输出共用 (status 同形 kv; --json 开关; tool emit)
-- @note   默认人读; --json|json|-j 机读; 开关须在子命令之后
-- @history 修改历史
--  \n [2026-10] 创建文件
--]]
local cjson = require("cjson")
local dispatch = require("klbagent.core.dispatch")
local codes = require("klbagent.code")


local M = {}


-- @brief 与 status.lua 同形: "key:          value"
M.print_kv = function (k, v)
	print(string.format("%-14s %s", k .. ":", tostring(v)))
end


M.print_json = function (obj)
	local ok, text = pcall(cjson.encode, obj)
	if not ok or type(text) ~= "string" then
		print("{}")
		return
	end

	print(text)
end


M.is_json_flag = function (a)
	return a == "json" or a == "--json" or a == "-j"
end


M.is_help_flag = function (a)
	return a == "help" or a == "--help" or a == "-h"
end


-- @return yes[boolean]
M.has_help_flag = function (argv)
	if type(argv) ~= "table" then
		return false
	end

	for i = 1, #argv do
		if M.is_help_flag(argv[i]) then
			return true
		end
	end

	return false
end


-- @return want_json[boolean]
-- @return rest[table]
M.take_json_flag = function (argv)
	local want = false
	local rest = {}
	if type(argv) ~= "table" then
		return want, rest
	end

	for i = 1, #argv do
		local a = argv[i]
		if M.is_json_flag(a) then
			want = true
		elseif not M.is_help_flag(a) then
			rest[#rest + 1] = a
		end
	end

	return want, rest
end


-- @return want_help[boolean]
-- @return want_json[boolean]
-- @return rest[table]
M.take_cli_flags = function (argv)
	local want_help = false
	local want_json = false
	local rest = {}
	if type(argv) ~= "table" then
		return want_help, want_json, rest
	end

	for i = 1, #argv do
		local a = argv[i]
		if M.is_help_flag(a) then
			want_help = true
		elseif M.is_json_flag(a) then
			want_json = true
		else
			rest[#rest + 1] = a
		end
	end

	return want_help, want_json, rest
end


-- @brief 调工具; 成功按 want_json 输出; 失败仍 JSON
-- @return ok[boolean]
-- @return code[number]
M.emit_tool = function (name, args, want_json, human_print)
	local ok, result = dispatch.call({
		name = name,
		args = args or {},
	})
	if not ok then
		M.print_json(result)
		local code = codes.cli.TOOL_FAILED
		if type(result) == "table" and type(result.code) == "number" then
			code = result.code
		end

		return false, code
	end

	if want_json then
		M.print_json({
			ok = true,
			result = result,
		})
	elseif type(human_print) == "function" then
		human_print(result)
	else
		M.print_json({
			ok = true,
			result = result,
		})
	end

	return true, codes.OK
end


return M
