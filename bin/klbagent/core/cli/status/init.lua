--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua status — 本机状态总览 / 模块分派 (运维优先)
-- @note   无参=简略总览; status <module>[/sub...] = 模块详情 (status/<module>.lua)
-- @history 修改历史
--  \n [2026-10] 创建文件 (D27 起, 原 status.lua); nested proxy + serve/peers/unit/config + probe/schema (M1-M3 / D52); 拆为 status/ 子目录 (init + common + 各模块); 嵌入式 init.d 单元
--]]


local cjson = require("cjson")
local version = require("klbagent.version")
local config = require("klbagent.core.config")
local hostinfo = require("klbagent.core.hostinfo")
local profiles = require("klbagent.core.profiles")
local packages = require("klbagent.packages")
local codes = require("klbagent.code")
local common = require("klbagent.core.cli.status.common")
local status_proxy = require("klbagent.core.cli.status.proxy")
local status_serve = require("klbagent.core.cli.status.serve")
local status_peers = require("klbagent.core.cli.status.peers")
local status_unit = require("klbagent.core.cli.status.unit")
local status_config = require("klbagent.core.cli.status.config")
local status_web = require("klbagent.core.cli.status.web")


local M = {}


-- json / 机读契约版本 (M3); 字段演进时升版
M.SCHEMA = common.SCHEMA


local function _caps_summary(caps)
	if type(caps) ~= "table" then
		return {}
	end

	local keys = {
		"http_listen",
		"deploy_pull",
		"deploy_source",
		"systemctl",
		"exec",
		"recommend_readonly",
	}
	local out = {}
	for i = 1, #keys do
		local k = keys[i]
		out[k] = caps[k] == true
	end

	return out
end


-- @brief 模块名 → 子模块
-- @param [in] name[string]
-- @return mod[table|nil]
local function _module(name)
	name = string.lower(tostring(name or ""))
	if name == "proxy" then
		return status_proxy
	end

	if name == "serve" then
		return status_serve
	end

	if name == "peers" then
		return status_peers
	end

	if name == "unit" then
		return status_unit
	end

	if name == "config" then
		return status_config
	end

	if name == "web" then
		return status_web
	end

	return nil
end


-- @brief 读 pref/serve.port 落盘监听口 (serve bind 后写入; 供无 --port 时兜底)
-- @param [in] pref[string]
-- @return port[number|nil]
local function _read_port_file(pref)
	local path = common.path_join(pref, "serve.port")
	local f = io.open(path, "rb")
	if f == nil then
		return nil
	end

	local data = f:read("*a") or ""
	f:close()
	local port = tonumber(string.match(data, "%d+") or "")
	if type(port) ~= "number" or port < 1 or 65535 < port then
		return nil
	end

	return port
end


-- @brief 收集本机状态表
-- @param [in] opts[table|nil]		可选 { port=number }
-- @return report[table]
M.collect = function (opts)
	opts = opts or {}
	local hi = hostinfo.probe()
	local rt = config.runtime.load()
	local pref = config.dir() or ""
	pref = string.gsub(pref, "[/\\]+$", "")

	-- 端口: 显式 --port 优先; 否则读 pref/serve.port 落盘口
	--   \n core.serve 包外契约已收口 (无 read_listen_port / DEFAULT_PORT), 故本地读文件
	local port = tonumber(opts.port)
	local port_file = nil
	if type(port) ~= "number" or port < 1 or 65535 < port then
		port = _read_port_file(pref)
		if type(port) == "number" then
			port_file = common.path_join(pref, "serve.port")
		end
	end

	local token = nil
	local token_src = common.token_source()

	-- 无口可探 (既无 --port 也无 serve.port): 降级为未监听, 不整体失败
	local listening, listen_how, listen_host = false, nil, nil
	if type(port) == "number" then
		listening, listen_how, listen_host = common.tcp_listening(port)
	end
	local health = {
		ok = false,
		reachable = listening,
		err = nil,
		body = nil,
		host = nil,
	}
	if type(token) == "string" then
		token = string.gsub(token, "[\r\n]", "")
		token = string.match(token, "^%s*(.-)%s*$") or token
	end

	if listening and type(token) == "string" and token ~= "" then
		local ok_h, body, err_h, health_host = common.health_get(port, token, listen_host)
		health.ok = ok_h == true and common.health_body_ok(body)
		health.body = body
		health.err = err_h
		health.host = health_host or listen_host
		if not health.ok and type(body) == "table" then
			health.err = common.health_err_of(body, err_h or "health_not_ok")
		end
	elseif listening then
		health.err = "no_token"
		health.host = listen_host
	elseif not listening then
		if type(port) == "number" then
			health.err = "not_listening"
		else
			health.err = "no_port"
		end
	end

	local unit = nil
	if hi.host_kind == "windows" then
		unit = common.unit_windows()
	elseif hi.host_kind == "embedded" then
		unit = common.unit_initd()
	elseif hi.host_kind == "linux" or hi.host_kind == "wsl" then
		unit = common.unit_linux()
	end

	local proxy = common.proxy_runtime_summary(pref)

	local pkg_list = packages.list() or {}
	local pkg_names = {}
	for i = 1, #pkg_list do
		local row = pkg_list[i]
		if type(row) == "table" and type(row.name) == "string" then
			pkg_names[#pkg_names + 1] = row.name
		end
	end

	local serve_info = nil

	local paths = common.paths_info(pref)
	local files = {
		serve_env = common.file_exists(paths.serve_env),
		serve_port = common.file_exists(paths.serve_port),
		serve_crt = common.file_exists(paths.serve_crt),
		serve_key = common.file_exists(paths.serve_key),
		runtime_json = common.file_exists(paths.runtime_json),
		log_json = common.file_exists(paths.log_json),
		env_overlay_json = common.file_exists(paths.env_overlay_json),
		proxy_json = common.file_exists(paths.proxy_json),
		serve_json = common.file_exists(paths.serve_json),
		peers_json = common.file_exists(paths.peers_json),
	}

	local peers_info = nil
	do
		local ok_p, peers_mod = pcall(require, "klbagent.core.config.peers")
		if ok_p and type(peers_mod) == "table" and type(peers_mod.load) == "function" then
			local cfg = peers_mod.load()
			peers_info = peers_mod.counts(cfg)
		end
	end

	return {
		ok = true,
		command = "status",
		schema = M.SCHEMA,
		version = version.string,
		profile = profiles.active() or (rt and rt.profile) or "readonly",
		io = (rt and rt.io) or "stdio",
		host = {
			host_kind = hi.host_kind,
			virt = hi.virt,
			arch = hi.arch,
			network = hi.network,
			sysname = hi.sysname,
			caps = _caps_summary(hi.caps),
		},
		pref = pref,
		paths = paths,
		token = {
			set = type(token) == "string" and token ~= "",
			source = token_src,
		},
		files = files,
		peers = peers_info,
		serve = {
			port = port,
			port_file = port_file,
			port_default = nil,
			listening = listening,
			listen_probe = listen_how,
			health = health,
			transport_capable = serve_info,
		},
		unit = unit,
		proxy = proxy,
		packages = {
			count = #pkg_names,
			names = pkg_names,
		},
	}
end


-- @brief 人类可读简略总览 (运维优先; 路径清单见 status config / json)
-- @param [in] report[table]
M.print_text = function (report)
	common.print_kv("version", report.version)
	local host = report.host or {}
	common.print_kv("host", string.format("%s / %s (virt=%s, network=%s)",
		tostring(host.host_kind),
		tostring(host.arch),
		tostring(host.virt),
		tostring(host.network)))
	common.print_kv("profile", string.format("%s (io=%s)", tostring(report.profile), tostring(report.io)))

	local tok = report.token or {}
	common.print_kv("token", string.format("%s (source=%s)", common.yn(tok.set), tostring(tok.source)))

	local sv = report.serve or {}
	local health = sv.health or {}
	local port_s = ""
	if type(sv.port) == "number" then
		port_s = string.format("  port=%d", sv.port)
	end

	if health.ok == true and type(health.body) == "table" then
		local b = health.body
		local via = ""
		if type(health.host) == "string" and health.host ~= "" then
			-- host:port 一眼可见实际探活落点
			if type(sv.port) == "number" then
				via = string.format(" via %s:%d", health.host, sv.port)
			else
				via = " via " .. health.host
			end
		end

		local tr = ""
		if type(b.transport) == "table" then
			tr = "  " .. table.concat(b.transport, ",")
		end

		common.print_kv("serve", string.format("running%s  %s  profile=%s%s",
			via,
			tostring(b.version or ""),
			tostring(b.profile or ""),
			tr))
	elseif sv.listening then
		common.print_kv("serve", string.format("listening%s health=%s",
			port_s,
			tostring(health.err or "?")))
	else
		common.print_kv("serve", string.format("not running%s", port_s))
	end

	local unit = report.unit
	if type(unit) == "table" then
		if unit.kind == "systemd" then
			common.print_kv("unit", string.format("%s active=%s enabled=%s scope=%s",
				tostring(unit.name),
				tostring(unit.active),
				tostring(unit.enabled),
				tostring(unit.scope)))
		elseif unit.kind == "task_scheduler" then
			common.print_kv("unit", string.format("%s state=%s",
				tostring(unit.name),
				tostring(unit.state or "unknown")))
		end
	end

	-- proxy 总览仅二级简略 (可否服务 / 端口 / 有无下级);  hop 细节见 alua status proxy
	local proxy = report.proxy
	if type(proxy) == "table" then
		local listen = proxy.listen or {}
		local addr = string.format("%s:%s",
			tostring(listen.host or "?"),
			common.port_str(listen.port))
		local n_chain = tonumber(proxy.chain_count) or 0
		local has_down = n_chain > 0
			or (type(proxy.active_hops) == "table" and #proxy.active_hops > 0)

		if proxy.running == true or proxy.configured == true then
			local top = (proxy.running == true) and "running" or "configured"
			common.print_kv("proxy", top)
			common.print_kv("  serve", string.format("%s  %s",
				common.yn(proxy.running == true), addr))
			if has_down then
				common.print_kv("  downstream", string.format("yes  chains=%s",
					tostring(n_chain)))
			else
				common.print_kv("  downstream", "no")
			end
		else
			common.print_kv("proxy", "not configured")
		end
	end

	local peers = report.peers or {}
	common.print_kv("peers", string.format("total=%s trusted=%s pending=%s revoked=%s",
		tostring(peers.total or 0),
		tostring(peers.trusted or 0),
		tostring(peers.pending or 0),
		tostring(peers.revoked or 0)))

	local pkgs = report.packages or {}
	common.print_kv("packages", tostring(pkgs.count or 0))

	local caps = host.caps or {}
	common.print_kv("caps", string.format(
		"listen=%s deploy_pull=%s deploy_source=%s systemctl=%s exec=%s",
		common.yn(caps.http_listen),
		common.yn(caps.deploy_pull),
		common.yn(caps.deploy_source),
		common.yn(caps.systemctl),
		common.yn(caps.exec)))
end


-- @brief 模块分发 (proxy nested + serve/peers/unit/config)
-- @param [in] path[table]		如 { "proxy" } / { "proxy", "chains" }
-- @param [in] opts[table|nil]	`{ port=, probe= }`
-- @return report[table]
M.collect_module = function (path, opts)
	opts = opts or {}
	local pref = config.dir() or ""
	pref = string.gsub(pref, "[/\\]+$", "")

	if type(path) ~= "table" or #path == 0 then
		return M.collect(opts)
	end

	local name = string.lower(tostring(path[1] or ""))
	local rest = {}
	for i = 2, #path do
		rest[#rest + 1] = path[i]
	end

	local mod = _module(name)
	if mod ~= nil then
		opts.pref = pref
		return mod.collect(rest, opts)
	end

	return {
		ok = false,
		command = "status",
		schema = M.SCHEMA,
		module = name,
		message = "unknown status module: " .. name,
		version = version.string,
	}
end


M.help = function ()
	print("Usage: alua status [module [sub...]] [--json] [--probe] [port]")
	print("  Brief overview (ops-first). Modules: proxy|serve|peers|unit|config|web.")
	print("  alua status              brief: serve/unit/proxy/peers/...")
	print("  alua status proxy        proxy detail + nested summary")
	print("  alua status proxy chains|listen|routes|stats|hop")
	print("  alua status serve|peers|unit|config")
	print("  alua status web          config page unit + listen group + port probe")
	print("  --probe | probe          ops soft hop probe (proxy / proxy hop); NOT proxy.check")
	print("  --json | json | -j       JSON (schema=" .. M.SCHEMA .. "); default = human text")
	print("  port                     probe serve port (overview / status serve); default pref/serve.port")
	print("  Does not print token values.")
	print("  Hard probe/grade: alua call proxy.check / pkg run proxy check (see docs/tools/proxy.md)")
end


-- @brief CLI 入口
-- @param [in] ...		[module [sub...]] [json] [probe] [port]
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local argv = { ... }
	local want_json = false
	local want_probe = false
	local port = nil
	local path = {}

	for i = 1, #argv do
		local a = argv[i]
		-- 壳 +hvp 后, 子命令侧 -h/--help/help 到此
		if a == "json" or a == "--json" or a == "-j" then
			want_json = true
		elseif a == "probe" or a == "--probe" then
			want_probe = true
		elseif a == "help" or a == "--help" or a == "-h" then
			M.help()
			return true, codes.OK
		elseif string.match(tostring(a), "^%d+$") then
			port = tonumber(a)
		elseif type(a) == "string" and a ~= "" then
			-- 支持 proxy/chains 或 proxy chains
			if string.find(a, "/", 1, true) then
				for seg in string.gmatch(a, "[^/]+") do
					path[#path + 1] = seg
				end
			else
				path[#path + 1] = a
			end
		else
			io.stderr:write("alua status: unknown arg: " .. tostring(a) .. "\n")
			M.help()
			return false, codes.cli.BAD_REQUEST
		end
	end

	local opts = { port = port, probe = want_probe }
	local report
	if #path > 0 then
		report = M.collect_module(path, opts)
	else
		report = M.collect(opts)
	end

	if want_json then
		local ok_j, text = pcall(cjson.encode, report)
		if not ok_j or type(text) ~= "string" then
			print("{}")
			return false, codes.cli.TOOL_FAILED
		end

		print(text)
	else
		if #path > 0 then
			local mod = _module(path[1])
			if mod ~= nil and type(mod.print_text) == "function" then
				mod.print_text(report)
			else
				common.print_kv("error", "unknown status module: " .. tostring(path[1]))
			end
		else
			M.print_text(report)
		end
	end

	if report.ok == false then
		return false, codes.cli.BAD_REQUEST
	end

	return true, codes.OK
end


return M

-- end
