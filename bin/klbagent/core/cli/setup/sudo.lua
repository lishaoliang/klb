--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   sudo.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua setup sudo / alua sudo — 目标机限定 NOPASSWD
-- @note   结果默认 kv; --json 机读; check 只探测; 仅 linux/wsl 有效
-- @history 修改历史
--  \n [2026-10] 创建文件 (自 cli/setup.lua 拆出)
--]]
local dispatch = require("klbagent.core.dispatch")
local codes = require("klbagent.code")
local profiles = require("klbagent.core.profiles")
local fmt = require("klbagent.core.cli.fmt")


local M = {}


M.help = function ()
	print("Usage: alua setup sudo [check] [--json]")
	print("       alua sudo [check] [--json]")
	fmt.print_kv("sudo", "once on target: limited NOPASSWD")
	fmt.print_kv("check", "probe only (no write)")
	fmt.print_kv("format", "--json|json|-j = JSON(AI)")
	fmt.print_kv("platform", "linux/wsl only")
end


local function _read_sudo_pass()
	local env = os.getenv("ALUA_DEPLOY_SUDO")
	if type(env) == "string" and env ~= "" then
		return env
	end

	io.stderr:write("sudo password (once, not stored): ")
	io.stderr:flush()
	os.execute("stty -echo 2>/dev/null")
	local pass = io.read("*l")
	os.execute("stty echo 2>/dev/null")
	io.stderr:write("\n")
	if type(pass) ~= "string" then
		return ""
	end

	return pass
end


local function _print_result_text(result, check_only)
	if check_only then
		local sn = type(result) == "table" and result.sudo_n == true
		fmt.print_kv("sudo_n", tostring(sn))
		fmt.print_kv("dropin", tostring(type(result) == "table" and result.dropin_present))
		return
	end

	fmt.print_kv("setup", "ok")
	fmt.print_kv("note", "limited NOPASSWD; later deploy can use sudo -n")
end


-- @brief sudo 子命令
-- @param [in] argv[table|string|nil]	{ "check" } | "check"
-- @param [in] want_json[boolean]
-- @return ok[boolean]
-- @return code[number]
M.run = function (argv, want_json)
	local mode = nil
	if type(argv) == "table" then
		mode = argv[1]
	else
		mode = argv
	end

	if fmt.is_help_flag(mode) then
		M.help()
		return true, codes.OK
	end

	local check_only = (mode == "check" or mode == "--check")

	local ok_pf, err_pf = profiles.set_override("unsafe")
	if not ok_pf then
		io.stderr:write("alua setup sudo: set profile fail: " .. tostring(err_pf) .. "\n")
		fmt.print_json(codes.make_error(codes.cli.TOOL_FAILED, tostring(err_pf or "profile")))
		return false, codes.cli.TOOL_FAILED
	end

	local args = {}
	if check_only then
		args.check_only = true
		io.stderr:write("alua setup sudo: check only\n")
	else
		args.confirm = true
		local pass = _read_sudo_pass()
		if pass ~= "" then
			args.sudo_pass = pass
		end
	end

	local ok, result = dispatch.call({
		name = "deploy.setup_priv",
		args = args,
	})
	if not ok then
		local msg = "setup sudo fail"
		if type(result) == "table" and type(result.message) == "string" then
			msg = result.message
		end

		io.stderr:write("alua setup sudo: " .. msg .. "\n")
		fmt.print_json(result)
		local code = codes.cli.TOOL_FAILED
		if type(result) == "table" and type(result.code) == "number" then
			code = result.code
		end

		return false, code
	end

	if want_json then
		fmt.print_json({
			ok = true,
			result = result,
		})
	else
		_print_result_text(result, check_only)
	end

	return true, codes.OK
end


return M

-- end
