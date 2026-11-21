--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   proxy.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua proxy — check 读现行配置; 下级 hop: ssid 证明票 ensure; 再探活
-- @note   alua proxy check [--json] [--force]; ≠ alua setup proxy / pkg run proxy serve
--]]


local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")


local M = {}


M.help = function ()
	print("Usage: alua proxy check [--json] [--force]")
	print("       alua proxy -h")
	fmt.print_kv("check", "load proxy.json; ensure ssid proof token; then probe")
	fmt.print_kv("force", "drop cached proof; re-derive from local raw")
	fmt.print_kv("format", "--json|json|-j = JSON(AI); never print token secrets")
	fmt.print_kv("also", "alua pkg run proxy check [proxy_url]; alua call proxy.check {}")
	fmt.print_kv("setup", "alua setup proxy yes|no  (unit enable; separate)")
end


local function _kv_args(args)
	local out = {}
	for i = 1, #args do
		local a = args[i]
		local k, v = string.match(tostring(a), "^%-%-([%w_]+)=(.*)$")
		if k ~= nil then
			if v == "true" then
				out[k] = true
			elseif v == "false" then
				out[k] = false
			else
				local n = tonumber(v)
				if n ~= nil and string.match(v, "^%-?%d+%.?%d*$") then
					out[k] = n
				else
					out[k] = v
				end
			end
		elseif a == "--json" or a == "json" or a == "-j" then
			out._json = true
		elseif a == "--force" then
			out.force = true
		end
	end

	return out
end


local function _hop_needs_token(hop)
	if type(hop) ~= "table" then
		return false
	end

	local t = hop.type or hop.kind
	if type(t) == "string" then
		t = string.lower(t)
	else
		t = ""
	end

	if t == "mnp" or t == "mnps" or t == "smp" or t == "smps"
		or t == "ws" or t == "wss" or t == "alua-tunnel" then
		return true
	end

	if type(hop.alua_ssid) == "string" and hop.alua_ssid ~= "" then
		return true
	end

	if type(hop.client_id) == "string" and hop.client_id ~= "" then
		return true
	end

	-- 兼容旧配置残留: mem: / file:temp.* 仍触发 ensure→ssid
	if type(hop.token_ref) == "string" and hop.token_ref ~= "" then
		if string.match(hop.token_ref, "^mem:") ~= nil
			or string.match(hop.token_ref, "^file:temp%.") ~= nil
			or string.match(hop.token_ref, "^temp%.") ~= nil then
			return true
		end
	end

	return false
end


local function _scrub_ensure_info(info)
	if type(info) ~= "table" then
		return tostring(info)
	end

	return {
		ok = info.ok,
		skipped = info.skipped,
		action = info.action,
		reason = info.reason,
		client_id = info.client_id,
		alua_ssid = info.alua_ssid,
		profile = info.profile,
		scheme = info.scheme,
		host = info.host,
		port = info.port,
		kind = info.kind,
		expire_ms = info.expire_ms,
		prev_err = info.prev_err,
		-- 禁 token 真值; 不回显 token_ref
	}
end


-- @brief 核心: 读配置 → ssid 证明票 ensure → 探活默认链
local function _do_check(opts)
	opts = opts or {}
	local force = opts.force == true

	local config = require("klbagent.packages.proxy.config")
	local hop_token = require("klbagent.packages.proxy.hop_token")
	local chain = require("klbagent.packages.proxy.chain")
	local cproxy = require("klbagent.core.config.proxy")

	local cfg = cproxy.load()
	if type(cfg) ~= "table" then
		cfg = config.defaults()
	end

	local listen = cfg.listen or config.defaults().listen
	local chain_id = nil
	if type(config.active_chain_id) == "function" then
		chain_id = config.active_chain_id(cfg)
	end
	if chain_id == nil and type(cfg.default) == "string" then
		chain_id = cfg.default
	end

	local hops = nil
	local hops_err = nil
	if chain_id ~= nil and type(config.chain_hops) == "function" then
		hops, hops_err = config.chain_hops(cfg, chain_id)
	end

	local result = {
		ok = true,
		command = "proxy.check",
		listen = {
			host = listen and listen.host or "127.0.0.1",
			port = listen and tonumber(listen.port) or 12818,
			protocols = listen and listen.protocols or nil,
		},
		active_chain = chain_id,
		downstream = false,
		ensure = {},
		probe = nil,
		listen_probe = nil,
		message = nil,
	}

	local has_downstream = false
	if type(hops) == "table" and #hops > 0 then
		for i = 1, #hops do
			if _hop_needs_token(hops[i]) then
				has_downstream = true
				break
			end
		end
	end
	result.downstream = has_downstream

	if hops_err ~= nil and hops == nil then
		result.ok = false
		result.message = "chain: " .. tostring(hops_err)
		return result
	end

	-- 有下级需 token 的 hop: ssid 证明票 ensure (--force 丢弃缓存重算)
	local ensure_actions = {}
	if has_downstream then
		local ensure_opts = { force = force }
		local all_ok, reports = hop_token.ensure_hops(hops, ensure_opts)
		for i = 1, #reports do
			local r = reports[i]
			local info = _scrub_ensure_info(r.info)
			result.ensure[#result.ensure + 1] = {
				index = r.index,
				ok = r.ok,
				info = info,
			}
			if type(info) == "table" and type(info.action) == "string" then
				ensure_actions[#ensure_actions + 1] = info.action
			end
			if not r.ok then
				all_ok = false
			end
		end
		if not all_ok then
			result.ok = false
			result.message = "ensure fail (need raw token: ALUA_SERVE_TOKEN / serve.env)"
			return result
		end
	end
	result.ensure_actions = ensure_actions

	-- 探活默认链 (硬)
	if type(hops) == "table" and #hops > 0 then
		local to = config.timeouts(cfg)
		local targets = config.probe_targets(cfg)
		local latency = config.latency_grades(cfg)
		local ok_p, probe = chain.probe_hops(hops, {
			probe_targets = targets,
			timeout_ms = to.probe_ms,
			latency = latency,
		})
		result.probe = probe
		if not ok_p then
			result.ok = false
			result.message = (probe and probe.message) or "hop probe fail"
			return result
		end
	else
		result.message = "no downstream hops (direct); skip ensure"
	end

	-- listen 软探活 (入口未起不硬失败)
	do
		local lh = {
			{
				type = "socks5",
				host = result.listen.host,
				port = result.listen.port,
			},
		}
		local to = config.timeouts(cfg)
		local ok_l, probe_l = chain.probe_hops(lh, {
			timeout_ms = to.probe_ms,
		})
		result.listen_probe = probe_l
		if not ok_l then
			-- soft
			if result.message == nil then
				result.message = "chain ok; listen soft down"
			else
				result.message = tostring(result.message) .. "; listen soft down"
			end
		end
	end

	if result.message == nil then
		if has_downstream then
			local act = table.concat(ensure_actions, ",")
			if act == "" then
				act = force and "issued" or "ensure"
			end
			result.message = act .. " + probe ok"
		else
			result.message = "probe ok"
		end
		if type(result.probe) == "table" and type(result.probe.message) == "string" then
			result.message = result.message .. "; " .. result.probe.message
		end
	end

	return result
end


local function _print_text(result)
	if type(result) ~= "table" then
		fmt.print_kv("proxy", "check")
		return
	end

	fmt.print_kv("ok", tostring(result.ok == true))
	fmt.print_kv("message", tostring(result.message or ""))
	local listen = result.listen or {}
	fmt.print_kv("listen", string.format("%s:%s", tostring(listen.host), tostring(listen.port)))
	fmt.print_kv("active_chain", tostring(result.active_chain or ""))
	fmt.print_kv("downstream", tostring(result.downstream == true))

	local ensures = result.ensure or {}
	for i = 1, #ensures do
		local e = ensures[i]
		local info = e.info or {}
		if type(info) == "table" then
			print(string.format(
				"  ensure[%s] ok=%s action=%s alua_ssid=%s scheme=%s",
				tostring(e.index),
				tostring(e.ok),
				tostring(info.action or ""),
				tostring(info.alua_ssid or info.client_id or ""),
				tostring(info.scheme or "")
			))
		else
			print(string.format("  ensure[%s] ok=%s info=%s", tostring(e.index), tostring(e.ok), tostring(info)))
		end
	end

	local probe = result.probe
	if type(probe) == "table" then
		fmt.print_kv("probe.ok", tostring(probe.ok == true))
		fmt.print_kv("probe.message", tostring(probe.message or ""))
		local hops = probe.hops or {}
		for i = 1, #hops do
			local h = hops[i]
			print(string.format(
				"  hop[%s] %s %s:%s ok=%s grade=%s latency_ms=%s err=%s",
				tostring(h.index or i),
				tostring(h.type or ""),
				tostring(h.host or ""),
				tostring(h.port or ""),
				tostring(h.ok),
				tostring(h.grade or ""),
				tostring(h.latency_ms or ""),
				tostring(h.err or "")
			))
		end
	end

	local lp = result.listen_probe
	if type(lp) == "table" then
		fmt.print_kv("listen_probe.ok", tostring(lp.ok == true))
		fmt.print_kv("listen_probe.message", tostring(lp.message or ""))
	end
end


M.run = function (...)
	local args = { ... }
	local sub = args[1]
	if sub == nil or sub == "-h" or sub == "--help" or sub == "help" then
		M.help()
		return true, 0
	end

	local rest = {}
	for i = 2, #args do
		rest[#rest + 1] = args[i]
	end

	local kv = _kv_args(rest)
	-- 允许: alua proxy check --json  或  alua proxy --json check
	if sub == "--json" or sub == "json" or sub == "-j" then
		kv._json = true
		sub = rest[1]
		local rest2 = {}
		for i = 2, #rest do
			rest2[#rest2 + 1] = rest[i]
		end
		local kv2 = _kv_args(rest2)
		for k, v in pairs(kv2) do
			kv[k] = v
		end
	end

	local want_json = kv._json == true

	if sub ~= "check" then
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "proxy: unknown subcommand (try: check)"))
		return false, codes.cli.BAD_REQUEST
	end

	local ok_r, result_or_err = pcall(_do_check, {
		force = kv.force == true,
	})
	if not ok_r then
		fmt.print_json(codes.make_error(codes.cli.TOOL_FAILED, "proxy check: " .. tostring(result_or_err)))
		return false, codes.cli.TOOL_FAILED
	end

	local result = result_or_err
	if want_json then
		fmt.print_json({ ok = result.ok == true, result = result })
	else
		_print_text(result)
	end

	if result.ok == true then
		return true, codes.OK
	end

	return false, codes.cli.TOOL_FAILED
end


return M

-- end
