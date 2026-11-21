--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  Agent Request/Response 信封编解码 (JSON)
-- @note   Request: { name, args?, id?, session_id? }; Response: { ok, result? } | { ok=false, code, message, name?, id? }
-- @history 修改历史
--  \n [2026-09] 创建文件; 迁入子目录 envelope/init.lua
--]]
local cjson = require("cjson")
local codes = require("klbagent.code")
local schema = require("klbagent.core.schema")


local M = {}


-- @brief 解析 Request JSON
-- @param [in] raw[string]
-- @return req[table|nil]
-- @return err[string|nil]
M.decode_request = function (raw)
	if type(raw) ~= "string" or raw == "" then
		return nil, "empty request"
	end

	local ok_j, obj = pcall(cjson.decode, raw)
	if not ok_j or type(obj) ~= "table" then
		return nil, "request must be json object"
	end

	local ok_v, err_v = schema.validate_request(obj)
	if not ok_v then
		return nil, err_v
	end

	-- via 由服务端入口注入 (serve/stdio/lpc); 忽略客户端自填
	obj.via = nil

	if type(obj.args) ~= "table" then
		obj.args = {}
	end

	return obj
end


-- @brief 编码成功 Response
-- @param [in] result[table]
-- @param [in] id[any|nil]
-- @return s[string|nil]
-- @return err[string|nil]
M.encode_ok = function (result, id)
	local res = {
		ok = true,
		result = result,
	}
	if id ~= nil then
		res.id = id
	end

	local ok_j, text = pcall(cjson.encode, res)
	if not ok_j or type(text) ~= "string" then
		return nil, "encode fail"
	end

	return text
end


-- @brief 编码失败 Response
-- @param [in] err[table|string]		codes.make 表或消息
-- @param [in] id[any|nil]
-- @return s[string|nil]
-- @return err[string|nil]
M.encode_err = function (err, id)
	local e = err
	if type(e) ~= "table" then
		e = codes.make_error(codes.envelope.TOOL_FAILED, tostring(err))
	end

	local res = {
		ok = false,
		code = e.code or codes.envelope.TOOL_FAILED,
		message = e.message or "error",
		name = e.name or codes.str(e.code),
	}
	if id ~= nil then
		res.id = id
	end

	local ok_j, text = pcall(cjson.encode, res)
	if not ok_j or type(text) ~= "string" then
		return nil, "encode fail"
	end

	return text
end


-- @brief 编码 Response (兼容桩名)
-- @param [in] res[table]			成功 { ok=true, result } 或错误表
-- @return s[string|nil]
-- @return err[string|nil]
M.encode_response = function (res)
	if type(res) ~= "table" then
		return M.encode_err(codes.make_error(codes.envelope.TOOL_FAILED, "bad response"))
	end

	if res.ok == true then
		return M.encode_ok(res.result, res.id)
	end

	return M.encode_err(res, res.id)
end


return M
