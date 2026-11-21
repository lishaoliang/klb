--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbagent CLI 主分派
-- @note   顶层命令各有 cli/<cmd>.lua; alua <cmd> -h = 该命令帮助; alua -h = 总表
-- @history 修改历史
--  \n [2026-09] 创建文件; 迁入子目录 cli/init.lua; 增加 serve / pkg
--  \n [2026-10] peer / ui|gui; 顶层命令拆至 cli/*.lua; init 仅分派; alua <cmd> -h 子命令帮助; auth 子命令; serve 启动前 _ensure_tools (D49); 启动时应用 runtime.loop_sleep_ms → kenv.set_loop_sleep; setup: sudo 入口装载 tools; <entry> -h 交由 setup.run 分派
--  \n [2026-11] up: 单进程服务宿主 (serve|proxy|svn_https)
--  \n [2026-11] 删除 alua auth 子命令 (对外改用 ssid)
--]]
local registry = require("klbagent.core.registry")
local catalog = require("klbagent.core.catalog")
local dispatch = require("klbagent.core.dispatch")
local codes = require("klbagent.code")
local stdio = require("klbagent.core.stdio")
local fmt = require("klbagent.core.cli.fmt")
local co_task = require("klbagent.core.cli.co_task")
local status = require("klbagent.core.cli.status")
local pkg = require("klbagent.core.cli.pkg")
local peer = require("klbagent.core.cli.peer")
local ssid_cli = require("klbagent.core.cli.ssid")
local proxy_cli = require("klbagent.core.cli.proxy")
local list = require("klbagent.core.cli.list")
local categories = require("klbagent.core.cli.categories")
local docs = require("klbagent.core.cli.docs")
local setup = require("klbagent.core.cli.setup")
local setup_sudo = require("klbagent.core.cli.setup.sudo")
local setup_serve = require("klbagent.core.cli.setup.serve")
local ui = require("klbagent.core.cli.ui")
local call = require("klbagent.core.cli.call")
local run = require("klbagent.core.cli.run")
local mcp = require("klbagent.core.cli.mcp")


local M = {}


local loaded = false


-- @brief 应用 runtime.loop_sleep_ms 到本 env (默认 5ms; 失败忽略)
local function _apply_loop_sleep()
	local ok_r, runtime = pcall(require, "klbagent.core.config.runtime")
	if not ok_r or type(runtime) ~= "table" or type(runtime.load) ~= "function" then
		return
	end

	local cfg = runtime.load()
	local ms = 5
	if type(cfg) == "table" and cfg.loop_sleep_ms ~= nil then
		ms = tonumber(cfg.loop_sleep_ms) or 5
	end

	if ms < 1 then
		ms = 1
	end

	local ok_e, kenv = pcall(require, "kenv")
	if ok_e and type(kenv) == "table" and type(kenv.set_loop_sleep) == "function" then
		kenv.set_loop_sleep(ms)
	end
end


local function _ensure_tools()
	if loaded then
		return true
	end

	local ok, err = registry.load_all(catalog, dispatch)
	if not ok then
		return false, err
	end

	loaded = true
	return true
end


local function _usage()
	print("Commands (klbagent)")
	print("")
	fmt.print_kv("help", "alua -h = this list; alua <cmd> -h = command help")
	fmt.print_kv("format", "default=text; --json|json|-j=JSON(AI); flag AFTER subcommand")
	fmt.print_kv("shell", "-h / -v / -p PATH  (BEFORE command only)")
	print("")
	print("  alua status [module…] [--json] [port]     overview; status proxy")
	print("  alua --serve|-s [port] [profile]          listen (def 12817 readonly)")
	print("  alua pkg list [--json]                    packages")
	print("  alua pkg run <name> [args...]             run package")
	print("  alua peer list [--json]                   peers")
	print("  alua peer ping|approve|revoke <id> [--json]")
	print("  alua ssid whoami|ensure|status|begin …    client SSID facade")
	print("  alua proxy check [--json] [--force]       try temp renew / else issue + probe")
	print("  alua ui|gui [args...]                     alua_ui")
	print("  alua setup [sudo|serve|proxy|web] …       maintenance entries; setup <e> -h")
	print("  alua serve yes|no [--port N] [--profile P]  persistent enable/disable")
	print("  alua docs [path] [--json]                 docs/")
	print("  alua list [category] [--json]             tools")
	print("  alua categories [--json]                  categories")
	print("  alua call <tool> [json_args]              always JSON")
	print("  alua run <path.lua> [args...]             always JSON (unsafe)")
	print("  alua stdio                                always JSON lines")
	print("  alua mcp serve|list|call|probe ...        MCP server (stdio) / HTTP client")
end


local function _serve_help()
	print("Usage: alua --serve|-s [port] [profile]   (listen)")
	print("       alua serve yes|no [--port N] [--profile P] [--json]")
	fmt.print_kv("serve", "HTTPS/mux peer listen (blocks); client side: use --serve")
	fmt.print_kv("port", "explicit port; bare = pool 12817.. then random")
	fmt.print_kv("profile", "default readonly; needs ALUA_SERVE_TOKEN")
end


local function _stdio_help()
	print("Usage: alua stdio")
	fmt.print_kv("stdio", "line JSON Request/Response until EOF (always JSON)")
end


-- @brief 若 argv 含 -h/--help/help 则打 mod.help, 不装载 tools
-- @return handled[boolean]
-- @return ok[boolean|nil]
-- @return code[number|nil]
local function _try_cmd_help(mod, argv)
	if type(mod) ~= "table" or type(mod.help) ~= "function" then
		return false
	end

	if not fmt.has_help_flag(argv) then
		return false
	end

	mod.help()
	return true, true, codes.OK
end


-- @brief 业务协程跑子命令 (分派与业务分离; 业务内可再 co_task.spawn)
local function _biz(fn, ...)
	return co_task.co_biz(fn, ...)
end


-- @brief CLI 入口 (由 klbagent.entry_cli 转发)
-- @param [in] ...					位置参数
-- @return ok[boolean]
-- @return code[number]
-- @note 由 entry_cli 包在 co_task 协程内; 业务经 co_biz fork; 常驻子命令 hold 后外层泵至 kenv.exit
M.entry = function (...)
	local argv = { ... }
	local cmd = argv[1]
	local args = { table.unpack(argv, 2) }

	-- 主 env 休眠上限: runtime.json loop_sleep_ms (默认 1)
	_apply_loop_sleep()

	-- 无命令 / 全局 help: 不装载 tools (同步, 非业务)
	if type(cmd) ~= "string" or cmd == "" then
		_usage()
		return true, codes.OK
	end

	if cmd == "help" or cmd == "-h" or cmd == "--help" then
		_usage()
		return true, codes.OK
	end

	-- 子命令帮助优先 (免装载 tools / 免进 serve)
	if cmd == "status" then
		local handled, ok_h, code_h = _try_cmd_help(status, args)
		if handled then
			return ok_h, code_h
		end

		return _biz(status.run, table.unpack(args))
	end

	if cmd == "setup" then
		-- setup <entry> -h: 由 setup.run 分派到子命令帮助 (勿在此统一拦截)
		-- sudo 入口依赖工具注册 (deploy.setup_priv); 其余入口保持纯 CLI (免 lpc 日志污染 --json)
		local _, _, rest_s = fmt.take_cli_flags(args)
		if rest_s[1] == "sudo" and not fmt.has_help_flag(args) then
			local ok_load, err_load = _ensure_tools()
			if not ok_load then
				fmt.print_json(codes.make_error(codes.cli.TOOL_FAILED, tostring(err_load)))
				return false, codes.cli.TOOL_FAILED
			end
		end

		return _biz(setup.run, table.unpack(args))
	end

	if cmd == "serve" then
		if fmt.has_help_flag(args) then
			local _, rest_h = fmt.take_json_flag(args)
			if rest_h[1] == "yes" or rest_h[1] == "no" then
				setup_serve.help()
				return true, codes.OK
			end

			_serve_help()
			return true, codes.OK
		end

		-- alua serve yes|no = 常驻启停语义 (分平台)
		local want_json_s, rest_s = fmt.take_json_flag(args)
		if rest_s[1] == "yes" or rest_s[1] == "no" then
			return _biz(setup_serve.run, rest_s, want_json_s)
		end

		-- 客户端侧不再起监听: 唯一服务入口为 alua --serve / alua -s
		io.stderr:write("alua serve: listen moved to 'alua --serve [port] [profile]' (or -s)\n")
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "serve listen moved to --serve"))
		return false, codes.cli.BAD_REQUEST
	end

	if cmd == "stdio" then
		if fmt.has_help_flag(args) then
			_stdio_help()
			return true, codes.OK
		end

		local ok_load, err_load = _ensure_tools()
		if not ok_load then
			fmt.print_json(codes.make_error(codes.cli.TOOL_FAILED, tostring(err_load)))
			return false, codes.cli.TOOL_FAILED
		end

		return _biz(stdio.loop)
	end

	do
		local mod = nil
		if cmd == "pkg" then
			mod = pkg
		elseif cmd == "peer" then
			mod = peer
		elseif cmd == "ssid" then
			mod = ssid_cli
		elseif cmd == "proxy" then
			mod = proxy_cli
		elseif cmd == "list" then
			mod = list
		elseif cmd == "categories" then
			mod = categories
		elseif cmd == "docs" then
			mod = docs
		elseif cmd == "setup" then
			mod = setup
		elseif cmd == "ui" or cmd == "gui" then
			mod = ui
		elseif cmd == "call" then
			mod = call
		elseif cmd == "run" then
			mod = run
		elseif cmd == "mcp" then
			mod = mcp
		elseif cmd == "sudo" then
			mod = setup
		end

		if mod ~= nil then
			local handled, ok_h, code_h = _try_cmd_help(mod, args)
			if handled then
				return ok_h, code_h
			end
		end
	end

	local ok_load, err_load = _ensure_tools()
	if not ok_load then
		fmt.print_json(codes.make_error(codes.cli.TOOL_FAILED, tostring(err_load)))
		return false, codes.cli.TOOL_FAILED
	end

	if cmd == "list" then
		return _biz(list.run, table.unpack(args))
	end

	if cmd == "categories" then
		return _biz(categories.run, table.unpack(args))
	end

	if cmd == "call" then
		return _biz(call.run, table.unpack(args))
	end

	if cmd == "docs" then
		return _biz(docs.run, table.unpack(args))
	end

	if cmd == "run" then
		return _biz(run.run, table.unpack(args))
	end

	if cmd == "mcp" then
		return _biz(mcp.run, table.unpack(args))
	end

	if cmd == "peer" then
		return _biz(peer.run, table.unpack(args))
	end

	if cmd == "ssid" then
		return _biz(ssid_cli.run, table.unpack(args))
	end

	if cmd == "proxy" then
		return _biz(proxy_cli.run, table.unpack(args))
	end

	if cmd == "pkg" then
		return _biz(pkg.run, table.unpack(args))
	end

	if cmd == "ui" or cmd == "gui" then
		return _biz(ui.run, table.unpack(args))
	end

	if cmd == "sudo" then
		local want_json, rest = fmt.take_json_flag(args)
		return _biz(setup_sudo.run, rest, want_json)
	end

	_usage()
	fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "unknown command"))
	return false, codes.cli.BAD_REQUEST
end


return M
