--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   chain.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  代理链引擎 (单跳/多跳 socks5|http-connect)
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 单跳 SOCKS5; 单跳 HTTP CONNECT; hops_from_proxy / resolve 简写; 多跳 A→B…→目标; P2-2 probe_hop / probe_hops 健康检查; P3-3 alua-tunnel hop 桩; 空 hops / type=direct 直连; session.upgrade_tls (https over tunnel); N5 P1: hop type=ws/wss (探活 + open_tunnel); N5 P2/P3: hop type=smp/mnp/smps/mnps; 直连/探活经 kproxy_io (co_*); 探活改调 klbsocks5 / http_connect 库 API; hop.crypto 透传 (与 TLS 正交); hop.token_ref / 缺省回落 serve.env (命令勿传原始 token); _now_ms 回退改 ktime 墙钟 (探活 latency 不再恒 0; Bug.2026-10)
--]]


local ws_client = require("klbagent.packages.proxy.transport.ws.client")
local smp_client = require("klbagent.packages.proxy.transport.smp.client")
local hop_pool = require("klbagent.packages.proxy.transport.hop_pool")
local hop_token_ensure = require("klbagent.packages.proxy.hop_token")
local klbsocks5 = require("klbcore.klbsocks5")
local http_connect_client = require("klbcore.klbhttp.proxy_connect.client")
local tunnel_client = require("klbcore.klbtunnel.client")
local kproxy_io = require("klbcore.klbproxy.io")


local M = {}


local MAX_HOPS = 8


local function _hop_type(hop)
	if type(hop) ~= "table" then
		return nil
	end

	local t = hop.type or hop.kind
	if type(t) == "string" then
		t = string.lower(t)
		if t == "socks" or t == "socks5h" then
			return "socks5"
		end
		if t == "http" or t == "http_connect" or t == "https" then
			return "http-connect"
		end
		if t == "tunnel" or t == "alua_tunnel" or t == "alua-tunnel" then
			return "alua-tunnel"
		end
		if t == "direct" or t == "none" or t == "off" then
			return "direct"
		end
		if t == "websocket" then
			return "ws"
		end
		if t == "websocket-secure" or t == "ws+tls" then
			return "wss"
		end
		return t
	end

	return nil
end


local function _hop_auth(hop)
	if type(hop) ~= "table" then
		return nil
	end

	if type(hop.auth) == "table" then
		return hop.auth
	end

	if hop.user ~= nil or hop.pass ~= nil then
		return { user = hop.user, pass = hop.pass }
	end

	return nil
end


-- @brief hop 鉴权材料: 显式 token(排障) > ssid 证明票 (hop_token)
-- \n 供 mnp/smp/ws/tunnel; 对外废除 hop.token_ref=file; 票由本机 raw 派生 (进程内缓存)
-- \n 热路径禁止每次建链同步 peer 校验 (peer_tool 内嵌套 kenv.loop_once → SEGV)
local function HopToken(hop)
	if type(hop) ~= "table" then
		return nil
	end

	local tok = hop_token_ensure.hop_token(hop)
	if type(tok) == "string" and tok ~= "" then
		return tok
	end

	return nil
end


-- @brief 将单跳 proxy 表转为 hops[] (ALUA_PROXY / args.proxy 简写)
-- @param [in]  proxy[table|nil]	`{ host, port, tls?, type?, auth? }`
-- @return [table|nil] hops; proxy 为 nil 则 nil
M.hops_from_proxy = function (proxy)
	if proxy == nil then
		return nil
	end

	if type(proxy) ~= "table" then
		return nil
	end

	if type(proxy.host) ~= "string" or proxy.host == "" then
		return nil
	end

	if tonumber(proxy.port) == nil then
		return nil
	end

	local htype = proxy.type or proxy.kind
	if type(htype) == "string" then
		htype = string.lower(htype)
	else
		htype = nil
	end

	if htype == "socks5" or htype == "socks" or htype == "socks5h" then
		return {
			{
				type = "socks5",
				host = proxy.host,
				port = tonumber(proxy.port),
				auth = proxy.auth,
			},
		}
	end

	-- 默认: HTTP CONNECT 正向代理 (含 https 代理侧)
	return {
		{
			type = "http-connect",
			host = proxy.host,
			port = tonumber(proxy.port),
			tls = proxy.tls == true,
			auth = proxy.auth,
		},
	}
end


local function _session_from_conn(conn, kind, hops_n)
	local session = {
		kind = kind,
		hops = hops_n,
		conn = conn,
	}

	session.close = function ()
		if session.conn ~= nil and type(session.conn.close) == "function" then
			session.conn.close()
		end
		session.conn = nil
	end

	session.send = function (_, data)
		if session.conn == nil then
			return false, "closed"
		end
		if session.conn.tcp ~= nil then
			return kproxy_io.send(session.conn.tcp, data)
		end
		return session.conn:send(data)
	end

	session.recv = function (_, max_n)
		if session.conn == nil then
			return nil, "closed"
		end
		if session.conn.tcp ~= nil then
			return kproxy_io.recv(session.conn.tcp, max_n or 4096)
		end
		return session.conn:recv(max_n)
	end

	-- @brief 隧道就地 TLS (https over socks/CONNECT); 优先 co_upgrade_tls
	-- @param hostname_or_opts[string|table|nil]
	session.upgrade_tls = function (_, hostname_or_opts, timeout_ms)
		if session.conn == nil then
			return false, "closed"
		end

		local tcp = session.conn.tcp
		if tcp == nil then
			return false, "upgrade_tls unavailable"
		end

		return kproxy_io.upgrade_tls(tcp, hostname_or_opts, timeout_ms)
	end

	return session
end


local function _copy_opts(opts)
	local out = {}
	if type(opts) == "table" then
		for k, v in pairs(opts) do
			if k ~= "tcp" and k ~= "auth" then
				out[k] = v
			end
		end
	end
	return out
end


-- @brief 直连 TCP 封装为 conn
-- @return [boolean] ok
-- @return [table|string] conn 或 err
local function _direct_connect(next_host, next_port, reuse_tcp, opts)
	if reuse_tcp ~= nil then
		return false, "direct hop cannot follow other hops"
	end

	if type(next_host) ~= "string" or next_host == "" then
		return false, "bad dest host"
	end

	if tonumber(next_port) == nil then
		return false, "bad dest port"
	end

	local timeout_ms = 15000
	if type(opts) == "table" and opts.timeout_ms ~= nil then
		timeout_ms = tonumber(opts.timeout_ms) or timeout_ms
	end

	local tcp, err_c = kproxy_io.tcp_connect(next_host, tonumber(next_port), timeout_ms, opts)
	if tcp == nil then
		return false, err_c or "connect fail"
	end

	local conn = {
		kind = "direct",
		dest_host = next_host,
		dest_port = tonumber(next_port),
		tcp = tcp,
	}

	conn.close = function ()
		if conn.tcp ~= nil and type(conn.tcp.close) == "function" then
			pcall(function ()
				conn.tcp:close()
			end)
		end
		conn.tcp = nil
	end

	conn.send = function (_, data)
		if conn.tcp == nil then
			return false, "closed"
		end
		return kproxy_io.send(conn.tcp, data)
	end

	conn.recv = function (_, max_n)
		if conn.tcp == nil then
			return nil, "closed"
		end
		return kproxy_io.recv(conn.tcp, max_n or 4096)
	end

	return true, conn
end


-- @brief 解析 hop[i] 应连接的下一端点 (跳过后续 direct)
-- @return host, port, err
local function _resolve_next_endpoint(hops, i, target)
	for j = i + 1, #hops do
		local nh = hops[j]
		if _hop_type(nh) ~= "direct" then
			if type(nh) ~= "table" or type(nh.host) ~= "string" or nh.host == "" then
				return nil, nil, "bad next hop.host"
			end
			if tonumber(nh.port) == nil then
				return nil, nil, "bad next hop.port"
			end
			return nh.host, tonumber(nh.port), nil
		end
	end

	if type(target) ~= "table" or type(target.host) ~= "string" or target.host == "" then
		return nil, nil, "bad target"
	end
	if tonumber(target.port) == nil then
		return nil, nil, "bad target.port"
	end

	return target.host, tonumber(target.port), nil
end


-- @brief 在一跳上握手到 next_host:next_port
-- @return [boolean] ok
-- @return [table|string] conn 或 err
local function _hop_connect(hop, next_host, next_port, reuse_tcp, opts)
	local htype = _hop_type(hop)
	if htype ~= "socks5" and htype ~= "http-connect" and htype ~= "alua-tunnel"
		and htype ~= "direct" and htype ~= "ws" and htype ~= "wss"
		and htype ~= "smp" and htype ~= "mnp" and htype ~= "smps" and htype ~= "mnps" then
		return false, "unsupported hop type"
	end

	if htype == "direct" then
		return _direct_connect(next_host, next_port, reuse_tcp, opts)
	end

	if type(hop.host) ~= "string" or hop.host == "" then
		return false, "bad hop.host"
	end

	if tonumber(hop.port) == nil then
		return false, "bad hop.port"
	end

	local auth = _hop_auth(hop)
	local hop_opts = _copy_opts(opts)
	if reuse_tcp ~= nil then
		hop_opts.tcp = reuse_tcp
	end

	if htype == "socks5" then
		local ok, conn, code_or_err = klbsocks5.connect(
			hop.host,
			tonumber(hop.port),
			next_host,
			tonumber(next_port),
			auth,
			hop_opts
		)
		if ok then
			return true, conn
		end

		return false, tostring(code_or_err or ("socks5 connect code=" .. tostring(conn)))
	end

	if htype == "alua-tunnel" then
		if auth ~= nil then
			hop_opts.auth = auth
		end
		if hop.key ~= nil then
			hop_opts.key = hop.key
		end
		local ok_t, conn_t, err_t = tunnel_client.connect(
			hop.host,
			tonumber(hop.port),
			next_host,
			tonumber(next_port),
			hop_opts
		)
		if ok_t then
			return true, conn_t
		end

		return false, err_t or tostring(conn_t)
	end

	if htype == "ws" or htype == "wss" then
		-- 多跳复用裸 tcp 与 WS 帧不兼容; 禁止 reuse
		if reuse_tcp ~= nil then
			return false, "ws/wss hop cannot reuse prior tcp"
		end
		hop_opts.tls = (htype == "wss") or (hop.tls == true)
		hop_opts.wss = hop_opts.tls
		if auth ~= nil then
			hop_opts.auth = auth
		end

		local tok_w = HopToken(hop)
		if type(tok_w) == "string" then
			hop_opts.token = tok_w
			hop_opts.alua_ssid = hop_token_ensure.hop_ssid(hop)
		end

		if hop.key ~= nil then
			hop_opts.key = hop.key
		end
		if type(hop.path) == "string" and hop.path ~= "" then
			hop_opts.path = hop.path
		end
		return ws_client.connect(
			hop.host,
			tonumber(hop.port),
			next_host,
			tonumber(next_port),
			hop_opts
		)
	end

	if htype == "smp" or htype == "mnp" or htype == "smps" or htype == "mnps" then
		if reuse_tcp ~= nil then
			return false, "smp/mnp hop cannot reuse prior tcp"
		end

		hop_opts.kind = (htype == "mnp" or htype == "mnps") and "mnp" or "smp"
		hop_opts.tls = (htype == "smps" or htype == "mnps") or (hop.tls == true)
		hop_opts.smps = (htype == "smps")
		hop_opts.mnps = (htype == "mnps")
		-- crypto 与 TLS 正交: true/false/table/string; 省略则 mnps/smps 默认开
		if hop.crypto ~= nil then
			hop_opts.crypto = hop.crypto
		end

		if auth ~= nil then
			hop_opts.auth = auth
		end

		local tok_m = HopToken(hop)
		if type(tok_m) == "string" then
			hop_opts.token = tok_m
			hop_opts.alua_ssid = hop_token_ensure.hop_ssid(hop)
		end

		if hop.key ~= nil then
			hop_opts.key = hop.key
		end

		-- 暖会话池 (鉴权长连); 死连接剔池; 隧道消耗后异步补槽; C Ping 保活
		local pool_key = hop_pool.key(htype, hop, hop_opts)
		local warm = hop_pool.take(pool_key)
		local ok_t, stream
		if warm ~= nil then
			ok_t, stream = smp_client.co_tunnel(warm, next_host, tonumber(next_port), hop_opts)
			if not ok_t then
				-- 暖连接失效: 关会话 + 剔同 key 残留, 再冷建
				if type(warm.close) == "function" then
					pcall(warm.close)
				end
				hop_pool.drop_key(pool_key)
				warm = nil
				ok_t, stream = smp_client.connect(
					hop.host,
					tonumber(hop.port),
					next_host,
					tonumber(next_port),
					hop_opts
				)
			end
		else
			ok_t, stream = smp_client.connect(
				hop.host,
				tonumber(hop.port),
				next_host,
				tonumber(next_port),
				hop_opts
			)
		end

		-- 瞬时断链 (auth early/timeout 等): 冷建再试一次
		if not ok_t and hop_token_ensure.auth_err_is_transient(stream) then
			ok_t, stream = smp_client.connect(
				hop.host,
				tonumber(hop.port),
				next_host,
				tonumber(next_port),
				hop_opts
			)
		end

		-- 鉴权拒 raw/过期/证明票不匹配: force ensure 后重试一次
		if not ok_t and hop_token_ensure.auth_err_needs_ensure(stream)
			and hop_token_ensure.client_id_of_hop(hop) ~= nil then
			pcall(hop_token_ensure.ensure_hop, hop, { force = true })
			local tok2 = HopToken(hop)
			if type(tok2) == "string" then
				hop_opts.token = tok2
				-- 重算票后 ssid 同步 (缺 alua_ssid 也会被判 auth deny)
				hop_opts.alua_ssid = hop_token_ensure.hop_ssid(hop)
			end
			ok_t, stream = smp_client.connect(
				hop.host,
				tonumber(hop.port),
				next_host,
				tonumber(next_port),
				hop_opts
			)
		end

		hop_pool.note_activity()
		local host_c = hop.host
		local port_c = tonumber(hop.port)
		local opts_c = hop_opts
		hop_pool.ensure_async(pool_key, function ()
			return smp_client.open_session(host_c, port_c, opts_c)
		end)

		return ok_t, stream
	end

	if auth ~= nil then
		hop_opts.auth = auth
	end
	if hop.tls == true then
		hop_opts.proxy_tls = true
	end

	return http_connect_client.connect(
		hop.host,
		tonumber(hop.port),
		next_host,
		tonumber(next_port),
		hop_opts
	)
end


-- @brief 经 hops 打开到目标的隧道 (支持多跳 / 直连)
-- @param [in]  target[table]	`{ host[string], port[number] }`
-- @param [in]  hops[table|nil]	hop 数组; nil/空/`type=direct` → 直连
-- @param [in]  opts[table|nil]	[可选] 传给底层 connect
-- @return [boolean] ok
-- @return [table|string] session 或 err
-- @note 第 i 跳隧道目标为下一非 direct 跳 host:port; 末跳目标为 target
M.open_tunnel = function (target, hops, opts)
	if type(target) ~= "table" or type(target.host) ~= "string" or target.host == "" then
		return false, "bad target"
	end

	if hops ~= nil and type(hops) ~= "table" then
		return false, "hops must be table"
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	if hops == nil or #hops == 0 then
		local ok_d, conn_d = _direct_connect(target.host, tonumber(target.port), nil, opts)
		if not ok_d then
			return false, conn_d
		end
		return true, _session_from_conn(conn_d, "direct", 0)
	end

	if #hops > MAX_HOPS then
		return false, string.format("too many hops (max %d)", MAX_HOPS)
	end

	local reuse_tcp = nil
	local last_conn = nil
	local last_kind = nil

	for i = 1, #hops do
		local hop = hops[i]
		local next_host, next_port, err_n = _resolve_next_endpoint(hops, i, target)
		if next_host == nil then
			if last_conn ~= nil and type(last_conn.close) == "function" then
				last_conn.close()
			end
			return false, string.format("hop[%d]: %s", i, tostring(err_n or "bad next"))
		end

		local ok, conn = _hop_connect(hop, next_host, next_port, reuse_tcp, opts)
		if not ok then
			-- _hop_connect 失败时已关闭 reuse_tcp/新建 tcp; 勿再关 last_conn (同一 tcp)
			return false, string.format("hop[%d]: %s", i, tostring(conn))
		end

		last_conn = conn
		-- 下跳复用端点: 裸 tcp 优先; mnps/ws 等无 tcp 时复用 conn 自身 (Bug.2026-1)
		if conn.tcp ~= nil then
			reuse_tcp = conn.tcp
		elseif type(conn.send) == "function" and type(conn.recv) == "function" then
			reuse_tcp = conn
		else
			reuse_tcp = nil
		end
		last_kind = _hop_type(hop)
	end

	local kind = "multi_hop_tunnel"
	if #hops == 1 then
		if last_kind == "socks5" then
			kind = "socks5_tunnel"
		elseif last_kind == "direct" then
			kind = "direct"
		elseif last_kind == "alua-tunnel" then
			kind = "alua_tunnel"
		elseif last_kind == "ws" then
			kind = "ws_tunnel"
		elseif last_kind == "wss" then
			kind = "wss_tunnel"
		elseif last_kind == "smp" then
			kind = "smp_tunnel"
		elseif last_kind == "mnp" then
			kind = "mnp_tunnel"
		elseif last_kind == "smps" then
			kind = "smps_tunnel"
		elseif last_kind == "mnps" then
			kind = "mnps_tunnel"
		else
			kind = "http_connect_tunnel"
		end
	elseif last_kind == "direct" and #hops >= 1 then
		-- 全 direct 或末跳 direct
		local all_direct = true
		for i = 1, #hops do
			if _hop_type(hops[i]) ~= "direct" then
				all_direct = false
				break
			end
		end
		if all_direct then
			kind = "direct"
		end
	end

	return true, _session_from_conn(last_conn, kind, #hops)
end


local function _close_tcp(tcp)
	if tcp ~= nil and type(tcp.close) == "function" then
		pcall(function ()
			tcp:close()
		end)
	end
end


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


-- @brief SOCKS5 探活 (库 API; 失败形状归一为 ok, errstr)
local function _probe_socks5(tcp, probe_target)
	local ok, code, err = klbsocks5.probe(tcp, probe_target)
	if ok then
		return true
	end

	return false, err or ("socks5 probe code=" .. tostring(code))
end


-- @brief HTTP CONNECT 探活 (库 API)
local function _probe_http_connect(tcp, probe_target)
	return http_connect_client.probe(tcp, probe_target)
end


-- @brief 探活单跳 (TCP + 协议问候; 可选 CONNECT 到 probe_target)
-- @param [in]  hop[table]		`{ type, host, port, auth? }`
-- @param [in]  opts[table|nil]	`{ timeout_ms, probe_target={host,port} }`
-- @return [boolean] ok
-- @return [table] report `{ index?, type, host, port, ok, err?, latency_ms }`
M.probe_hop = function (hop, opts)
	local report = {
		ok = false,
		type = nil,
		host = nil,
		port = nil,
		err = nil,
		latency_ms = nil,
	}

	if type(hop) ~= "table" then
		report.err = "bad hop"
		return false, report
	end

	local htype = _hop_type(hop)
	report.type = htype
	report.host = hop.host
	report.port = tonumber(hop.port)

	if htype ~= "socks5" and htype ~= "http-connect" and htype ~= "alua-tunnel"
		and htype ~= "direct" and htype ~= "ws" and htype ~= "wss"
		and htype ~= "smp" and htype ~= "mnp" and htype ~= "smps" and htype ~= "mnps" then
		report.err = "unsupported hop type"
		return false, report
	end

	if htype == "direct" then
		-- 无 host 的 direct: 仅标记能力; 有 host 则 TCP 探活
		if type(hop.host) ~= "string" or hop.host == "" or tonumber(hop.port) == nil then
			report.ok = true
			report.latency_ms = 0
			return true, report
		end
	elseif type(hop.host) ~= "string" or hop.host == "" then
		report.err = "bad hop.host"
		return false, report
	elseif tonumber(hop.port) == nil then
		report.err = "bad hop.port"
		return false, report
	end

	if opts ~= nil and type(opts) ~= "table" then
		report.err = "bad opts"
		return false, report
	end

	local timeout_ms = 5000
	local probe_target = nil
	local probe_targets = nil
	local latency_cfg = nil
	if opts ~= nil then
		if opts.timeout_ms ~= nil then
			timeout_ms = tonumber(opts.timeout_ms) or timeout_ms
		end
		if type(opts.probe_target) == "table" then
			probe_target = opts.probe_target
		end
		if type(opts.probe_targets) == "table" then
			probe_targets = opts.probe_targets
		end
		if type(opts.latency) == "table" then
			latency_cfg = opts.latency
		end
	end

	local function _grade(ms)
		local ok_ms = 500
		local slow_ms = 2000
		if latency_cfg ~= nil then
			ok_ms = tonumber(latency_cfg.ok_ms) or ok_ms
			slow_ms = tonumber(latency_cfg.slow_ms) or slow_ms
		end
		ms = tonumber(ms) or 0
		if ms <= ok_ms then
			return "ok"
		end
		if ms <= slow_ms then
			return "slow"
		end
		return "degraded"
	end

	-- ws/wss: WebSocket 升级探活 (默认 /alua-tunnel)
	if htype == "ws" or htype == "wss" then
		local t0 = _now_ms()
		local hop_opts = {
			timeout_ms = timeout_ms,
			tls = (htype == "wss") or (hop.tls == true),
			wss = (htype == "wss") or (hop.tls == true),
			path = hop.path,
			token = HopToken(hop),
			alua_ssid = hop_token_ensure.hop_ssid(hop),
			auth = _hop_auth(hop),
		}
		local ok_w, err_w = ws_client.handshake(hop.host, tonumber(hop.port), hop_opts)
		report.latency_ms = _now_ms() - t0
		if not ok_w then
			report.err = err_w or "ws handshake fail"
			report.grade = "fail"
			return false, report
		end
		report.ok = true
		report.grade = _grade(report.latency_ms)
		return true, report
	end

	-- smp/mnp/smps/mnps: TEXT auth 探活
	if htype == "smp" or htype == "mnp" or htype == "smps" or htype == "mnps" then
		local t0 = _now_ms()
		local hop_opts = {
			timeout_ms = timeout_ms,
			kind = (htype == "mnp" or htype == "mnps") and "mnp" or "smp",
			tls = (htype == "smps" or htype == "mnps") or (hop.tls == true),
			smps = (htype == "smps"),
			mnps = (htype == "mnps"),
			crypto = hop.crypto,
			token = HopToken(hop),
			alua_ssid = hop_token_ensure.hop_ssid(hop),
			auth = _hop_auth(hop),
			key = hop.key,
		}
		local ok_s, err_s = smp_client.handshake(hop.host, tonumber(hop.port), hop_opts)
		-- 瞬时断链先冷试一次
		if not ok_s and hop_token_ensure.auth_err_is_transient(err_s) then
			ok_s, err_s = smp_client.handshake(hop.host, tonumber(hop.port), hop_opts)
		end
		if not ok_s and hop_token_ensure.auth_err_needs_ensure(err_s)
			and hop_token_ensure.client_id_of_hop(hop) ~= nil then
			pcall(hop_token_ensure.ensure_hop, hop, { force = true })
			hop_opts.token = HopToken(hop)
			hop_opts.alua_ssid = hop_token_ensure.hop_ssid(hop)
			ok_s, err_s = smp_client.handshake(hop.host, tonumber(hop.port), hop_opts)
		end
		report.latency_ms = _now_ms() - t0
		if not ok_s then
			report.err = err_s or "smp handshake fail"
			report.grade = "fail"
			return false, report
		end

		report.ok = true
		report.grade = _grade(report.latency_ms)
		return true, report
	end

	-- 多 target: 任一成功即 ok; 无 target 时仍做协议问候/TCP
	local targets = {}
	if type(probe_targets) == "table" and #probe_targets > 0 then
		for i = 1, #probe_targets do
			local t = probe_targets[i]
			if type(t) == "table" and type(t.host) == "string" and t.host ~= "" then
				targets[#targets + 1] = {
					host = t.host,
					port = tonumber(t.port) or 80,
				}
			end
		end
	elseif type(probe_target) == "table" and type(probe_target.host) == "string" then
		targets[1] = {
			host = probe_target.host,
			port = tonumber(probe_target.port) or 80,
		}
	end

	if #targets == 0 then
		targets[1] = false -- 哨兵: 无 CONNECT 目标
	end

	report.targets = {}
	local any_ok = false
	local best_ms = nil
	local last_err = nil

	for ti = 1, #targets do
		local one_target = targets[ti]
		if one_target == false then
			one_target = nil
		end
		local t0 = _now_ms()
		local tcp, err_c = kproxy_io.tcp_connect(
			hop.host,
			tonumber(hop.port),
			timeout_ms,
			{ async = false }
		)
		local one = {
			host = one_target and one_target.host or nil,
			port = one_target and one_target.port or nil,
			ok = false,
			latency_ms = nil,
			grade = nil,
			err = nil,
		}
		if tcp == nil then
			one.err = err_c or "connect fail"
			one.latency_ms = _now_ms() - t0
			one.grade = "fail"
			last_err = one.err
			report.targets[#report.targets + 1] = one
		else
			local ok_p, err_p
			if htype == "socks5" then
				ok_p, err_p = _probe_socks5(tcp, one_target)
			elseif htype == "alua-tunnel" or htype == "direct" then
				ok_p, err_p = true, nil
			else
				ok_p, err_p = _probe_http_connect(tcp, one_target)
			end
			_close_tcp(tcp)
			one.latency_ms = _now_ms() - t0
			if not ok_p then
				one.err = err_p or "probe fail"
				one.grade = "fail"
				last_err = one.err
			else
				one.ok = true
				one.grade = _grade(one.latency_ms)
				any_ok = true
				if best_ms == nil or one.latency_ms < best_ms then
					best_ms = one.latency_ms
				end
			end
			report.targets[#report.targets + 1] = one
		end

		if any_ok and (opts == nil or opts.all_targets ~= true) then
			break
		end
	end

	report.latency_ms = best_ms
	if any_ok then
		report.ok = true
		report.grade = _grade(best_ms)
		return true, report
	end

	report.err = last_err or "probe fail"
	report.grade = "fail"
	return false, report
end


-- @brief 按序探活 hops[]; 遇失败仍继续 (汇总 failed_at)
-- @param [in]  hops[table]		hop 数组
-- @param [in]  opts[table|nil]	同 probe_hop
-- @return [boolean] ok 全部成功
-- @return [table] `{ ok, hops=[report...], failed_at[number|nil], message }`
M.probe_hops = function (hops, opts)
	local out = {
		ok = false,
		hops = {},
		failed_at = nil,
		message = nil,
	}

	if type(hops) ~= "table" or #hops == 0 then
		out.message = "need hops"
		return false, out
	end

	if #hops > MAX_HOPS then
		out.message = string.format("too many hops (max %d)", MAX_HOPS)
		return false, out
	end

	local all_ok = true
	for i = 1, #hops do
		local ok_h, rep = M.probe_hop(hops[i], opts)
		rep.index = i
		out.hops[i] = rep
		if not ok_h then
			all_ok = false
			if out.failed_at == nil then
				out.failed_at = i
			end
		end
	end

	out.ok = all_ok
	if all_ok then
		out.message = string.format("all %d hop(s) ok", #hops)
	else
		local fr = out.hops[out.failed_at]
		out.message = string.format(
			"hop[%d] %s:%s fail: %s",
			out.failed_at,
			tostring(fr and fr.host),
			tostring(fr and fr.port),
			tostring(fr and fr.err)
		)
	end

	return all_ok, out
end


-- @brief 导出 hop token 解析 (ssid 唯一使用入口; 显式 token 仅排障)
M.hop_token = HopToken


return M

-- end
