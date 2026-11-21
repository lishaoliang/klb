--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   bench.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy 全链路测速 (分阶段 phases; 多轮汇总)
-- @note   主路径 smp/mnp/smps/mnps; 其它 hop 记 connect/total; 可选直连对照
-- @history 修改历史
--  \n [2026-10] 创建文件 (M1 / Todo.2026-44); _now_ms 改 ktime 墙钟; 轮间 wait 补池使后续轮 hit (Bug.2026-13)
--]]
local chain = require("klbagent.packages.proxy.chain")
local hop_token_ensure = require("klbagent.packages.proxy.hop_token")
local config = require("klbagent.packages.proxy.config")
local hop_pool = require("klbagent.packages.proxy.transport.hop_pool")
local smp_client = require("klbagent.packages.proxy.transport.smp.client")
local kproxy_io = require("klbcore.klbproxy.io")


local M = {}


-- @brief 墙钟毫秒; ktime 直读系统滴答 (kenv 是每轮 loop 缓存, 同轮差值为 0)
local function _now_ms()
	local ok_t, ktime = pcall(require, "ktime")
	if ok_t and type(ktime) == "table" and type(ktime.tick_count) == "function" then
		return ktime.tick_count()
	end

	local ok_e, kenv = pcall(require, "kenv")
	if ok_e and type(kenv) == "table" and type(kenv.tick_count) == "function" then
		return kenv.tick_count()
	end

	return os.time() * 1000
end


local function _hop_type(hop)
	if type(hop) ~= "table" then
		return nil
	end

	local t = hop.type or hop.kind
	if type(t) ~= "string" then
		return nil
	end

	t = string.lower(t)
	if t == "socks" or t == "socks5h" then
		return "socks5"
	end

	if t == "http" or t == "http_connect" or t == "https" then
		return "http-connect"
	end

	return t
end


local function _phase_add(phases, name, ms)
	if type(phases) ~= "table" or type(name) ~= "string" then
		return
	end

	local n = tonumber(ms)
	if n == nil then
		return
	end

	if n < 0 then
		n = 0
	end

	phases[#phases + 1] = { name = name, ms = n }
	if type(phases.map) ~= "table" then
		phases.map = {}
	end

	phases.map[name] = n
end


local function _close_stream(stream)
	if type(stream) ~= "table" then
		return
	end

	if type(stream.close) == "function" then
		pcall(function ()
			stream:close()
		end)
	elseif type(stream.tcp) == "table" and type(stream.tcp.close) == "function" then
		pcall(function ()
			stream.tcp:close()
		end)
	end
end


local function _hop_opts(hop)
	local htype = _hop_type(hop)
	local hop_opts = {
		kind = (htype == "mnp" or htype == "mnps") and "mnp" or "smp",
		tls = (htype == "smps" or htype == "mnps") or (hop.tls == true),
		smps = (htype == "smps"),
		mnps = (htype == "mnps"),
	}

	if hop.crypto ~= nil then
		hop_opts.crypto = hop.crypto
	end

	if type(hop.auth) == "table" then
		hop_opts.auth = hop.auth
	end

	local tok = nil
	if type(chain.hop_token) == "function" then
		tok = chain.hop_token(hop)
	elseif type(hop.token) == "string" then
		tok = hop.token
	end

	if type(tok) == "string" then
		hop_opts.token = tok
		-- 证明票须同送 alua_ssid (2026-11 ssid 契约); 漏送 → 对端 need alua_ssid
		hop_opts.alua_ssid = hop_token_ensure.hop_ssid(hop)
	end

	if hop.key ~= nil then
		hop_opts.key = hop.key
	end

	return hop_opts
end


-- @brief 单跳 timed 开隧道 (须在 kco 协程内)
-- @return [table] round report
local function _co_bench_one_hop(hop, target, opts)
	local report = {
		ok = false,
		type = _hop_type(hop),
		host = hop and hop.host,
		port = hop and tonumber(hop.port),
		pool = "n/a",
		phases = { map = {} },
		err = nil,
		total_ms = nil,
	}

	local t0 = _now_ms()
	local htype = report.type
	local dest_host = target.host
	local dest_port = tonumber(target.port)
	local timeout_ms = 15000
	if type(opts) == "table" and opts.timeout_ms ~= nil then
		timeout_ms = tonumber(opts.timeout_ms) or timeout_ms
	end

	if htype == "smp" or htype == "mnp" or htype == "smps" or htype == "mnps" then
		local hop_opts = _hop_opts(hop)
		hop_opts.timeout_ms = timeout_ms
		local pool_key = hop_pool.key(htype, hop, hop_opts)
		local warm = hop_pool.take(pool_key)
		local phases = report.phases
		local ok_t, stream

		if warm ~= nil then
			report.pool = "hit"
			_phase_add(phases, "pool", 0)
			_phase_add(phases, "resolve", 0)
			_phase_add(phases, "connect", 0)
			_phase_add(phases, "auth", 0)
			ok_t, stream, phases = smp_client.co_tunnel_timed(
				warm, dest_host, dest_port, hop_opts, phases)
			report.phases = phases
			if not ok_t then
				-- 暖连接失效: 剔同 key 池后再冷建
				if type(warm.close) == "function" then
					pcall(warm.close)
				end
				hop_pool.drop_key(pool_key)
				report.pool = "hit_fail_retry"
				phases = { map = {} }
				report.phases = phases
				_phase_add(phases, "pool", 0)
				ok_t, stream, phases = smp_client.co_connect_timed(
					hop.host, tonumber(hop.port), dest_host, dest_port, hop_opts)
				report.phases = phases
			end
		else
			report.pool = "miss"
			_phase_add(phases, "pool", 0)
			ok_t, stream, phases = smp_client.co_connect_timed(
				hop.host, tonumber(hop.port), dest_host, dest_port, hop_opts)
			report.phases = phases
		end

		hop_pool.note_activity()
		local host_c = hop.host
		local port_c = tonumber(hop.port)
		local opts_c = hop_opts
		hop_pool.ensure_async(pool_key, function ()
			return smp_client.open_session(host_c, port_c, opts_c)
		end)

		_phase_add(report.phases, "total", _now_ms() - t0)
		report.total_ms = report.phases.map.total
		if not ok_t then
			report.err = tostring(stream)
			return report
		end

		_close_stream(stream)
		report.ok = true

		-- 轮间等补池: 连测单协程不主动 yield, 须显式让 refill 协程完成 (Bug.2026-13)
		local wait_ms = 0
		if type(opts) == "table" and opts.wait_refill_ms ~= nil then
			wait_ms = tonumber(opts.wait_refill_ms) or 0
		end
		if wait_ms > 0 then
			report.pool_wait = hop_pool.wait(pool_key, wait_ms)
		end

		return report
	end

	-- 其它 hop: open_tunnel 整段记 connect+total
	local ok_o, sess = chain.open_tunnel(target, { hop }, {
		timeout_ms = timeout_ms,
	})
	_phase_add(report.phases, "connect", _now_ms() - t0)
	_phase_add(report.phases, "total", _now_ms() - t0)
	report.total_ms = report.phases.map.total
	if not ok_o then
		report.err = tostring(sess)
		return report
	end

	_close_stream(sess)
	report.ok = true
	return report
end


-- @brief 直连对照 (仅 TCP)
local function _co_bench_direct(target, opts)
	local report = {
		ok = false,
		type = "direct",
		host = target.host,
		port = tonumber(target.port),
		pool = "n/a",
		phases = { map = {} },
		err = nil,
		total_ms = nil,
	}

	local timeout_ms = 5000
	if type(opts) == "table" and opts.timeout_ms ~= nil then
		timeout_ms = tonumber(opts.timeout_ms) or timeout_ms
	end

	local t0 = _now_ms()
	local tcp, err_c = kproxy_io.tcp_connect(
		target.host,
		tonumber(target.port),
		timeout_ms,
		{ async = false }
	)
	local ms = _now_ms() - t0
	_phase_add(report.phases, "connect", ms)
	_phase_add(report.phases, "total", ms)
	report.total_ms = ms
	if tcp == nil then
		report.err = err_c or "direct connect fail"
		return report
	end

	if type(tcp.close) == "function" then
		pcall(function ()
			tcp:close()
		end)
	end

	report.ok = true
	return report
end


local function _run_in_co(timeout_ms, fn)
	local co_util = require("klbagent.core.co_util")
	return co_util.co_run(timeout_ms or 60000, fn, "bench")
end


local function _resolve_hops(args, host_hint)
	if type(args) == "table" and type(args.hops) == "table" and #args.hops > 0 then
		return args.hops, "args"
	end

	if type(args) == "table" and args.proxy ~= nil then
		local ok_u, util = pcall(require, "klbagent.core.tools.net.util")
		if ok_u and type(util) == "table" and type(util.proxy_normalize) == "function" then
			local proxy = util.proxy_normalize(args.proxy)
			if proxy ~= nil and type(chain.hops_from_proxy) == "function" then
				return chain.hops_from_proxy(proxy), "args.proxy"
			end
		end
	end

	local cfg = nil
	local ok_c, cproxy = pcall(require, "klbagent.core.config.proxy")
	if ok_c and type(cproxy) == "table" and type(cproxy.load) == "function" then
		cfg = cproxy.load()
	end

	if type(cfg) ~= "table" then
		cfg = config.defaults()
	end

	local hint = host_hint
	if type(hint) ~= "string" or hint == "" then
		hint = "example.com"
	end

	if type(config.select_hops) == "function" then
		local hops, info = config.select_hops(cfg, hint, 443)
		if hops ~= nil and type(hops) == "table" and #hops > 0 then
			return hops, (info and info.via) or "config"
		end
	end

	if type(config.chain_hops) == "function" then
		local id = config.active_chain_id(cfg)
		if id ~= nil then
			local hops = config.chain_hops(cfg, id)
			if hops ~= nil and type(hops) == "table" and #hops > 0 then
				return hops, "config.default"
			end
		end
	end

	return nil, nil, "no hops (pass hops/proxy or configure proxy.json chains)"
end


local function _summarize(totals)
	local n = #totals
	if n == 0 then
		return { count = 0 }
	end

	local min_v = totals[1]
	local max_v = totals[1]
	local sum = 0
	for i = 1, n do
		local v = totals[i]
		sum = sum + v
		if v < min_v then
			min_v = v
		end

		if v > max_v then
			max_v = v
		end
	end

	return {
		count = n,
		min_ms = min_v,
		max_ms = max_v,
		avg_ms = math.floor(sum / n + 0.5),
	}
end


local function _format_text(out)
	local lines = {}
	lines[#lines + 1] = string.format(
		"proxy.bench target=%s:%s rounds=%d compare_direct=%s",
		tostring(out.target and out.target.host),
		tostring(out.target and out.target.port),
		tonumber(out.rounds) or 0,
		tostring(out.compare_direct == true)
	)

	for i = 1, #(out.rounds_detail or {}) do
		local r = out.rounds_detail[i]
		local ph = {}
		if type(r.phases) == "table" then
			for j = 1, #r.phases do
				local p = r.phases[j]
				if type(p) == "table" then
					ph[#ph + 1] = string.format("%s=%s", tostring(p.name), tostring(p.ms))
				end
			end
		end

		lines[#lines + 1] = string.format(
			"round=%d ok=%s pool=%s total=%sms hop=%s:%s %s err=%s",
			i,
			tostring(r.ok),
			tostring(r.pool),
			tostring(r.total_ms),
			tostring(r.host),
			tostring(r.port),
			table.concat(ph, " "),
			tostring(r.err or "")
		)
	end

	if type(out.summary) == "table" then
		lines[#lines + 1] = string.format(
			"summary hop: count=%s min=%s avg=%s max=%s pool_hit=%s/%s",
			tostring(out.summary.count),
			tostring(out.summary.min_ms),
			tostring(out.summary.avg_ms),
			tostring(out.summary.max_ms),
			tostring(out.pool_hit or 0),
			tostring(out.pool_rounds or 0)
		)
	end

	if type(out.direct) == "table" and type(out.direct.summary) == "table" then
		local d = out.direct.summary
		lines[#lines + 1] = string.format(
			"summary direct: count=%s min=%s avg=%s max=%s",
			tostring(d.count),
			tostring(d.min_ms),
			tostring(d.avg_ms),
			tostring(d.max_ms)
		)
	end

	if out.message then
		lines[#lines + 1] = "message: " .. tostring(out.message)
	end

	return table.concat(lines, "\n")
end


-- @brief 跑测速
-- @param [in] args[table]
--   target={host,port} | host/port
--   hops / proxy
--   rounds=number (default 3)
--   timeout_ms
--   compare_direct=boolean
--   text=boolean 附带 text 字段
-- @return [table] report
M.run = function (args)
	args = args or {}
	local out = {
		ok = false,
		stub = false,
		target = nil,
		rounds = 0,
		rounds_detail = {},
		summary = nil,
		pool_hit = 0,
		pool_rounds = 0,
		compare_direct = args.compare_direct == true,
		direct = nil,
		hop_pool = nil,
		message = nil,
		text = nil,
	}

	local th = args.target
	local host = nil
	local port = nil
	if type(th) == "table" then
		host = th.host
		port = th.port
	end

	if host == nil then
		host = args.host
	end

	if port == nil then
		port = args.port
	end

	if type(host) ~= "string" or host == "" then
		out.message = "need target.host (or host=)"
		out.text = out.message
		return out
	end

	port = tonumber(port) or 80
	out.target = { host = host, port = port }

	local hops, source, err_h = _resolve_hops(args, host)
	if hops == nil then
		out.message = err_h or "no hops"
		out.text = out.message
		return out
	end

	out.hops_source = source
	out.hops = hops

	local rounds = tonumber(args.rounds) or 3
	if rounds < 1 then
		rounds = 1
	elseif rounds > 32 then
		rounds = 32
	end

	out.rounds = rounds
	local per_ms = tonumber(args.timeout_ms) or 15000
	local wall = per_ms * rounds * (#hops + 2) + 5000

	local ok_r, err_r = _run_in_co(wall, function ()
		local totals = {}
		local hit = 0
		local pool_n = 0

		for i = 1, rounds do
			-- 多跳: 逐跳测末跳到 target; 单跳最常见
			local hop = hops[#hops]
			local rep = _co_bench_one_hop(hop, out.target, {
				timeout_ms = per_ms,
				wait_refill_ms = (i < rounds) and 5000 or 0,
			})
			rep.round = i
			out.rounds_detail[i] = rep
			if rep.ok and type(rep.total_ms) == "number" then
				totals[#totals + 1] = rep.total_ms
			end

			if rep.pool == "hit" or rep.pool == "hit_fail_retry" then
				pool_n = pool_n + 1
				if rep.pool == "hit" and rep.ok then
					hit = hit + 1
				end
			elseif rep.pool == "miss" then
				pool_n = pool_n + 1
			end
		end

		out.summary = _summarize(totals)
		out.pool_hit = hit
		out.pool_rounds = pool_n
		out.ok = (#totals == rounds)

		if out.compare_direct then
			local d_totals = {}
			local d_detail = {}
			for i = 1, rounds do
				local dr = _co_bench_direct(out.target, { timeout_ms = per_ms })
				dr.round = i
				d_detail[i] = dr
				if dr.ok and type(dr.total_ms) == "number" then
					d_totals[#d_totals + 1] = dr.total_ms
				end
			end

			out.direct = {
				rounds_detail = d_detail,
				summary = _summarize(d_totals),
			}
		end

		if type(hop_pool.stats) == "function" then
			out.hop_pool = hop_pool.stats()
		end

		return true
	end)

	if not ok_r then
		out.ok = false
		out.message = tostring(err_r)
	elseif out.ok then
		out.message = string.format(
			"bench ok rounds=%d avg=%sms",
			rounds,
			tostring(out.summary and out.summary.avg_ms)
		)
	else
		out.message = "bench partial fail (see rounds_detail)"
	end

	if args.text ~= false then
		out.text = _format_text(out)
	end

	return out
end


-- @brief 仅格式化已有报告
M.format_text = _format_text


return M

-- end
