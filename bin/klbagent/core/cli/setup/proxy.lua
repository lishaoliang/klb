--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   proxy.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua setup proxy yes|no — 本地代理入口常驻启停 + 配置
-- @note   linux: systemd system|user (无单元则生成 user 单元); win: 计划任务 klua\alua-proxy
--   \n ExecStart/Task 命令: alua pkg run proxy serve @<pref>/proxy.json; --port 写 proxy.json
-- @history 修改历史
--  \n [2026-10] 创建文件 (setup 子命令分文件; M2 桩); 分平台常驻启停 + --port; 缺 yes|no 时 --json 模式不混帮助文本
--]]
local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")
local common = require("klbagent.core.cli.setup.common")


local M = {}

-- 子命令名
M.NAME = "proxy"


M.help = function ()
	print("Usage: alua setup proxy yes|no [--port N] [--json]")
	fmt.print_kv("yes", "enable local proxy entry (persistent)")
	fmt.print_kv("no", "disable local proxy entry")
	fmt.print_kv("--port", "listen port (default 12818; writes proxy.json)")
	fmt.print_kv("platform", "linux: systemd system|user; win: Task Scheduler; no systemd: unsupported")
	fmt.print_kv("foreground", "alua pkg run proxy serve @proxy.json")
end


-- @brief 解析 yes|no 与选项
-- @param [in] argv[table]
-- @return opt[table]
local function _parse(argv)
	local opt = {
		action = nil,
		port = nil,
		port_raw = nil,
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
		else
			i = i + 1
		end
	end

	return opt
end


-- @brief proxy.json 配置文件路径 (单元/@ 参数)
-- @return path[string]
local function _cfg_path()
	local pref = common.pref_dir()
	if pref == "" then
		return ""
	end

	return pref .. "/proxy.json"
end


-- @brief 确保 proxy.json 存在 (缺则落默认)
-- @return ok[boolean]
local function _ensure_cfg()
	local ok_m, cproxy = pcall(require, "klbagent.core.config.proxy")
	if not ok_m or type(cproxy) ~= "table" then
		return false
	end

	cproxy.load()
	return true
end


-- @brief 写 listen.port 到 proxy.json
-- @return ok[boolean]
-- @return err[string|nil]
local function _apply_port(port)
	local ok_m, cproxy = pcall(require, "klbagent.core.config.proxy")
	if not ok_m or type(cproxy) ~= "table" then
		return false, "config.proxy unavailable"
	end

	local ok_p, px = pcall(require, "klbagent.packages.proxy.config")
	if not ok_p or type(px) ~= "table" or type(px.apply_listen) ~= "function" then
		return false, "packages.proxy.config unavailable"
	end

	local cfg = cproxy.load()
	local ok_l, err_l = px.apply_listen(cfg, nil, port)
	if not ok_l then
		return false, tostring(err_l or "bad listen")
	end

	if not cproxy.save(cfg) then
		return false, "save proxy.json fail"
	end

	return true, nil
end


-- @brief 生成 proxy 的 user 单元
local function _unit_text(bin, cfg_path)
	local lines = {
		"[Unit]",
		"Description=alua proxy entry (SOCKS5/HTTP CONNECT)",
		"After=network-online.target",
		"Wants=network-online.target",
		"",
		"[Service]",
		"Type=simple",
		"ExecStart=" .. common.quote_exec_bin(bin) .. " pkg run proxy serve @" .. cfg_path,
		"Restart=on-failure",
		"RestartSec=3",
		"",
		"[Install]",
		"WantedBy=default.target",
		"",
	}

	return table.concat(lines, "\n")
end


-- @brief 单元已存在时确保 ExecStart 指向当前 alua 与 proxy.json
-- @return changed[boolean]
-- @return err[string|nil]
local function _sync_exec(path, scope)
	local content, err_c = common.read_unit(path, scope)
	if content == nil then
		return false, err_c
	end

	local cur_exec = common.exec_start(content)
	local bin = common.exec_bin(cur_exec) or common.alua_bin()
	local new_exec = common.quote_exec_bin(bin) .. " pkg run proxy serve @" .. _cfg_path()
	local cur = string.match(cur_exec or "", "^%s*(.-)%s*$") or ""
	if new_exec == cur then
		return false, nil
	end

	local ok_w, err_w = common.write_unit(path, common.set_exec_start(content, new_exec), scope)
	if not ok_w then
		return false, err_w
	end

	return true, nil
end


-- @brief linux 分支: systemd system|user
-- @param [in] opt[table]
-- @param [in,out] result[table]
-- @return ok[boolean]
-- @return err[string|nil]
local function _linux(opt, result)
	local unit = common.PROXY_UNIT
	local scope = common.unit_scope(unit)
	result.scope = scope

	_ensure_cfg()

	if type(opt.port) == "number" then
		local ok_p, err_p = _apply_port(opt.port)
		if not ok_p then
			return false, err_p
		end

		result.port = opt.port
		result.changed = true
	end

	if scope == nil then
		if opt.action == "no" then
			result.message = "alua proxy unit not installed; nothing to disable"
			return true, nil
		end

		local cfg_path = _cfg_path()
		if cfg_path == "" then
			return false, "pref dir unavailable"
		end

		scope = "user"
		result.scope = scope
		local path = common.unit_path(unit, scope)
		local ok_w, err_w = common.write_unit(path, _unit_text(common.alua_bin(), cfg_path), scope)
		if not ok_w then
			return false, err_w
		end

		local ok_r, err_r = common.daemon_reload(scope)
		if not ok_r then
			return false, err_r
		end

		result.changed = true
	else
		local path = common.unit_path(unit, scope)
		local changed, err_s = _sync_exec(path, scope)
		if err_s ~= nil then
			return false, err_s
		end

		if changed then
			result.changed = true
			local ok_r, err_r = common.daemon_reload(scope)
			if not ok_r then
				return false, err_r
			end
		end
	end

	if opt.action == "no" then
		local ok_d, err_d = common.disable_unit(unit, scope)
		if not ok_d then
			return false, err_d
		end

		result.changed = true
		result.message = "alua proxy disabled"
		return true, nil
	end

	local state = common.unit_state(unit, scope)
	local ok_e, err_e = common.enable_unit(unit, scope)
	if not ok_e then
		return false, err_e
	end

	if result.changed or state ~= "active" then
		local ok_s, err_s = common.restart_unit(unit, scope)
		if not ok_s then
			return false, err_s
		end
	end

	result.message = "alua proxy enabled"
	return true, nil
end


-- @brief win 分支: 计划任务 klua\alua-proxy
-- @param [in] opt[table]
-- @param [in,out] result[table]
-- @return ok[boolean]
-- @return err[string|nil]
local function _windows(opt, result)
	local task = common.PROXY_TASK
	result.task = task

	if type(opt.port) == "number" then
		local ok_p, err_p = _apply_port(opt.port)
		if not ok_p then
			return false, err_p
		end

		result.port = opt.port
		result.changed = true
	end

	if opt.action == "no" then
		common.run('schtasks /End /TN "' .. task .. '"')
		local ok_d = common.run('schtasks /Change /TN "' .. task .. '" /DISABLE')
		if not ok_d then
			result.message = "alua proxy task not installed; nothing to disable"
			return true, nil
		end

		result.changed = true
		result.message = "alua proxy task disabled"
		return true, nil
	end

	local cfg_path = _cfg_path()
	if cfg_path == "" then
		return false, "pref dir unavailable"
	end

	local tr = '\\"' .. common.alua_bin() .. '\\" pkg run proxy serve @' .. cfg_path
	local tr_q = '"' .. tr .. '"'
	local ok_q = common.run('schtasks /Query /TN "' .. task .. '"')
	if not ok_q then
		local ok_c, out_c = common.run('schtasks /Create /TN "' .. task .. '" /TR ' .. tr_q
			.. ' /SC ONLOGON /RL LIMITED /F')
		if not ok_c then
			return false, "schtasks create fail: " .. tostring(out_c or "")
		end

		result.changed = true
	else
		local ok_t = common.run('schtasks /Change /TN "' .. task .. '" /TR ' .. tr_q)
		if not ok_t then
			return false, "schtasks change fail"
		end
	end

	local ok_e, out_e = common.run('schtasks /Change /TN "' .. task .. '" /ENABLE')
	if not ok_e then
		return false, "schtasks enable fail: " .. tostring(out_e or "")
	end

	local ok_r, out_r = common.run('schtasks /Run /TN "' .. task .. '"')
	if not ok_r then
		return false, "schtasks run fail: " .. tostring(out_r or "")
	end

	result.message = "alua proxy task enabled"
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

	if type(result.port) == "number" then
		fmt.print_kv("port", result.port)
	end

	if result.changed ~= nil then
		fmt.print_kv("changed", tostring(result.changed))
	end

	fmt.print_kv("message", result.message or "ok")
end


-- @brief proxy 子命令: 常驻启停 + 端口配置
-- @param [in] argv[table]			{ "yes"|"no", "--port", N }
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
			fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "alua setup proxy yes|no"))
		else
			M.help()
			io.stderr:write("alua setup proxy: need yes|no\n")
		end

		return false, codes.cli.BAD_REQUEST
	end

	if opt.port_raw ~= nil and opt.port == nil then
		local msg = "alua setup proxy: bad --port (1-65535)"
		io.stderr:write(msg .. "\n")
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, msg))
		return false, codes.cli.BAD_REQUEST
	end

	if type(opt.port) == "number" and (opt.port < 1 or 65535 < opt.port) then
		local msg = "alua setup proxy: bad --port (1-65535)"
		io.stderr:write(msg .. "\n")
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, msg))
		return false, codes.cli.BAD_REQUEST
	end

	local result = {
		command = "setup",
		schema = common.SCHEMA,
		entry = "proxy",
		action = opt.action,
		platform = nil,
		service = common.PROXY_UNIT,
	}

	local ok = false
	local err = nil
	if common.is_windows() then
		result.platform = "windows"
		ok, err = _windows(opt, result)
	elseif common.has_systemctl() then
		result.platform = "linux"
		ok, err = _linux(opt, result)
	else
		result.platform = "unsupported"
		err = "no systemd / Task Scheduler; persistent enable unsupported;"
			.. " foreground: alua pkg run proxy serve @proxy.json"
	end

	if ok then
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
		io.stderr:write("alua setup proxy: " .. result.message .. "\n")
	end

	if result.platform == "unsupported" then
		return false, codes.cli.NOT_IMPLEMENTED
	end

	return false, codes.cli.TOOL_FAILED
end


return M

-- end
