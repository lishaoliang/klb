--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   transport.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  remote 传输层 (HTTP(S) + WS/WSS + SMP/MNP[+TLS] + *rpc)
-- @note   peer_cert / http_post_once / ws_call_once / smp_call_once / call_remote / peer_tool / build_stage
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; ws/wss 经 klbws 同 envelope; co_recv 忽略 KLB_SOCKET_CONNECT(63)/WBUF_EMPTY(67536); smp/mnp/smps/mnps TEXT; smprpc/mnprpc; PC 默认 CRYPTO CHACHA20/44; embedded 默认 AES_CTR/24
--]]


local cjson = require("cjson")
local dispatch = require("klbagent.core.dispatch")
local codes = require("klbagent.code")
local log = require("klbagent.core.log")
local util = require("klbagent.core.tools.remote.util")


local M = {}

-- kws 网络码: 连接成功 / 写缓存空 — 非致命, 与 httpclient:co_recv_text 一致
local KLB_SOCKET_CONNECT = 63
local KLB_NETCODE_WBUF_EMPTY = 67536


-- @return soft[boolean]  true=可继续收
local function _ws_soft_error(msg, head)
	if "error" ~= msg then
		return false
	end

	local code = tonumber(head) or 0
	if KLB_SOCKET_CONNECT == code or KLB_NETCODE_WBUF_EMPTY == code then
		return true
	end

	return false
end


-- 读取对端证书 SHA256 (openssl); 失败返回 nil
function M.peer_cert_sha256(host, port)
	local cmd = string.format(
		"echo | openssl s_client -connect %s:%d -servername %s 2>/dev/null | openssl x509 -fingerprint -sha256 -noout 2>/dev/null",
		tostring(host), tonumber(port) or 0, tostring(host))
	local f = io.popen(cmd, "r")
	if f == nil then
		return nil
	end

	local out = f:read("*a") or ""
	f:close()
	local fp = string.match(out, "Fingerprint=([%w:]+)")
	return util.norm_fp(fp)
end


function M.build_stage(source_tree, stage_root)
	local deploy = require("klbagent.core.tools.deploy")
	if type(deploy) ~= "table" or type(deploy.build_stage) ~= "function" then
		return false, "need klbagent.core.tools.deploy.build_stage"
	end

	return deploy.build_stage(source_tree, stage_root)
end


local function _pump_until(state, timeout_sec)
	local co_util = require("klbagent.core.co_util")
	local scheme = "call"
	if type(state) == "table" and type(state.scheme) == "string" and state.scheme ~= "" then
		scheme = state.scheme
	end

	-- 须在 kco / co_main_loop 下; await_done 用 co_sleep
	co_util.await_done(state, timeout_sec, "remote " .. scheme)
end


-- @brief 给 peer 响应补 hop 字段; 清洗 uart 错误
local function _annotate_peer_obj(obj, host, port, name, scheme)
	obj.host = host
	obj.port = port
	obj.scheme = scheme
	obj.hop = "remote"
	obj.peer_tool = name

	if obj.ok == false then
		obj.peer_code = obj.code
		obj.peer_name = obj.name
		local msg = obj.message
		if type(msg) == "string" then
			local tag = string.match(msg, "uart:([%w_]+)")
			if tag ~= nil then
				obj.uart_err = tag
				local clean = string.match(msg, "(uart:[%w_]+[:]?[^%c]*)")
				if type(clean) == "string" and clean ~= "" then
					obj.message = clean
				end
			elseif string.sub(msg, 1, 7) ~= "remote:" then
				obj.peer_message = msg
			end
		end
		log.write("warn", "remote peer tool fail", {
			module = "remote",
			host = host,
			port = port,
			tool = name,
			scheme = scheme,
			peer_code = obj.peer_code,
			uart_err = obj.uart_err or "",
			message = tostring(obj.message or ""),
		})
	end

	return obj
end


-- 单次 POST; 返回 ok_transport, http_res_or_err
function M.http_post_once(scheme, host, port, name, tool_args, token, timeout_sec, max_bytes, ca_path, session_id, alua_ssid)
	local url = string.format("%s://%s:%d/v1/call", scheme, host, port)
	local payload = {
		name = name,
		args = tool_args or {},
	}
	if type(session_id) == "string" and session_id ~= "" then
		payload.session_id = session_id
	end

	local headers = {}
	if type(token) == "string" and token ~= "" then
		headers["X-Alua-Token"] = token
	end
	if type(alua_ssid) == "string" and alua_ssid ~= "" then
		headers["X-Alua-Ssid"] = alua_ssid
	end

	local post_args = {
		url = url,
		json = payload,
		timeout_sec = timeout_sec or 30,
		max_bytes = max_bytes or (2 * 1024 * 1024),
	}
	if next(headers) ~= nil then
		post_args.headers = headers
	end

	if scheme == "https" and type(ca_path) == "string" and ca_path ~= "" then
		post_args.ca_path = ca_path
	end

	local ok_h, http_res = dispatch.call({
		name = "net.http_post",
		args = post_args,
	})
	return ok_h, http_res
end


-- @brief 单次 WS/WSS: 握手 /v1/ws + 文本 envelope; 须 kco 泵
-- @return ok[boolean], obj_or_err[table|string]
function M.ws_call_once(scheme, host, port, name, tool_args, token, timeout_sec, session_id, alua_ssid)
	local kco = require("kco")
	local klbws = require("klbcore.klbws")
	if type(klbws) ~= "table" or type(klbws.connect) ~= "function" then
		return false, "klbws.connect unavailable"
	end

	local state = {
		done = false,
		ok = false,
		obj = nil,
		err = nil,
		scheme = scheme,
	}

	kco.fork(function ()
		local client = nil
		local ok_body, err_body = pcall(function ()
			local headers = {}
			if type(token) == "string" and token ~= "" then
				headers["X-Alua-Token"] = token
			end
			if type(alua_ssid) == "string" and alua_ssid ~= "" then
				headers["X-Alua-Ssid"] = alua_ssid
			end

			client = klbws.connect(host, port, "/v1/ws", {
				tls = (scheme == "wss"),
				headers = headers,
			})
			if client == nil then
				error("ws connect fail")
			end

			-- 等到 open (或直接 text); 跳过 CONNECT(63)/WBUF_EMPTY(67536)
			local opened = false
			for _ = 1, 16 do
				local msg, head, body = client:co_recv()
				if "open" == msg then
					opened = true
					break
				end

				if "text" == msg then
					-- 异常: 握手后已有帧
					opened = true
					local ok_j, obj = pcall(cjson.decode, body or "{}")
					if ok_j and type(obj) == "table" then
						state.ok = true
						state.obj = _annotate_peer_obj(obj, host, port, name, scheme)
						return
					end
					error("ws unexpected text before request")
				end

				if _ws_soft_error(msg, head) then
					-- 继续
				elseif "pong" == msg then
					-- 继续
				elseif "error" == msg or "exit" == msg or "close" == msg then
					error("ws early " .. tostring(msg) .. "/" .. tostring(head))
				end
			end

			if not opened then
				error("ws open timeout")
			end

			local req_obj = {
				name = name,
				args = tool_args or {},
			}
			if type(session_id) == "string" and session_id ~= "" then
				req_obj.session_id = session_id
			end

			local req_text = cjson.encode(req_obj)
			local send_rc = client:send_text(req_text)
			if 0 ~= send_rc then
				error("ws send fail")
			end

			local body = nil
			local got_text = false
			for _ = 1, 16 do
				local msg, head, b = client:co_recv()
				if "text" == msg then
					body = b
					got_text = true
					break
				end

				if _ws_soft_error(msg, head) then
					-- 继续
				elseif "pong" == msg or "open" == msg then
					-- 继续
				elseif "error" == msg or "exit" == msg or "close" == msg then
					error("ws recv bad: " .. tostring(msg) .. "/" .. tostring(head))
				end
			end

			if not got_text then
				error("ws recv timeout")
			end

			local ok_j, obj = pcall(cjson.decode, body or "")
			if not ok_j or type(obj) ~= "table" then
				error("ws bad json")
			end

			state.ok = true
			state.obj = _annotate_peer_obj(obj, host, port, name, scheme)
		end)

		if client ~= nil then
			pcall(function ()
				client:disconnect()
			end)
		end

		if not ok_body then
			state.err = tostring(err_body)
			log.write("warn", "remote ws call fail", {
				module = "remote",
				host = host,
				port = port,
				tool = name,
				scheme = scheme,
				err = state.err,
			})
		end

		state.done = true
	end)

	_pump_until(state, timeout_sec)

	if state.err ~= nil then
		return false, state.err
	end

	if state.ok and type(state.obj) == "table" then
		return true, state.obj
	end

	return false, "ws call fail"
end


local function _smp_soft_error(msg, head)
	if "error" ~= msg then
		return false
	end

	local code = tonumber(head) or 0
	if KLB_SOCKET_CONNECT == code or KLB_NETCODE_WBUF_EMPTY == code then
		return true
	end

	return false
end


-- @brief PC 宿主 (非 embedded) 时为 true; 探测失败按 PC 处理
local function _host_is_pc()
	local ok, hostinfo = pcall(require, "klbagent.core.hostinfo")
	if not ok or type(hostinfo) ~= "table" or type(hostinfo.probe) ~= "function" then
		return true
	end

	local hi = hostinfo.probe()
	local kind = (type(hi) == "table" and type(hi.host_kind) == "string") and hi.host_kind or "linux"
	return kind ~= "embedded"
end


-- @brief 按 host_kind 默认 CRYPTO: PC=CHACHA20/44; embedded=AES_CTR/24
local function _default_smp_crypto()
	if _host_is_pc() then
		return { format = "CHACHA20", key_size = 44 }
	end

	-- embedded: 相对低 CPU、仍有 AES-128 流式强度
	return { format = "AES_CTR", key_size = 24 }
end


-- @brief 连接 klbsmp/klbmnp; tls 时须 C 侧 connect(..., {tls=true})
-- \n 默认附带 crypto(PC=CHACHA20/44; embedded=AES_CTR/24); 与 tls 正交
local function _smp_connect(scheme, host, port)
	local use_tls = (scheme == "smps" or scheme == "mnps")
	local use_mnp = (scheme == "mnp" or scheme == "mnps")
	local opts = {}
	local crypto = _default_smp_crypto()

	if use_tls then
		opts.tls = true
	end

	if crypto ~= nil then
		opts.crypto = crypto
	end

	if next(opts) == nil then
		opts = nil
	end

	if use_mnp then
		local klbmnp = require("klbcore.klbmnp")
		if type(klbmnp) ~= "table" or type(klbmnp.connect) ~= "function" then
			return nil, "klbmnp.connect unavailable"
		end

		local client = klbmnp.connect(host, port, opts)
		if client == nil then
			return nil, "mnp connect fail"
		end

		return client, nil
	end

	local klbsmp = require("klbcore.klbsmp")
	if type(klbsmp) ~= "table" or type(klbsmp.connect) ~= "function" then
		return nil, "klbsmp.connect unavailable"
	end

	local client = klbsmp.connect(host, port, opts)
	if client == nil then
		return nil, "smp connect fail"
	end

	return client, nil
end


-- @brief 单次 SMP/MNP TEXT: alua.auth + alua.call (同 /v1/call envelope)
-- @return ok[boolean], obj_or_err[table|string]
function M.smp_call_once(scheme, host, port, name, tool_args, token, timeout_sec, session_id, alua_ssid)
	local kco = require("kco")
	local state = {
		done = false,
		ok = false,
		obj = nil,
		err = nil,
		scheme = scheme,
	}

	kco.fork(function ()
		local client = nil
		local ok_body, err_body = pcall(function ()
			local c, err_c = _smp_connect(scheme, host, port)
			if c == nil then
				error(tostring(err_c or "smp connect fail"))
			end

			client = c

			local auth_obj = { token = token or "" }
			if type(alua_ssid) == "string" and alua_ssid ~= "" then
				auth_obj.alua_ssid = alua_ssid
			end
			local auth_body = cjson.encode(auth_obj)
			local send_rc = client:send_text("alua.auth", auth_body)
			if 0 ~= send_rc then
				error("smp auth send fail")
			end

			local auth_ok = false
			for _ = 1, 16 do
				local msg, head, body = client:co_recv()
				if "text" == msg and head == "alua.auth" then
					local ok_j, aobj = pcall(cjson.decode, body or "{}")
					if ok_j and type(aobj) == "table" and aobj.ok == true then
						auth_ok = true
						break
					end

					error("smp auth deny: " .. tostring(body))
				end

				if _smp_soft_error(msg, head) then
					-- 继续
				elseif "pong" == msg then
					-- 继续
				elseif "error" == msg or "exit" == msg or "close" == msg then
					error("smp auth early " .. tostring(msg) .. "/" .. tostring(head))
				end
			end

			if not auth_ok then
				error("smp auth timeout")
			end

			local req_obj = {
				name = name,
				args = tool_args or {},
			}
			if type(session_id) == "string" and session_id ~= "" then
				req_obj.session_id = session_id
			end

			local req_text = cjson.encode(req_obj)
			send_rc = client:send_text("alua.call", req_text)
			if 0 ~= send_rc then
				error("smp call send fail")
			end

			local body = nil
			local got = false
			for _ = 1, 16 do
				local msg, head, b = client:co_recv()
				if "text" == msg and (head == "alua.call" or head == "alua.auth" or head == nil or head == "") then
					body = b
					got = true
					break
				end

				if "text" == msg then
					body = b
					got = true
					break
				end

				if _smp_soft_error(msg, head) then
					-- 继续
				elseif "pong" == msg then
					-- 继续
				elseif "error" == msg or "exit" == msg or "close" == msg then
					error("smp recv bad: " .. tostring(msg) .. "/" .. tostring(head))
				end
			end

			if not got then
				error("smp recv timeout")
			end

			local ok_j, obj = pcall(cjson.decode, body or "")
			if not ok_j or type(obj) ~= "table" then
				error("smp bad json")
			end

			state.ok = true
			state.obj = _annotate_peer_obj(obj, host, port, name, scheme)
		end)

		if client ~= nil then
			pcall(function ()
				client:disconnect()
			end)
		end

		if not ok_body then
			state.err = tostring(err_body)
			log.write("warn", "remote smp call fail", {
				module = "remote",
				host = host,
				port = port,
				tool = name,
				scheme = scheme,
				err = state.err,
			})
		end

		state.done = true
	end)

	_pump_until(state, timeout_sec)

	if state.err ~= nil then
		return false, state.err
	end

	if state.ok and type(state.obj) == "table" then
		return true, state.obj
	end

	return false, "smp call fail"
end


-- @brief 单次 smprpc/mnprpc (与 remote.rpc_call 同会话)
-- @return ok[boolean], obj_or_err[table|string]
function M.rpc_call_once(scheme, host, port, name, tool_args, token, timeout_sec)
	local kco = require("kco")
	local state = {
		done = false,
		ok = false,
		obj = nil,
		err = nil,
		scheme = scheme,
	}

	local transport = (scheme == "mnprpc") and "mnprpc" or "smprpc"

	kco.fork(function ()
		local ok_body, err_body = pcall(function ()
			local client = nil
			if transport == "mnprpc" then
				local klbmnp = require("klbcore.klbmnp")
				if type(klbmnp) ~= "table" or type(klbmnp.connect_rpc) ~= "function" then
					error("klbmnp.connect_rpc unavailable")
				end

				client = klbmnp.connect_rpc(host, port)
			else
				local klbsmp = require("klbcore.klbsmp")
				if type(klbsmp) ~= "table" or type(klbsmp.connect_rpc) ~= "function" then
					error("klbsmp.connect_rpc unavailable")
				end

				client = klbsmp.connect_rpc(host, port)
			end

			if client == nil then
				error("connect_rpc fail")
			end

			local a1, a2 = client:co_call("alua.auth", token)
			if a1 ~= true then
				pcall(function ()
					client:disconnect()
				end)
				error("auth fail: " .. tostring(a1) .. " " .. tostring(a2))
			end

			local p1, p2 = client:co_call(name, tool_args)
			pcall(function ()
				client:disconnect()
			end)

			if p1 == true then
				state.ok = true
				state.obj = _annotate_peer_obj({
					ok = true,
					result = p2,
				}, host, port, name, scheme)
			else
				local message = p2
				local peer_code = nil
				local peer_name = nil
				if type(p2) == "table" then
					message = p2.message or p2
					peer_code = p2.code
					peer_name = p2.name
				end

				state.ok = true
				state.obj = _annotate_peer_obj({
					ok = false,
					code = codes.tools.TOOL_FAILED,
					message = tostring(message or "rpc tool fail"),
					name = codes.str(codes.tools.TOOL_FAILED),
					peer_code = peer_code,
					peer_name = peer_name,
				}, host, port, name, scheme)
			end
		end)

		if not ok_body then
			state.err = tostring(err_body)
			log.write("warn", "remote rpc scheme call fail", {
				module = "remote",
				host = host,
				port = port,
				tool = name,
				scheme = scheme,
				err = state.err,
			})
		end

		state.done = true
	end)

	_pump_until(state, timeout_sec)

	if state.err ~= nil then
		return false, state.err
	end

	if state.ok and type(state.obj) == "table" then
		return true, state.obj
	end

	return false, "rpc call fail"
end


local function _http_try_once(s, host, port, name, tool_args, token, timeout_sec, max_bytes, ca_path, session_id, alua_ssid)
	local ok_h, http_res = M.http_post_once(
		s, host, port, name, tool_args, token, timeout_sec, max_bytes, ca_path, session_id, alua_ssid)
	if not ok_h then
		if type(http_res) == "table" then
			return false, http_res.message or "http_post fail", true
		end

		return false, "http_post fail", true
	end

	local status = http_res.status or 0
	local body = http_res.body or ""
	if status < 200 or 300 <= status then
		log.write("warn", "remote http status", {
			module = "remote",
			host = host,
			port = port,
			tool = name,
			scheme = s,
			http_status = status,
		})
		return true, {
			ok = false,
			code = codes.tools.TOOL_FAILED,
			message = "remote:http_status:" .. tostring(status),
			name = codes.str(codes.tools.TOOL_FAILED),
			hop = "remote",
			peer_tool = name,
			http_status = status,
			body = body,
			host = host,
			port = port,
			scheme = s,
		}, false
	end

	local ok_j, obj = pcall(cjson.decode, body)
	if not ok_j or type(obj) ~= "table" then
		log.write("warn", "remote response not json", {
			module = "remote",
			host = host,
			port = port,
			tool = name,
			scheme = s,
		})
		return true, {
			ok = false,
			code = codes.tools.TOOL_FAILED,
			message = "remote:bad_json",
			name = codes.str(codes.tools.TOOL_FAILED),
			hop = "remote",
			peer_tool = name,
			body = body,
			host = host,
			port = port,
			scheme = s,
		}, false
	end

	return true, _annotate_peer_obj(obj, host, port, name, s), false
end


-- @brief 远程调用; 缺省 mnps(TLS+MNP+CRYPTO); 显式不回落; auto 仅 https→http
-- @param session_id[string|nil]		透传到 peer 信封顶层 (http/ws/smp TEXT)
-- @param alua_ssid[string|nil]		证明票所属 ssid; 有则随 auth 帧/头发送
function M.call_remote(host, port, name, tool_args, token, timeout_sec, max_bytes, scheme, pin, ca_path, session_id, alua_ssid)
	local schemes = util.scheme_list(scheme)
	if pin ~= nil then
		-- 钉扎强制 TLS 侧
		local only = {}
		for i = 1, #schemes do
			if util.scheme_is_tls(schemes[i]) then
				only[#only + 1] = schemes[i]
			end
		end

		if #only == 0 then
			only = { "mnps" }
		end

		schemes = only
	end

	local last_err = "remote transport fail"

	for i = 1, #schemes do
		local s = schemes[i]
		if util.scheme_is_tls(s) and pin ~= nil then
			local got = M.peer_cert_sha256(host, port)
			if got == nil then
				error("cert fingerprint unavailable")
			end

			if got ~= pin then
				error("cert fingerprint mismatch (pin=" .. pin .. " got=" .. tostring(got) .. ")")
			end
		end

		if util.scheme_is_ws(s) then
			local ok_w, obj_or_err = M.ws_call_once(
				s, host, port, name, tool_args, token, timeout_sec, session_id, alua_ssid)
			if ok_w then
				return obj_or_err
			end

			last_err = tostring(obj_or_err or "ws fail")
			if #schemes == 1 then
				break
			end
		elseif util.scheme_is_smp(s) then
			local ok_s, obj_or_err = M.smp_call_once(
				s, host, port, name, tool_args, token, timeout_sec, session_id, alua_ssid)
			if ok_s then
				return obj_or_err
			end

			last_err = tostring(obj_or_err or "smp fail")
			if #schemes == 1 then
				break
			end
		elseif util.scheme_is_rpc(s) then
			local ok_r, obj_or_err = M.rpc_call_once(
				s, host, port, name, tool_args, token, timeout_sec)
			if ok_r then
				return obj_or_err
			end

			last_err = tostring(obj_or_err or "rpc fail")
			if #schemes == 1 then
				break
			end
		else
			local got_resp, obj_or_err, is_transport_fail = _http_try_once(
				s, host, port, name, tool_args, token, timeout_sec, max_bytes, ca_path, session_id, alua_ssid)
			if got_resp then
				return obj_or_err
			end

			last_err = tostring(obj_or_err or "http_post fail")
			-- 仅传输失败才换下一 scheme
			if not is_transport_fail then
				break
			end
		end
	end

	log.write("error", "remote call transport fail", {
		module = "remote",
		host = host,
		port = port,
		tool = name,
		err = tostring(last_err),
	})
	error(last_err)
end


-- 兼容旧名
function M.http_call_remote(host, port, name, tool_args, token, timeout_sec, max_bytes, scheme, pin, ca_path, session_id, alua_ssid)
	return M.call_remote(host, port, name, tool_args, token, timeout_sec, max_bytes, scheme, pin, ca_path, session_id, alua_ssid)
end


function M.peer_tool(host, port, token, timeout_sec, name, tool_args, max_bytes, scheme, pin, ca_path, alua_ssid)
	local resp = M.call_remote(host, port, name, tool_args, token, timeout_sec, max_bytes, scheme, pin, ca_path, nil, alua_ssid)
	if type(resp) ~= "table" then
		error("remote bad response")
	end

	if resp.ok ~= true then
		error(resp.message or ("remote " .. name .. " failed"))
	end

	if type(resp.result) ~= "table" then
		error("remote " .. name .. " missing result")
	end

	return resp.result, resp.scheme
end


return M

-- end
