--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rpc_call.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  remote.rpc_call (smprpc/mnprpc 会话 auth+tool)
-- @note   remote.call 仍仅 HTTP(S); 本工具为 R2 一等 RPC 客户端
-- @history 修改历史
--  \n [2026-10] 创建文件 (RPC R2); 支持 peer=; tls 面改走同协议 TEXT (线上无 *rpcs RPC 投递面)
--]]


local codes = require("klbagent.code")
local log = require("klbagent.core.log")
local util = require("klbagent.core.tools.remote.util")
local transport_mod = require("klbagent.core.tools.remote.transport")


local M = {}


local function _normalize_transport(t)
	if type(t) ~= "string" or t == "" then
		return "smprpc"
	end

	local low = string.lower(t)
	if low == "smprpc" or low == "smp" then
		return "smprpc"
	end

	if low == "mnprpc" or low == "mnp" then
		return "mnprpc"
	end

	error("transport must be smprpc|mnprpc")
end


local function _pump_until(state, timeout_sec)
	local co_util = require("klbagent.core.co_util")
	co_util.await_done(state, timeout_sec, "remote.rpc_call")
end


local function _rpc_call(args)
	args = util.expand_peer_args(args)

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

	-- 原始 token 仅可发往本机回环或已信任 peer (与 remote.call 同门槛)
	local ok_raw, err_raw = util.check_raw_target(host, token, args._peer_row)
	if not ok_raw then
		error(err_raw)
	end

	local transport = _normalize_transport(args.transport)
	local tls = args.tls == true

	-- 原始 token 仅可走 MNP RPC (帧加密); smprpc 为明文面禁用
	-- 线上是否 TLS 仍由服务端强制 (嵌入式常无 TLS, 此处不自动升)
	-- serve.token_expect 包外已收口; 无法判定 raw, 强制 mnprpc 由调用方显式选择

	-- TLS 面: C mux 仅向 MNP/SMP TEXT 投递, 线上无 *rpcs RPC 面
	--   \n mnprpc+{tls=true} 会被 TEXT 服务端丢包 (客户端挂起); TLS 改走同协议 TEXT
	if tls then
		local scheme = (transport == "mnprpc") and "mnps" or "smps"
		local resp = transport_mod.call_remote(host, port, name, tool_args, token, timeout_sec, nil, scheme)
		if type(resp) ~= "table" then
			error("remote.rpc_call bad response")
		end

		if resp.ok == true then
			return {
				ok = true,
				result = resp.result,
				host = host,
				port = port,
				transport = transport,
				protocol = scheme,
				tls = true,
				hop = "remote_rpc",
				peer_tool = name,
			}
		end

		local message = resp.message
		if type(message) ~= "string" then
			message = tostring(message or "rpc tool fail")
		end

		return {
			ok = false,
			code = codes.tools.TOOL_FAILED,
			message = message,
			name = codes.str(codes.tools.TOOL_FAILED),
			hop = "remote_rpc",
			peer_tool = name,
			peer_code = resp.peer_code or resp.code,
			peer_name = resp.peer_name or resp.name,
			host = host,
			port = port,
			transport = transport,
			protocol = scheme,
			tls = true,
		}
	end

	local kco = require("kco")
	local state = {
		done = false,
		obj = nil,
		err = nil,
	}

	kco.fork(function ()
		local ok_body, err_body = pcall(function ()
			local client = nil
			if transport == "mnprpc" then
				local klbmnp = require("klbcore.klbmnp")
				if type(klbmnp) ~= "table" or type(klbmnp.connect_rpc) ~= "function" then
					error("klbmnp.connect_rpc unavailable")
				end

				client = klbmnp.connect_rpc(host, port, { tls = tls })
			else
				local klbsmp = require("klbcore.klbsmp")
				if type(klbsmp) ~= "table" or type(klbsmp.connect_rpc) ~= "function" then
					error("klbsmp.connect_rpc unavailable")
				end

				client = klbsmp.connect_rpc(host, port, { tls = tls })
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
				state.obj = {
					ok = true,
					result = p2,
					host = host,
					port = port,
					transport = transport,
					hop = "remote_rpc",
					peer_tool = name,
				}
			else
				local message = p2
				local peer_code = nil
				local peer_name = nil
				if type(p2) == "table" then
					message = p2.message or p2
					peer_code = p2.code
					peer_name = p2.name
				end

				state.obj = {
					ok = false,
					code = codes.tools.TOOL_FAILED,
					message = tostring(message or "rpc tool fail"),
					name = codes.str(codes.tools.TOOL_FAILED),
					hop = "remote_rpc",
					peer_tool = name,
					peer_code = peer_code,
					peer_name = peer_name,
					host = host,
					port = port,
					transport = transport,
				}
			end
		end)

		if not ok_body then
			state.err = tostring(err_body)
			log.write("warn", "remote.rpc_call fail", {
				module = "remote",
				host = host,
				port = port,
				tool = name,
				transport = transport,
				err = state.err,
			})
		end

		state.done = true
	end)

	_pump_until(state, timeout_sec)

	if state.err ~= nil then
		error(state.err)
	end

	if type(state.obj) ~= "table" then
		error("remote.rpc_call no result")
	end

	return state.obj
end


M.register = function (catalog, dispatch_reg)
	local ok, err = catalog.register({
		name = "remote.rpc_call",
		category = "net",
		title = "call remote alua via smprpc/mnprpc",
		summary = "Native RPC session: alua.auth then tool. Needs profile net. Prefer over remote.call.scheme=smprpc.",
		min_profile = "net",
		args_schema = {
			host = "string optional remote host (or use peer=)",
			peer = "string optional trusted peer id",
			port = "number optional (default 12817)",
			name = "string remote tool name",
			args = "table optional remote tool args",
			token = "string optional debug only (normally omit; ssid ensure)",
			alua_ssid = "string optional bind ssid (default current); unique auth entry",
			allow_raw_token = "boolean optional allow raw serve.env on TLS only (default false)",
			transport = "string optional smprpc|mnprpc (default smprpc)",
			tls = "boolean optional TLS text face (mnps/smps; online has no *rpcs RPC face)",
			timeout_sec = "number optional (default 30)",
		},
		examples = {
			{
				args = {
					host = "127.0.0.1",
					port = 12817,
					name = "runtime.ping",
					args = {},
					transport = "smprpc",
				},
			},
		},
		docs = "tools/remote.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("remote.rpc_call", _rpc_call)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
