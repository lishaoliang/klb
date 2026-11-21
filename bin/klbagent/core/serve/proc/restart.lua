--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   restart.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  异步重启本机 alua serve (deploy / tls_resign 共用)
-- @stability evolving
-- @impl evolving
-- @note   须先返回 HTTP/工具响应再睡延迟; linux: systemctl (无 sudo 时 kill 触发 Restart=on-failure);
--   \n windows: schtasks; 无 unit 时 exec 新 alua serve + kill 旧 pid
-- @history 修改历史
--  \n [2026-11] 自旧 serve/init schedule_restart 迁回 (serve 瘦身后 stub 已清)
--]]
local hostinfo = require("klbagent.core.hostinfo")
local state = require("klbagent.core.serve.base.state")


local M = {}


local function _sh_quote(s)
	return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end


local function _self_pid()
	local f = io.open("/proc/self/stat", "r")
	if f == nil then
		return nil
	end

	local line = f:read("*l") or ""
	f:close()
	return tonumber(string.match(line, "^(%d+)"))
end


local function _is_windows()
	return package.config:sub(1, 1) == "\\"
end


local function _alua_bin()
	if _is_windows() then
		return "alua.exe"
	end

	local f = io.popen("readlink -f /proc/self/exe 2>/dev/null", "r")
	if f ~= nil then
		local path = f:read("*l") or ""
		f:close()
		if type(path) == "string" and path ~= "" then
			return path
		end
	end

	return "/usr/local/bin/alua"
end


-- @brief 异步重启本机 alua serve (须先返回工具/HTTP 响应)
-- @param [in] opts[table|nil]		sudo_pass[string] delay_sec[number]
-- @return ok[boolean]
-- @return mode[string]				systemctl | schtasks | exec | none
M.schedule_restart = function (opts)
	opts = opts or {}
	local delay = opts.delay_sec
	if type(delay) ~= "number" or delay < 0.2 then
		delay = 2
	end

	local sudo_pass = opts.sudo_pass
	if type(sudo_pass) ~= "string" or sudo_pass == "" then
		sudo_pass = os.getenv("ALUA_DEPLOY_SUDO") or ""
	end

	local hi = hostinfo.probe()

	-- Win: Task Scheduler 常驻任务 (勿假报 systemctl)
	if _is_windows() or (type(hi) == "table" and hi.host_kind == "windows") then
		local ping_n = math.floor(delay) + 1
		if ping_n < 2 then
			ping_n = 2
		end

		local line = string.format(
			'start "" /B cmd /c "ping -n %d 127.0.0.1 >NUL'
				.. " & schtasks /End /TN klua\\alua-serve >NUL 2>&1"
				.. ' & schtasks /Run /TN klua\\alua-serve"',
			ping_n)
		local ok_ex = os.execute(line)
		if ok_ex == true or ok_ex == 0 then
			return true, "schtasks"
		end

		return false, "none"
	end

	local has_systemctl = type(hi.caps) == "table" and hi.caps.systemctl == true

	-- systemctl: 先认 active 的 unit (system 或 --user); 无 sudo 时 kill MainPID 触发 Restart=on-failure
	if has_systemctl then
		local line = nil
		if sudo_pass ~= "" then
			line = string.format(
				"( sleep %s; "
					.. "if systemctl is-active --quiet alua-serve.service 2>/dev/null; then "
					.. "printf '%%s\\n' %s | sudo -S systemctl restart alua-serve.service; "
					.. "elif systemctl --user is-active --quiet alua-serve.service 2>/dev/null; then "
					.. "systemctl --user restart alua-serve.service; "
					.. "else "
					.. "systemctl --user restart alua-serve.service 2>/dev/null "
					.. "|| printf '%%s\\n' %s | sudo -S systemctl restart alua-serve.service; "
					.. "fi ) >/tmp/alua_serve_restart.log 2>&1 &",
				tostring(delay), _sh_quote(sudo_pass), _sh_quote(sudo_pass))
		else
			line = string.format(
				"( sleep %s; "
					.. "if systemctl --user is-active --quiet alua-serve.service 2>/dev/null; then "
					.. "systemctl --user restart alua-serve.service; "
					.. "elif systemctl is-active --quiet alua-serve.service 2>/dev/null; then "
					.. "sudo -n systemctl restart alua-serve.service 2>/dev/null "
					.. "|| kill -9 $(systemctl show -p MainPID --value alua-serve.service 2>/dev/null); "
					.. "else "
					.. "systemctl --user restart alua-serve.service 2>/dev/null "
					.. "|| sudo -n systemctl restart alua-serve.service 2>/dev/null "
					.. "|| kill -9 $(systemctl show -p MainPID --value alua-serve.service 2>/dev/null); "
					.. "fi ) >/tmp/alua_serve_restart.log 2>&1 &",
				tostring(delay))
		end

		local ok_ex = os.execute(line)
		if ok_ex == true or ok_ex == 0 then
			return true, "systemctl"
		end
	end

	local serve_opts = state.serve_opts
	if type(serve_opts) ~= "table" then
		return false, "none"
	end

	local pid = _self_pid()
	if type(pid) ~= "number" or pid < 2 then
		return false, "none"
	end

	local bin = _alua_bin()
	local port = serve_opts.port
	local profile = serve_opts.profile
	if type(port) ~= "number" then
		port = 12817
	end

	if type(profile) ~= "string" or profile == "" then
		profile = "net"
	end

	local line = string.format(
		"( sleep %s; kill %d 2>/dev/null; sleep 0.4; exec %s --serve %d %s )"
			.. " >/tmp/alua_serve_restart.log 2>&1 &",
		tostring(delay), pid, _sh_quote(bin), port, _sh_quote(profile))
	local ok_ex = os.execute(line)
	if ok_ex == true or ok_ex == 0 then
		return true, "exec"
	end

	return false, "none"
end


return M

-- end
