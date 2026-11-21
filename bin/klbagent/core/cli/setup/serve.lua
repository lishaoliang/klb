--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   serve.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve yes|no / alua setup serve yes|no — 主服务常驻启停 + 配置
-- @note   linux: systemd system|user (无单元则生成 user 单元); win: 计划任务 klua\alua-serve
--   \n embedded / 无 systemd: SysV init.d 脚本 (默认 /etc/init.d/S99_alua)
--   \n 连 init.d 也没有: 明确不支持常驻, 回落前台 alua --serve [port] [profile]
-- @history 修改历史
--  \n [2026-10] 创建文件 (setup 子命令分文件; M1 桩); 分平台常驻启停 + --port/--profile; SysV init.d 分支 (嵌入式 SXX_alua); 缺 yes|no 时 --json 模式不混帮助文本; init.d 归属标记 + 外来脚本拒改; 启动前日志 >1MB 轮转; setup schema; init.d: 导出 HOME (busybox init HOME=/ 致 pref 分裂); 启动存活校验/失败清 pid
--]]
local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")
local common = require("klbagent.core.cli.setup.common")
local config = require("klbagent.core.config")


local M = {}

-- 子命令名 (顶层 alua serve yes|no 与 setup serve yes|no 共用)
M.NAME = "serve"


local PROFILES = {
	readonly = true,
	net = true,
	device = true,
	unsafe = true,
}


M.help = function ()
	print("Usage: alua setup serve yes|no [--port N] [--profile P] [--json]")
	print("       alua serve yes|no [--port N] [--profile P] [--json]")
	fmt.print_kv("yes", "enable alua serve (persistent)")
	fmt.print_kv("no", "disable alua serve")
	fmt.print_kv("--port", "listen port (default 12817)")
	fmt.print_kv("--profile", "readonly|net|device|unsafe")
	fmt.print_kv("platform", "linux: systemd system|user; embedded: init.d; win: Task Scheduler")
	fmt.print_kv("foreground", "alua --serve [port] [profile]  (or -s)")
end


local function _profile_ok(p)
	return type(p) == "string" and PROFILES[string.lower(p)] == true
end


-- @brief 解析 yes|no 与选项
-- @param [in] argv[table]
-- @return opt[table]
local function _parse(argv)
	local opt = {
		action = nil,
		port = nil,
		port_raw = nil,
		profile = nil,
		profile_raw = nil,
	}

	local i = 1
	while i <= #argv do
		local a = argv[i]
		if a == "yes" or a == "no" then
			opt.action = a
			i = i + 1
		elseif a == "--port" or a == "-p" then
			opt.port_raw = argv[i + 1]
			opt.port = tonumber(argv[i + 1])
			i = i + 2
		elseif a == "--profile" then
			opt.profile_raw = argv[i + 1]
			opt.profile = argv[i + 1]
			i = i + 2
		else
			i = i + 1
		end
	end

	return opt
end


-- @brief 新建单元时的 port / profile (缺省用 pref runtime)
-- @return port[number]
-- @return profile[string]
local function _new_unit_opts(opt)
	local port = opt.port
	if type(port) ~= "number" then
		error("alua.core.serve.UNAVAILABLE: need --port (DEFAULT_PORT closed)")
	end

	local profile = opt.profile
	if not _profile_ok(profile) then
		local rt = config.runtime.load()
		if type(rt) == "table" and _profile_ok(rt.profile) then
			profile = rt.profile
		else
			profile = "readonly"
		end
	end

	profile = string.lower(profile)
	return port, profile
end


-- @brief --profile 同时落 pref runtime.json (前台 alua serve 也生效)
local function _save_profile(profile)
	if not _profile_ok(profile) then
		return true
	end

	local rt = config.runtime.load()
	if type(rt) ~= "table" then
		rt = config.runtime.defaults()
	end

	if rt.profile == profile then
		return true
	end

	rt.profile = profile
	return config.runtime.save(rt)
end


-- @brief 生成 serve 的 user 单元 (免 root; token 由 pref serve.env 提供)
local function _unit_text(bin, port, profile)
	local pref = common.pref_dir()
	local home = os.getenv("HOME") or ""

	local lines = {
		"[Unit]",
		"Description=alua serve (klbagent HTTP peer)",
		"After=network-online.target",
		"Wants=network-online.target",
		"",
		"[Service]",
		"Type=simple",
		"ExecStart=" .. common.quote_exec_bin(bin) .. " --serve " .. tostring(port) .. " " .. profile,
		"EnvironmentFile=-" .. pref .. "/serve.env",
		"Environment=HOME=" .. home,
		"Restart=on-failure",
		"RestartSec=3",
		"",
		"[Install]",
		"WantedBy=default.target",
		"",
	}

	return table.concat(lines, "\n")
end


-- @brief linux 分支: systemd system|user
-- @param [in] opt[table]
-- @param [in,out] result[table]
-- @return ok[boolean]
-- @return err[string|nil]
local function _linux(opt, result)
	local unit = common.SERVE_UNIT
	local scope = common.unit_scope(unit)
	result.scope = scope

	if scope == nil then
		if opt.action == "no" then
			result.changed = false
			result.message = "alua serve unit not installed; nothing to disable"
			return true, nil
		end

		-- 无单元: 生成 user 单元 (system scope 需用户自行安装)
		scope = "user"
		result.scope = scope
		local port, profile = _new_unit_opts(opt)
		local path = common.unit_path(unit, scope)
		local ok_w, err_w = common.write_unit(path, _unit_text(common.alua_bin(), port, profile), scope)
		if not ok_w then
			return false, err_w
		end

		result.changed = true
		result.port = port
		result.profile = profile
	else
		local path = common.unit_path(unit, scope)
		local content, err_c = common.read_unit(path, scope)
		if content == nil then
			return false, err_c
		end

		local exec = common.exec_start(content)
		local cur_port, cur_profile = common.exec_serve_opts(exec)
		local port = opt.port
		if type(port) ~= "number" then
			port = cur_port
		end

		local profile = opt.profile
		if not _profile_ok(profile) then
			profile = cur_profile
		end

		if type(port) ~= "number" then
			error("alua.core.serve.UNAVAILABLE: need --port")
		end

		if not _profile_ok(profile) then
			local rt = config.runtime.load()
			if type(rt) == "table" and _profile_ok(rt.profile) then
				profile = rt.profile
			else
				profile = "readonly"
			end
		end

		profile = string.lower(profile)
		result.port = port
		result.profile = profile

		-- 仅显式 --port/--profile 才改写单元启动参数
		if opt.port ~= nil or opt.profile ~= nil then
			local bin = common.exec_bin(exec) or common.alua_bin()
			local new_exec = common.quote_exec_bin(bin) .. " --serve " .. tostring(port) .. " " .. profile
			local cur_exec = string.match(exec or "", "^%s*(.-)%s*$") or ""
			if new_exec ~= cur_exec then
				local ok_w, err_w = common.write_unit(path, common.set_exec_start(content, new_exec), scope)
				if not ok_w then
					return false, err_w
				end

				result.changed = true
			end
		end
	end

	if opt.action == "no" then
		local ok_d, err_d = common.disable_unit(unit, scope)
		if not ok_d then
			return false, err_d
		end

		result.changed = true
		result.message = "alua serve disabled"
		return true, nil
	end

	local state = common.unit_state(unit, scope)
	local ok_e, err_e = common.enable_unit(unit, scope)
	if not ok_e then
		return false, err_e
	end

	if result.changed then
		local ok_r, err_r = common.daemon_reload(scope)
		if not ok_r then
			return false, err_r
		end
	end

	-- 改动启动参数或当前未运行时重启 (restart 对 inactive 即 start)
	if result.changed or state ~= "active" then
		local ok_s, err_s = common.restart_unit(unit, scope)
		if not ok_s then
			return false, err_s
		end
	end

	result.message = "alua serve enabled"
	return true, nil
end


-- @brief 从 pref 反推 HOME (pref 形如 <home>/.local/share/klua/alua)
-- @param [in] pref[string]
-- @return home[string]			推不出时为空串
local function _pref_home(pref)
	local home = string.match(pref, "^(.-)/%.local/share/klua/alua$")
	if type(home) == "string" and home ~= "" then
		return home
	end

	return ""
end


-- @brief 生成 serve 的 SysV init.d 脚本 (嵌入式; PID/日志/ENV 均落 pref)
-- @param [in] bin[string]
-- @param [in] port[number]
-- @param [in] profile[string]
-- @return text[string]
-- @note busybox init 只给 HOME=/; 脚本须导出安装时 HOME, 否则 daemon 的 pref 与 PID/日志分裂
local function _initd_text(bin, port, profile)
	local pref = common.pref_dir()
	local home = _pref_home(pref)
	local args = "--serve " .. tostring(port) .. " " .. profile

	local lines = {
		"#!/bin/sh",
		common.INITD_MARK,
		"### BEGIN INIT INFO",
		"# Provides:          alua",
		"# Required-Start:    $network $remote_fs",
		"# Required-Stop:     $network $remote_fs",
		"# Default-Start:     2 3 4 5",
		"# Default-Stop:      0 1 6",
		"# Short-Description: alua serve (klbagent HTTP peer)",
		"### END INIT INFO",
		"",
		"DAEMON=" .. common.sh_quote(bin),
		'ALUA_ARGS="' .. args .. '"',
		'PIDFILE="' .. pref .. '/alua-serve.pid"',
		'LOGFILE="' .. pref .. '/alua-serve.log"',
		'ENVFILE="' .. pref .. '/serve.env"',
		'PIDDIR="' .. pref .. '"',
		"",
		-- HOME 先导出再 source serve.env (env 仍可覆盖); 与 systemd Environment=HOME= 对齐
		(home ~= "" and ("HOME=" .. common.sh_quote(home)) or ""),
		(home ~= "" and "export HOME" or ""),
		"",
		'[ -f "$ENVFILE" ] && . "$ENVFILE"',
		"",
		"is_running() {",
		'\t[ -f "$PIDFILE" ] || return 1',
		'\tPID=$(cat "$PIDFILE" 2>/dev/null)',
		'\t[ -n "$PID" ] || return 1',
		'\tkill -0 "$PID" 2>/dev/null || return 1',
		'\t[ "$(cat /proc/$PID/comm 2>/dev/null)" = "alua" ] || return 1',
		"}",
		"",
		"do_start() {",
		"\tif is_running; then",
		'\t\techo "alua serve already running (pid $PID)"',
		"\t\treturn 0",
		"\tfi",
		'\trm -f "$PIDFILE"',
		'\t[ -d "$PIDDIR" ] || mkdir -p "$PIDDIR"',
		'\tif [ -f "$LOGFILE" ] && [ "$(wc -c < "$LOGFILE")" -gt 1048576 ]; then',
		'\t\tmv -f "$LOGFILE" "$LOGFILE.1" 2>/dev/null',
		"\tfi",
		'\t"$DAEMON" $ALUA_ARGS >> "$LOGFILE" 2>&1 &',
		"\tNEWPID=$!",
		'\techo "$NEWPID" > "$PIDFILE"',
		"\tsleep 1",
		'\tif ! kill -0 "$NEWPID" 2>/dev/null; then',
		'\t\trm -f "$PIDFILE"',
		'\t\techo "alua serve failed to start; see $LOGFILE"',
		"\t\treturn 1",
		"\tfi",
		'\techo "alua serve started (pid $NEWPID)"',
		"}",
		"",
		"do_stop() {",
		"\tif ! is_running; then",
		'\t\trm -f "$PIDFILE"',
		'\t\techo "alua serve not running"',
		"\t\treturn 0",
		"\tfi",
		'\tkill "$PID" 2>/dev/null',
		"\ti=0",
		'\twhile kill -0 "$PID" 2>/dev/null && [ "$i" -lt 30 ]; do',
		"\t\tsleep 1",
		"\t\ti=$((i + 1))",
		"\tdone",
		'\tkill -9 "$PID" 2>/dev/null',
		'\trm -f "$PIDFILE"',
		'\techo "alua serve stopped"',
		"}",
		"",
		"do_status() {",
		"\tif is_running; then",
		'\t\techo "alua serve running (pid $PID)"',
		"\t\treturn 0",
		"\tfi",
		'\techo "alua serve stopped"',
		"\treturn 3",
		"}",
		"",
		'case "$1" in',
		"\tstart) do_start ;;",
		"\tstop) do_stop ;;",
		"\trestart) do_stop; do_start ;;",
		"\tstatus) do_status ;;",
		'\t*) echo "Usage: $0 {start|stop|restart|status}"; exit 1 ;;',
		"esac",
		"",
	}

	return table.concat(lines, "\n")
end


-- @brief 从 init.d 的 ALUA_ARGS 取 serve 的 port / profile
-- @param [in] args[string|nil]
-- @return port[number|nil]
-- @return profile[string|nil]
local function _initd_opts(args)
	local port = nil
	local profile = nil

	for tok in string.gmatch(args or "", "%S+") do
		if string.match(tok, "^%d+$") ~= nil then
			port = tonumber(tok)
		elseif _profile_ok(tok) then
			profile = string.lower(tok)
		end
	end

	return port, profile
end


-- @brief 嵌入式 / 无 systemd 分支: SysV init.d 脚本启停
-- @param [in] opt[table]
-- @param [in,out] result[table]
-- @return ok[boolean]
-- @return err[string|nil]
local function _initd(opt, result)
	local path = common.initd_path()
	result.scope = "initd"
	result.service = common.INITD_NAME
	result.init = path

	local content = common.read_initd(path)
	local installed = content ~= nil

	-- 外来脚本拒改 (M7): 只认 alua setup 生成的标记
	if installed and not common.initd_owned(content) then
		result.code = codes.cli.BAD_REQUEST
		return false, "init.d script exists but not alua-managed: " .. path
			.. " (refuse to modify; remove it manually first)"
	end

	if opt.action == "no" then
		if not installed then
			result.changed = false
			result.message = "alua serve init.d script not installed; nothing to disable"
			return true, nil
		end

		local ok_s, out_s = common.run_initd(path, "stop")
		if not ok_s then
			return false, "init.d stop fail: " .. tostring(out_s or "")
		end

		local ok_x, err_x = common.chmod_exec(path, false)
		if not ok_x then
			return false, err_x
		end

		result.changed = true
		result.message = "alua serve init.d disabled (chmod -x; rcS skips)"
		return true, nil
	end

	if installed then
		local cur_port, cur_profile = _initd_opts(common.initd_args(content))
		local port = opt.port
		if type(port) ~= "number" then
			port = cur_port
		end

		local profile = opt.profile
		if not _profile_ok(profile) then
			profile = cur_profile
		end

		if type(port) ~= "number" then
			error("alua.core.serve.UNAVAILABLE: need --port")
		end

		if not _profile_ok(profile) then
			local rt = config.runtime.load()
			if type(rt) == "table" and _profile_ok(rt.profile) then
				profile = rt.profile
			else
				profile = "readonly"
			end
		end

		profile = string.lower(profile)
		result.port = port
		result.profile = profile

		-- 仅显式 --port/--profile 才改写 ALUA_ARGS
		if opt.port ~= nil or opt.profile ~= nil then
			local new_args = "--serve " .. tostring(port) .. " " .. profile
			if common.initd_args(content) ~= new_args then
				local ok_w, err_w = common.write_initd(path, common.set_initd_args(content, new_args))
				if not ok_w then
					return false, err_w
				end

				result.changed = true
			end
		end
	else
		local port, profile = _new_unit_opts(opt)
		result.port = port
		result.profile = profile

		local ok_w, err_w = common.write_initd(path, _initd_text(common.alua_bin(), port, profile))
		if not ok_w then
			return false, err_w
		end

		result.changed = true
	end

	-- rcS 只跑可执行的 SXX*; 启用即恢复 +x
	local ok_x, err_x = common.chmod_exec(path, true)
	if not ok_x then
		return false, err_x
	end

	if result.changed or not common.initd_running(path) then
		local ok_r, out_r = common.run_initd(path, "restart")
		if not ok_r then
			return false, "init.d start fail: " .. tostring(out_r or "")
		end
	end

	result.message = "alua serve init.d enabled"
	return true, nil
end


-- @brief win 分支: 计划任务 klua\alua-serve
-- @param [in] opt[table]
-- @param [in,out] result[table]
-- @return ok[boolean]
-- @return err[string|nil]
local function _windows(opt, result)
	local task = common.SERVE_TASK
	result.task = task

	if opt.action == "no" then
		common.run('schtasks /End /TN "' .. task .. '"')
		local ok_d = common.run('schtasks /Change /TN "' .. task .. '" /DISABLE')
		if not ok_d then
			result.changed = false
			result.message = "alua serve task not installed; nothing to disable"
			return true, nil
		end

		result.changed = true
		result.message = "alua serve task disabled"
		return true, nil
	end

	local port, profile = _new_unit_opts(opt)
	result.port = port
	result.profile = profile

	local tr = '\\"' .. common.alua_bin() .. '\\" --serve ' .. tostring(port) .. " " .. profile
	local tr_q = '"' .. tr .. '"'
	local ok_q = common.run('schtasks /Query /TN "' .. task .. '"')
	if not ok_q then
		local ok_c, out_c = common.run('schtasks /Create /TN "' .. task .. '" /TR ' .. tr_q
			.. ' /SC ONLOGON /RL LIMITED /F')
		if not ok_c then
			return false, "schtasks create fail: " .. tostring(out_c or "")
		end

		result.changed = true
	elseif opt.port ~= nil or opt.profile ~= nil then
		local ok_t = common.run('schtasks /Change /TN "' .. task .. '" /TR ' .. tr_q)
		if not ok_t then
			return false, "schtasks change fail"
		end

		result.changed = true
	end

	local ok_e, out_e = common.run('schtasks /Change /TN "' .. task .. '" /ENABLE')
	if not ok_e then
		return false, "schtasks enable fail: " .. tostring(out_e or "")
	end

	local ok_r, out_r = common.run('schtasks /Run /TN "' .. task .. '"')
	if not ok_r then
		return false, "schtasks run fail: " .. tostring(out_r or "")
	end

	result.message = "alua serve task enabled"
	return true, nil
end


local function _print_text(result)
	fmt.print_kv("entry", result.entry)
	fmt.print_kv("action", result.action)
	fmt.print_kv("platform", result.platform)
	fmt.print_kv("service", result.service)
	if result.task ~= nil then
		fmt.print_kv("task", result.task)
	end

	if result.scope ~= nil then
		fmt.print_kv("scope", result.scope)
	end

	if result.init ~= nil then
		fmt.print_kv("init", result.init)
	end

	if type(result.port) == "number" then
		fmt.print_kv("port", result.port)
	end

	if result.profile ~= nil then
		fmt.print_kv("profile", result.profile)
	end

	if result.changed ~= nil then
		fmt.print_kv("changed", tostring(result.changed))
	end

	fmt.print_kv("message", result.message or "ok")
end


-- @brief serve 子命令: 常驻启停 + 配置
-- @param [in] argv[table]			{ "yes"|"no", "--port", N, "--profile", P }
-- @param [in] want_json[boolean]
-- @return ok[boolean]
-- @return code[number]
M.run = function (argv, want_json)
	argv = argv or {}
	if fmt.has_help_flag(argv) then
		M.help()
		return true, codes.OK
	end

	local opt = _parse(argv)
	if opt.action == nil then
		if want_json then
			fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "alua serve yes|no"))
		else
			M.help()
			io.stderr:write("alua serve: need yes|no\n")
		end

		return false, codes.cli.BAD_REQUEST
	end

	if opt.port_raw ~= nil and opt.port == nil then
		local msg = "alua serve: bad --port (1-65535)"
		io.stderr:write(msg .. "\n")
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, msg))
		return false, codes.cli.BAD_REQUEST
	end

	if type(opt.port) == "number" and (opt.port < 1 or 65535 < opt.port) then
		local msg = "alua serve: bad --port (1-65535)"
		io.stderr:write(msg .. "\n")
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, msg))
		return false, codes.cli.BAD_REQUEST
	end

	if opt.profile_raw ~= nil and not _profile_ok(opt.profile) then
		local msg = "alua serve: bad --profile (readonly|net|device|unsafe)"
		io.stderr:write(msg .. "\n")
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, msg))
		return false, codes.cli.BAD_REQUEST
	end

	local result = {
		command = "setup",
		schema = common.SCHEMA,
		entry = "serve",
		action = opt.action,
		platform = nil,
		service = common.SERVE_UNIT,
	}

	local ok = false
	local err = nil
	if common.is_windows() then
		result.platform = "windows"
		ok, err = _windows(opt, result)
	elseif common.has_systemctl() then
		result.platform = "linux"
		ok, err = _linux(opt, result)
	elseif common.has_initd() then
		result.platform = "linux-initd"
		ok, err = _initd(opt, result)
	else
		result.platform = "unsupported"
		err = "no systemd / init.d / Task Scheduler; persistent enable unsupported;"
			.. " foreground: alua --serve [port] [profile]"
	end

	if ok then
		-- 仅成功时同步 pref runtime.json (失败不应暗中改档)
		if opt.profile ~= nil and _profile_ok(opt.profile) then
			_save_profile(string.lower(opt.profile))
		end

		result.ok = true
		if want_json then
			fmt.print_json(result)
		else
			_print_text(result)
		end

		return true, codes.OK
	end

	result.ok = false
	result.message = tostring(err or "fail")
	if want_json then
		fmt.print_json(result)
	else
		io.stderr:write("alua setup serve: " .. result.message .. "\n")
	end

	if result.platform == "unsupported" then
		return false, codes.cli.NOT_IMPLEMENTED
	end

	if type(result.code) == "number" then
		return false, result.code
	end

	return false, codes.cli.TOOL_FAILED
end


return M

-- end
