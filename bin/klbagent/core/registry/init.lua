--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  工具注册中枢
-- @note   加载 core.tools.* 并 register 到 catalog / dispatch
-- @history 修改历史
--  \n [2026-09] 创建文件
--  \n [2026-10] 注册 tools.proxy (桩); 注册 tools.auth (授权模块)
--  \n [2026-11] 删除 tools.auth (对外改用 ssid.*)
--  \n [2026-11] 去掉 load_all 末尾无条件 lpc_bridge.ensure (常驻协程致 CLI 不退出); 改按需
--]]


local M = {}


local PACKAGES = {
	"klbagent.core.tools.runtime",
	"klbagent.core.tools.catalog",
	"klbagent.core.tools.docs",
	"klbagent.core.tools.config",
	"klbagent.core.tools.ssid",
	"klbagent.core.tools.fs",
	"klbagent.core.tools.net",
	"klbagent.core.tools.proxy",
	"klbagent.core.tools.remote",
	"klbagent.core.tools.peer",
	"klbagent.core.tools.lpc",
	"klbagent.core.tools.deploy",
	"klbagent.core.tools.device",
	"klbagent.core.tools.unsafe",
	"klbagent.core.tools.log",
}



-- @brief 注册全部内置工具
-- @param [in] catalog[table]
-- @param [in] dispatch[table]
-- @return ok[boolean]
-- @return err[string|nil]
M.load_all = function (catalog, dispatch)
	for i = 1, #PACKAGES do
		local name = PACKAGES[i]
		local mod = require(name)
		local ok, err = mod.register(catalog, dispatch)
		if not ok then
			return false, tostring(name) .. ": " .. tostring(err)
		end
	end

	-- LPC alua.tools 不在此接线: ensure 会 fork 常驻收包协程, 令 CLI 全局协程数永不归零
	--   按需点: core/tools/lpc/call.lua (sync_tool_call)、core/tools/unsafe/run_file.lua、
	--   core/serve/base/wboot.lua (worker)
	return true
end


return M
