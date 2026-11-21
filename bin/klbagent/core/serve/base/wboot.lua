--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   wboot.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  serve/base: 消费线程本 env 启动前置 (tools / profile / LPC)
-- @stability internal
-- @impl evolving
-- @note   各消费 kthread 独立 klua_env; 须在本 env 再 ensure (主线程注册不到本 env)
--   \n 供 http/long/rpc 入口调用; 勿作对外契约面
-- @history 修改历史
--  \n [2026-11] 创建文件; P0 对接 worker 消费端; 自 worker/ 迁入 base (worker 仅留线程入口)
--]]
local boot = require("klbagent.core.serve.base.boot")
local state = require("klbagent.core.serve.base.state")
local auth = require("klbagent.core.serve.security.auth")
local profile = require("klbagent.core.serve.security.profile")


local M = {}


-- @brief 本消费 env 启动前置 (幂等 tools; 透传 profile / serve_opts)
-- @param [in] opts[table|nil]			profile / port / port_mode / tls_info
-- @return ok[boolean]
-- @return err[string|nil]
M.ensure = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local ok_t, err_t = boot.ensure_tools()
	if not ok_t then
		return false, err_t
	end

	do
		local ok_g, ai_load_gate = pcall(require, "klbagent.core.ai_load_gate")
		if ok_g and type(ai_load_gate) == "table" and type(ai_load_gate.set_channel) == "function" then
			pcall(ai_load_gate.set_channel, "serve")
		end
	end

	auth.ensure_sweeper()

	if type(opts.profile) == "string" and opts.profile ~= "" then
		local ok_pf, err_pf = profile.apply_profile(opts.profile)
		if not ok_pf then
			return false, err_pf
		end
	end

	state.serve_opts = {
		port = opts.port,
		port_mode = opts.port_mode,
		profile = opts.profile,
	}

	do
		local ok_lpc, lpc_bridge = pcall(require, "klbagent.core.lpc_bridge")
		if ok_lpc and type(lpc_bridge) == "table" and type(lpc_bridge.ensure) == "function" then
			pcall(lpc_bridge.ensure)
		end
	end

	return true
end


return M

-- end
