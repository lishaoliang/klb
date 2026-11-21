--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   session.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve/mux 子模块: SMP/MNP 会话 (alua.auth 后 TEXT/BINARY/MEDIA)
-- @note   首帧须 alua.auth TEXT; 其后 TEXT=call envelope / BINARY alua-tunnel 出口 / MEDIA 旁路
--   \n @stability internal: 实现文件, 勿 require 子路径
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 _mux_auth_and_loop
--]]
local kenv = require("kenv")
local kco = require("kco")
local envelope = require("klbagent.core.envelope")
local schema = require("klbagent.core.schema")
local codes = require("klbagent.code")
local dispatch = require("klbagent.core.dispatch")
local log = require("klbagent.core.log")
local auth = require("klbagent.core.serve.security.auth")
local tunnel = require("klbagent.core.serve.mux.tunnel")


local M = {}


-- @brief 处理一条 TEXT 为 /v1/call 同 envelope; 返回响应 JSON 文本
-- @param [in] raw[string]
-- @param [in] via[string]
-- @return text[string]
local function _call_text(raw, via)
	local robj, err_dec = envelope.decode_request(raw or "")
	if robj == nil then
		return envelope.encode_err(
			codes.make_error(codes.serve.BAD_REQUEST, tostring(err_dec or "bad request")),
			nil) or "{}"
	end

	-- via 由服务端入口注入 (禁客户端自填); 同步 host_tool attrs (M2)
	auth.attach_via_fingerprint(robj, via or "mux", nil)

	local ok_a, err_a = schema.validate_args(robj.name, robj.args)
	if not ok_a then
		return envelope.encode_err(codes.make_error(codes.serve.BAD_REQUEST, err_a), robj.id) or "{}"
	end

	local ok, result = dispatch.call(robj)
	if not ok then
		return envelope.encode_err(result, robj.id) or "{}"
	end

	return envelope.encode_ok(result, robj.id) or "{}"
end


-- @brief 鉴权拒绝后让出调度, 便于 TEXT 出站刷完再 disconnect (Bug.2026-10)
-- @note 须在 kco 协程内; 仅 co_sleep (禁 kco.sleep 无前缀名)
local function _auth_deny_flush()
	for _ = 1, 8 do
		kco.co_sleep(10)
	end
end


-- @brief SMP/MNP 会话: 首帧 alua.auth, 其后 TEXT=call / BINARY alua-tunnel 出口 / MEDIA 旁路
-- @note 须在 kco 协程内
-- @param [in] serve[table]				会话
-- @param [in] proto[string]				mux 协议名 (smp/smps/mnp/mnps)
M.co_auth_and_loop = function (serve, proto)
	local authed = false
	local tunnel_handed = false
	local session_tok = nil
	local session_ssid = nil

	while not kenv.is_exit() do
		local msg, head, body = serve:co_recv()
		if msg == "exit" or msg == "error" or msg == "close" then
			break
		end

		if msg == "text" then
			if not authed then
				if head ~= auth.AUTH_HEAD then
					serve:send_text(auth.AUTH_HEAD, '{"ok":false,"message":"need alua.auth"}')
					_auth_deny_flush()
					break
				end

				local tok = auth.token_from_mux_body(body)
				local sid = auth.ssid_from_mux_body(body)
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

					serve:send_text(auth.AUTH_HEAD,
						'{"ok":false,"code":' .. tostring(out_code)
						.. ',"message":"' .. msg .. '"}')
					log.write("warn", "serve mux auth deny", {
						module = "serve",
						proto = proto,
						err = msg,
						code = out_code,
					})
					_auth_deny_flush()
					break
				end

				session_tok = tok
				session_ssid = sid
				serve:send_text(auth.AUTH_HEAD, '{"ok":true}')
				authed = true
			else
				local raw = body or ""
				if type(head) == "string" and head ~= "" and head ~= auth.CALL_HEAD and head ~= auth.AUTH_HEAD then
					-- 非 call 头: 旁路忽略
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

						serve:send_text(auth.AUTH_HEAD,
							'{"ok":false,"code":' .. tostring(out_code)
							.. ',"message":"' .. msg .. '"}')
						break
					end

					auth.bind(info)
					local text = _call_text(raw, proto)
					auth.unbind()
					serve:send_text(auth.CALL_HEAD, text)
				end
			end
		elseif msg == "media" then
			if not authed then
				break
			end

			-- 媒体旁路: 暂无 hub, 释放 buf 保持连接
			if type(serve.free_media) == "function" then
				pcall(function()
					serve:free_media(head)
				end)
			end
		elseif msg == "binary" then
			if not authed then
				break
			end


			-- 代理 hop mnps: BINARY head=alua-tunnel → 出口隧道 (≠ tool RPC)
			if head == nil or head == "" or head == "alua-tunnel" then
				tunnel_handed = true
				tunnel.co_tunnel_exit(serve, proto, body)
				break
			end
			-- 其他二进制旁路: 忽略载荷
		end
	end

	if not tunnel_handed and type(serve.disconnect) == "function" then
		pcall(function()
			serve:disconnect()
		end)
	end
end


return M

-- end
