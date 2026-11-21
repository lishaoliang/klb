--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   service.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy listen 实现 (SOCKS5/HTTP CONNECT/alua-tunnel)
-- @stability internal
-- @impl evolving
-- @note   包外勿直 require; 对外面见 entry/init.lua
--   \n alua pkg run proxy serve; 与 alua serve 分端口; 同口按 demux 首字节分流
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); SOCKS5 / HTTP CONNECT / 明文 HTTP 与 alua-tunnel/SMP/MNP 同口分流; 鉴权透传与按 routes/no_proxy 选链; co_accept/co_connect + pump duplex 中继; apply_live 热更链/路由; policy 拒连; 修 treq 空崩溃与上游复用 (Bug.2026-8/11)
--  \n [2026-11] 拆 start_embed/pump_embed/stop_embed; start 保留阻塞形态
--]]


local config = require("klbagent.packages.proxy.config")
local demux = require("klbagent.packages.proxy.relay.demux")
local chain = require("klbagent.packages.proxy.chain")
local stats = require("klbagent.packages.proxy.stats")
local acl = require("klbagent.packages.proxy.acl")
local pool = require("klbagent.packages.proxy.relay.pool")
local hop_pool = require("klbagent.packages.proxy.transport.hop_pool")
local smp_server = require("klbagent.packages.proxy.transport.smp.server")
local klbsocks5 = require("klbcore.klbsocks5")
local net = require("klbcore.klbcode.netcode")
local http_connect_server = require("klbcore.klbhttp.proxy_connect.server")
local tunnel_server = require("klbcore.klbtunnel.serve")
local tunnel_crypto = require("klbcore.klbtunnel.crypto")
local pump = require("klbagent.packages.proxy.relay.pump")
local kproxy_io = require("klbcore.klbproxy.io")


local M = {}


local _running = false
local _cfg = nil
-- kproxy 监听组: { { listener=, listen= }, ... }
local _listeners = {}
-- mnps/smps klblisten mux 组
local _mnps_muxes = {}
local _guard = nil
-- 入口并发: kco.fork + co_accept + pump.co_relay_duplex
local _coop = false
local _active = 0
-- 阻塞/宿主共用的 accept 参数与计数
local _accept_ms = 100
local _accept_need = 0
local _handled = 0


-- @param [in] src[table|nil]	listen 表, 或含 .listen 的 cfg
local function _has_proto(src, name)
	if type(src) == "table" and type(src.protocols) == "table" then
		return config.listen_has_proto(src, name)
	end

	if type(src) == "table" and type(src.listen) == "table" then
		return config.listen_has_proto(src.listen, name)
	end

	return config.listen_has_proto(src, name)
end


local function _close_listener_item(item)
	if type(item) ~= "table" then
		return
	end

	local listener = item.listener
	if listener ~= nil and type(listener.close) == "function" then
		pcall(function ()
			listener:close()
		end)
	end
end


local function _close_all_listeners()
	for i = 1, #_listeners do
		_close_listener_item(_listeners[i])
	end
	_listeners = {}
end


local function _close_all_mnps()
	for i = 1, #_mnps_muxes do
		local mux = _mnps_muxes[i]
		if mux ~= nil and type(mux.close) == "function" then
			pcall(function ()
				mux:close()
			end)
		end
	end
	_mnps_muxes = {}
end


local function _close_tcp(tcp)
	if tcp ~= nil and type(tcp.close) == "function" then
		pcall(function ()
			tcp:close()
		end)
	end
end


-- @return upstream[tcp|stream|nil], session[table|nil], err[string|nil]
local function _open_upstream(cfg, host, port)
	local hops, info = config.select_hops(cfg, host, port)
	if type(info) == "table" and info.via == "route_error" then
		return nil, nil, info.err or "route error"
	end

	if type(info) == "table" and info.reject == true then
		return nil, nil, info.err or "policy reject"
	end

	local has_hops = type(hops) == "table" and #hops > 0

	if has_hops then
		hop_pool.note_activity()
		local to = config.timeouts(cfg)
		local ok_t, sess = chain.open_tunnel({ host = host, port = port }, hops, {
			timeout_ms = to.connect_ms,
		})
		if not ok_t then
			return nil, nil, sess
		end

		if type(sess.conn) == "table" and sess.conn.tcp ~= nil then
			return sess.conn.tcp, sess, nil
		end

		-- mnps/ws/smp 等无裸 tcp: 用 conn 自身作 send/recv 上游 (Bug.2026-2)
		if type(sess.conn) == "table"
			and type(sess.conn.send) == "function"
			and type(sess.conn.recv) == "function" then
			return sess.conn, sess, nil
		end

		if type(sess.close) == "function" then
			sess.close()
		end

		return nil, nil, "chain tcp missing"
	end

	local to = config.timeouts(cfg)
	local tcp, err_c = kproxy_io.tcp_connect(host, port, to.connect_ms)
	if tcp == nil then
		return nil, nil, err_c or "connect fail"
	end

	return tcp, nil, nil
end


-- @brief mnps 流上按「整包 MNP binary = 一帧 tunnel」握手 (库 API)
-- @note TEXT alua.auth 已在 accept_session 完成; HELLO 不再二次验密
local function _mnps_serve_tunnel(stream)
	return tunnel_server.serve_conn_packet(stream)
end


-- @brief 归一 tunnel serve_conn / serve_conn_packet 返回
-- @note 成功 true,req; 失败 false,code,err — 勿把 req 解到第三返回值
-- @return ok[boolean]
-- @return req_or_err[table|string]
local function _tunnel_serve_result(ok, a, b)
	if ok then
		return true, a
	end

	if type(b) == "string" and b ~= "" then
		return false, b
	end

	return false, a
end


local function _is_alua_hop_session(session)
	if type(session) ~= "table" then
		return false
	end

	local k = session.kind
	if k == "mnp_tunnel" or k == "smp_tunnel"
		or k == "mnps_tunnel" or k == "smps_tunnel" then
		return true
	end

	local ck = nil
	if type(session.conn) == "table" then
		ck = session.conn.kind
	end
	if type(ck) == "string" then
		if string.find(ck, "mnp", 1, true) or string.find(ck, "smp", 1, true) then
			return true
		end
	end

	return false
end


local function _pump_opts(cfg, extra, session)
	local to = config.timeouts(cfg)
	local idle = to.idle_ms
	-- M2: hop 中继勿因短 idle 掐断; 对齐 hop_idle (C Ping 亦经 touch_idle)
	if _is_alua_hop_session(session) then
		local hop_idle = tonumber(to.hop_idle_ms) or 3600000
		if hop_idle > idle then
			idle = hop_idle
		end
	end

	local opts = {
		timeout_ms = to.poll_ms,
		idle_ms = idle,
	}
	if type(extra) == "table" then
		for k, v in pairs(extra) do
			opts[k] = v
		end
	end
	return opts
end


local function _pump_pair(a, b, opts)
	-- 正式路径: 有 kco 一律 co_relay_duplex (userdata 走 co_*; mnps 亦然)
	if _coop then
		return pump.co_relay_duplex(a, b, opts)
	end

	if pump.is_klb_stream(a) or pump.is_klb_stream(b) then
		local poll_ms = 40
		if type(opts) == "table" and opts.timeout_ms ~= nil then
			poll_ms = tonumber(opts.timeout_ms) or poll_ms
		end
		return pump.relay_poll(a, b, { timeout_ms = poll_ms })
	end

	return pump.relay(a, b, opts)
end


local function _relay_and_close(client, upstream, session, cfg, keep_client, keep_upstream, extra)
	-- 正式中继仅经 pump duplex/poll (勿默认阻塞 kproxy.relay)
	local ok_p, err_p = _pump_pair(client, upstream, _pump_opts(cfg, extra, session))
	_ = ok_p
	_ = err_p

	if not keep_client then
		_close_tcp(client)
	end

	if keep_upstream then
		-- 上游留给连接池 / 同连接复用
		return
	end

	_close_tcp(upstream)
	if session ~= nil and type(session.close) == "function" then
		session.close()
	end
end


local function _peer_gate(client)
	local ip = acl.peer_ip(client)
	if _guard == nil then
		return true, ip
	end
	local ok, err = _guard.check_peer(ip)
	if not ok then
		stats.on_reject()
		if _guard.quiet then
			_close_tcp(client)
		else
			_close_tcp(client)
		end
		return false, ip, err or "acl deny"
	end
	return true, ip
end


local function _route_allow(cfg, host, port, peer_ip)
	if type(cfg) ~= "table" or type(cfg.routes) ~= "table" then
		return true
	end
	for i = 1, #cfg.routes do
		local r = cfg.routes[i]
		if type(r) == "table" and type(r.allow_from) == "table" then
			-- 仅当 route 带 allow_from 且 match 命中目的地时收紧
			local match = r.match
			local hit = false
			if match == nil or match == "*" or match == "default" then
				hit = true
			elseif type(match) == "string" and type(host) == "string" then
				if match == host or match == (host .. ":" .. tostring(port)) then
					hit = true
				elseif string.sub(match, 1, 1) == "." and string.find(host, match, 1, true) then
					hit = true
				end
			end
			if hit and not acl.allow_from_ok(peer_ip, r.allow_from) then
				return false
			end
		end
	end
	return true
end


-- @brief 从上游 tcp/session 回读本地绑定 (SOCKS BND)
local function _bnd_from_upstream(upstream, session)
	local tcp = nil
	if type(upstream) == "userdata" and type(upstream.local_addr) == "function" then
		tcp = upstream
	elseif type(session) == "table" and type(session.conn) == "table"
		and type(session.conn.tcp) == "userdata"
		and type(session.conn.tcp.local_addr) == "function" then
		tcp = session.conn.tcp
	end

	if tcp == nil then
		return nil
	end

	local ok, ip, port = pcall(function ()
		return tcp:local_addr()
	end)
	if not ok or type(ip) ~= "string" or ip == "" then
		return nil
	end

	return { host = ip, port = tonumber(port) or 0 }
end


local function _handle_socks5(client, cfg, prefix, peer_ip)
	local ok_h, req, err_h = klbsocks5.serve_conn(client, { auth = cfg.auth, prefix = prefix })
	if not ok_h then
		local err_s = err_h or tostring(req)
		if type(err_s) == "string" and string.find(err_s, "auth", 1, true) then
			stats.on_auth_fail()
			if _guard ~= nil then
				_guard.note_auth_fail(peer_ip)
			end
		end
		_close_tcp(client)
		return false, err_s
	end

	if _guard ~= nil then
		_guard.note_auth_ok(peer_ip)
	end

	if not _route_allow(cfg, req.host, req.port, peer_ip) then
		stats.on_reject()
		klbsocks5.reply(client, 0x02)
		_close_tcp(client)
		return false, "route allow_from deny"
	end

	local upstream, session, err_u = _open_upstream(cfg, req.host, req.port)
	if upstream == nil then
		klbsocks5.reply(client, klbsocks5.rep_for_upstream_err(err_u))
		_close_tcp(client)
		return false, err_u
	end

	local ok_r = klbsocks5.reply(client, klbsocks5.REP_SUCCEEDED, _bnd_from_upstream(upstream, session))
	if not ok_r then
		_close_tcp(upstream)
		if session ~= nil and type(session.close) == "function" then
			session.close()
		end
		_close_tcp(client)
		return false, "reply fail"
	end

	_relay_and_close(client, upstream, session, cfg)
	return true
end


local function _handle_ws_tunnel(client, cfg, req)
	local allow_ws = _has_proto(cfg, "ws") or _has_proto(cfg, "wss")
		or _has_proto(cfg, "alua-tunnel") or _has_proto(cfg, "alua_tunnel")
		or _has_proto(cfg, "http-connect") or _has_proto(cfg, "http_connect")
	if not allow_ws then
		_close_tcp(client)
		return false, "ws tunnel disabled"
	end

	local ok_ws, ws_server = pcall(require, "klbagent.packages.proxy.transport.ws.server")
	if not ok_ws or type(ws_server) ~= "table" then
		_close_tcp(client)
		return false, "ws.server unavailable"
	end

	if type(ws_server.is_tunnel_path) == "function" and not ws_server.is_tunnel_path(req.path) then
		http_connect_server.reply(client, net.HTTP_NOT_FOUND, "Not Found")
		_close_tcp(client)
		return false, "bad ws path"
	end

	local ok_a, stream = ws_server.accept(client, req.key)
	if not ok_a then
		_close_tcp(client)
		return false, stream
	end

	-- 隧道鉴权: 入口 auth 非 none 时用 HELLO token; 头 X-Alua-Token 作回退
	local serve_opts = { auth = cfg.auth, prefix = "" }
	if type(cfg) == "table" and cfg.key ~= nil then
		serve_opts.key = cfg.key
	end
	if type(req.token) == "string" and req.token ~= "" then
		if type(serve_opts.auth) ~= "table" then
			serve_opts.auth = { mode = "token", token = req.token }
		elseif serve_opts.auth.mode == "none" or serve_opts.auth.mode == nil then
			-- 无入口鉴权时不强制头 token
		end
	end

	local ok_h, treq = _tunnel_serve_result(tunnel_server.serve_conn(stream, serve_opts))
	if not ok_h then
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		else
			_close_tcp(client)
		end
		return false, treq
	end

	local upstream, session, err_u = _open_upstream(cfg, treq.host, treq.port)
	if upstream == nil then
		tunnel_server.reply(stream, false, err_u or "upstream fail")
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		else
			_close_tcp(client)
		end
		return false, err_u
	end

	local ok_r = tunnel_server.reply(stream, true)
	if not ok_r then
		_close_tcp(upstream)
		if session ~= nil and type(session.close) == "function" then
			session.close()
		end
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		else
			_close_tcp(client)
		end
		return false, "reply fail"
	end

	if type(treq.session_key) == "string" and treq.session_key ~= "" then
		tunnel_crypto.relay_framed_raw(stream, upstream, treq.session_key, { coop = false })
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		else
			_close_tcp(client)
		end
		_close_tcp(upstream)
		if session ~= nil and type(session.close) == "function" then
			session.close()
		end
		return true
	end

	-- WS 流为 table; 上游若为 mnps 则走 duplex
	_pump_pair(stream, upstream, _pump_opts(cfg))
	if stream ~= nil and type(stream.close) == "function" then
		stream.close()
	else
		_close_tcp(client)
	end
	_close_tcp(upstream)
	if session ~= nil and type(session.close) == "function" then
		session.close()
	end
	return true
end


local function _handle_http_connect(client, cfg, prefix, peer_ip)
	local next_prefix = prefix
	local loops = 0
	local max_reuse = 32

	while loops < max_reuse do
		loops = loops + 1
		local ok_h, req = http_connect_server.serve_conn(client, {
			auth = cfg.auth,
			prefix = next_prefix,
		})
		next_prefix = nil

		if not ok_h then
			if type(req) == "string" and string.find(req, "auth", 1, true) then
				stats.on_auth_fail()
				if _guard ~= nil then
					_guard.note_auth_fail(peer_ip)
				end
			end
			_close_tcp(client)
			return false, req
		end

		if _guard ~= nil then
			_guard.note_auth_ok(peer_ip)
		end

		if type(req) == "table" and req.kind == "websocket" then
			return _handle_ws_tunnel(client, cfg, req)
		end

		if not _route_allow(cfg, req.host, req.port, peer_ip) then
			stats.on_reject()
			http_connect_server.reply(client, net.HTTP_FORBIDDEN, "Forbidden")
			_close_tcp(client)
			return false, "route allow_from deny"
		end

		local upstream, session, err_u = _open_upstream(cfg, req.host, req.port)
		if upstream == nil then
			http_connect_server.reply(client, net.HTTP_BAD_GATEWAY, "Bad Gateway")
			_close_tcp(client)
			return false, err_u
		end

		-- 明文绝对 URI: 改写为 origin-form 发往源站, 再双向中继
		if req.kind == "forward" then
			local out = req.origin_hdr or ""
			if type(req.body_prefix) == "string" and #req.body_prefix > 0 then
				out = out .. req.body_prefix
			end

			local ok_s = kproxy_io.send(upstream, out)
			if not ok_s then
				_close_tcp(upstream)
				if session ~= nil and type(session.close) == "function" then
					session.close()
				end
				http_connect_server.reply(client, net.HTTP_BAD_GATEWAY, "Bad Gateway")
				_close_tcp(client)
				return false, "forward send fail"
			end

			local reuse = (req.keep_alive == true)
			-- 泵结束即关上游: 复用已关闭的上游会致后续响应 0 字节 (Bug.2026-8)
			-- drain_ms: 客户端先 EOF 时短收尾, 勿等满 idle
			_relay_and_close(client, upstream, session, cfg, reuse, false, { drain_ms = 2000 })
			if reuse then
				-- 客户端 keep-alive: 源站已 EOF, 继续读下一请求
			else
				return true
			end
		else
			-- CONNECT: HTTP_OK 后裸 TCP 中继 (不复用)
			local ok_r = http_connect_server.reply(client, net.HTTP_OK)
			if not ok_r then
				_close_tcp(upstream)
				if session ~= nil and type(session.close) == "function" then
					session.close()
				end
				_close_tcp(client)
				return false, "reply fail"
			end

			_relay_and_close(client, upstream, session, cfg)
			return true
		end
	end

	_close_tcp(client)
	return true
end


local function _handle_alua_tunnel(client, cfg, prefix)
	local serve_opts = { auth = cfg.auth, prefix = prefix }
	if type(cfg) == "table" and cfg.key ~= nil then
		serve_opts.key = cfg.key
	end

	local ok_h, req = _tunnel_serve_result(tunnel_server.serve_conn(client, serve_opts))
	if not ok_h then
		_close_tcp(client)
		return false, req
	end

	local upstream, session, err_u = _open_upstream(cfg, req.host, req.port)
	if upstream == nil then
		tunnel_server.reply(client, false, err_u or "upstream fail")
		_close_tcp(client)
		return false, err_u
	end

	local ok_r = tunnel_server.reply(client, true)
	if not ok_r then
		_close_tcp(upstream)
		if session ~= nil and type(session.close) == "function" then
			session.close()
		end
		_close_tcp(client)
		return false, "reply fail"
	end

	if type(req.session_key) == "string" and req.session_key ~= "" then
		tunnel_crypto.relay_framed_raw(client, upstream, req.session_key, { coop = _coop })
		_close_tcp(client)
		_close_tcp(upstream)
		if session ~= nil and type(session.close) == "function" then
			session.close()
		end
		return true
	end

	_relay_and_close(client, upstream, session, cfg)
	return true
end


-- @brief klbmnp/klbsmp 会话: auth + alua-tunnel HELLO/CONNECT + 泵送 (mnps+CRYPTO)
local function _handle_mnps_session(serve, cfg)
	local ok_a, stream = smp_server.accept_session(serve, { auth = cfg.auth })
	if not ok_a then
		if serve ~= nil and type(serve.disconnect) == "function" then
			pcall(function ()
				serve:disconnect()
			end)
		end
		return false, stream
	end

	local ok_h, treq = _tunnel_serve_result(_mnps_serve_tunnel(stream))
	if not ok_h then
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		end
		return false, treq
	end

	local upstream, session, err_u = _open_upstream(cfg, treq.host, treq.port)
	if upstream == nil then
		tunnel_server.reply(stream, false, err_u or "upstream fail")
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		end
		return false, err_u
	end

	local ok_r = tunnel_server.reply(stream, true)
	if not ok_r then
		_close_tcp(upstream)
		if session ~= nil and type(session.close) == "function" then
			session.close()
		end
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		end
		return false, "reply fail"
	end

	-- CRYPTO 在 MNP 层; 隧道帧明文泵送 (mnps 流须 duplex)
	_pump_pair(stream, upstream, _pump_opts(cfg))

	if stream ~= nil and type(stream.close) == "function" then
		stream.close()
	end
	_close_tcp(upstream)
	if session ~= nil and type(session.close) == "function" then
		session.close()
	end
	return true
end


local function _handle_smp_tunnel(client, cfg, _prefix)
	-- 同口 demux 后走 C 连接层 (ksmp/kmnp.new_serve); 与 mnps/smps 同 accept_session
	-- _prefix 仅兼容旧调用; 入口已改 peek, 数据未消费
	local allow_smp = _has_proto(cfg, "smp") or _has_proto(cfg, "mnp")
		or _has_proto(cfg, "alua-tunnel") or _has_proto(cfg, "alua_tunnel")
	if not allow_smp then
		_close_tcp(client)
		return false, "smp tunnel disabled"
	end

	local ok_a, stream = smp_server.accept(client, {
		auth = cfg.auth,
	})
	if not ok_a then
		_close_tcp(client)
		return false, stream
	end

	local serve_opts = { auth = { mode = "none" }, prefix = "" }
	if type(cfg) == "table" and cfg.key ~= nil then
		serve_opts.key = cfg.key
	end

	-- 若入口强制 token, HELLO 再验
	if type(cfg.auth) == "table" and cfg.auth.mode ~= nil and cfg.auth.mode ~= "none" then
		serve_opts.auth = cfg.auth
	end

	local ok_h, treq = _tunnel_serve_result(tunnel_server.serve_conn(stream, serve_opts))
	if not ok_h then
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		else
			_close_tcp(client)
		end
		return false, treq
	end

	local upstream, session, err_u = _open_upstream(cfg, treq.host, treq.port)
	if upstream == nil then
		tunnel_server.reply(stream, false, err_u or "upstream fail")
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		else
			_close_tcp(client)
		end
		return false, err_u
	end

	local ok_r = tunnel_server.reply(stream, true)
	if not ok_r then
		_close_tcp(upstream)
		if session ~= nil and type(session.close) == "function" then
			session.close()
		end
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		else
			_close_tcp(client)
		end
		return false, "reply fail"
	end

	if type(treq.session_key) == "string" and treq.session_key ~= "" then
		tunnel_crypto.relay_framed_raw(stream, upstream, treq.session_key, { coop = false })
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		else
			_close_tcp(client)
		end
		_close_tcp(upstream)
		if session ~= nil and type(session.close) == "function" then
			session.close()
		end
		return true
	end

	_pump_pair(stream, upstream, _pump_opts(cfg))

	if stream ~= nil and type(stream.close) == "function" then
		stream.close()
	else
		_close_tcp(client)
	end
	_close_tcp(upstream)
	if session ~= nil and type(session.close) == "function" then
		session.close()
	end
	return true
end


local function _dispatch_client(client, cfg, listen)
	local L = listen
	if type(L) ~= "table" then
		L = cfg and cfg.listen
	end

	local allow_socks = _has_proto(L, "socks5")
	local allow_http = _has_proto(L, "http-connect") or _has_proto(L, "http_connect")
	local allow_tunnel = _has_proto(L, "alua-tunnel") or _has_proto(L, "alua_tunnel")
	local allow_smp = _has_proto(L, "smp") or _has_proto(L, "mnp")

	local ok_g, peer_ip, err_g = _peer_gate(client)
	if not ok_g then
		return false, err_g or "acl deny"
	end

	-- 先窥探首字节分流; SMP/MNP 不消费, 便于 steal 给 C 连接层
	local peek, err_p = kproxy_io.recv_peek(client, 1)
	if type(peek) ~= "string" or #peek == 0 then
		_close_tcp(client)
		return false, err_p or "peek fail"
	end

	local proto = demux.classify(peek)

	if proto == demux.PROTO_SMP then
		if not allow_smp and not allow_tunnel then
			_close_tcp(client)
			return false, "smp tunnel disabled"
		end
		return _handle_smp_tunnel(client, cfg, nil)
	end

	-- 其余协议仍消费首字节作 prefix
	local first, err = kproxy_io.recv(client, 1)
	if type(first) ~= "string" or #first == 0 then
		_close_tcp(client)
		return false, err or "recv fail"
	end

	if proto == demux.PROTO_ALUA_TUNNEL then
		if not allow_tunnel then
			_close_tcp(client)
			return false, "alua-tunnel disabled"
		end
		return _handle_alua_tunnel(client, cfg, first)
	end

	if proto == demux.PROTO_SOCKS5 then
		if not allow_socks then
			_close_tcp(client)
			return false, "socks5 disabled"
		end
		return _handle_socks5(client, cfg, first, peer_ip)
	end

	if proto == demux.PROTO_HTTP then
		if not allow_http then
			_close_tcp(client)
			return false, "http-connect disabled"
		end
		return _handle_http_connect(client, cfg, first, peer_ip)
	end

	-- 探针噪音: 未知首字节静默关
	stats.on_probe_noise()
	_close_tcp(client)
	return false, "unknown protocol"
end


local function _resolve_mnps_tls()
	return nil, "alua.core.serve.UNAVAILABLE"
end


local function _start_mnps_mux(cfg, port, kco, listen)
	local tls_info, err_t = _resolve_mnps_tls()
	if tls_info == nil then
		return nil, err_t
	end

	local ok_l, klblisten = pcall(require, "klbcore.klblisten")
	if not ok_l or type(klblisten) ~= "table" or type(klblisten.listen) ~= "function" then
		return nil, "klbcore.klblisten unavailable"
	end

	local L = listen
	if type(L) ~= "table" then
		L = cfg and cfg.listen
	end

	local as_mnp = _has_proto(L, "mnps") or (not _has_proto(L, "smps"))
	local mux_cfg = {
		tls = true,
		cert = tls_info.cert,
		key = tls_info.key,
		tls_as_mnp = as_mnp,
		tls_as_http = false,
	}

	local mux = klblisten.listen(tonumber(port), mux_cfg)
	if mux == nil then
		return nil, "klblisten.bind fail port=" .. tostring(port)
	end

	if kco == nil then
		mux:close()
		return nil, "mnps listen need kco"
	end

	kco.fork(function ()
		while _running do
			local ok_e, kenv = pcall(require, "kenv")
			if ok_e and type(kenv) == "table" and type(kenv.is_exit) == "function" and kenv.is_exit() then
				break
			end

			-- klblisten face: TLS → mnps/smps; plain → mnp/smp
			local proto, serve, msg = mux:co_accept()
			if msg == "exit" then
				break
			end

			if msg ~= "ok" or serve == nil then
				-- timeout / error
			elseif proto == "mnp" or proto == "mnps"
				or proto == "smp" or proto == "smps" then
				_active = _active + 1
				kco.fork(function ()
					pcall(_handle_mnps_session, serve, cfg)
					_active = _active - 1
				end)
			elseif type(serve.disconnect) == "function" then
				pcall(function ()
					serve:disconnect()
				end)
			end
		end
	end)

	return mux, nil
end


-- @brief 启动入口 (accept 循环, 直至 stop / kenv 退出 / max_conns)
-- @note 有 kco/kenv 时 co_accept fork + duplex; 否则串行同步 accept
-- @param [in]  cfg[table|nil]		省略则 defaults
-- @param [in]  opts[table|nil]		`{ max_conns[number], accept_timeout_ms[number] }`
-- @return [boolean] ok
-- @return [string|nil] err
-- @brief 已 TEXT alua.auth 的 mnps/smps 会话: 走 alua-tunnel HELLO/CONNECT 并出口泵送
-- @note 须在 kco 协程内; 供 core.serve mux 在 BINARY head=alua-tunnel 时移交
-- @param [in]  serve[table]		klbmnp/klbsmp 会话
-- @param [in]  cfg[table|nil]		proxy 配置 (选链/直连); nil=defaults
-- @param [in]  opts[table|nil]		`{ prime[string]=已读首包 BINARY 体 }`
-- @return [boolean] ok
-- @return [string|nil] err
M.co_mnps_exit_authed = function (serve, cfg, opts)
	if serve == nil then
		return false, "need serve"
	end

	if cfg == nil then
		cfg = config.defaults()
	end

	if type(cfg) ~= "table" then
		return false, "bad cfg"
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	local wrap_opts = nil
	if type(opts) == "table" and type(opts.prime) == "string" then
		wrap_opts = { prime = opts.prime }
	end

	local ok_w, stream = smp_server.wrap_session(serve, wrap_opts)
	if not ok_w then
		if serve ~= nil and type(serve.disconnect) == "function" then
			pcall(function ()
				serve:disconnect()
			end)
		end
		return false, stream
	end

	local ok_h, treq = _tunnel_serve_result(_mnps_serve_tunnel(stream))
	if not ok_h then
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		end
		return false, treq
	end

	local upstream, session, err_u = _open_upstream(cfg, treq.host, treq.port)
	if upstream == nil then
		tunnel_server.reply(stream, false, err_u or "upstream fail")
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		end
		return false, err_u
	end

	local ok_r = tunnel_server.reply(stream, true)
	if not ok_r then
		_close_tcp(upstream)
		if session ~= nil and type(session.close) == "function" then
			session.close()
		end
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		end
		return false, "reply fail"
	end

	-- serve 出口路径: 勿依赖 entry._coop; 强制 duplex (stream table + tcp userdata)
	local ok_p, err_p = pump.co_relay_duplex(stream, upstream, _pump_opts(cfg))
	if not ok_p then
		if stream ~= nil and type(stream.close) == "function" then
			stream.close()
		end
		_close_tcp(upstream)
		if session ~= nil and type(session.close) == "function" then
			session.close()
		end
		return false, err_p
	end

	if stream ~= nil and type(stream.close) == "function" then
		stream.close()
	end
	_close_tcp(upstream)
	if session ~= nil and type(session.close) == "function" then
		session.close()
	end
	return true
end


-- @param [in] skip_ports[table|nil]	已占用口集合 {[port]=true}
-- @return listener, bound_port, err
local function _open_kproxy_listen(host, port, backlog, fallback_n, skip_ports)
	local ok_kp, kproxy = pcall(require, "kproxy")
	if not ok_kp or type(kproxy) ~= "table" or type(kproxy.listen) ~= "function" then
		return nil, nil, "kproxy.listen unavailable"
	end

	local listener, err_l = nil, nil
	local bound_port = port
	for i = 0, fallback_n do
		local try_port = port + i
		if try_port > 65535 then
			break
		end
		if type(skip_ports) == "table" and skip_ports[try_port] then
			-- 组内其它 listen/mnps 已占, 跳过
		else
			listener, err_l = tunnel_server.listen(host, try_port, backlog)
			if listener == nil then
				listener, err_l = kproxy.listen(host, try_port, backlog)
			end
			if listener ~= nil then
				bound_port = try_port
				break
			end
		end
	end
	if listener == nil then
		return nil, nil, err_l or "listen fail"
	end

	if type(listener.local_addr) == "function" then
		local ok_la, _lip, lport = pcall(function ()
			return listener:local_addr()
		end)
		if ok_la and tonumber(lport) ~= nil and tonumber(lport) > 0 then
			bound_port = tonumber(lport)
		end
	end

	return listener, bound_port, nil
end


-- @brief 非阻塞启动: 校验配置 → 绑定监听 → fork accept 协程
-- @param [in] cfg[table|nil]		配置; nil = config.defaults()
-- @param [in] opts[table|nil]	max_conns / accept_timeout_ms
-- @return ok[boolean]
-- @return err[string|nil]
-- @note 须宿主泵送 kenv.loop_once; 停止调 M.stop_embed
M.start_embed = function (cfg, opts)
	if cfg == nil then
		cfg = config.defaults()
	end

	local ok, err = config.validate(cfg)
	if not ok then
		return false, err
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	local group = config.listen_group(cfg)
	if #group == 0 then
		return false, "no listen"
	end

	local any_kproxy = false
	local any_mnps = false
	for i = 1, #group do
		if config.listen_need_kproxy(group[i]) then
			any_kproxy = true
		end
		if config.listen_allow_mnps(group[i]) then
			any_mnps = true
		end
	end
	if not any_kproxy and not any_mnps then
		return false, "no listen.protocols"
	end

	local ok_e, kenv = pcall(require, "kenv")
	if not ok_e or type(kenv) ~= "table" then
		kenv = nil
	end

	local kco = nil
	do
		local ok_c, mod = pcall(require, "kco")
		if ok_c and type(mod) == "table"
			and type(mod.fork) == "function"
			and type(mod.co_sleep) == "function"
			and kenv ~= nil
			and type(kenv.loop_once) == "function" then
			kco = mod
		end
	end

	_coop = kco ~= nil
	_active = 0
	_cfg = cfg
	_running = true
	_listeners = {}
	_mnps_muxes = {}
	_guard = acl.new_guard(cfg)
	-- 启动回落 default: 清临时切链 (proxy_use.json)
	pcall(function ()
		config.clear_use()
	end)

	-- M2: hop 长连池 (鉴权暖会话; 默认空闲 1h 自断)
	do
		local to = config.timeouts(cfg)
		local hp = (type(cfg.hop_pool) == "table") and cfg.hop_pool or {}
		local max_n = tonumber(hp.max)
		if max_n == nil then
			max_n = 2
		end
		hop_pool.configure({
			enabled = true,
			max = max_n,
			idle_ms = to.hop_idle_ms,
		})
		if kco ~= nil then
			-- 启动即落一次运行态快照 (跨进程 status 可见)
			stats.save_runtime()
			kco.fork(function ()
				while _running do
					kco.co_sleep(30000)
					if not _running then
						break
					end
					hop_pool.sweep()
					stats.save_runtime()
				end
			end)
		end
	end

	local default_backlog = tonumber(cfg.listen.backlog) or 128
	local default_fallback = tonumber(cfg.listen.port_fallback)
	if default_fallback == nil then
		default_fallback = 8
	end

	-- 组内预占口: 避免主 listen port_fallback 撞上附加口/mnps
	local skip_ports = {}
	for i = 1, #group do
		local L = group[i]
		if config.listen_need_kproxy(L) then
			local p = tonumber(L.port)
			if p ~= nil then
				skip_ports[p] = true
			end
		end
		local mp = config.listen_mnps_port(L)
		if mp ~= nil then
			skip_ports[mp] = true
		end
	end

	for i = 1, #group do
		local L = group[i]
		local backlog = tonumber(L.backlog) or default_backlog
		local fallback_n = tonumber(L.port_fallback)
		if fallback_n == nil then
			fallback_n = default_fallback
		end

		-- 附加口默认不做端口回退, 避免悄然占口; 主 listen 仍可用 port_fallback
		if L ~= cfg.listen and L.port_fallback == nil then
			fallback_n = 0
		end

		if config.listen_allow_mnps(L) then
			local mnps_port, err_mp = config.listen_mnps_port(L)
			if mnps_port == nil then
				_running = false
				_guard = nil
				_close_all_listeners()
				_close_all_mnps()
				return false, err_mp or "mnps_port bad"
			end

			local mux, err_m = _start_mnps_mux(cfg, mnps_port, kco, L)
			if mux == nil then
				_running = false
				_guard = nil
				_close_all_listeners()
				_close_all_mnps()
				return false, err_m
			end

			_mnps_muxes[#_mnps_muxes + 1] = mux
			L._mnps_port_bound = mnps_port
		end

		if config.listen_need_kproxy(L) then
			local host = L.host or "127.0.0.1"
			local port = tonumber(L.port) or 12818
			-- 本口自身不跳过
			skip_ports[port] = nil
			local listener, bound_port, err_l = _open_kproxy_listen(
				host, port, backlog, fallback_n, skip_ports)
			if listener == nil then
				_running = false
				_guard = nil
				_close_all_listeners()
				_close_all_mnps()
				return false, err_l or "listen fail"
			end

			L.port = bound_port
			L._bound_port = bound_port
			skip_ports[bound_port] = true
			_listeners[#_listeners + 1] = {
				listener = listener,
				listen = L,
			}
		end
	end

	local max_conns = nil
	local to = config.timeouts(cfg)
	local accept_ms = to.accept_ms
	if type(opts) == "table" then
		if opts.max_conns ~= nil then
			max_conns = tonumber(opts.max_conns)
		end
		if opts.accept_timeout_ms ~= nil then
			accept_ms = tonumber(opts.accept_timeout_ms) or accept_ms
		end
	end

	_accept_ms = accept_ms
	_accept_need = #_listeners
	_handled = 0

	-- 并发: 每监听器一协程 co_accept; 宿主只泵 loop_once
	if _coop and _accept_need > 0 then
		for li = 1, _accept_need do
			local item = _listeners[li]
			kco.fork(function ()
				while _running do
					if kenv ~= nil and type(kenv.is_exit) == "function" and kenv.is_exit() then
						break
					end

					local client, err_a = kproxy_io.accept(item.listener)
					if client == nil then
						if not _running then
							break
						end
						kco.co_sleep(10)
					else
						stats.on_accept()
						_active = _active + 1
						kco.fork(function ()
							pcall(_dispatch_client, client, cfg, item.listen)
							_active = _active - 1
							stats.on_close()
						end)

						_handled = _handled + 1
						if max_conns ~= nil and _handled >= max_conns then
							_running = false
							break
						end
					end
				end
			end)
		end
	end

	return true, nil
end


-- @brief 泵送一次 (宿主主循环调用; 全进程只允许一处)
-- @return ms[number]	kenv.loop_once 建议休眠毫秒; 未运行/无 kenv 为 0
M.pump_embed = function ()
	if not _running or not _coop then
		return 0
	end

	local ok_e, kenv = pcall(require, "kenv")
	if not ok_e or type(kenv) ~= "table" or type(kenv.loop_once) ~= "function" then
		return 0
	end

	local ms = kenv.loop_once()
	if type(ms) ~= "number" or ms < 0 then
		return 0
	end

	return ms
end


-- @brief 阻塞启动 (alua pkg run proxy serve; start_embed + 自泵至 stop)
-- @param [in] cfg[table|nil]
-- @param [in] opts[table|nil]
-- @return ok[boolean]
-- @return err[string|nil]
-- @note _coop: co_util.run_host_loop (CLI hold / 独立自泵) 至 _running=false 或 kenv.exit; embed 仍 pump_embed
M.start = function (cfg, opts)
	local ok_s, err_s = M.start_embed(cfg, opts)
	if not ok_s then
		return false, err_s
	end

	if not _coop then
		M.stop_embed()
		return false, "kco unavailable (proxy start requires co_main_loop)"
	end

	-- CLI 宿主 (entry_cli/co_task) 已在泵: hold; 独立进程入口才自调 co_main_loop
	local ok_u, co_util = pcall(require, "klbagent.core.co_util")
	if not ok_u or type(co_util) ~= "table" or type(co_util.run_host_loop) ~= "function" then
		M.stop_embed()
		return false, "co_util.run_host_loop unavailable"
	end

	local ok_h, mode_or_err = pcall(co_util.run_host_loop, {
		until_fn = function ()
			return not _running
		end,
	})
	if not ok_h then
		M.stop_embed()
		return false, tostring(mode_or_err)
	end

	-- hold: 宿主继续泵; 禁在此 stop_embed (否则 listen 立即关掉)
	if mode_or_err == "hold" then
		return true, nil
	end

	-- 自泵结束 (mode==self) 后收尾; co_main_loop 退出时已 drain 协程
	M.stop_embed()
	return true, nil
end


-- @brief 停止入口 (打断下次 accept 检查; 幂等)
-- @return [boolean] ok
-- @return [string|nil] err
M.stop_embed = function ()
	_running = false
	_coop = false
	_active = 0
	_accept_need = 0
	_handled = 0
	_close_all_listeners()
	_close_all_mnps()
	pool.clear()
	hop_pool.clear()
	stats.clear_runtime()
	_cfg = nil
	_guard = nil
	return true, nil
end


-- @brief 是否在跑
-- @return [boolean]
M.is_running = function ()
	return _running == true
end


-- @brief 观测计数快照
-- @return [table]
M.stats = function ()
	local s = stats.snapshot()
	if type(s) == "table" then
		s.pool = pool.stats()
		s.hop_pool = hop_pool.stats()
	end
	return s
end


-- @brief 当前配置快照 (无则 defaults)
-- @return [table]
M.current_cfg = function ()
	if type(_cfg) == "table" then
		return _cfg
	end

	return config.defaults()
end


-- @brief 热更新运行中入口配置 (M1.c)
-- @note 监听组 host/port/mnps_port 变则须重启; 链/路由/鉴权/protocols 可热换
-- @param [in] cfg[table]
-- @return ok[boolean]
-- @return err[string|nil]
-- @return restart_needed[boolean]
M.apply_live = function (cfg)
	if type(cfg) ~= "table" then
		return false, "cfg must be table", false
	end

	local ok_v, err_v = config.validate(cfg)
	if not ok_v then
		return false, err_v or "validate fail", false
	end

	-- 入口未跑: 仅校验通过, 下次 start 读 live/pref
	if _running ~= true or type(_cfg) ~= "table" then
		return true, nil, false
	end

	local old_fp = config.listen_bind_fingerprint(_cfg)
	local new_fp = config.listen_bind_fingerprint(cfg)
	if old_fp ~= new_fp then
		-- 仍写入 _cfg 供 current_cfg 可见; 监听口不变直至重启
		_cfg = cfg
		return true, nil, true
	end

	_cfg = cfg
	_guard = acl.new_guard(cfg)

	do
		local to = config.timeouts(cfg)
		local hp = (type(cfg.hop_pool) == "table") and cfg.hop_pool or {}
		local max_n = tonumber(hp.max)
		if max_n == nil then
			max_n = 2
		end
		hop_pool.configure({
			enabled = true,
			max = max_n,
			idle_ms = to.hop_idle_ms,
		})
	end

	-- 同绑定面: 刷新各 accept 项的 listen 引用 (protocols 等热生效)
	local group = config.listen_group(cfg)
	for i = 1, #_listeners do
		local item = _listeners[i]
		local old_L = item.listen
		local old_host = tostring(old_L and old_L.host or "")
		local old_port = tonumber(old_L and (old_L._bound_port or old_L.port))
		for gi = 1, #group do
			local L = group[gi]
			if tostring(L.host or "") == old_host
				and tonumber(L._bound_port or L.port) == old_port then
				item.listen = L
				break
			end
		end
	end

	return true, nil, false
end


return M

-- end
