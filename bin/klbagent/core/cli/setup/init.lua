--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua setup — 人维护入口分派 (sudo / serve / proxy / web)
-- @note   无参列维护项清单 + 平台有效性; 子命令各一文件 (setup/<name>.lua); --json 机读
--   \n serve 常驻后端: systemd / SysV init.d / win 计划任务; proxy 仅 systemd / win
-- @history 修改历史
--  \n [2026-10] 创建文件 (自 cli/setup.lua 拆出; 子命令分文件); 平台标注支持 init.d (嵌入式); <entry> -h 路由; 未知入口 JSON 只给错误; 缺动作不混帮助; 帮助补 --profile/后端; JSON schema + host caps; 新增 web 入口 (桩; 端口组 HTTP+HTTPS)
--]]
local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")
local hostinfo = require("klbagent.core.hostinfo")
local common = require("klbagent.core.cli.setup.common")
local sudo = require("klbagent.core.cli.setup.sudo")
local serve = require("klbagent.core.cli.setup.serve")
local proxy = require("klbagent.core.cli.setup.proxy")
local web = require("klbagent.core.cli.setup.web")


-- 子命令模块 (setup <entry> -h 路由到对应帮助)
local _ENTRY_MOD = {
	sudo = sudo,
	serve = serve,
	proxy = proxy,
	web = web,
}


local M = {}


-- @brief 本机平台能力 (sudo 仅 linux/wsl; serve 后端 systemd|init.d|win)
-- @return caps[table]
M.caps = function ()
	local kind = "unknown"
	local arch = ""
	local systemctl = false
	do
		local ok_h, hi = pcall(hostinfo.probe)
		if ok_h and type(hi) == "table" then
			kind = tostring(hi.host_kind or kind)
			arch = string.lower(tostring(hi.arch or ""))
			systemctl = type(hi.caps) == "table" and hi.caps.systemctl == true
		end
	end

	local arm = string.find(arch, "arm", 1, true) ~= nil
		or string.find(arch, "aarch", 1, true) ~= nil
	local posix = (kind == "linux" or kind == "wsl")
	local initd = common.has_initd()

	return {
		host_kind = kind,
		arch = arch,
		arm = arm,
		sudo = posix,
		systemctl = systemctl,
		initd = initd,
		serve = true,
		proxy = (kind == "windows") or systemctl,
		web = true,
	}
end


-- @brief 维护项清单 (顺序:sudo / serve / proxy / web)
-- @param [in] caps[table]
-- @return entries[table]
local function _entries(caps)
	local sudo_platform = "linux/wsl"
	if not caps.sudo then
		sudo_platform = "n/a (" .. tostring(caps.host_kind) .. ")"
	end

	local branch = "linux systemd"
	if caps.host_kind == "windows" then
		branch = "win task"
	elseif not caps.systemctl then
		if caps.initd then
			branch = "linux init.d"
		else
			branch = "linux (foreground only)"
		end
	elseif caps.arm then
		branch = "linux/arm systemd"
	end

	local proxy_branch = branch
	if branch == "linux init.d" then
		proxy_branch = "linux init.d (serve only)"
	end

	return {
		{
			name = "sudo",
			supported = caps.sudo,
			platform = sudo_platform,
			cmd = "alua setup sudo [check]",
			note = "target NOPASSWD once (linux/wsl only)",
		},
		{
			name = "serve",
			supported = caps.serve,
			platform = branch,
			cmd = "alua serve yes|no [--port N] [--profile P]",
			note = "main service; persistent enable/disable",
		},
		{
			name = "proxy",
			supported = caps.proxy,
			platform = proxy_branch,
			cmd = "alua setup proxy yes|no [--port N]",
			note = "local proxy entry; persistent enable/disable",
		},
		{
			name = "web",
			supported = caps.web,
			platform = branch,
			cmd = "alua setup web yes|no [--listen PORT:PROTO] [--addr IP]",
			note = "config web page; persistent enable/disable",
		},
	}
end


M.help = function ()
	print("Usage: alua setup [sudo|serve|proxy|web] ...")
	print("       alua setup                list maintenance entries")
	fmt.print_kv("format", "--json|json|-j = JSON(AI)")
	fmt.print_kv("sudo", "alua setup sudo [check]")
	fmt.print_kv("serve", "alua serve yes|no [--port N] [--profile P]")
	fmt.print_kv("proxy", "alua setup proxy yes|no [--port N]")
	fmt.print_kv("web", "alua setup web yes|no [--listen PORT:PROTO] [--addr IP]")
	fmt.print_kv("backend", "linux: systemd system|user; embedded: init.d; win: Task Scheduler")
	fmt.print_kv("note", "system scope needs sudo; proxy persistent = systemd/win only")
end


-- @brief 无参: 列维护项清单 + 平台有效性
-- @param [in] want_json[boolean]
M.list = function (want_json)
	local caps = M.caps()
	local entries = _entries(caps)

	if want_json then
		fmt.print_json({
			ok = true,
			command = "setup",
			schema = common.SCHEMA,
			host = {
				host_kind = caps.host_kind,
				arch = caps.arch,
				arm = caps.arm,
				caps = {
					sudo = caps.sudo,
					systemctl = caps.systemctl,
					initd = caps.initd,
				},
			},
			entries = entries,
		})
		return
	end

	print("maintenance entries (alua setup):")
	for i = 1, #entries do
		local e = entries[i]
		local mark = e.supported and "+" or "-"
		print(string.format("  %s %-6s platform=%-12s %-42s %s",
			mark, e.name, e.platform, e.cmd, e.note))
	end

	fmt.print_kv("host", caps.host_kind .. " / " .. tostring(caps.arch))
	fmt.print_kv("format", "--json|json|-j = JSON(AI)")
	fmt.print_kv("help", "alua setup <entry> -h")
end


-- @brief CLI 入口: setup 无参 | setup <sub> ...
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local want_help, want_json, rest = fmt.take_cli_flags({ ... })
	local sub = rest[1]

	-- setup <entry> -h → 该子命令帮助; 其余 -h / 无参 → 清单
	if want_help then
		local mod = (type(sub) == "string") and _ENTRY_MOD[sub] or nil
		if mod ~= nil then
			mod.help()
			return true, codes.OK
		end

		M.list(want_json)
		return true, codes.OK
	end

	if #rest == 0 then
		M.list(want_json)
		return true, codes.OK
	end

	local argv = {}
	for i = 2, #rest do
		argv[#argv + 1] = rest[i]
	end

	if sub == "sudo" then
		return sudo.run(argv, want_json)
	end

	if sub == "serve" then
		return serve.run(argv, want_json)
	end

	if sub == "proxy" then
		return proxy.run(argv, want_json)
	end

	if sub == "web" then
		return web.run(argv, want_json)
	end

	-- 未知入口: 机读只给错误 JSON (勿先打清单混淆)
	if want_json then
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "unknown setup entry: " .. tostring(sub)))
	else
		M.list(false)
		io.stderr:write("alua setup: unknown entry: " .. tostring(sub) .. "\n")
	end

	return false, codes.cli.BAD_REQUEST
end


return M

-- end
