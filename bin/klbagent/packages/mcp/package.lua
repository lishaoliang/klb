--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   package.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  mcp 可选包清单 (MCP server stdio + HTTP client)
-- @note   tier=optional; kind=lib (仅 require; 入口走 alua mcp)
--   \n 目录: base/ server/ client/ schema/ http/
--   \n 对外入口: require("klbagent.packages.mcp") → .server() / .client() / .http()
-- @history 修改历史
--  \n [2026-10] 创建文件 (P1-1)
--  \n [2026-11] 目录化 base/server/client/schema/http
--  \n [2026-11] 根/子面禁代接: 取面函数 + 实体转发
--]]

return {
	name = "mcp",
	version = "0.1.8",
	tier = "optional",
	kind = "lib",
	summary = "MCP: stdio server (tools) + HTTP client (initialize/tools list/call)",
	main = "klbagent.packages.mcp",
}

-- end
