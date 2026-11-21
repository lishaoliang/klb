--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   server.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  代理入口 SMP/MNP (明文/密文均经 C 连接层)
-- @note   accept: kproxy.tcp steal → ksmp/kmnp.new_serve → accept_session
-- \n       accept_session / wrap_session: 已有 klbmnp/klbsmp 会话
-- @history 修改历史
--  \n [2026-10] 创建文件 (N5 P2 / M1); wrap_session: 已鉴权 mnps 会话导出 (供 alua serve 出口隧道); 心跳仅 C: _tunnel_stream 不发 Ping; 明文同口亦走 C new_serve; 废弃 Lua 帧拼装; adopt 经 klbsmp/klbmnp (禁直调 ksmp/kmnp)
--]]


local smpframe = require("klbagent.packages.proxy.transport.smp.frame")
local kproxy_io = require("klbcore.klbproxy.io")
local klbsmp = require("klbcore.klbsmp")
local klbmnp = require("klbcore.klbmnp")


local M = {}


local HEAD_AUTH = "alua.auth"
local HEAD_TUNNEL = "alua-tunnel"


local function _auth_mode(opts)
	if type(opts) ~= "table" or type(opts.auth) ~= "table" then
		return "none"
	end

	local mode = opts.auth.mode
	if type(mode) == "string" and mode ~= "" then
		return string.lower(mode)
	end

	return "none"
end


local function _check_token(token, opts)
	local mode = _auth_mode(opts)
	if mode == "none" or mode == "" then
		return true
	end

	local expect = ""
	if type(opts) == "table" and type(opts.auth) == "table" then
		if type(opts.auth.token) == "string" then
			expect = opts.auth.token
		else
			local user = tostring(opts.auth.user or "")
			local pass = tostring(opts.auth.pass or "")
			expect = user .. ":" .. pass
		end
	end

	return tostring(token or "") == expect
end


local function _token_from_body(body)
	if type(body) ~= "string" or body == "" then
		return ""
	end

	local ok_j, cjson = pcall(require, "cjson")
	if ok_j and type(cjson) == "table" and type(cjson.decode) == "function" then
		local ok_d, obj = pcall(cjson.decode, body)
		if ok_d and type(obj) == "table" and type(obj.token) == "string" then
			return obj.token
		end
	end

	return body
end


local function _session_tunnel_stream(serve, prime_bin)
	local data_leftover = ""
	if type(prime_bin) == "string" and prime_bin ~= "" then
		data_leftover = prime_bin
	end

	local stream = {
		kind = "smp-session",
		_serve = serve,
	}

	-- C 层吞 PING/PONG; 经 last_recv_ms 供泵送刷新空闲
	stream.idle_touch_ms = function ()
		local s = stream._serve
		if s ~= nil and type(s.last_recv_ms) == "function" then
			local ok, v = pcall(function ()
				return s:last_recv_ms()
			end)
			if ok and type(v) == "number" and v > 0 then
				return v
			end
		end
		return stream._idle_touch_ms
	end

	stream.close = function ()
		if serve ~= nil and type(serve.disconnect) == "function" then
			pcall(function ()
				serve:disconnect()
			end)
		end
		stream._serve = nil
	end

	stream.send = function (_, data)
		if stream._serve == nil then
			return false, "closed"
		end

		local rc = stream._serve:send_binary(HEAD_TUNNEL, data or "")
		return 0 == rc
	end

	stream.recv = function (_, max_n)
		if stream._serve == nil then
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
			local msg, head, body = stream._serve:co_recv()
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
				if type(stream._serve.free_media) == "function" then
					pcall(function ()
						stream._serve:free_media(head)
					end)
				end
			elseif msg == "pong" or msg == "text" then
				-- C 吞心跳; 偶发 pong 上抛则忽略
			elseif msg == "error" or msg == "exit" or msg == "close" then
				-- error 的第二返回为 klb_netcode 码 (如 -104 ECONNRESET); 带上便于诊断
				if msg == "error" and head ~= nil then
					return nil, "error:" .. tostring(head)
				end
				return nil, tostring(msg)
			end
		end

		return nil, "timeout"
	end

	return stream
end


-- @brief 窥探满 4 字节魔数, 区分 smp/mnp (不消费)
local function _peek_kind(tcp, kind_hint)
	if kind_hint == "smp" or kind_hint == "mnp" then
		return kind_hint
	end

	local ok_c, kco = pcall(require, "kco")
	local rounds = 50
	for _ = 1, rounds do
		local buf, err = kproxy_io.recv_peek(tcp, 4)
		if type(buf) == "string" and #buf >= 4 then
			local kind = smpframe.classify_magic(buf)
			if kind == "smp" or kind == "mnp" then
				return kind
			end

			return nil, "bad smp magic"
		end

		if err == "eof" or err == "closed" then
			return nil, err or "eof"
		end

		if ok_c and type(kco) == "table" and type(kco.co_sleep) == "function" then
			kco.co_sleep(10)
		else
			return nil, err or "need 4 magic bytes"
		end
	end

	return nil, "magic peek timeout"
end


-- @brief kproxy.tcp → C smp/mnp serve (经 klbcore.adopt; 帧/心跳在连接层)
local function _adopt_tcp(tcp, kind)
	if tcp == nil or type(tcp.steal_listen_socket) ~= "function" then
		return nil, "need kproxy.tcp steal_listen_socket"
	end

	local sock, err_s = tcp:steal_listen_socket()
	if sock == nil then
		return nil, tostring(err_s or "steal fail")
	end

	if kind == "mnp" then
		local serve, code = klbmnp.adopt(sock)
		if serve == nil then
			return nil, "klbmnp.adopt fail code=" .. tostring(code)
		end
		return serve, nil
	end

	local serve, code = klbsmp.adopt(sock)
	if serve == nil then
		return nil, "klbsmp.adopt fail code=" .. tostring(code)
	end
	return serve, nil
end


-- @brief klbmnp/klbsmp 会话鉴权, 返回可喂给 tunnel.server 的 stream
-- @param [in]  serve[table]	mnpserver/smpserver (send_text/co_recv)
-- @param [in]  opts[table|nil]	`{ auth }`
-- @return [boolean] ok
-- @return [table|string] stream 或 err
M.accept_session = function (serve, opts)
	if serve == nil or type(serve.send_text) ~= "function" or type(serve.co_recv) ~= "function" then
		return false, "need mnp/smp serve"
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	local got = false
	local token = ""
	for _ = 1, 16 do
		local msg, head, body = serve:co_recv()
		if msg == "text" and head == HEAD_AUTH then
			token = _token_from_body(body)
			got = true
			break
		end

		if msg == "error" or msg == "exit" or msg == "close" then
			return false, "auth early " .. tostring(msg)
		end
	end

	if not got then
		pcall(function ()
			serve:send_text(HEAD_AUTH, '{"ok":false,"message":"need alua.auth"}')
		end)
		return false, "need alua.auth"
	end

	if not _check_token(token, opts) then
		pcall(function ()
			serve:send_text(HEAD_AUTH, '{"ok":false,"message":"bad token"}')
		end)
		return false, "bad token"
	end

	local rc = serve:send_text(HEAD_AUTH, '{"ok":true}')
	if 0 ~= rc then
		return false, "auth ok send fail"
	end

	return true, _session_tunnel_stream(serve)
end


-- @brief 已鉴权的 klbmnp/klbsmp 会话包装为隧道 stream (供 alua serve mux 出口)
-- @param [in]  serve[table]		mnpserver/smpserver
-- @param [in]  opts[table|nil]		`{ prime[string]=已读 BINARY 体 }`
-- @return [boolean] ok
-- @return [table|string] stream 或 err
M.wrap_session = function (serve, opts)
	if serve == nil or type(serve.send_binary) ~= "function" or type(serve.co_recv) ~= "function" then
		return false, "need mnp/smp serve"
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	local prime = nil
	if type(opts) == "table" and type(opts.prime) == "string" then
		prime = opts.prime
	end

	return true, _session_tunnel_stream(serve, prime)
end


-- @brief 同口 demux 后的 kproxy.tcp → C 连接层鉴权隧道
-- @note 须在 kco 协程内; tcp 上数据须尚未消费 (入口用 recv_peek)
-- @param [in]  client[userdata]	kproxy.tcp
-- @param [in]  opts[table|nil]		`{ auth, kind }`
-- @return [boolean] ok
-- @return [table|string] stream 或 err
M.accept = function (client, opts)
	if client == nil then
		return false, "need client"
	end

	if opts ~= nil and type(opts) ~= "table" then
		return false, "bad opts"
	end

	local kind_hint = nil
	if type(opts) == "table" and (opts.kind == "smp" or opts.kind == "mnp") then
		kind_hint = opts.kind
	end

	local kind, err_k = _peek_kind(client, kind_hint)
	if kind == nil then
		return false, err_k or "bad kind"
	end

	local serve, err_a = _adopt_tcp(client, kind)
	if serve == nil then
		return false, err_a
	end

	return M.accept_session(serve, opts)
end


return M

-- end
