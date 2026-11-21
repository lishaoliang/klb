--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   boot.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 子模块: 启动前置 (注册内置 tools 到 catalog / dispatch)
-- @note   进程内只做一次; serve 入口不依赖调用方预注册, 否则 /v1/call 一律 TOOL_NOT_FOUND
--   \n @stability internal: 实现文件, 勿 require 子路径
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 _ensure_tools
--]]
local registry = require("klbagent.core.registry")
local catalog = require("klbagent.core.catalog")
local dispatch = require("klbagent.core.dispatch")
local state = require("klbagent.core.serve.base.state")


local M = {}


-- @brief 注册内置 tools (进程内只做一次)
-- @return ok[boolean]
-- @return err[string|nil]
M.ensure_tools = function ()
	if state.tools_loaded then
		return true
	end

	local ok, err = registry.load_all(catalog, dispatch)
	if not ok then
		return false, err
	end

	state.tools_loaded = true
	return true
end


return M

-- end
