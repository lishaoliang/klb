--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   ws.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve/http 子模块: WS /v1/ws 文本帧处理 (与 POST /v1/call 同 envelope)
-- @note   握手门禁见 http/init.lua; 本文件只处理已升级连接
--   \n @stability internal: 实现文件, 勿 require 子路径
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 _handle_ws
--]]
local envelope = require("klbagent.core.envelope")
local schema = require("klbagent.core.schema")
local codes = require("klbagent.code")
local dispatch = require("klbagent.core.dispatch")
local auth = require("klbagent.core.serve.security.auth")


local M = {}


-- @brief WS 文本帧: 与 POST /v1/call 同 envelope
-- @note 须在 kco 协程内
-- @param [in] req[table]
-- @param [in] ws[table]
M.handle_ws = function (req, ws)
	local headers = req and req.headers
	local session_tok = auth.token_from_headers(headers)
	local session_ssid = auth.ssid_from_headers(headers)
	local tls_hint = auth.tls_of(req)

	while true do
		local msg, _head, body = ws:co_recv()
		if msg == "exit" or msg == "close" or msg == "error" then
			break
		end

		if msg == "text" then
			local raw = body or ""
			local robj, err_dec = envelope.decode_request(raw)
			if robj ~= nil then
				-- via 由服务端入口注入 (禁客户端自填); 同步 host_tool attrs (M2)
				auth.attach_via_fingerprint(robj, tls_hint and "wss" or "ws", headers)
			end
			if robj == nil then
				local text = envelope.encode_err(
					codes.make_error(codes.serve.BAD_REQUEST, tostring(err_dec or "bad request")),
					nil)
				ws:send_text(text or "{}")
			else
				local ok_a, err_a = schema.validate_args(robj.name, robj.args)
				if not ok_a then
					local text = envelope.encode_err(codes.make_error(codes.serve.BAD_REQUEST, err_a), robj.id)
					ws:send_text(text or "{}")
				else
					local info, err_v, err_code = auth.verify_token(session_tok, tls_hint, session_ssid)
					if info == nil then
						local err_code_out = codes.serve.DENIED
						local msg = "token expired"
						if auth.is_ssid_unavail(err_v, err_code) then
							err_code_out = codes.serve.UNAVAILABLE
							msg = err_v or "ssid client unavailable"
						elseif type(err_v) == "string" and err_v ~= "" then
							msg = err_v
						end

						local text = envelope.encode_err(
							codes.make_error(err_code_out, msg),
							robj.id)
						ws:send_text(text or "{}")
						break
					end

					auth.bind(info)
					local ok, result = dispatch.call(robj)
					auth.unbind()
					if not ok then
						local text = envelope.encode_err(result, robj.id)
						ws:send_text(text or "{}")
					else
						local text = envelope.encode_ok(result, robj.id)
						ws:send_text(text or "{}")
					end
				end
			end
		end
	end

	if type(ws.disconnect) == "function" then
		pcall(function()
			ws:disconnect()
		end)
	end
end


return M

-- end
