--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   call.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  lpc.call — 进程内经 klblpc→dispatch
-- @history 修改历史
--  \n [2026-10] 创建文件 (LPC L1)
--]]


local lpc_bridge = require("klbagent.core.lpc_bridge")


local M = {}


local function _call(args)
	local name = args.name
	if type(name) ~= "string" or name == "" then
		error("need name")
	end

	local tool_args = args.args
	if type(tool_args) ~= "table" then
		tool_args = {}
	end

	-- 默认按 readonly 裁剪; 显式 false 才用调用者生效档 (bridge 内绑定)
	local readonly_only = true
	if args.readonly_only == false then
		readonly_only = false
	end

	local opts = { readonly_only = readonly_only }
	if not readonly_only then
		local ok_a, auth = pcall(require, "klbagent.core.auth")
		if not ok_a or type(auth) ~= "table" or type(auth.effective_profile) ~= "function" then
			error("lpc caller profile unavailable")
		end

		opts.profile = auth.effective_profile()
	end

	local timeout_ms = nil
	if type(args.timeout_sec) == "number" and args.timeout_sec > 0 then
		timeout_ms = math.floor(args.timeout_sec * 1000)
	end

	local ok, result = lpc_bridge.sync_tool_call(name, tool_args, opts, timeout_ms)
	if ok then
		return {
			ok = true,
			result = result,
			via = "lpc",
			module = lpc_bridge.MODULE_NAME,
		}
	end

	if type(result) == "table" then
		result.via = "lpc"
		result.ok = false
		return result
	end

	error(tostring(result or "lpc call fail"))
end


M.register = function (catalog, dispatch_reg)
	local ok, err = catalog.register({
		name = "lpc.call",
		category = "runtime",
		title = "in-process LPC tool call",
		summary = "Call catalog tool via klblpc module alua.tools (same process). Needs readonly+.",
		min_profile = "readonly",
		args_schema = {
			name = "string tool name",
			args = "table optional tool args",
			readonly_only = "boolean optional; default true clamps to readonly; false uses caller profile",
			timeout_sec = "number optional (default 15)",
		},
		examples = {
			{ args = { name = "runtime.ping", args = {} } },
		},
		docs = "tools/serve_rpc_lpc.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch_reg.register("lpc.call", _call)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
