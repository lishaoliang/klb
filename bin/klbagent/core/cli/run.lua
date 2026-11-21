--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   run.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua run — unsafe.run_file (始终 JSON)
-- @note   须 runtime.profile=unsafe; 可按脚本 os.exit 回传进程码
-- @history 修改历史
--  \n [2026-10] 创建文件 (自 cli/init 拆出)
--]]
local dispatch = require("klbagent.core.dispatch")
local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")


local M = {}


M.help = function ()
	print("Usage: alua run <path.lua> [args...]")
	fmt.print_kv("run", "unsafe.run_file; always JSON; needs profile=unsafe")
end


-- @brief CLI 入口
-- @param [in] ...		path.lua [args...]
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local want_help, _, rest = fmt.take_cli_flags({ ... })
	if want_help then
		M.help()
		return true, codes.OK
	end

	local path = rest[1]
	if type(path) ~= "string" or path == "" then
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "need lua path"))
		return false, codes.cli.BAD_REQUEST
	end

	local script_argv = { table.unpack(rest, 2) }
	local ok, result = dispatch.call({
		name = "unsafe.run_file",
		args = {
			path = path,
			argv = script_argv,
		},
	})
	if not ok then
		fmt.print_json(result)
		local code = codes.cli.TOOL_FAILED
		if type(result) == "table" and type(result.code) == "number" then
			code = result.code
		end

		return false, code
	end

	fmt.print_json({
		ok = true,
		result = result,
	})

	if type(result) == "table"
		and result.exit_intercepted == true
		and result.exit_via == "os.exit"
		and type(result.exit_code) == "number" then
		local code = math.floor(result.exit_code)
		if code < 0 then
			code = 1
		elseif code > 255 then
			code = code % 256
		end

		if code == 0 then
			return true, codes.OK
		end

		return false, code
	end

	return true, codes.OK
end


return M
