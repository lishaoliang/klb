--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  mcp 包对外入口 (server / client / http)
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构 (base/schema 等), 不得破坏本文件导出签名与可观察语义
-- @note   kind=lib; 包外须 require("klbagent.packages.mcp") 使用本入口
--   \n 例: mcp.server().serve(opts) 阻塞常驻; serve_entry 仅 embed 挂接
--   \n 公开表禁止子模块代接: server/client/http 为取面函数 (对齐 ssid.new_*)
--   \n CLI: alua mcp serve|list|call|probe (core/cli/mcp)
--   \n 不导出: base / schema (内部; 包外禁止直 require)
-- @history 修改历史
--  \n [2026-10] 创建文件 (P1-1)
--  \n [2026-11] 按 base/server/client/schema/http 目录重调接口
--  \n [2026-11] 对外入口仅 server/client/http; 禁止表代接改为取面函数
--]]


local server = require("klbagent.packages.mcp.server")
local http = require("klbagent.packages.mcp.http")
local client = require("klbagent.packages.mcp.client")


local M = {}


--------------------------------------------------------------------------------------------
-- Server / HTTP 挂载


-- @brief 取 MCP server 面
-- @return server[table]
-- @stability stable
-- @note stdio serve + HTTP 会话辅助
M.server = function ()
	return server
end


-- @brief 取 Streamable HTTP 挂载面
-- @return http[table]
-- @stability stable
-- @note mount / handle → alua serve /mcp
M.http = function ()
	return http
end


--------------------------------------------------------------------------------------------
-- Client


-- @brief 取 MCP HTTP client 面
-- @return client[table]
-- @stability stable
-- @note initialize / tools list|call / listen|close
M.client = function ()
	return client
end


return M

-- end
