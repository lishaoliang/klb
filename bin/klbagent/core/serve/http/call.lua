--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   call.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve/http 子模块: POST /v1/call (envelope 请求/响应)
-- @note   token 门禁 → envelope 解码 → schema 校验 → dispatch.call
--   \n raw token 走明文入口时单独回 "raw token requires tls" (而非笼统 bad token)
--   \n @stability internal: 实现文件, 勿 require 子路径
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 _handle_call
--]]
local envelope = require("klbagent.core.envelope")
local schema = require("klbagent.core.schema")
local codes = require("klbagent.code")
local dispatch = require("klbagent.core.dispatch")
local auth = require("klbagent.core.serve.security.auth")


local M = {}


-- @brief POST /v1/call: 校验 token 后按 envelope 分派
-- @param [in] req[table]
-- @param [in] res[table]
M.handle_call = function (req, res)
	local headers = req and req.headers
	local tok = auth.token_from_headers(headers)
	local sid = auth.ssid_from_headers(headers)
	local info, err_v, err_code = auth.verify_token(tok, auth.tls_of(req), sid)
	if info == nil then
		auth.deny_http(res, err_v, err_code)
		return
	end

	local raw = nil
	if type(req.json) == "table" and type(req.json.name) == "string" then
		local ok_e, text = pcall(require("cjson").encode, req.json)
		if ok_e and type(text) == "string" then
			raw = text
		end
	end

	if raw == nil then
		raw = req.body or ""
	end

	local robj, err_dec = envelope.decode_request(raw)
	if robj == nil then
		res:co_json({
			ok = false,
			code = codes.serve.BAD_REQUEST,
			message = tostring(err_dec or "bad request"),
			name = codes.str(codes.serve.BAD_REQUEST),
		}, 400)
		return
	end

	local ok_a, err_a = schema.validate_args(robj.name, robj.args)
	if not ok_a then
		local text = envelope.encode_err(codes.make_error(codes.serve.BAD_REQUEST, err_a), robj.id)
		res:co_status(400, "application/json; charset=utf-8", text or "{}")
		return
	end

	-- via 由服务端入口注入 (禁客户端自填); 同步 host_tool attrs (M2)
	auth.attach_via_fingerprint(robj, auth.tls_of(req) and "https" or "http", req and req.headers)

	auth.bind(info)
	local ok, result = dispatch.call(robj)
	auth.unbind()
	if not ok then
		local text = envelope.encode_err(result, robj.id)
		res:co_status(200, "application/json; charset=utf-8", text or "{}")
		return
	end

	local text = envelope.encode_ok(result, robj.id)
	res:co_status(200, "application/json; charset=utf-8", text or "{}")
end


return M

-- end
