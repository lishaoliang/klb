--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   lpccall.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid LPC 传输调用 (须在 kco 协程内)
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.client.lpccall")
-- @history 修改历史
--  \n [2026-11] 自 lpcclient 迁出
--]]
local const = require("klbagent.core.ssid.base.const")
local util = require("klbagent.core.ssid.client.util")
local klblpc = require("klbcore.klblpc")


local M = {}


-- @brief 经 LPC 调 owner
-- @param [in] method[string]
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return result[any]
-- @return err[string|nil]
M.co_call = function (method, opts, timeout_ms)
	-- Lua 5.2+: running() 在主线程仍返回 thread; 须看 ismain
	local co, is_main = coroutine.running()
	if co == nil or is_main == true then
		return nil, "ssid lpc: need kco coroutine"
	end

	local ok_c, a1, a2 = pcall(klblpc.co_call_timed,
		const.MODULE_NAME,
		util.norm_timeout_ms(timeout_ms),
		method,
		util.norm_opts(opts))
	if not ok_c then
		return nil, "ssid lpc fail: " .. tostring(a1)
	end

	if a1 == true then
		return a2
	end

	return nil, a2 or a1 or "ssid lpc fail"
end


return M

-- end
