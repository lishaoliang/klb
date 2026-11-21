--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  进程内 klblpc 模块 alua.tools → dispatch (LPC L1/L2)
-- @note   等待经 co_util.await_done (须 kco / co_main_loop; co_sleep); worker 只读经 readonly_only
-- @history 修改历史
--  \n [2026-10] 创建文件 (RPC/LPC L1/L2)
--]]


local dispatch = require("klbagent.core.dispatch")
local catalog = require("klbagent.core.catalog")
local profiles = require("klbagent.core.profiles")
local log = require("klbagent.core.log")


local M = {}


M.MODULE_NAME = "alua.tools"


local g_ready = false


-- @brief 注册 tool 方法并启动 LPC serve (幂等)
-- @return ok[boolean]
-- @return err[string|nil]
M.ensure = function ()
	if g_ready then
		return true
	end

	local ok_l, klblpc = pcall(require, "klbcore.klblpc")
	if not ok_l or type(klblpc) ~= "table" then
		return false, "klblpc unavailable"
	end

	if type(klblpc.setup) ~= "function" or type(klblpc.serve) ~= "function" then
		return false, "klblpc api missing"
	end

	if not klblpc.setup({ name = M.MODULE_NAME }) then
		return false, "klblpc.setup fail"
	end

	-- REQUEST: tool <name> [args] [opts]
	-- opts.readonly_only=true → 仅允许 min_profile=readonly 的工具 (L2 worker)
	local ok_reg, err_reg = pcall(function ()
		klblpc.call("tool", function (req)
			local args = req.args or {}
			local tool_name = args[1]
			local tool_args = args[2]
			local opts = args[3]

			if type(tool_name) ~= "string" or tool_name == "" then
				return false, "need tool name"
			end

			if type(tool_args) ~= "table" then
				tool_args = {}
			end

			-- 默认只读; 仅显式 false 才按 opts.profile (调用者生效档) 放行
			local readonly_only = true
			if type(opts) == "table" and opts.readonly_only == false then
				readonly_only = false
			end

			if readonly_only then
				local desc = catalog.get(tool_name)
				if desc == nil then
					return false, "tool not found"
				end

				local minp = desc.min_profile
				if type(minp) ~= "string" or minp == "" then
					minp = "readonly"
				end

				if not profiles.allows("readonly", minp) then
					return false, "lpc readonly_only denied"
				end
			end

			-- 绑定本调用档 (默认 readonly; 显式 false 用 opts.profile, 缺省仍 readonly)
			local bound_profile = "readonly"
			if not readonly_only and type(opts) == "table" and profiles.is_profile(opts.profile) then
				bound_profile = opts.profile
			end

			-- 绑档只经 ssid 客户端面 (根包仅 new_serve/new_client; Bug.2026-26)
			local ok_s, ssid_pkg = pcall(require, "klbagent.core.ssid")
			local ssid = nil
			if ok_s and type(ssid_pkg) == "table" and type(ssid_pkg.new_client) == "function" then
				ssid = ssid_pkg.new_client({ transport = "lpc" })
			end

			if type(ssid) ~= "table"
				or type(ssid.bind_request) ~= "function"
				or type(ssid.restore_request) ~= "function" then
				return false, "ssid bind unavailable"
			end

			local prev = nil
			if type(ssid.get_request) == "function" then
				prev = ssid.get_request()
			end

			local ok_b, err_b = ssid.bind_request({
				profile = bound_profile,
				via = "lpc",
			})
			if not ok_b then
				return false, err_b or "ssid.bind_request fail"
			end

			-- pcall: dispatch 抛错时仍须恢复外层绑定
			local ok_p, ok, result = pcall(dispatch.call, {
				name = tool_name,
				args = tool_args,
				via = "lpc",
			})

			pcall(ssid.restore_request, prev)

			if not ok_p then
				return false, tostring(ok)
			end

			if ok then
				return true, result
			end

			if type(result) == "table" then
				return false, result
			end

			return false, tostring(result)
		end)
	end)

	if not ok_reg then
		return false, "klblpc.call register fail: " .. tostring(err_reg)
	end

	if not klblpc.serve() then
		return false, "klblpc.serve fail"
	end

	g_ready = true
	log.write("info", "lpc bridge ready", {
		module = "lpc_bridge",
		name = M.MODULE_NAME,
	})

	return true
end


-- @brief 是否已 ensure
-- @return ready[boolean]
M.ready = function ()
	return g_ready
end


-- @brief 同步 co_call tool (本 env 内 fork+泵送)
-- @param [in] tool_name[string]
-- @param [in] tool_args[table|nil]
-- @param [in] opts[table|nil]			可选 readonly_only
-- @param [in] timeout_ms[number|nil]	默认 15000
-- @return ok[boolean]
-- @return result_or_err[any]
M.sync_tool_call = function (tool_name, tool_args, opts, timeout_ms)
	local ok_e = M.ensure()
	if not ok_e then
		return false, "lpc bridge ensure fail"
	end

	local kco = require("kco")
	local kenv = require("kenv")
	local klblpc = require("klbcore.klblpc")
	local co_util = require("klbagent.core.co_util")

	if type(timeout_ms) ~= "number" or timeout_ms <= 0 then
		timeout_ms = 15000
	end

	local state = {
		done = false,
		ok = false,
		a1 = nil,
		a2 = nil,
	}

	kco.fork(function ()
		local a1, a2 = klblpc.co_call(
			M.MODULE_NAME,
			"tool",
			tool_name,
			tool_args or {},
			opts or {})
		state.ok = (a1 == true)
		state.a1 = a1
		state.a2 = a2
		state.done = true
	end)

	-- 刷新 env 滴答: 长静置后 tc 陈旧会假超时 (Bug.2026-20)
	if type(kenv.update_tick_count) == "function" then
		kenv.update_tick_count()
	end

	local ok_w, err_w = pcall(co_util.await_done, state, timeout_ms / 1000, "lpc")
	if not ok_w then
		return false, tostring(err_w or "lpc timeout")
	end

	if state.ok then
		return true, state.a2
	end

	return false, state.a2 or state.a1
end


return M

-- end
