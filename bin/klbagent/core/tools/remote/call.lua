--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   call.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  remote.call
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; 透传 peer 信封 session_id (Bug.2026-7); 支持 peer= 解析 (须 trusted)
--  \n [2026-11] M1-M5: ssid 内沉取证; DENIED 一次 re-ensure 重试
--]]


local transport = require("klbagent.core.tools.remote.transport")
local util = require("klbagent.core.tools.remote.util")


local M = {}


local function _call_once(args)
	local host = args.host
	if type(host) ~= "string" or host == "" then
		error("need host or peer")
	end

	local name = args.name
	if type(name) ~= "string" or name == "" then
		error("need name")
	end

	local port = args.port
	if type(port) ~= "number" or port < 1 or 65535 < port then
		error("alua.core.serve.UNAVAILABLE: need port")
	end

	local tool_args = args.args
	if type(tool_args) ~= "table" then
		tool_args = {}
	end

	local timeout_sec = args.timeout_sec
	if type(timeout_sec) ~= "number" or timeout_sec <= 0 then
		timeout_sec = 30
	end

	local token = util.resolve_call_token(args)
	if type(token) ~= "string" or token == "" then
		error("need token (ssid ensure failed; set serve.env / allow_raw_token)")
	end

	local session_id = args.session_id
	if type(session_id) ~= "string" or session_id == "" then
		session_id = nil
	end

	local scheme = util.normalize_scheme(args.scheme)

	-- 原始 token 仅经显式 TLS scheme 且发往本机/已登记 peers
	-- serve.token_expect 包外已收口; 无法判定 raw↔TLS

	local ok_raw, err_raw = util.check_raw_target(host, token, args._peer_row)
	if not ok_raw then
		error(err_raw)
	end

	local pin = util.resolve_pin(args)
	local ca_path = util.resolve_ca(args)
	-- 证明票(p.)须随 auth 帧带 alua_ssid 供服务端重算校验
	local alua_ssid = args._resolved_ssid or util.current_alua_ssid(args)
	return transport.call_remote(
		host, port, name, tool_args, token, timeout_sec, args.max_bytes, scheme, pin, ca_path, session_id, alua_ssid)
end


local function _call(args)
	args = util.expand_peer_args(args)

	local ok, resp_or_err = pcall(_call_once, args)
	if ok then
		-- 业务面 DENIED: 一次 force re-ensure 后重试 (M5)
		if type(resp_or_err) == "table" and resp_or_err.ok == false
			and util.is_auth_denied(resp_or_err) and args._retried_auth ~= true then
			args._retried_auth = true
			args._force_renew = true
			args.token = nil
			return _call_once(args)
		end

		return resp_or_err
	end

	if util.is_auth_denied(resp_or_err) and args._retried_auth ~= true then
		args._retried_auth = true
		args._force_renew = true
		args.token = nil
		return _call_once(args)
	end

	error(resp_or_err)
end


M.register = function (catalog, dispatch_reg)
	local ok, err = catalog.register({
		name = "remote.call",
		category = "net",
		title = "call remote alua serve",
		summary = "Call remote alua serve via HTTP(S)/WS(S)/SMP(S)/MNP(S)/*rpc. Needs profile net. Default port 12817. Default scheme=mnps. Auth via ssid only (no token_ref). Optional peer=id (trusted).",
		min_profile = "net",
		args_schema = {
			host = "string optional remote host (or use peer=)",
			peer = "string optional trusted peer id (linux|wsl|windows); fills host/port/scheme/pin",
			port = "number optional (default 12817)",
			name = "string remote tool name",
			args = "table optional remote tool args",
			session_id = "string optional peer envelope session_id (process-local on peer)",
			id = "any optional request id",
			alua_ssid = "string optional bind ssid (default current MCP/session); unique auth entry",
			token = "string optional debug only (normally omit; ssid ensure)",
			allow_raw_token = "boolean optional allow raw serve.env on TLS only (default false)",
			scheme = "string optional auto|https|http|ws|wss|smp|mnp|smps|mnps|smprpc|mnprpc (default mnps; auto=https→http; explicit no fallback; raw token needs explicit TLS scheme, auto rejected)",
			cert_sha256 = "string optional peer cert SHA256 pin (else peers.json / ALUA_SERVE_CERT_SHA256); forces TLS side",
			ca_path = "string optional CA PEM path (else ALUA_SERVE_CA); curl --cacert (http path)",
			timeout_sec = "number optional (default 30)",
			max_bytes = "number optional response cap",
		},
		examples = {
			{
				args = {
					host = "127.0.0.1",
					port = 22817,
					name = "runtime.ping",
					args = {},
				},
			},
			{
				args = {
					peer = "windows",
					name = "runtime.ping",
					args = {},
				},
			},
			{
				args = {
					host = "127.0.0.1",
					port = 12817,
					scheme = "wss",
					name = "runtime.ping",
					args = {},
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("remote.call", _call)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
