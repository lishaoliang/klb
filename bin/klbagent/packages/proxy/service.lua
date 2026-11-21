--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   service.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy 包 CLI / selfcheck 实现
-- @stability internal
-- @impl evolving
-- @note   包外勿直 require; 对外面见 init.lua (entry / selfcheck)
-- @history 修改历史
--  \n [2026-10] 原在 init.lua
--  \n [2026-11] 自根 init 拆出实现
--]]


local codes = require("klbagent.code")
local config = require("klbagent.packages.proxy.config")
local entry = require("klbagent.packages.proxy.entry")
local chain = require("klbagent.packages.proxy.chain")
local pac = require("klbagent.packages.proxy.pac")
local hop_token = require("klbagent.packages.proxy.hop_token")
local pump = require("klbagent.packages.proxy.relay.pump")
local kproxy_io = require("klbcore.klbproxy.io")


local M = {}


-- @brief 包自检
-- @return [table]
M.selfcheck = function ()
	local ok_kp, kproxy = pcall(require, "kproxy")
	local tcp_ok = ok_kp and type(kproxy) == "table" and type(kproxy.tcp_connect) == "function"
	local listen_ok = ok_kp and type(kproxy) == "table" and type(kproxy.listen) == "function"
	local relay_ok = ok_kp and type(kproxy) == "table" and type(kproxy.relay) == "function"
	local co_ok = kproxy_io.co_available()
	local pump_ok = pump.available()

	local cfg_now = entry.current_cfg()
	return {
		ok = true,
		stub = false,
		package = "proxy",
		entry_running = entry.is_running(),
		listen = cfg_now.listen or config.defaults().listen,
		listens = config.listen_group(cfg_now),
		socks5_client = true,
		socks5_entry = listen_ok and true or false,
		kproxy_tcp = tcp_ok and true or false,
		kproxy_listen = listen_ok and true or false,
		kproxy_relay = relay_ok and true or false,
		kproxy_co = co_ok and true or false,
		pump_backend = pump.backend(),
		features = {
			socks5_client = true,
			socks5_entry = listen_ok and true or false,
			http_connect_client = true,
			http_connect_entry = listen_ok and true or false,
			chain_single_hop = true,
			multi_hop = true,
			hop_probe = true,
			pac = true,
			alua_tunnel = true,
			alua_tunnel_crypto = true,
			same_port_demux = true,
			entry_auth = true,
			listen_host = true,
			routes = true,
			http_forward = true,
			direct_hop = true,
			hop_ws = true,
			hop_wss = true,
			hop_smp = true,
			hop_mnp = true,
			hop_smps = true,
			hop_mnps = true,
			hop_mnps_crypto = true,
			hop_crypto_orthogonal = true,
			hop_crypto_formats = true,
			alua_tunnel_aead = true,
			configurable_timeouts = true,
			socks5_bnd_local = true,
			http_auth_bearer = true,
			http_auth_multi_user = true,
			http_forward_keepalive = true,
			conn_stats = true,
			listen_backlog = true,
			listen_port_fallback = true,
			multi_listen = true,
			tls_opts = true,
			upstream_pool = true,
			hop_long_conn = true,
			hop_bench = true,
			dest_policy = true,
			check_multi_target = true,
			check_latency_grade = true,
			allow_from = true,
			auth_fail_limit = true,
			quiet_reject = true,
			pump = pump_ok and true or false,
			pump_co_duplex = co_ok and true or false,
			no_proxy = true,
		},
		stats = entry.stats(),
		message = "socks5+http-connect+forward; multi_hop; multi_listen; AEAD; timeouts; BND; Bearer/users; keepalive; stats; listen fallback; check grade; allow_from/rate-limit; hop_ws/wss/smp/mnp",
	}
end


-- @brief alua pkg run proxy …
-- @param [in]  argv[table|nil]
-- @param [in]  _manifest[table|nil]
-- @return [boolean] ok
-- @return [number] code
-- @return [string|nil] message
M.entry = function (argv, _manifest)
	local cmd = nil
	if type(argv) == "table" and type(argv[1]) == "string" and argv[1] ~= "" then
		cmd = argv[1]
	end

	-- 无子命令时打印 usage (勿默认 serve 阻塞)
	if cmd == nil then
		return false, codes.proxy.BAD_REQUEST,
			"usage: alua pkg run proxy [serve [config path|@path|file.lua|.json] [host:]port [user:pass]|check [proxy_url]|pac [prefer] [port]]"
	end

	if cmd == "check" then
		-- 无 proxy_url: 走 CLI 同语义 (读 proxy.json → raw 换新 temp → 探活默认链)
		-- 有 proxy_url / ALUA_PROXY: 仍按简写硬探活
		local proxy_arg = nil
		if type(argv) == "table" and type(argv[2]) == "string" and argv[2] ~= "" then
			proxy_arg = argv[2]
		end

		local ok_u, util = pcall(require, "klbagent.core.tools.net.util")
		if proxy_arg == nil then
			local env = os.getenv("ALUA_PROXY")
			if type(env) == "string" and env ~= "" then
				proxy_arg = env
			end
		end

		if proxy_arg == nil then
			local proxy_cli = require("klbagent.core.cli.proxy")
			-- 复用 alua proxy check: 试 temp 续期 / 无效再 raw 新签
			local ok_c, code_c = proxy_cli.run("check")
			if ok_c then
				return true, 0, "proxy check ok"
			end
			return false, code_c or codes.proxy.TOOL_FAILED, "proxy check fail"
		end

		local sc = M.selfcheck()
		local msg = sc.message
		local proxy = ok_u and util.proxy_parse(proxy_arg) or nil
		if proxy == nil then
			return false, codes.proxy.BAD_REQUEST, "bad proxy arg"
		end
		local hops = chain.hops_from_proxy(proxy)
		if hops ~= nil then
			local to = config.timeouts(sc)
			local targets = config.probe_targets(sc)
			local latency = config.latency_grades(sc)
			local ok_p, probe = chain.probe_hops(hops, {
				probe_targets = targets,
				timeout_ms = to.probe_ms,
				latency = latency,
			})
			msg = msg .. "; " .. tostring(probe and probe.message or (ok_p and "probe ok" or "probe fail"))
			if type(probe) == "table" and type(probe.hops) == "table" and probe.hops[1] ~= nil then
				local g = probe.hops[1].grade
				if g ~= nil then
					msg = msg .. " grade=" .. tostring(g)
				end
			end
			if not ok_p then
				return false, codes.proxy.TOOL_FAILED, msg
			end
		end

		return true, 0, msg
	end

	if cmd == "pac" then
		local opts = nil
		if type(argv) == "table" and tonumber(argv[2]) ~= nil then
			opts = { port = tonumber(argv[2]) }
		end
		if type(argv) == "table" and type(argv[2]) == "string" and argv[2] ~= "" and tonumber(argv[2]) == nil then
			-- prefer: socks5|http|socks5-only|http-only
			opts = { prefer = argv[2] }
			if tonumber(argv[3]) ~= nil then
				opts.port = tonumber(argv[3])
			end
		end

		local text, err = pac.generate(opts)
		if text == nil then
			return false, codes.proxy.TOOL_FAILED, tostring(err or "pac fail")
		end

		return true, 0, text
	end

	if cmd == "serve" then
		local cfg_path = nil
		local auth_src = nil
		local host_cli = nil
		local port_cli = nil
		local args = {}

		-- 抽出 --config/-c/config/@path; 其余留给 listen/auth
		if type(argv) == "table" then
			local i = 2
			while i <= #argv do
				local a = argv[i]
				if type(a) ~= "string" or a == "" then
					i = i + 1
				elseif a == "config" then
					-- 注意: alua C 壳会吃掉 --config/-c; 须用 config / @path / *.lua|*.json
					local p = argv[i + 1]
					if type(p) ~= "string" or p == "" then
						return false, codes.proxy.BAD_REQUEST, "serve config needs path"
					end
					cfg_path = p
					i = i + 2
				elseif string.sub(a, 1, 1) == "@" and #a > 1 then
					cfg_path = string.sub(a, 2)
					i = i + 1
				else
					args[#args + 1] = a
					i = i + 1
				end
			end
		end

		-- 首参像配置文件且未显式指定时按路径读
		if cfg_path == nil and type(args[1]) == "string" then
			local a1 = args[1]
			local lower = string.lower(a1)
			if string.match(lower, "%.lua$") or string.match(lower, "%.json$") then
				local f = io.open(a1, "rb")
				if f ~= nil then
					f:close()
					cfg_path = a1
					table.remove(args, 1)
				end
			end
		end

		local cfg, err_load = config.load(cfg_path)
		if err_load ~= nil then
			return false, codes.proxy.BAD_REQUEST, tostring(err_load)
		end

		local a2 = args[1]
		local a3 = args[2]
		local a4 = args[3]

		if tonumber(a2) ~= nil then
			-- serve PORT [user:pass]
			port_cli = tonumber(a2)
			if type(a3) == "string" and a3 ~= "" then
				auth_src = a3
			end
		elseif type(a2) == "string" and a2 ~= "" then
			local hp_host, hp_port = config.parse_hostport(a2)
			if hp_host ~= nil then
				-- serve HOST:PORT [user:pass]
				host_cli = hp_host
				port_cli = hp_port
				if type(a3) == "string" and a3 ~= "" then
					auth_src = a3
				end
			elseif tonumber(a3) ~= nil then
				-- serve HOST PORT [user:pass]
				host_cli = a2
				port_cli = tonumber(a3)
				if type(a4) == "string" and a4 ~= "" then
					auth_src = a4
				end
			else
				-- serve user:pass
				auth_src = a2
			end
		end

		if host_cli ~= nil or port_cli ~= nil then
			local ok_l, err_l = config.apply_listen(cfg, host_cli, port_cli)
			if not ok_l then
				return false, codes.proxy.BAD_REQUEST, tostring(err_l or "bad listen")
			end
		end

		if auth_src ~= nil then
			local ok_a, err_a = config.apply_entry_auth(cfg, auth_src)
			if not ok_a then
				return false, codes.proxy.BAD_REQUEST, tostring(err_a or "bad auth")
			end
		end

		local ok_v, err_v = config.validate(cfg)
		if not ok_v then
			return false, codes.proxy.BAD_REQUEST, tostring(err_v or "bad cfg")
		end

		-- 启动前: 默认链 hop 自动 auth ensure (raw TLS → temp)
		do
			local id = nil
			if type(config.active_chain_id) == "function" then
				id = config.active_chain_id(cfg)
			end
			if id == nil and type(cfg.default) == "string" then
				id = cfg.default
			end
			local hops = nil
			if id ~= nil and type(config.chain_hops) == "function" then
				hops = config.chain_hops(cfg, id)
			end
			if type(hops) == "table" and #hops > 0 then
				pcall(hop_token.ensure_hops, hops, nil)
			end
		end

		local ok, err = entry.start(cfg)
		if ok then
			return true, 0, "proxy serve stopped"
		end

		return false, codes.proxy.TOOL_FAILED, tostring(err or "serve fail")
	end

	return false, codes.proxy.BAD_REQUEST, "usage: alua pkg run proxy [serve [config path|@path|file.lua|.json] [host:]port [user:pass]|check [proxy_url]|pac [prefer] [port]]"
end


return M

-- end
