--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   proxy.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua status proxy — 代理运行面详情 (chains/listen/routes/stats/hop)
-- @note   nested 五级; --probe 可选 hop 探活; 禁 token 真值
-- @history 修改历史
--  \n [2026-10] 创建文件 (由 status.lua 拆分)
--]]


local version = require("klbagent.version")
local common = require("klbagent.core.cli.status.common")


local M = {}


-- @brief chains 详情表
-- @param [in] cfg[table|nil]
-- @param [in] summary[table]
-- @return list[table]
local function _build_chains_detail(cfg, summary)
	local chains_detail = {}
	if type(cfg) ~= "table" or type(cfg.chains) ~= "table" or type(summary) ~= "table" then
		return chains_detail
	end

	local names = summary.chain_names or common.chain_names(cfg.chains)
	for i = 1, #names do
		local cn = names[i]
		local hops_raw = cfg.chains[cn]
		local hops = {}
		if type(hops_raw) == "table" then
			for j = 1, #hops_raw do
				hops[#hops + 1] = common.hop_row(hops_raw[j])
			end
		end

		local roles = {}
		if cn == cfg.default then
			roles[#roles + 1] = "default"
		end
		if cn == cfg.debug then
			roles[#roles + 1] = "debug"
		end
		if cn == summary.active_chain then
			roles[#roles + 1] = "active"
		end

		chains_detail[#chains_detail + 1] = {
			name = cn,
			active = (cn == summary.active_chain),
			roles = roles,
			hops = hops,
			hop_summary = common.hops_one_line(hops_raw),
		}
	end

	return chains_detail
end


-- @brief routes 脱敏行
-- @param [in] cfg[table|nil]
-- @return list[table]
local function _routes_rows(cfg)
	local out = {}
	if type(cfg) ~= "table" or type(cfg.routes) ~= "table" then
		return out
	end

	for i = 1, #cfg.routes do
		local r = cfg.routes[i]
		if type(r) == "table" then
			local row = {
				match = tostring(r.match or "*"),
				chain = tostring(r.chain or "default"),
			}
			if type(r.allow_from) == "table" then
				row.allow_from = r.allow_from
			end
			out[#out + 1] = row
		end
	end

	return out
end


-- @brief listen 详情 (含 backlog / allow_from 摘要)
-- @param [in] cfg[table|nil]
-- @param [in] summary[table]
-- @return listen[table]
local function _listen_detail(cfg, summary)
	local base = (summary and summary.listen) or {
		host = "127.0.0.1",
		port = 12818,
		protocols = { "socks5", "http-connect", "alua-tunnel" },
	}
	local out = {
		host = base.host,
		port = base.port,
		protocols = base.protocols,
		ok = summary and summary.listen_ok == true,
		probe = summary and summary.listen_probe or nil,
	}
	if type(cfg) == "table" and type(cfg.listen) == "table" then
		local n = tonumber(cfg.listen.backlog)
		if type(n) == "number" then
			out.backlog = math.floor(n)
		end
		if cfg.listen.port_fallback == true then
			out.port_fallback = true
		end
	end

	if type(cfg) == "table" and type(cfg.allow_from) == "table" then
		out.allow_from = cfg.allow_from
	end

	if type(cfg) == "table" and type(cfg.auth) == "table" then
		out.auth_mode = tostring(cfg.auth.mode or "none")
	end

	return out
end


-- @brief 进程内 stats (入口未跑则为 0 + note)
-- @return stats[table]
local function _proxy_stats_snapshot()
	local ok_e, entry = pcall(require, "klbagent.packages.proxy.entry")
	if ok_e and type(entry) == "table" and type(entry.stats) == "function" then
		local ok_s, st = pcall(entry.stats)
		if ok_s and type(st) == "table" then
			local running = false
			if type(entry.is_running) == "function" then
				running = entry.is_running() == true
			end

			return {
				entry_running = running,
				accepts = tonumber(st.accepts) or 0,
				active = tonumber(st.active) or 0,
				closed = tonumber(st.closed) or 0,
				auth_fail = tonumber(st.auth_fail) or 0,
				reject = tonumber(st.reject) or 0,
				probe_noise = tonumber(st.probe_noise) or 0,
				bytes_c2u = tonumber(st.bytes_c2u) or 0,
				bytes_u2c = tonumber(st.bytes_u2c) or 0,
			}
		end
	end

	return {
		entry_running = false,
		accepts = 0,
		active = 0,
		closed = 0,
		auth_fail = 0,
		reject = 0,
		probe_noise = 0,
		bytes_c2u = 0,
		bytes_u2c = 0,
		note = "entry not in this process (status is CLI; stats live in proxy serve)",
	}
end


-- @brief 可选 hop 探活 (M3 --probe)
-- @param [in] hops_raw[table|nil]
-- @return probe[table|nil]
local function _optional_hop_probe(hops_raw)
	if type(hops_raw) ~= "table" or #hops_raw == 0 then
		return {
			ok = true,
			skipped = true,
			message = "no hops (direct)",
		}
	end

	local ok_c, chain = pcall(require, "klbagent.packages.proxy.chain")
	if not ok_c or type(chain) ~= "table" or type(chain.probe_hops) ~= "function" then
		return {
			ok = false,
			message = "probe unavailable",
		}
	end

	local ok_p, probe = pcall(chain.probe_hops, hops_raw, { timeout_ms = 3000 })
	if not ok_p then
		return {
			ok = false,
			message = tostring(probe),
		}
	end

	if type(probe) ~= "table" then
		return { ok = ok_p == true, message = "probe done" }
	end

	-- 脱敏: 去掉可能残留的 token
	local hops_out = {}
	if type(probe.hops) == "table" then
		for i = 1, #probe.hops do
			local h = probe.hops[i]
			if type(h) == "table" then
				hops_out[#hops_out + 1] = {
					ok = h.ok == true,
					grade = h.grade,
					ms = h.ms,
					message = h.message,
					type = h.type,
					host = h.host,
					port = h.port,
				}
			end
		end
	end

	return {
		ok = ok_p == true or probe.ok == true,
		message = probe.message,
		hops = hops_out,
	}
end


-- @brief proxy 模块详情 (nested: chains/listen/routes/stats/hop)
-- @param [in] path_rest[table]
-- @param [in] opts[table|nil]  `{ probe=boolean, pref=string }`
-- @return report[table]
M.collect = function (path_rest, opts)
	opts = opts or {}
	local pref = opts.pref or ""
	local summary = common.proxy_runtime_summary(pref)
	local cfg = common.load_proxy_cfg()
	local chains_detail = _build_chains_detail(cfg, summary)
	local routes = _routes_rows(cfg)
	local listen = _listen_detail(cfg, summary)
	local stats = _proxy_stats_snapshot()
	local level = nil
	if type(path_rest) == "table" and type(path_rest[1]) == "string" then
		level = string.lower(path_rest[1])
	end

	local hop_level = {
		active_chain = summary.active_chain,
		active_via = summary.active_via,
		hop_summary = summary.hop_summary,
		hops = summary.active_hops or {},
	}
	if opts.probe == true then
		local hops_raw = nil
		if type(cfg) == "table" and type(cfg.chains) == "table"
			and type(summary.active_chain) == "string" then
			hops_raw = cfg.chains[summary.active_chain]
		end

		hop_level.probe = _optional_hop_probe(hops_raw)
	end

	local function _base(lv, path)
		return {
			ok = true,
			command = "status",
			schema = common.SCHEMA,
			module = "proxy",
			level = lv,
			path = path,
			version = version.string,
			proxy = summary,
		}
	end

	if level == nil or level == "" then
		local rep = _base("root", { "proxy" })
		rep.chains = chains_detail
		rep.listen = listen
		rep.routes = routes
		rep.stats = stats
		rep.hop = hop_level
		rep.nested_levels = { "chains", "listen", "routes", "stats", "hop" }
		if opts.probe == true then
			rep.probe = hop_level.probe
		end

		return rep
	end

	if level == "chains" then
		local rep = _base("chains", { "proxy", "chains" })
		rep.chains = chains_detail
		return rep
	end

	if level == "listen" then
		local rep = _base("listen", { "proxy", "listen" })
		rep.listen = listen
		return rep
	end

	if level == "routes" then
		local rep = _base("routes", { "proxy", "routes" })
		rep.routes = routes
		local np = nil
		if type(cfg) == "table" then
			np = cfg.no_proxy
		end

		rep.no_proxy = np
		return rep
	end

	if level == "stats" then
		local rep = _base("stats", { "proxy", "stats" })
		rep.stats = stats
		return rep
	end

	if level == "hop" or level == "hops" then
		local rep = _base("hop", { "proxy", "hop" })
		rep.hop = hop_level
		if opts.probe == true then
			rep.probe = hop_level.probe
		end

		return rep
	end

	return {
		ok = false,
		command = "status",
		schema = common.SCHEMA,
		module = "proxy",
		level = level,
		path = { "proxy", level },
		version = version.string,
		message = "unknown status proxy level: " .. tostring(level),
		nested_levels = { "chains", "listen", "routes", "stats", "hop" },
	}
end


-- @brief 打印 chains 列表
-- @param [in] chains[table]
local function _print_chains_list(chains)
	if type(chains) ~= "table" or #chains == 0 then
		common.print_kv("chains", "(empty -> entry direct)")
		return
	end

	print("chains:")
	for i = 1, #chains do
		local c = chains[i]
		local mark = (c.active == true) and "*" or " "
		local role_s = ""
		if type(c.roles) == "table" and #c.roles > 0 then
			role_s = " [" .. table.concat(c.roles, ",") .. "]"
		end

		print(string.format("  %s %-12s hops=%d%s  %s",
			mark,
			tostring(c.name),
			#(c.hops or {}),
			role_s,
			tostring(c.hop_summary or "")))
	end
end


-- @brief 打印 proxy 模块详情
-- @param [in] report[table]
M.print_text = function (report)
	if report.ok == false then
		common.print_kv("error", report.message or "fail")
		return
	end

	local lv = tostring(report.level or "root")
	local px = report.proxy or {}

	if lv == "chains" then
		_print_chains_list(report.chains)
		return
	end

	if lv == "listen" then
		local listen = report.listen or {}
		common.print_kv("listen", string.format("%s:%s  ok=%s  probe=%s",
			tostring(listen.host or "?"),
			common.port_str(listen.port),
			common.yn(listen.ok == true),
			tostring(listen.probe or "")))
		if type(listen.protocols) == "table" then
			common.print_kv("protocols", table.concat(listen.protocols, ","))
		end

		common.print_kv("auth_mode", tostring(listen.auth_mode or "none"))
		if listen.backlog ~= nil then
			common.print_kv("backlog", tostring(listen.backlog))
		end

		if type(listen.allow_from) == "table" and #listen.allow_from > 0 then
			common.print_kv("allow_from", table.concat(listen.allow_from, ","))
		end

		return
	end

	if lv == "routes" then
		local routes = report.routes or {}
		if #routes == 0 then
			common.print_kv("routes", "(empty)")
		else
			print("routes:")
			for i = 1, #routes do
				local r = routes[i]
				print(string.format("  %-16s -> %s",
					tostring(r.match), tostring(r.chain)))
			end
		end

		if report.no_proxy ~= nil then
			local np = report.no_proxy
			if type(np) == "table" then
				common.print_kv("no_proxy", table.concat(np, ","))
			else
				common.print_kv("no_proxy", tostring(np))
			end
		end

		return
	end

	if lv == "stats" then
		local st = report.stats or {}
		common.print_kv("entry_running", common.yn(st.entry_running == true))
		common.print_kv("accepts", tostring(st.accepts or 0))
		common.print_kv("active", tostring(st.active or 0))
		common.print_kv("closed", tostring(st.closed or 0))
		common.print_kv("auth_fail", tostring(st.auth_fail or 0))
		common.print_kv("reject", tostring(st.reject or 0))
		common.print_kv("probe_noise", tostring(st.probe_noise or 0))
		common.print_kv("bytes", string.format("c2u=%s u2c=%s",
			tostring(st.bytes_c2u or 0), tostring(st.bytes_u2c or 0)))
		if type(st.note) == "string" and st.note ~= "" then
			common.print_kv("note", st.note)
		end

		return
	end

	if lv == "hop" then
		local hop = report.hop or {}
		common.print_kv("active", string.format("%s  via=%s",
			tostring(hop.active_chain or "(none)"),
			tostring(hop.active_via or "")))
		common.print_kv("hop", tostring(hop.hop_summary or "direct"))
		local hops = hop.hops or {}
		if #hops > 0 then
			print("hops:")
			for i = 1, #hops do
				local h = hops[i]
				print(string.format("  %d. %s %s:%s token=%s",
					i, tostring(h.type), tostring(h.host),
					common.port_str(h.port), common.yn(h.token_set == true)))
			end
		end

		local pr = report.probe or hop.probe
		if type(pr) == "table" then
			common.print_kv("probe", string.format("ok=%s  %s",
				common.yn(pr.ok == true), tostring(pr.message or "")))
		end

		return
	end

	-- root
	local listen = report.listen or (px.listen or {})
	common.print_kv("running", string.format("%s  listen=%s:%s  probe=%s",
		common.yn(px.running == true),
		tostring(listen.host or "?"),
		common.port_str(listen.port),
		tostring(px.listen_probe or listen.probe or "")))
	if type(listen.protocols) == "table" then
		common.print_kv("protocols", table.concat(listen.protocols, ","))
	end

	common.print_kv("pref", string.format("%s  file=%s",
		tostring(px.pref_path or ""),
		common.yn(px.file == true)))
	common.print_kv("roles", string.format("default=%s  debug=%s",
		tostring(px.default_role or "(none)"),
		tostring(px.debug_role or "(none)")))
	do
		local via = tostring(px.active_via or "")
		local live_mark = ""
		if via == "use" then
			live_mark = "  (proxy.use; entry restart->default)"
		end

		common.print_kv("active", string.format("%s  via=%s%s",
			tostring(px.active_chain or "(none)"),
			via,
			live_mark))
	end

	common.print_kv("hop", tostring(px.hop_summary or "direct"))
	_print_chains_list(report.chains)

	local routes = report.routes or {}
	common.print_kv("routes", tostring(#routes))
	local st = report.stats or {}
	common.print_kv("stats", string.format("accepts=%s active=%s (entry=%s)",
		tostring(st.accepts or 0),
		tostring(st.active or 0),
		common.yn(st.entry_running == true)))

	local env = px.env or {}
	common.print_kv("ALUA_PROXY", (env.configured == true)
		and ("set invalid=" .. common.yn(env.invalid == true))
		or "unset")
	if type(report.nested_levels) == "table" then
		common.print_kv("nested", table.concat(report.nested_levels, ","))
	end

	if type(report.probe) == "table" then
		common.print_kv("probe", string.format("ok=%s  %s",
			common.yn(report.probe.ok == true),
			tostring(report.probe.message or "")))
	end
end


return M

-- end
