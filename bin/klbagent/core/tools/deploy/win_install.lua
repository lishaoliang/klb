--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   win_install.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  deploy.win_install: Win 自举静默安装 klua-*-setup.exe
-- @note   host_kind=windows; NSIS /S = 完全安装; GUI 人工安装不变
--   \n 默认异步延迟执行(避免 serve 自升级时被安装器杀进程截断响应)
--   \n sync=true 时等待结束并校验安装根 VERSION; 禁 linux tgz / deploy_pull
--]]


local version = require("klbagent.version")
local hostinfo = require("klbagent.core.hostinfo")
local util = require("klbagent.core.tools.deploy.util")
local unsafe_util = require("klbagent.core.tools.unsafe.util")
local serve = require("klbagent.core.serve")



local M = {}


local ALUA_SERVE_TASK = "klua\\alua-serve"
local VERSION_REL = "klbagent\\VERSION"


local function _is_windows_host()
	local hi = hostinfo.probe()
	return type(hi) == "table" and hi.host_kind == "windows"
end


local function _is_abs_win_path(path)
	if type(path) ~= "string" or path == "" then
		return false
	end

	if string.match(path, "^[A-Za-z]:[\\/]") ~= nil then
		return true
	end

	if string.match(path, "^\\\\") ~= nil then
		return true
	end

	return false
end


local function _install_root()
	local la = os.getenv("LOCALAPPDATA")
	if type(la) ~= "string" or la == "" then
		error("LOCALAPPDATA missing")
	end

	return la .. "\\Programs\\klua"
end


local function _read_installed_version(root)
	local file_ver = util.read_text(root .. "\\" .. VERSION_REL)
	if file_ver == "" then
		file_ver = util.read_text(root .. "\\VERSION")
	end

	local alua = root .. "\\alua.exe"
	local ver_out = ""
	local ok_v = false
	local f = io.open(alua, "rb")
	if f ~= nil then
		f:close()
		ok_v, ver_out = util.run(unsafe_util.win_quote(alua) .. " -v")
		ver_out = util.trim(ver_out or "")
	end

	return file_ver, ver_out, ok_v
end


local function _silent_args(args)
	local s = args.silent_args
	if type(s) == "string" and s ~= "" then
		return s
	end

	-- NSIS (pack.sh 主路径): /S = bootstrap full
	-- Inno 旁路: /VERYSILENT /SUPPRESSMSGBOXES /TYPE=full /TASKS=aluaServe
	return "/S"
end


local function _download_setup(url, dest)
	if not string.match(url, "^https?://") then
		error("url must be http:// or https://")
	end

	local q_dest = unsafe_util.win_quote(dest)
	local q_url = unsafe_util.win_quote(url)
	local dl = "curl.exe -fsSL --connect-timeout 10 --max-time 600 -o "
		.. q_dest .. " " .. q_url
	local ok, out = util.run(dl)
	if ok then
		return
	end

	-- Win10+ 常有 curl; 缺则 PowerShell
	local ps_body = string.format(
		"[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12;"
			.. "Invoke-WebRequest -UseBasicParsing -Uri %s -OutFile %s",
		q_url, q_dest)
	local ps = "powershell.exe -WindowStyle Hidden -NoProfile -Command " .. unsafe_util.win_quote(ps_body)
	ok, out = util.run(ps)
	if not ok then
		error("download fail: " .. tostring(out))
	end
end


local function _schedule_silent(setup_path, silent, delay_sec)
	local n = math.floor(delay_sec)
	if n < 1 then
		n = 1
	end

	-- ping -n N ≈ N-1 秒; 延迟后再跑 setup, 让 HTTP 响应先返回
	local ping_n = n + 1
	local line = string.format(
		'start "" /B cmd /c "ping -n %d 127.0.0.1 >NUL & %s %s"',
		ping_n,
		unsafe_util.win_quote(setup_path),
		silent)
	local ok_ex = os.execute(line)
	if ok_ex == true or ok_ex == 0 then
		return true
	end

	return false
end


local function _run_silent_sync(setup_path, silent)
	local line = string.format(
		"%s %s",
		unsafe_util.win_quote(setup_path),
		silent)
	local ok_ex, what, code = os.execute(line)
	if ok_ex == true or ok_ex == 0 then
		return true, 0
	end

	if what == "exit" and type(code) == "number" then
		return false, code
	end

	return false, 1
end


local function _win_install(args)
	if not _is_windows_host() then
		error("deploy.win_install requires host_kind=windows (use setup.exe on Win; ≠ linux usrlocal.tgz)")
	end

	if not unsafe_util.is_windows() then
		error("deploy.win_install must run on Win32 Lua host")
	end

	local path = args.path
	local url = args.url
	local cleanup = nil

	if type(path) == "string" and path ~= "" then
		if string.find(path, "[\r\n\0]") ~= nil then
			error("bad path")
		end

		if not _is_abs_win_path(path) then
			error("path must be absolute Win path (C:\\... or \\\\share\\...)")
		end

		if string.match(string.lower(path), "%.tgz$")
			or string.match(string.lower(path), "%.tar%.gz$")
		then
			error("linux tgz not allowed on windows; need klua-*-setup.exe")
		end
	elseif type(url) == "string" and url ~= "" then
		local temp = os.getenv("TEMP") or os.getenv("TMP")
		if type(temp) ~= "string" or temp == "" then
			error("TEMP missing")
		end

		path = temp .. "\\klua-setup-" .. tostring(math.floor(os.time())) .. ".exe"
		_download_setup(url, path)
		cleanup = path
	else
		error("need path or url to setup.exe")
	end

	local st = io.open(path, "rb")
	if st == nil then
		error("setup.exe not found: " .. path)
	end

	st:close()

	local silent = _silent_args(args)
	local root = _install_root()
	local expect_version, min_version = util.pin_from(args)
	local restart = util.restart_flag(args)
	local sync = args.sync == true
	local delay = args.delay_sec
	if type(delay) ~= "number" or delay < 0.2 then
		delay = 2
	end

	if not sync then
		local ok_sched = _schedule_silent(path, silent, delay)
		if not ok_sched then
			if cleanup ~= nil then
				os.remove(cleanup)
			end

			error("schedule silent setup fail")
		end

		return {
			ok = true,
			scheduled = true,
			installed = false,
			sync = false,
			path = path,
			url = url,
			silent_args = silent,
			install_root = root,
			delay_sec = delay,
			restart_requested = restart,
			restart_mode = "setup_task",
			expect_version = expect_version,
			min_version = min_version,
			agent_version = version.string,
			note = "silent /S full install scheduled; installer stops old serve and starts task if selected; verify pin after serve recovers",
		}
	end

	local ok_run, rc = _run_silent_sync(path, silent)
	if cleanup ~= nil then
		pcall(os.remove, cleanup)
	end

	if not ok_run then
		error("silent setup fail rc=" .. tostring(rc))
	end

	local file_ver, ver_out = _read_installed_version(root)
	local ok_pin, err_pin = util.check_version_pin(file_ver, ver_out, expect_version, min_version)
	if not ok_pin then
		error("win install " .. err_pin)
	end

	local restart_done = false
	local restart_mode = "none"
	if restart then
		local ok_r, mode = serve.schedule_restart({ delay_sec = 0.5 })
		restart_done = ok_r == true
		restart_mode = mode or "none"
	end

	return {
		ok = true,
		scheduled = false,
		installed = true,
		sync = true,
		path = path,
		url = url,
		silent_args = silent,
		install_root = root,
		file_version = file_ver,
		alua_v = ver_out,
		agent_version = version.string,
		restart = restart_done,
		restart_requested = restart,
		restart_mode = restart_mode,
		task = ALUA_SERVE_TASK,
	}
end


-- @brief 注册 deploy.win_install
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "deploy.win_install",
		category = "unsafe",
		title = "silent full install Win setup.exe",
		summary = "host_kind=windows only. Run klua-*-setup.exe silent full (/S; all sections). Default async schedule so serve can return first. sync=true waits and pins VERSION. Not linux tgz.",
		min_profile = "unsafe",
		args_schema = {
			path = "string optional absolute path to setup.exe",
			url = "string optional http(s) URL to setup.exe",
			sync = "boolean optional default false (async schedule); true=wait+verify",
			delay_sec = "number optional async delay before setup (default 2)",
			silent_args = "string optional default /S (NSIS); Inno: /VERYSILENT /SUPPRESSMSGBOXES /TYPE=full /TASKS=aluaServe",
			restart = "boolean optional default true (sync path: schedule_restart after verify)",
			expect_version = "string optional exact pin (klbagent VERSION or alua -v)",
			min_version = "string optional numeric floor on VERSION",
		},
		examples = {
			{
				args = {
					path = "C:\\Users\\me\\Downloads\\klua-0.1.3.202610-setup.exe",
					expect_version = "0.1.111.202610",
				},
			},
			{
				args = {
					url = "http://192.0.2.10:18080/klua-setup.exe",
					sync = true,
					restart = true,
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("deploy.win_install", _win_install)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
