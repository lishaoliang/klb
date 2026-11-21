--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   exec.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  unsafe.exec
-- @note   cmd 或 argv 二选一; Win32 cmd.exe 语义
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; BusyBox 兼容: timeout -k 2 <SECS>; cwd 改 sh 内 cd (Bug.2026-14)
--]]


local lfs = require("lfs")
local util = require("klbagent.core.tools.unsafe.util")


local M = {}


local function _merge_env(args_env)
	local runtime = require("klbagent.core.tools.runtime")
	local set, unset = runtime.child_env()
	if type(args_env) == "table" then
		for k, v in pairs(args_env) do
			if type(k) == "string" and k ~= "" and string.find(k, "[\r\n\0=]") == nil then
				if v == nil then
					unset[#unset + 1] = k
					set[k] = nil
				else
					set[k] = tostring(v)
				end
			end
		end
	end

	return set, unset
end


-- @brief Win32 临时文件对 (os.tmpname 在 Win 落盘符根目录, 改用 %TEMP%)
local function _win_tmp_paths()
	local dir = os.getenv("TEMP") or os.getenv("TMP") or "."
	local uniq = tostring({}):match("(%x+)$") or "0"
	return dir .. "\\alua_" .. uniq .. "_out.tmp", dir .. "\\alua_" .. uniq .. "_err.tmp"
end


-- @brief 解析 unsafe.exec 目标: cmd 或 argv 二选一
local function _parse_exec_target(args)
	local cmd = args.cmd
	local argv = args.argv
	local has_cmd = type(cmd) == "string" and cmd ~= ""
	local has_argv = type(argv) == "table" and #argv >= 1

	if has_cmd and has_argv then
		error("use cmd or argv, not both")
	end

	if has_cmd then
		if string.find(cmd, "[\0]") ~= nil then
			error("bad cmd")
		end

		return "cmd", cmd, nil
	end

	if has_argv then
		local prog_argv = {}
		for i = 1, #argv do
			local v = argv[i]
			if type(v) ~= "string" then
				error("argv must be strings")
			end

			if string.find(v, "[\0]") ~= nil then
				error("bad argv")
			end

			prog_argv[#prog_argv + 1] = v
		end

		return "argv", nil, prog_argv
	end

	error("need cmd or argv")
end


-- @brief Win32 分支: cmd.exe 语义; 无 timeout 强制 (返回 timed_out 恒 false), unset 退化为置空
local function _exec_win32(mode, cmd, prog_argv, cwd, set, unset, timeout_sec, max_bytes)
	local out_path, err_path = _win_tmp_paths()

	local chain = {}
	if cwd ~= nil then
		chain[#chain + 1] = "cd /d " .. util.win_quote(cwd)
	end

	for i = 1, #unset do
		chain[#chain + 1] = 'set "' .. unset[i] .. '="'
	end

	for k, v in pairs(set) do
		chain[#chain + 1] = 'set "' .. k .. "=" .. string.gsub(v, '"', "") .. '"'
	end

	if mode == "cmd" then
		chain[#chain + 1] = cmd
	else
		local cmdline = {}
		for i = 1, #prog_argv do
			cmdline[#cmdline + 1] = util.win_quote(prog_argv[i])
		end

		chain[#chain + 1] = table.concat(cmdline, " ")
	end

	local line = table.concat(chain, " && ")
		.. " >" .. util.win_quote(out_path)
		.. " 2>" .. util.win_quote(err_path)

	local ok_ex, what, code = os.execute(line)
	local exit_code = 0
	if ok_ex == true then
		exit_code = 0
	elseif what == "exit" and type(code) == "number" then
		exit_code = code
	else
		exit_code = 1
	end

	local stdout, stdout_n, trunc_out = util.read_trunc(out_path, max_bytes)
	local stderr, stderr_n, trunc_err = util.read_trunc(err_path, max_bytes)
	os.remove(out_path)
	os.remove(err_path)

	local ret = {
		mode = mode,
		cwd = cwd,
		exit_code = exit_code,
		timed_out = false,
		timeout_sec = timeout_sec,
		stdout = stdout,
		stderr = stderr,
		stdout_bytes = stdout_n,
		stderr_bytes = stderr_n,
		stdout_truncated = trunc_out,
		stderr_truncated = trunc_err,
		ok = exit_code == 0,
	}
	if mode == "cmd" then
		ret.cmd = cmd
	else
		ret.argv = prog_argv
	end

	return ret
end


local function _exec(args)
	local mode, cmd, prog_argv = _parse_exec_target(args)

	local timeout_sec = util.clamp_timeout(args.timeout_sec)
	local max_bytes = util.clamp_max_bytes(args.max_bytes)
	local cwd = args.cwd
	if cwd ~= nil then
		if type(cwd) ~= "string" or cwd == "" or string.find(cwd, "[\r\n\0]") ~= nil then
			error("bad cwd")
		end

		local a = lfs.attributes(cwd)
		if a == nil or a.mode ~= "directory" then
			error("cwd not a directory")
		end
	end

	local set, unset = _merge_env(args.env)

	if util.is_windows() then
		return _exec_win32(mode, cmd, prog_argv, cwd, set, unset, timeout_sec, max_bytes)
	end

	local out_path = tostring(os.tmpname()) .. "_alua_stdout"
	local err_path = tostring(os.tmpname()) .. "_alua_stderr"

	local parts = {}
	if util.has_timeout() then
		-- GNU/BusyBox 通用: -k KILL_SECS + 裸秒数 (勿用 --kill-after=Ns / Ns)
		parts[#parts + 1] = "timeout"
		parts[#parts + 1] = "-k"
		parts[#parts + 1] = "2"
		parts[#parts + 1] = tostring(timeout_sec)
	end

	parts[#parts + 1] = "env"

	for i = 1, #unset do
		parts[#parts + 1] = "-u"
		parts[#parts + 1] = unset[i]
	end

	for k, v in pairs(set) do
		parts[#parts + 1] = k .. "=" .. v
	end

	if mode == "cmd" then
		parts[#parts + 1] = "sh"
		parts[#parts + 1] = "-c"
		parts[#parts + 1] = cmd
	else
		for i = 1, #prog_argv do
			parts[#parts + 1] = prog_argv[i]
		end
	end

	local cmdline = {}
	for i = 1, #parts do
		cmdline[#cmdline + 1] = util.shell_quote(parts[i])
	end

	local line = table.concat(cmdline, " ")
		.. " >" .. util.shell_quote(out_path)
		.. " 2>" .. util.shell_quote(err_path)

	-- BusyBox env 无 -C: 统一在 sh 内 cd (GNU/BusyBox 通用)
	if cwd ~= nil then
		line = "cd " .. util.shell_quote(cwd) .. " && " .. line
	end

	local ok_ex, what, code = os.execute(line)
	local exit_code = 0
	local timed_out = false
	if ok_ex == true then
		exit_code = 0
	elseif what == "exit" and type(code) == "number" then
		exit_code = code
		-- 124=GNU timeout; 137/143=BusyBox timeout (-k KILL / SIGTERM)
		if code == 124 or code == 137 or code == 143 then
			timed_out = true
		end
	elseif what == "signal" and type(code) == "number" then
		exit_code = 128 + code
	else
		exit_code = 1
	end

	local stdout, stdout_n, trunc_out = util.read_trunc(out_path, max_bytes)
	local stderr, stderr_n, trunc_err = util.read_trunc(err_path, max_bytes)
	os.remove(out_path)
	os.remove(err_path)

	local ret = {
		mode = mode,
		cwd = cwd,
		exit_code = exit_code,
		timed_out = timed_out,
		timeout_sec = timeout_sec,
		stdout = stdout,
		stderr = stderr,
		stdout_bytes = stdout_n,
		stderr_bytes = stderr_n,
		stdout_truncated = trunc_out,
		stderr_truncated = trunc_err,
		ok = exit_code == 0 and not timed_out,
	}
	if mode == "cmd" then
		ret.cmd = cmd
	else
		ret.argv = prog_argv
	end

	return ret
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "unsafe.exec",
		category = "unsafe",
		title = "run external command",
		summary = "Run cmd or argv (mutually exclusive) with optional cwd/env/timeout; capture stdout/stderr. Prefer argv (no shell). Needs unsafe.",
		min_profile = "unsafe",
		args_schema = {
			cmd = "string optional shell command line (mutually exclusive with argv)",
			argv = "array optional strings; argv[1]=program, no shell (mutually exclusive with cmd)",
			cwd = "string optional working directory",
			env = "table optional extra env name->string",
			timeout_sec = "number optional default 60 max 300",
			max_bytes = "number optional stdout/stderr cap",
		},
		examples = {
			{ args = { argv = { "uname", "-a" }, timeout_sec = 10 } },
			{ args = { cmd = "make -j4", cwd = "/tmp", timeout_sec = 120 } },
		},
		docs = "tools/unsafe.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("unsafe.exec", _exec)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
