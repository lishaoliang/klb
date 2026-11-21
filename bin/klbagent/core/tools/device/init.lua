--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  device.* 工具入口
-- @note   device.status; device.uart.* 见 uart.lua
-- @history 修改历史
--  \n [2026-09] 创建文件
--  \n [2026-10] U-P0 device.uart.*
--]]


local uart = require("klbagent.core.tools.device.uart")


local M = {}


local function _status(_args)
	local kuart_ok = false
	local platform = ""
	local ok, mod = pcall(require, "kuart")
	if ok and type(mod) == "table" and type(mod.available) == "function" then
		local av, plat = mod.available()
		kuart_ok = av and true or false
		platform = plat or ""
	end

	local open_handles = {}
	if type(uart.snapshot_handles) == "function" then
		open_handles = uart.snapshot_handles()
	end
	local open_count = #open_handles
	if open_count == 0 then
		local ok_cj, cjson = pcall(require, "cjson")
		if ok_cj and type(cjson) == "table" and cjson.empty_array ~= nil then
			open_handles = cjson.empty_array
		end
	end

	return {
		available = true,
		kuart = kuart_ok,
		platform = platform,
		open_handles = open_handles,
		open_count = open_count,
		tools = {
			"device.status",
			"device.uart.list",
			"device.uart.open",
			"device.uart.read",
			"device.uart.write",
			"device.uart.flush",
			"device.uart.close",
			"device.uart.handles",
			"device.uart.transact",
		},
		message = kuart_ok and "device.uart ready" or "kuart binding missing",
	}
end


-- @brief 注册 device 工具
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "device.status",
		category = "device",
		title = "device capability status",
		summary = "Report device tools and kuart binding availability.",
		min_profile = "device",
		args_schema = {},
		examples = {},
		docs = "tools/device.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("device.status", _status)
	if not ok then
		return false, err
	end

	ok, err = uart.register(catalog, dispatch)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
