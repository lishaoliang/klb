--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   ssid.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua ssid — whoami|ensure|status|begin|gc
-- @note   一律 ssid.new_client(XPC → 本机 serve owner); 禁 new_serve; 须在 entry_cli 协程内
-- @history 修改历史
--  \n [2026-11] 创建文件 (M8); 改 client-only + 协程 CLI
--]]


local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")


-- @brief 取 ssid 客户端面 (XPC → 本机 serve alua.ssid)
-- @return face[table]
local function _ssid()
	local pkg = require("klbagent.core.ssid")
	return pkg.new_client({ transport = "xpc" })
end


-- @brief 业务回显脱敏: 禁 token / token_ref; 仅 has_token
-- @param [in] info[table]
-- @return info[table]
local function _public_ssid_result(info)
	if type(info) ~= "table" then
		return info
	end

	local has = false
	if type(info.token) == "string" and info.token ~= "" then
		has = true
	elseif type(info.token_ref) == "string" and info.token_ref ~= "" then
		has = true
	elseif info.has_token == true or info.has_token_ref == true then
		has = true
	end

	info.token = nil
	info.token_ref = nil
	info.has_token = has
	return info
end


local M = {}


M.help = function ()
	print("Usage: alua ssid whoami|ensure|status|begin|gc [--json]")
	fmt.print_kv("whoami", "show alua_ssid summary")
	fmt.print_kv("ensure", "ensure temp token via ssid (unique auth entry)")
	fmt.print_kv("status", "redacted counts")
	fmt.print_kv("begin", "begin_ctx (bind request)")
	fmt.print_kv("gc", "gc stale ssid pref (default dry-run)")
end


M.run = function (...)
	local want_help, want_json, rest = fmt.take_cli_flags({ ... })
	if want_help then
		M.help()
		return true, codes.OK
	end

	local sub = rest[1]
	if type(sub) ~= "string" or sub == "" then
		M.help()
		return false, codes.cli.BAD_REQUEST
	end

	local face = _ssid()

	if sub == "whoami" then
		local info, err = face.co_whoami({ via = "call" })
		if info == nil then
			fmt.print_json(codes.make_error(codes.cli.TOOL_FAILED, tostring(err)))
			return false, codes.cli.TOOL_FAILED
		end

		if want_json then
			fmt.print_json({ ok = true, result = info })
		else
			fmt.print_kv("alua_ssid", info.alua_ssid)
			fmt.print_kv("via", info.via)
			fmt.print_kv("caller_kind", info.caller_kind)
			fmt.print_kv("host_tool", info.host_tool)
			fmt.print_kv("profile", info.profile)
		end

		return true, codes.OK
	end

	if sub == "ensure" then
		local info, err = face.co_ensure({ via = "call", profile = "net" })
		if info == nil then
			fmt.print_json(codes.make_error(codes.cli.TOOL_FAILED, tostring(err)))
			return false, codes.cli.TOOL_FAILED
		end

		info = _public_ssid_result(info)
		if want_json then
			fmt.print_json({ ok = true, result = info })
		else
			fmt.print_kv("alua_ssid", info.alua_ssid)
			fmt.print_kv("has_token", info.has_token)
			fmt.print_kv("profile", info.profile)
			fmt.print_kv("renewed", info.renewed)
			fmt.print_kv("expire_ms", info.expire_ms)
		end

		return true, codes.OK
	end

	if sub == "status" then
		local info, err = face.co_status()
		if info == nil then
			fmt.print_json(codes.make_error(codes.cli.TOOL_FAILED, tostring(err)))
			return false, codes.cli.TOOL_FAILED
		end

		if want_json then
			fmt.print_json({ ok = true, result = info })
		else
			fmt.print_kv("count", info.count)
			fmt.print_kv("lpc_ready", info.lpc_ready)
			fmt.print_kv("xpc_ready", info.xpc_ready)
			fmt.print_kv("mode", tostring(face.transport or "xpc"))
		end

		return true, codes.OK
	end

	if sub == "begin" then
		local ctx, err = face.co_begin_ctx({ via = "call", profile = "net" })
		if ctx == nil then
			fmt.print_json(codes.make_error(codes.cli.TOOL_FAILED, tostring(err)))
			return false, codes.cli.TOOL_FAILED
		end

		ctx = _public_ssid_result(ctx)
		if want_json then
			fmt.print_json({ ok = true, result = ctx })
		else
			fmt.print_kv("alua_ssid", ctx.alua_ssid)
			fmt.print_kv("host_tool", ctx.host_tool)
			fmt.print_kv("has_token", ctx.has_token)
			fmt.print_kv("profile", ctx.profile)
			fmt.print_kv("bound", ctx.bound)
		end

		return true, codes.OK
	end

	if sub == "gc" then
		local dry = true
		for i = 2, #rest do
			if rest[i] == "--apply" then
				dry = false
			end
		end

		local info, err = face.co_gc({ dry_run = dry })
		if info == nil then
			fmt.print_json(codes.make_error(codes.cli.TOOL_FAILED, tostring(err)))
			return false, codes.cli.TOOL_FAILED
		end

		if want_json then
			fmt.print_json({ ok = true, result = info })
		else
			fmt.print_kv("removed", info.removed)
			fmt.print_kv("kept", info.kept)
			fmt.print_kv("dry_run", info.dry_run)
			fmt.print_kv("max_age_ms", info.max_age_ms)
		end

		return true, codes.OK
	end

	M.help()
	return false, codes.cli.BAD_REQUEST
end


return M
