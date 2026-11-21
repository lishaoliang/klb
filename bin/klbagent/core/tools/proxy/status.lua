--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   status.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy.status / proxy.check
-- @note   category=net; 能力状态 + 默认 listen 探活 + 可选 hop 探活
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); http_connect_entry 纳入 available; check 支持 hops/proxy 探活; status 收口; 默认 listen 探活; 无显式链时回落 proxy.json 默认链; 读入口运行态快照 (Bug.2026-11/12)
--]]


local M = {}


local function _has_hop_probe(st)
	return type(st) == "table" and type(st.features) == "table" and st.features.hop_probe == true
end


local function _defaults_listen()
	local ok_c, config = pcall(require, "klbagent.packages.proxy.config")
	if ok_c and type(config) == "table" and type(config.defaults) == "function" then
		local cfg = config.defaults()
		if type(cfg) == "table" and type(cfg.listen) == "table" then
			return cfg.listen
		end
	end

	return { host = "127.0.0.1", port = 12818, protocols = { "socks5", "http-connect" } }
end


local function _resolve_hops(args, host_hint, port_hint)
	if type(args) ~= "table" then
		args = {}
	end

	if type(args.hops) == "table" and #args.hops > 0 then
		return args.hops, "args"
	end

	if args.proxy ~= nil then
		local ok_u, util = pcall(require, "klbagent.core.tools.net.util")
		if not ok_u or type(util) ~= "table" or type(util.proxy_normalize) ~= "function" then
			return nil, nil, "net.util unavailable"
		end

		local proxy = util.proxy_normalize(args.proxy)
		if proxy == nil then
			return nil, nil, "bad proxy"
		end

		local ok_c, chain = pcall(require, "klbagent.packages.proxy.chain")
		if not ok_c or type(chain.hops_from_proxy) ~= "function" then
			return nil, nil, "chain unavailable"
		end

		return chain.hops_from_proxy(proxy), "args"
	end

	local env = os.getenv("ALUA_PROXY")
	if type(env) == "string" and env ~= "" then
		local ok_u, util = pcall(require, "klbagent.core.tools.net.util")
		if ok_u and type(util.proxy_parse) == "function" then
			local proxy = util.proxy_parse(env)
			if proxy == nil then
				return nil, nil, "ALUA_PROXY invalid"
			end

			local ok_c, chain = pcall(require, "klbagent.packages.proxy.chain")
			if ok_c and type(chain.hops_from_proxy) == "function" then
				return chain.hops_from_proxy(proxy), "env"
			end
		end
	end

	-- 无显式链/环境: 回落 proxy.json 默认链 (Bug.2026-12)
	local ok_c, config = pcall(require, "klbagent.packages.proxy.config")
	if not ok_c or type(config) ~= "table" then
		return nil, nil
	end

	local cfg = nil
	local ok_p, cproxy = pcall(require, "klbagent.core.config.proxy")
	if ok_p and type(cproxy) == "table" and type(cproxy.load) == "function" then
		cfg = cproxy.load()
	end

	if type(cfg) ~= "table" and type(config.defaults) == "function" then
		cfg = config.defaults()
	end

	local hint = host_hint
	if type(hint) ~= "string" or hint == "" then
		hint = "example.com"
	end

	local port = tonumber(port_hint) or 443

	if type(config.select_hops) == "function" then
		local hops, info = config.select_hops(cfg, hint, port)
		if type(hops) == "table" and #hops > 0 then
			return hops, (type(info) == "table" and info.via) or "config"
		end
	end

	if type(config.chain_hops) == "function" and type(config.active_chain_id) == "function" then
		local id = config.active_chain_id(cfg)
		if id ~= nil then
			local hops = config.chain_hops(cfg, id)
			if type(hops) == "table" and #hops > 0 then
				return hops, "config.default"
			end
		end
	end

	return nil, nil
end


local function _listen_hops(listen)
	if type(listen) ~= "table" then
		return nil
	end

	local host = listen.host or "127.0.0.1"
	local port = tonumber(listen.port) or 12818
	local protocols = listen.protocols
	local prefer = "socks5"
	if type(protocols) == "table" then
		for i = 1, #protocols do
			local p = string.lower(tostring(protocols[i]))
			if p == "socks5" then
				prefer = "socks5"
				break
			elseif p == "http-connect" or p == "http_connect" then
				prefer = "http-connect"
			end
		end
	end

	return { { type = prefer, host = host, port = port } }
end


local function _do_probe(hops, args)
	local ok_c, chain = pcall(require, "klbagent.packages.proxy.chain")
	if not ok_c or type(chain.probe_hops) ~= "function" then
		return false, { ok = false, message = "chain.probe_hops unavailable" }
	end

	local ok_cfg, config = pcall(require, "klbagent.packages.proxy.config")
	local opts = {
		timeout_ms = nil,
		probe_targets = nil,
		latency = nil,
	}
	if ok_cfg and type(config) == "table" then
		if type(config.probe_targets) == "function" then
			opts.probe_targets = config.probe_targets(nil)
		end
		if type(config.latency_grades) == "function" then
			opts.latency = config.latency_grades(nil)
		end
		if type(config.timeouts) == "function" then
			opts.timeout_ms = config.timeouts(nil).probe_ms
		end
	end

	if type(args) == "table" then
		if args.timeout_ms ~= nil then
			opts.timeout_ms = args.timeout_ms
		end
		if type(args.probe_target) == "table" then
			opts.probe_target = args.probe_target
			opts.probe_targets = nil
		end
		if type(args.probe_targets) == "table" then
			opts.probe_targets = args.probe_targets
		end
		if type(args.latency) == "table" then
			opts.latency = args.latency
		end
		if args.all_targets == true then
			opts.all_targets = true
		end
	end

	return chain.probe_hops(hops, opts)
end


local function _status(_args)
	local ok_pkg, proxy = pcall(require, "klbagent.packages.proxy")
	local listen = _defaults_listen()
	local entry_running = false
	local pkg = {
		installed = ok_pkg and type(proxy) == "table",
		stub = false,
	}

	local features = {
		socks5_client = false,
		socks5_entry = false,
		http_connect_entry = false,
		multi_hop = false,
		hop_probe = false,
		pump = false,
	}

	if pkg.installed and type(proxy.selfcheck) == "function" then
		local sc = proxy.selfcheck()
		if type(sc) == "table" then
			pkg.selfcheck = sc
			entry_running = sc.entry_running == true
			if type(sc.listen) == "table" then
				listen = sc.listen
			end
			if type(sc.features) == "table" then
				for k, v in pairs(sc.features) do
					features[k] = v
				end
			end
		end
	end

	local available = (features.socks5_client == true)
		and (features.socks5_entry == true)
		and (features.http_connect_entry == true)
		and (features.hop_probe == true)

	local conn_stats = nil
	local in_proc = false
	local listen_mod = nil
	if pkg.installed and type(proxy.listen) == "function" then
		local ok_l, mod = pcall(proxy.listen)
		if ok_l and type(mod) == "table" then
			listen_mod = mod
		end
	end

	if type(listen_mod) == "table" and type(listen_mod.is_running) == "function" then
		in_proc = listen_mod.is_running() == true
	end

	if in_proc and type(listen_mod) == "table" and type(listen_mod.stats) == "function" then
		conn_stats = listen_mod.stats()
	else
		-- 入口常驻于独立进程: 读运行态快照 (Bug.2026-11)
		local rt = nil
		local stats_mod = nil
		if pkg.installed and type(proxy.stats) == "function" then
			local ok_s, mod = pcall(proxy.stats)
			if ok_s and type(mod) == "table" then
				stats_mod = mod
			end
		end
		if type(stats_mod) == "table" and type(stats_mod.load_runtime) == "function" then
			rt = stats_mod.load_runtime()
		end

		if type(rt) == "table" then
			entry_running = true
			conn_stats = rt
		elseif type(pkg.selfcheck) == "table" and type(pkg.selfcheck.stats) == "table" then
			conn_stats = pkg.selfcheck.stats
		end
	end

	local listens = nil
	if type(pkg.selfcheck) == "table" and type(pkg.selfcheck.listens) == "table" then
		listens = pkg.selfcheck.listens
	end

	return {
		available = available,
		stub = false,
		message = available and "proxy ready (socks5+http-connect+multi_hop+probe)" or "proxy partial",
		package = pkg,
		features = features,
		listen = listen,
		listens = listens,
		entry_running = entry_running,
		stats = conn_stats,
	}
end


local function _check(args)
	local st = _status(args)
	local result = {
		ok = st.available == true,
		stub = false,
		status = st,
		message = st.message,
		probe = nil,
		listen_probe = nil,
		probe_source = nil,
	}

	local hint_host = nil
	local hint_port = nil
	if type(args) == "table" and type(args.probe_target) == "table" then
		hint_host = args.probe_target.host
		hint_port = args.probe_target.port
	end

	local hops, source, err_h = _resolve_hops(args, hint_host, hint_port)
	if err_h ~= nil then
		result.ok = false
		result.message = err_h
		return result
	end

	if hops ~= nil then
		-- 探活前: 对 temp.* hop 自动 auth ensure
		do
			local ok_ht, hop_token = pcall(require, "klbagent.packages.proxy.hop_token")
			if ok_ht and type(hop_token) == "table" and type(hop_token.ensure_hops) == "function" then
				pcall(hop_token.ensure_hops, hops, nil)
			end
		end

		local ok_p, probe = _do_probe(hops, args)
		result.probe = probe
		result.probe_source = source
		if not ok_p then
			result.ok = false
			result.message = (probe and probe.message) or "hop probe fail"
		else
			result.ok = st.available == true
			result.message = (probe and probe.message) or result.message
		end
		return result
	end

	-- 无显式链: 对默认 listen 做软探活 (入口未起不判失败)
	local lh = _listen_hops(st.listen)
	if lh ~= nil and _has_hop_probe(st) then
		local ok_l, probe_l = _do_probe(lh, args)
		result.listen_probe = probe_l
		result.probe_source = "listen"
		if ok_l then
			result.message = st.message .. "; listen " .. tostring(probe_l.message)
		else
			result.message = st.message .. "; listen down (" .. tostring(probe_l and probe_l.message) .. ")"
		end
	end

	return result
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "proxy.status",
		category = "net",
		title = "proxy capability status",
		summary = "Capability + default listen; features socks5/http-connect/multi_hop/probe.",
		min_profile = "net",
		args_schema = {},
		examples = { { args = {} } },
		docs = "tools/proxy.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("proxy.status", _status)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "proxy.check",
		category = "net",
		title = "proxy health check",
		summary = "Capability + default chain/listen probe; optional hops/proxy/ALUA_PROXY.",
		min_profile = "net",
		args_schema = {
			hops = "table optional hop array",
			proxy = "string|table optional single-hop shorthand",
			probe_target = "table optional {host,port} for http-connect CONNECT probe",
			timeout_ms = "number optional per-hop connect timeout",
		},
		examples = {
			{ args = {} },
			{ args = { proxy = "socks5h://127.0.0.1:12818" } },
			{ args = { hops = { { type = "socks5", host = "127.0.0.1", port = 12818 } } } },
		},
		docs = "tools/proxy.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("proxy.check", _check)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
