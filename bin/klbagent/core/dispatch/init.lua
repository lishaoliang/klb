--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  tool 名分发到 handler
-- @note   调用前按生效 profile + catalog.min_profile/side_effects 门禁
-- @history 修改历史
--  \n [2026-09] 创建文件; 迁入子目录 dispatch/init.lua
--  \n [2026-10] 原始 token 工具白名单 (auth/deploy; Todo.2026-34); 临时 token 按签发 profile/caps 裁剪 (Todo.2026-33 P2 / D52); embedded: 原始 token 免 TLS/白名单, 与临时 token 等价
--]]
local codes = require("klbagent.code")
local catalog = require("klbagent.core.catalog")
local profiles = require("klbagent.core.profiles")
local schema = require("klbagent.core.schema")
local log = require("klbagent.core.log")
local session = require("klbagent.core.session")
local ai_load_gate = require("klbagent.core.ai_load_gate")


local M = {}


local handlers = {}


-- @brief 去掉 Lua error 自带的 path:line 前缀, 保留稳定语义串
-- @param [in] err[any]
-- @return msg[string]
local function _stable_tool_err(err)
	local s = tostring(err)
	local msg = string.match(s, "^.-:%d+:%s*(.+)$")
	if type(msg) == "string" and msg ~= "" then
		return msg
	end

	return s
end


-- @brief 注册 handler
-- @param [in] name[string]
-- @param [in] fn[function]			function(args) -> result
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (name, fn)
	if type(name) ~= "string" or name == "" then
		return false, "bad name"
	end

	if type(fn) ~= "function" then
		return false, "bad handler"
	end

	handlers[name] = fn
	return true
end


-- @brief 执行一次工具调用
-- @param [in] req[table]			须含 name; 可选 args[table]
-- @return ok[boolean]
-- @return result_or_err[table]
M.call = function (req)
	local ok_req, err_req = schema.validate_request(req)
	if not ok_req then
		return false, codes.make_error(codes.dispatch.BAD_REQUEST, err_req or "bad request")
	end

	local fn = handlers[req.name]
	if fn == nil then
		return false, codes.make_error(codes.dispatch.TOOL_NOT_FOUND, "tool not found")
	end

	local desc = catalog.get(req.name)
	if desc == nil then
		return false, codes.make_error(codes.dispatch.TOOL_NOT_FOUND, "tool not in catalog")
	end

	local ok_a, auth = pcall(require, "klbagent.core.auth")
	if not ok_a then
		auth = nil
	end

	-- 生效档: 临时 token 取 min(进程档, 签发档); 无绑定则进程档
	local active = profiles.active()
	if type(auth) == "table" and type(auth.effective_profile) == "function" then
		active = auth.effective_profile()
	end

	if not profiles.allows(active, desc.min_profile) then
		return false, codes.make_error(codes.dispatch.DENIED, "profile denied")
	end

	if not profiles.allow(active, desc.side_effects) then
		return false, codes.make_error(codes.dispatch.DENIED, "side_effects denied")
	end

	local sid = req.session_id
	if type(sid) ~= "string" then
		sid = nil
	end

	local ok_gate, err_gate = ai_load_gate.check(req.name, sid)
	if not ok_gate then
		return false, codes.make_error(codes.dispatch.DENIED, err_gate or "ai_load ack required")
	end

	-- 原始 token 白名单 + 临时 token caps 裁剪; 本机 lpc 除外
	if type(auth) == "table" and type(auth.request_auth) == "function" then
		local ra = auth.request_auth()
		if type(ra) == "table" then
			local via = req.via
			if via ~= "lpc" then
				if ra.kind == "raw" then
					-- embedded: 原始 token 与临时 token 等价 (免签发), 免 TLS/白名单裁剪
					local raw_ok = type(auth.host_is_embedded) == "function" and auth.host_is_embedded()
					if not raw_ok then
						if ra.tls ~= true then
							return false, codes.make_error(codes.dispatch.DENIED, "raw token requires tls")
						end

						if type(auth.allows_raw_tool) == "function" and not auth.allows_raw_tool(req.name) then
							return false, codes.make_error(
								codes.dispatch.DENIED,
								"raw token only for auth/deploy (issue temp token for business)")
						end
					end
				elseif ra.kind == "temp" then
					if type(auth.allows_caps) == "function" then
						local ok_c, code_c = auth.allows_caps(ra.caps, req.name, desc.category)
						if not ok_c then
							return false, codes.make_error(
								codes.dispatch.DENIED,
								auth.auth_err_name(code_c))
						end
					end
				end
			end
		end
	end

	local args = req.args
	if type(args) ~= "table" then
		args = {}
	end

	local ok_args, err_args = schema.validate_args(req.name, args)
	if not ok_args then
		return false, codes.make_error(codes.dispatch.BAD_REQUEST, err_args or "bad args")
	end

	if type(sid) == "string" and sid ~= "" then
		if session.get(sid) == nil then
			return false, codes.make_error(codes.dispatch.BAD_REQUEST, "session not found")
		end
	end

	local ok, result = pcall(fn, args)
	if not ok then
		local raw_err = tostring(result)
		local stable = _stable_tool_err(result)
		local fields = {
			module = "dispatch",
			tool = req.name,
			err = stable,
			err_raw = raw_err,
		}
		if req.id ~= nil then
			fields.request_id = req.id
		end

		log.write("error", "tool failed", fields)
		return false, codes.make_error(codes.dispatch.TOOL_FAILED, stable)
	end

	if type(result) ~= "table" then
		result = { value = result }
	end

	if type(req.session_id) == "string" and req.session_id ~= "" then
		session.note_call(req.session_id, req.name)
	end

	return true, result
end


-- @brief 清空 handler (测试/重载)
M.clear = function ()
	handlers = {}
end


return M
