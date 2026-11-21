--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rpc.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve/mux 子模块: smprpc/mnprpc 会话 (首 REQUEST=alua.auth, 其后方法名→dispatch)
-- @note   REQUEST 有回应; POST/NOTIFY 无回应 (fire-and-forget)
--   \n @stability internal: 实现文件, 勿 require 子路径
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 _mux_rpc_auth_and_loop
--]]
local kenv = require("kenv")
local schema = require("klbagent.core.schema")
local codes = require("klbagent.code")
local dispatch = require("klbagent.core.dispatch")
local log = require("klbagent.core.log")
local auth = require("klbagent.core.serve.security.auth")


local M = {}


-- @brief RPC 方法参数 → dispatch args table
-- @param [in] ...						方法参数
-- @return args[table]
local function _args_to_table(...)
	local n = select("#", ...)
	if n < 1 then
		return {}
	end

	if n == 1 then
		local a1 = select(1, ...)
		if type(a1) == "table" then
			return a1
		end

		if a1 == nil then
			return {}
		end

		return { a1 }
	end

	return { ... }
end


-- @brief 保住 nil 洞与个数
-- @param [in] ...						任意返回值
-- @return cap[table]					{ n, ... }
local function _capture(...)
	return {
		n = select("#", ...),
		...,
	}
end


-- @brief smprpc/mnprpc: 首 REQUEST=alua.auth, 其后方法名→dispatch
-- @note 须在 kco 协程内
-- @param [in] serve[table]				会话
-- @param [in] proto[string]				mux 协议名 (smprpc/mnprpc 及 TLS 变体)
M.co_rpc_auth_and_loop = function (serve, proto)
	local authed = false
	local session_tok = nil
	local session_ssid = nil

	while not kenv.is_exit() do
		local cap = _capture(serve:co_recv())
		if 0 == cap.n or nil == cap[1] then
			break
		end

		local state = {}
		if type(serve.status) == "function" then
			local ok_st, st = pcall(function()
				return serve:status()
			end)
			if ok_st and type(st) == "table" then
				state = st
			end
		end

		local method = state.method or ""
		local sequence = state.sequence or 0
		local name = cap[1]

		if "string" ~= type(name) or name == "" then
			if method == "REQUEST" and type(serve.response) == "function" then
				pcall(function()
					serve:response(sequence, false, "unknown")
				end)
			end
		elseif method == "REQUEST" then
			if not authed then
				if name ~= auth.AUTH_HEAD then
					pcall(function()
						serve:response(sequence, false, "need alua.auth")
					end)
					break
				end

				local tok = auth.token_from_rpc_args(table.unpack(cap, 2, cap.n))
				local sid = auth.ssid_from_rpc_args(table.unpack(cap, 2, cap.n))
				local info, err_v, err_code = auth.verify_token(tok, proto, sid)
				if info == nil then
					local msg = "bad token"
					if type(err_v) == "string" and err_v ~= "" then
						msg = err_v
					end

					local out_code = codes.serve.DENIED
					if auth.is_ssid_unavail(err_v, err_code) then
						out_code = codes.serve.UNAVAILABLE
					end

					-- RPC 失败第三参保持 string (兼容旧客户端); code 仅日志
					pcall(function()
						serve:response(sequence, false, msg)
					end)
					log.write("warn", "serve mux rpc auth deny", {
						module = "serve",
						proto = proto,
						via = proto,
						err = msg,
						code = out_code,
					})
					break
				end

				session_tok = tok
				session_ssid = sid
				local rc = 1
				pcall(function()
					rc = serve:response(sequence, true)
				end)
				if 0 ~= rc then
					break
				end

				authed = true
			else
				local args = _args_to_table(table.unpack(cap, 2, cap.n))
				local ok_a, err_a = schema.validate_args(name, args)
				if not ok_a then
					pcall(function()
						serve:response(sequence, false, tostring(err_a or "bad args"))
					end)
				else
					local info, err_v, err_code = auth.verify_token(session_tok, proto, session_ssid)
					if info == nil then
						local msg = "token expired"
						local out_code = codes.serve.DENIED
						if auth.is_ssid_unavail(err_v, err_code) then
							out_code = codes.serve.UNAVAILABLE
							msg = err_v or "ssid client unavailable"
						elseif type(err_v) == "string" and err_v ~= "" then
							msg = err_v
						end

						pcall(function()
							serve:response(sequence, false, msg)
						end)
						log.write("warn", "serve mux rpc reauth deny", {
							module = "serve",
							proto = proto,
							err = msg,
							code = out_code,
						})
						break
					end

					auth.bind(info)
					local robj = {
						name = name,
						args = args,
					}
					auth.attach_via_fingerprint(robj, proto, nil)
					local ok, result = dispatch.call(robj)
					auth.unbind()
					local rc = 1
					if not ok then
						local msg = "error"
						if type(result) == "table" and type(result.message) == "string" then
							msg = result.message
						elseif type(result) == "string" then
							msg = result
						end

						pcall(function()
							rc = serve:response(sequence, false, msg)
						end)
					else
						pcall(function()
							rc = serve:response(sequence, true, result)
						end)
					end

					if 0 ~= rc then
						break
					end
				end
			end
		elseif method == "POST" or method == "NOTIFY" then
			if not authed then
				break
			end

			-- 无回应; 仍走 dispatch (fire-and-forget)
			local args = _args_to_table(table.unpack(cap, 2, cap.n))
			local ok_a = schema.validate_args(name, args)
			if ok_a then
				local info = auth.verify_token(session_tok, proto, session_ssid)
				if info ~= nil then
					auth.bind(info)
					pcall(function()
						local robj = {
							name = name,
							args = args,
						}
						auth.attach_via_fingerprint(robj, proto, nil)
						dispatch.call(robj)
					end)
					auth.unbind()
				end
			end
		end
	end

	if type(serve.disconnect) == "function" then
		pcall(function()
			serve:disconnect()
		end)
	end
end


return M

-- end
