--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  代理 hop SMP/MNP 出站 (smp/mnp/smps/mnps + alua-tunnel)
-- @note   走 klbmnp/klbsmp; TLS(smps/mnps) 与 CRYPTO_PSK 正交; hop.crypto 可任意搭配
-- @history 修改历史
--  \n [2026-10] 创建文件 (N5 P2 / M1+M2); 明文亦走 C 真栈; 废弃 Lua smpframe 编解码; HELLO/CONNECT 经 klbtunnel.hello_connect; hop.crypto 与 TLS 正交; 许可 mnp/smp+CRYPTO 与 format 选型; open_session_timed / co_tunnel_timed / co_connect_timed (bench phases)
--]]


local tunnel_client = require("klbcore.klbtunnel.client")


local M = {}


local HEAD_AUTH = "alua.auth"
local HEAD_TUNNEL = "alua-tunnel"


local function _auth_token(opts)
	if type(opts) ~= "table" then
		return ""
	end

	if type(opts.token) == "string" and opts.token ~= "" then
		return opts.token
	end

	if type(opts.auth) == "table" then
		if type(opts.auth.token) == "string" then
			return opts.auth.token
		end

		local user = tostring(opts.auth.user or "")
		local pass = tostring(opts.auth.pass or "")
		if user ~= "" or pass ~= "" then
			return user .. ":" .. pass
		end
	end

	return ""
end


local function _kind_tls(opts)
	local kind = "smp"
	local use_tls = false
	if type(opts) == "table" then
		if opts.kind == "mnp" or opts.mnp == true then
			kind = "mnp"
		end

		use_tls = (opts.tls == true) or (opts.smps == true) or (opts.mnps == true)
		if opts.mnps == true then
			kind = "mnp"
			use_tls = true
		elseif opts.smps == true then
			kind = "smp"
			use_tls = true
		end
	end

	return kind, use_tls
end


local function _host_is_pc()
	local ok, hostinfo = pcall(require, "klbagent.core.hostinfo")
	if not ok or type(hostinfo) ~= "table" or type(hostinfo.probe) ~= "function" then
		return true
	end

	local hi = hostinfo.probe()
	local kind = (type(hi) == "table" and type(hi.host_kind) == "string") and hi.host_kind or "linux"
	return kind ~= "embedded"
end


-- @brief 默认 CRYPTO: 对齐 tools/remote/transport._default_smp_crypto
local function _default_crypto()
	if _host_is_pc() then
		return { format = "CHACHA20", key_size = 44 }
	end

	return { format = "AES_CTR", key_size = 24 }
end


-- @brief 按 format 名补默认 key_size(仅缺省时)
local function _default_key_size(format)
	local f = string.upper(tostring(format or ""))
	if f == "CHACHA20" then
		return 44
	end

	if f == "AES_CTR" then
		return 24
	end

	if f == "AES_OFB" or f == "AES_CBC" then
		return 32
	end

	if f == "AES_ECB" then
		return 16
	end

	if f == "XOR" then
		return 16
	end

	return nil
end


-- @brief 解析 hop.crypto; 与 TLS 正交
-- \n false=强制无 CRYPTO; true=默认; table/string=显式 format; nil 时 TLS 跳默认开 CRYPTO
local function _resolve_crypto(opts, use_tls)
	if type(opts) ~= "table" then
		if use_tls then
			return _default_crypto()
		end

		return nil
	end

	local c = opts.crypto
	if c == false then
		return nil
	end

	if c == true then
		return _default_crypto()
	end

	if type(c) == "string" and c ~= "" then
		local ks = _default_key_size(c)
		if ks == nil then
			return nil, "crypto format unsupported: " .. c
		end

		return { format = c, key_size = ks }
	end

	if type(c) == "table" then
		local out = {}
		for k, v in pairs(c) do
			out[k] = v
		end

		if type(out.format) == "string" and out.format ~= "" then
			if out.key_size == nil then
				out.key_size = _default_key_size(out.format)
			end
		elseif out.format == nil then
			local d = _default_crypto()
			out.format = d.format
			if out.key_size == nil then
				out.key_size = d.key_size
			end
		end

		if out.key_size == nil then
			return nil, "crypto need key_size"
		end

		return out
	end

	-- 未声明 crypto: mnps/smps 默认开; mnp/smp 默认关
	if use_tls then
		return _default_crypto()
	end

	return nil
end


-- @brief alua.auth TEXT body; 证明票须同送 alua_ssid (2026-11 ssid 契约)
local function _auth_body(token, ssid)
	local tok = token or ""
	local sid = ""
	if type(ssid) == "string" then
		sid = ssid
	end

	local ok_j, cjson = pcall(require, "cjson")
	if ok_j and type(cjson) == "table" and type(cjson.encode) == "function" then
		local obj = { token = tok }
		if sid ~= "" then
			obj.alua_ssid = sid
		end

		local ok_e, text = pcall(cjson.encode, obj)
		if ok_e and type(text) == "string" then
			return text
		end
	end

	-- 无 cjson 回退: token/ssid 均为 [%w._%-] 安全字符
	if sid ~= "" then
		return string.format('{"token":"%s","alua_ssid":"%s"}', tok, sid)
	end

	return tok
end


local function _auth_ok_body(body)
	if type(body) ~= "string" then
		return false
	end

	if string.find(body, '"ok"%s*:%s*true', 1) ~= nil then
		return true
	end

	if string.find(body, "false", 1, true) ~= nil then
		return false
	end

	local ok_j, cjson = pcall(require, "cjson")
	if ok_j and type(cjson) == "table" and type(cjson.decode) == "function" then
		local ok_d, obj = pcall(cjson.decode, body)
		if ok_d and type(obj) == "table" and obj.ok == true then
			return true
		end
	end

	return false
end


-- @brief fork 到 kco 跑 fn; 等待走 co_util.co_run (须 co_main_loop)
local function _run_in_co(timeout_ms, fn)
	local co_util = require("klbagent.core.co_util")
	return co_util.co_run(timeout_ms, fn, "klb hop")
end


local function _disconnect_klb(client)
	if client ~= nil and type(client.disconnect) == "function" then
		pcall(function ()
			client:disconnect()
		end)
	end
end


local function _klb_open(kind, use_tls, host, port, crypto_spec)
	-- TLS 与 CRYPTO 正交: smps/mnps 可无 crypto; mnp/smp 可带 CRYPTO_PSK
	local opts = {}
	if use_tls then
		opts.tls = true
	end

	if crypto_spec ~= nil then
		opts.crypto = crypto_spec
	end

	if kind == "mnp" then
		local ok_m, klbmnp = pcall(require, "klbcore.klbmnp")
		if not ok_m or type(klbmnp) ~= "table" or type(klbmnp.connect) ~= "function" then
			return nil, "klbmnp.connect unavailable"
		end

		local client = klbmnp.connect(host, tonumber(port), opts)
		if client == nil then
			return nil, "mnp connect fail"
		end

		return client, nil
	end

	local ok_s, klbsmp = pcall(require, "klbcore.klbsmp")
	if not ok_s or type(klbsmp) ~= "table" or type(klbsmp.connect) ~= "function" then
		return nil, "klbsmp.connect unavailable"
	end

	local client = klbsmp.connect(host, tonumber(port), opts)
	if client == nil then
		return nil, "smp connect fail"
	end

	return client, nil
end


local function _klb_recv_text_auth(client, rounds)
	local n = tonumber(rounds) or 16
	for _ = 1, n do
		local msg, head, body = client:co_recv()
		if msg == "text" and head == HEAD_AUTH then
			return true, body
		end

		if msg == "pong" then
			-- continue
		elseif msg == "error" or msg == "exit" or msg == "close" then
			return false, "auth early " .. tostring(msg)
		end
	end

	return false, "auth timeout"
end


local function _wrap_klb_stream(client, label, meta)
	local data_leftover = ""
	local stream = {
		kind = label,
		proxy_host = meta.proxy_host,
		proxy_port = meta.proxy_port,
		dest_host = meta.dest_host,
		dest_port = meta.dest_port,
		crypto = (meta.crypto == true),
		_mnp = client,
	}

	-- C 层吞 PING/PONG; 经 last_recv_ms 供泵送刷新空闲
	stream.idle_touch_ms = function ()
		local c = stream._mnp
		if c ~= nil and type(c.last_recv_ms) == "function" then
			local ok, v = pcall(function ()
				return c:last_recv_ms()
			end)
			if ok and type(v) == "number" and v > 0 then
				return v
			end
		end
		return stream._idle_touch_ms
	end

	stream.close = function ()
		_disconnect_klb(client)
		stream._mnp = nil
	end

	stream.send = function (_, data)
		if stream._mnp == nil then
			return false, "closed"
		end

		local rc = stream._mnp:send_binary(HEAD_TUNNEL, data or "")
		return 0 == rc
	end

	stream.recv = function (_, max_n, _timeout_ms)
		if stream._mnp == nil then
			return nil, "closed"
		end

		max_n = tonumber(max_n) or 4096
		if #data_leftover > 0 then
			local out = data_leftover
			if max_n > 0 and #out > max_n then
				data_leftover = string.sub(out, max_n + 1)
				return string.sub(out, 1, max_n)
			end

			data_leftover = ""
			return out
		end

		for _ = 1, 32 do
			local msg, head, body = stream._mnp:co_recv()
			if msg == "binary" then
				if head == nil or head == "" or head == HEAD_TUNNEL then
					body = body or ""
					if max_n > 0 and #body > max_n then
						data_leftover = string.sub(body, max_n + 1)
						body = string.sub(body, 1, max_n)
					end

					return body
				end
			elseif msg == "media" then
				if type(stream._mnp.free_media) == "function" then
					pcall(function ()
						stream._mnp:free_media(head)
					end)
				end
			elseif msg == "pong" or msg == "text" then
				-- C 吞心跳; 偶发 pong 上抛则忽略 (空闲靠 last_recv_ms)
			elseif msg == "error" or msg == "exit" or msg == "close" then
				-- error 的第二返回为 klb_netcode 码 (如 -104 ECONNRESET); 带上便于诊断 (Bug.2026-8)
				if msg == "error" and head ~= nil then
					return nil, "error:" .. tostring(head)
				end
				return nil, tostring(msg)
			end
		end

		return nil, "timeout"
	end

	-- 兼容 chain reuse_tcp 探测: 无裸 tcp
	stream.tcp = nil

	return stream
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


-- @brief 追加有序 phase; phases.map[name]=ms
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


-- @brief 建链并完成 TEXT alua.auth; 保持会话 (供 hop 长连池)
-- @param [in]  phases[table|nil]	可选; 填 connect/auth
-- @return [boolean] ok
-- @return [table|string] session 或 err
local function _klb_open_session(proxy_host, proxy_port, opts, phases)
	local kind, use_tls = _kind_tls(opts)
	local token = _auth_token(opts)
	local auth_ssid = nil
	if type(opts) == "table" then
		auth_ssid = opts.alua_ssid
	end
	local crypto_spec, err_cr = _resolve_crypto(opts, use_tls)
	if err_cr ~= nil then
		return false, err_cr
	end

	-- resolve: 字面 IP 记 0; 域名解析在 C connect 内并入 connect 段
	local host_s = tostring(proxy_host or "")
	if host_s:match("^%d+%.%d+%.%d+%.%d+$") or host_s:find(":", 1, true) then
		_phase_add(phases, "resolve", 0)
	end

	local t_conn = _now_ms()
	local client, err_c = _klb_open(kind, use_tls, proxy_host, proxy_port, crypto_spec)
	_phase_add(phases, "connect", _now_ms() - t_conn)
	if client == nil then
		return false, err_c
	end

	local function fail(msg)
		_disconnect_klb(client)
		return false, msg
	end

	local t_auth = _now_ms()
	local send_rc = client:send_text(HEAD_AUTH, _auth_body(token, auth_ssid))
	if 0 ~= send_rc then
		_phase_add(phases, "auth", _now_ms() - t_auth)
		return fail("auth send fail")
	end

	local ok_a, body_or_err = _klb_recv_text_auth(client, 16)
	_phase_add(phases, "auth", _now_ms() - t_auth)
	if not ok_a then
		return fail(tostring(body_or_err))
	end

	if not _auth_ok_body(body_or_err) then
		return fail("auth deny: " .. tostring(body_or_err))
	end

	local label = use_tls and ((kind == "mnp") and "mnps" or "smps")
		or ((kind == "mnp") and "mnp" or "smp")

	local session = {
		kind = label,
		proxy_host = proxy_host,
		proxy_port = tonumber(proxy_port),
		crypto = (crypto_spec ~= nil),
		_client = client,
		_token = token,
		_opts = opts,
	}

	session.close = function ()
		_disconnect_klb(session._client)
		session._client = nil
	end

	return true, session
end


-- @brief 在已鉴权会话上 HELLO/CONNECT 到目标 (消耗会话; 须在 kco 协程内)
-- @param [in]  phases[table|nil]	可选; 填 hello_connect / first_byte
-- @return [boolean] ok
-- @return [table|string] stream 或 err
local function _klb_co_tunnel(session, dest_host, dest_port, opts, phases)
	if type(session) ~= "table" or session._client == nil then
		return false, "bad session"
	end

	local client = session._client
	local token = session._token or _auth_token(opts)
	local base_opts = opts
	if type(base_opts) ~= "table" then
		base_opts = session._opts
	end

	local hc_opts
	if type(base_opts) ~= "table" then
		hc_opts = { token = token, skip_crypto = true }
	else
		hc_opts = {}
		for k, v in pairs(base_opts) do
			hc_opts[k] = v
		end
		hc_opts.token = token
		hc_opts.skip_crypto = true
	end

	local t_hc = _now_ms()
	local ok_t, code_t, err_t = tunnel_client.hello_connect(
		tunnel_client.io_from_klb_binary(client, HEAD_TUNNEL),
		dest_host, tonumber(dest_port), hc_opts)
	local hc_ms = _now_ms() - t_hc
	_phase_add(phases, "hello_connect", hc_ms)
	-- 隧道就绪视为首字节可达 (无额外读)
	_phase_add(phases, "first_byte", hc_ms)
	if not ok_t then
		_disconnect_klb(client)
		session._client = nil
		return false, tostring(err_t)
	end

	-- 所有权交给 stream; session 不再 close 以免双关
	session._client = nil

	local label = tostring(session.kind or "smp") .. "-tunnel"
	local stream = _wrap_klb_stream(client, label, {
		proxy_host = session.proxy_host,
		proxy_port = session.proxy_port,
		dest_host = dest_host,
		dest_port = tonumber(dest_port),
		crypto = (session.crypto == true),
	})

	return true, stream
end


local function _klb_handshake(proxy_host, proxy_port, opts)
	local ok_s, sess = _klb_open_session(proxy_host, proxy_port, opts, nil)
	if not ok_s then
		return false, sess
	end

	if type(sess.close) == "function" then
		sess.close()
	end

	return true
end


local function _klb_connect(proxy_host, proxy_port, dest_host, dest_port, opts, phases)
	local ok_s, sess = _klb_open_session(proxy_host, proxy_port, opts, phases)
	if not ok_s then
		return false, sess
	end

	return _klb_co_tunnel(sess, dest_host, dest_port, opts, phases)
end


-- @brief 仅 SMP/MNP 探活 (connect + auth); 帧在 C 连接层
M.handshake = function (proxy_host, proxy_port, opts)
	if type(proxy_host) ~= "string" or proxy_host == "" then
		return false, "bad proxy_host"
	end

	if tonumber(proxy_port) == nil then
		return false, "bad proxy_port"
	end

	local timeout_ms = 5000
	if type(opts) == "table" and opts.timeout_ms ~= nil then
		timeout_ms = tonumber(opts.timeout_ms) or timeout_ms
	end

	return _run_in_co(timeout_ms, function ()
		return _klb_handshake(proxy_host, tonumber(proxy_port), opts)
	end)
end


-- @brief 建鉴权会话 (不 CONNECT; 须在 kco 协程内; 供 hop 长连池)
M.open_session = function (proxy_host, proxy_port, opts)
	if type(proxy_host) ~= "string" or proxy_host == "" then
		return false, "bad proxy_host"
	end

	if tonumber(proxy_port) == nil then
		return false, "bad proxy_port"
	end

	return _klb_open_session(proxy_host, tonumber(proxy_port), opts, nil)
end


-- @brief 建鉴权会话并填 phases (须在 kco 协程内)
-- @return ok, session|err, phases
M.open_session_timed = function (proxy_host, proxy_port, opts)
	if type(proxy_host) ~= "string" or proxy_host == "" then
		return false, "bad proxy_host", { map = {} }
	end

	if tonumber(proxy_port) == nil then
		return false, "bad proxy_port", { map = {} }
	end

	local phases = { map = {} }
	local ok, sess = _klb_open_session(proxy_host, tonumber(proxy_port), opts, phases)
	return ok, sess, phases
end


-- @brief 在已鉴权会话上开隧道 (须在 kco 协程内; 消耗 session)
M.co_tunnel = function (session, dest_host, dest_port, opts)
	if type(dest_host) ~= "string" or dest_host == "" then
		return false, "bad dest_host"
	end

	if tonumber(dest_port) == nil then
		return false, "bad dest_port"
	end

	return _klb_co_tunnel(session, dest_host, tonumber(dest_port), opts, nil)
end


-- @brief 开隧道并追加 phases (须在 kco 协程内)
-- @return ok, stream|err, phases
M.co_tunnel_timed = function (session, dest_host, dest_port, opts, phases)
	if type(dest_host) ~= "string" or dest_host == "" then
		return false, "bad dest_host", phases or { map = {} }
	end

	if tonumber(dest_port) == nil then
		return false, "bad dest_port", phases or { map = {} }
	end

	if type(phases) ~= "table" then
		phases = { map = {} }
	end

	local ok, stream = _klb_co_tunnel(session, dest_host, tonumber(dest_port), opts, phases)
	return ok, stream, phases
end


-- @brief 经 smp/mnp[/s] hop 打开到目标的隧道 (须在 kco 协程内)
M.connect = function (proxy_host, proxy_port, dest_host, dest_port, opts)
	if type(proxy_host) ~= "string" or proxy_host == "" then
		return false, "bad proxy_host"
	end

	if tonumber(proxy_port) == nil then
		return false, "bad proxy_port"
	end

	if type(dest_host) ~= "string" or dest_host == "" then
		return false, "bad dest_host"
	end

	if tonumber(dest_port) == nil then
		return false, "bad dest_port"
	end

	-- 须在 kco 协程内 (entry fork / 调用方 fork); 勿再套 _run_in_co 以免嵌套泵送
	return _klb_connect(proxy_host, tonumber(proxy_port), dest_host, tonumber(dest_port), opts, nil)
end


-- @brief 建隧道并返回 phases (须在 kco 协程内)
-- @return ok, stream|err, phases
M.co_connect_timed = function (proxy_host, proxy_port, dest_host, dest_port, opts)
	local phases = { map = {} }
	if type(proxy_host) ~= "string" or proxy_host == "" then
		return false, "bad proxy_host", phases
	end

	if tonumber(proxy_port) == nil then
		return false, "bad proxy_port", phases
	end

	if type(dest_host) ~= "string" or dest_host == "" then
		return false, "bad dest_host", phases
	end

	if tonumber(dest_port) == nil then
		return false, "bad dest_port", phases
	end

	local ok, stream = _klb_connect(
		proxy_host, tonumber(proxy_port), dest_host, tonumber(dest_port), opts, phases)
	return ok, stream, phases
end


return M

-- end
