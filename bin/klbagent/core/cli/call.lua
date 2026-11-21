--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   call.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua call — 单次工具调用 (始终 JSON)
-- @note   alua call <tool> [json_args]; 须在 entry_cli 协程内; ssid 仅 new_client(XPC)
-- @history 修改历史
--  \n [2026-10] 创建文件 (自 cli/init 拆出)
--  \n [2026-11] 去掉 new_serve 回退; 依赖 CLI 协程 + owner 线程
--]]
local cjson = require("cjson")
local codes = require("klbagent.code")
local fmt = require("klbagent.core.cli.fmt")


local M = {}


M.help = function ()
	print("Usage: alua call <tool> [json_args]")
	fmt.print_kv("call", "one tool; always JSON envelope")
	fmt.print_kv("args", "JSON object string (optional)")
end


-- @brief CLI 入口
-- @param [in] ...		tool [json_args]
-- @return ok[boolean]
-- @return code[number]
M.run = function (...)
	local want_help, _, rest = fmt.take_cli_flags({ ... })
	if want_help then
		M.help()
		return true, codes.OK
	end

	local name = rest[1]
	local raw_args = rest[2]
	if type(name) ~= "string" or name == "" then
		fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "need tool name"))
		return false, codes.cli.BAD_REQUEST
	end

	local args = {}
	if type(raw_args) == "string" and raw_args ~= "" then
		local ok_j, obj = pcall(cjson.decode, raw_args)
		if not ok_j or type(obj) ~= "table" then
			fmt.print_json(codes.make_error(codes.cli.BAD_REQUEST, "args must be json object"))
			return false, codes.cli.BAD_REQUEST
		end

		args = obj
	end

	-- cli → ssid.begin_ctx → 执行; 同步落 core.session attrs (via/host_tool)
	-- profile: 跟进程档 (勿写死 net, 否则 deploy/unsafe 被裁成 profile denied)
	do
		local bind_profile = "net"
		local ok_p, profiles = pcall(require, "klbagent.core.profiles")
		if ok_p and type(profiles) == "table" and type(profiles.active) == "function" then
			local ap = profiles.active()
			if type(ap) == "string" and ap ~= "" then
				bind_profile = ap
			end
		end

		local ok_s, pkg = pcall(require, "klbagent.core.ssid")
		local ssid = nil
		if ok_s and type(pkg) == "table" and type(pkg.new_client) == "function" then
			ssid = pkg.new_client({ transport = "xpc" })
		end

		if type(ssid) == "table" and type(ssid.co_begin_ctx) == "function" then
			local ctx_ok, ctx = pcall(ssid.co_begin_ctx, {
				via = "call",
				profile = bind_profile,
			})
			if ctx_ok and type(ctx) == "table" then
				local ok_sess, sess = pcall(require, "klbagent.core.session")
				if ok_sess and type(sess) == "table" and type(sess.begin) == "function" then
					pcall(sess.begin, {
						attrs = {
							via = ctx.via or "call",
							caller_kind = ctx.caller_kind,
							host_tool = ctx.host_tool or "",
							alua_ssid = ctx.alua_ssid,
						},
					})
				end
			end
		end
	end

	return fmt.emit_tool(name, args, true, nil)
end


return M
