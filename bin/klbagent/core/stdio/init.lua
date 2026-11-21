--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  Agent stdin/stdout 行协议 IO
-- @note   每行一个 JSON Request; 每行一个 JSON Response; 空行跳过; EOF 结束
-- @history 修改历史
--  \n [2026-09] 创建文件
--]]
local envelope = require("klbagent.core.envelope")
local dispatch = require("klbagent.core.dispatch")
local schema = require("klbagent.core.schema")
local codes = require("klbagent.code")


local M = {}


-- @brief 从 stdin 读一行 Request 原文 (去尾空白)
-- @return raw[string|nil]			nil=EOF
-- @return err[string|nil]
M.read_request = function ()
	local line = io.read("*l")
	if line == nil then
		return nil
	end

	return line
end


-- @brief 向 stdout 写一行 Response
-- @param [in] raw[string]
-- @return ok[boolean]
-- @return err[string|nil]
M.write_response = function (raw)
	if type(raw) ~= "string" then
		return false, "bad response"
	end

	io.write(raw)
	io.write("\n")
	io.flush()
	return true
end


-- @brief 处理单行 Request 原文并写 Response
-- @param [in] raw[string]
-- @return ok[boolean]				工具是否成功
-- @return code[number]
M.handle_line = function (raw)
	if type(raw) ~= "string" then
		local text = envelope.encode_err(codes.make_error(codes.stdio.BAD_REQUEST, "empty request"))
		M.write_response(text or "{}")
		return false, codes.stdio.BAD_REQUEST
	end

	local trimmed = string.match(raw, "^%s*(.-)%s*$") or ""
	if trimmed == "" then
		return true, codes.OK
	end

	local req, err_dec = envelope.decode_request(trimmed)
	if req == nil then
		local text = envelope.encode_err(codes.make_error(codes.stdio.BAD_REQUEST, err_dec))
		M.write_response(text or "{}")
		return false, codes.stdio.BAD_REQUEST
	end

	-- via 由服务端入口注入; stdio 为本地信任通道
	req.via = "stdio"

	local ok_a, err_a = schema.validate_args(req.name, req.args)
	if not ok_a then
		local text = envelope.encode_err(codes.make_error(codes.stdio.BAD_REQUEST, err_a), req.id)
		M.write_response(text or "{}")
		return false, codes.stdio.BAD_REQUEST
	end

	-- 经 ssid 绑本地上下文; 一律 new_client(XPC → 本机 serve owner)
	local ok_s, pkg = pcall(require, "klbagent.core.ssid")
	local ssid = nil
	if ok_s and type(pkg) == "table" and type(pkg.new_client) == "function" then
		ssid = pkg.new_client({ transport = "xpc" })
	end

	if type(ssid) == "table" and type(ssid.co_begin_ctx) == "function" then
		local ctx_ok, ctx = pcall(ssid.co_begin_ctx, {
			via = "stdio",
			skip_token = true,
			profile = "net",
		})
		if ctx_ok and type(ctx) == "table" then
			local ok_sess, sess = pcall(require, "klbagent.core.session")
			if ok_sess and type(sess) == "table" then
				local cur = nil
				if type(sess.current_id) == "function" then
					cur = sess.current_id()
				end

				if type(cur) ~= "string" or cur == "" then
					local info = sess.begin({
						attrs = {
							via = "stdio",
							caller_kind = ctx.caller_kind,
							host_tool = ctx.host_tool or "",
							alua_ssid = ctx.alua_ssid,
						},
					})
					if type(info) == "table" then
						cur = info.id
					end
				else
					pcall(sess.set_attrs, cur, {
						via = "stdio",
						caller_kind = ctx.caller_kind,
						host_tool = ctx.host_tool or "",
						alua_ssid = ctx.alua_ssid,
					})
				end

				if type(cur) == "string" and cur ~= "" then
					req.session_id = cur
				end
			end
		end
	end

	local ok, result = dispatch.call(req)

	if type(ssid) == "table" and type(ssid.clear_request) == "function" then
		pcall(ssid.clear_request)
	end

	if not ok then
		local text = envelope.encode_err(result, req.id)
		M.write_response(text or "{}")
		local code = codes.stdio.TOOL_FAILED
		if type(result) == "table" and type(result.code) == "number" then
			code = result.code
		end

		return false, code
	end

	local text = envelope.encode_ok(result, req.id)
	M.write_response(text or "{}")
	return true, codes.OK
end


-- @brief stdio 读循环 (须在 kco 协程内; EOF 后 kenv.exit)
-- @param [in] ssid[table|nil]
local function _co_stdio_loop(ssid)
	local kenv = require("kenv")

	while not kenv.is_exit() do
		local ok_r, raw = pcall(M.read_request)
		if (not ok_r) or raw == nil then
			break
		end

		M.handle_line(raw)
	end

	if type(ssid) == "table" and type(ssid.clear_request) == "function" then
		pcall(ssid.clear_request)
	end

	if not kenv.is_exit() then
		kenv.exit()
	end
end


-- @brief 循环读 stdin 直到 EOF (fork 读循环 + run_host_loop; 对齐 mcp serve)
-- @return ok[boolean]				挂接成功为 true; CLI hold 时主体在 run_host_loop 返回后继续
-- @return code[number]
-- @note 禁入口内 while 卡死主体; 长期读 stdin 在 kco.fork 内; 泵送走 co_util
M.loop = function ()
	local kco = require("kco")
	local co_util = require("klbagent.core.co_util")
	local ai_load_gate = require("klbagent.core.ai_load_gate")
	ai_load_gate.set_channel("stdio")

	local ok_s, pkg = pcall(require, "klbagent.core.ssid")
	local ssid = nil
	if ok_s and type(pkg) == "table" and type(pkg.new_client) == "function" then
		ssid = pkg.new_client({ transport = "xpc" })
	end

	if type(ssid) == "table" and type(ssid.co_begin_ctx) == "function" then
		local ctx_ok, ctx = pcall(ssid.co_begin_ctx, {
			via = "stdio",
			skip_token = true,
			profile = "net",
		})
		if ctx_ok and type(ctx) == "table" then
			local ok_sess, sess = pcall(require, "klbagent.core.session")
			if ok_sess and type(sess) == "table" and type(sess.begin) == "function" then
				pcall(sess.begin, {
					attrs = {
						via = "stdio",
						caller_kind = ctx.caller_kind,
						host_tool = ctx.host_tool or "",
						alua_ssid = ctx.alua_ssid,
					},
				})
			end
		end
	end

	kco.fork(function ()
		_co_stdio_loop(ssid)
	end)

	-- CLI 宿主 hold; 独立 env 自调 co_main_loop; EOF/信号 → kenv.exit
	local ok_h, err_h = pcall(co_util.run_host_loop)
	if not ok_h then
		io.stderr:write("alua stdio: host loop fail: " .. tostring(err_h) .. "\n")
		return false, codes.stdio.TOOL_FAILED
	end

	return true, codes.OK
end


return M
