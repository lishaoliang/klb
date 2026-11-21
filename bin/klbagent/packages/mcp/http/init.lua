--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  MCP Streamable HTTP 对外接口面
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 实现见 handler.lua; 不得破坏本文件导出签名与可观察语义
-- @note   经 require("klbagent.packages.mcp").http; 实现 klbagent.packages.mcp.http.handler
--   \n 公开表禁止代接: 函数须实体转发
-- @history 修改历史
--  \n [2026-10] 创建文件
--  \n [2026-11] 入口仅保留对外导出; 实现迁 handler.lua
--  \n [2026-11] 按禁止代接规则改为实体函数转发
--]]


local handler = require("klbagent.packages.mcp.http.handler")


local M = {}


-- @brief 处理 Streamable HTTP /mcp 请求 (auth 由调用方先校验; 须在 kco 协程内)
-- @param [in] req[table]
-- @param [in] res[table]
-- @return handled[boolean]
-- @stability stable
M.co_handle = function (req, res)
	return handler.co_handle(req, res)
end


-- @brief 挂到 klbweb app: POST/GET/DELETE /mcp
-- @param [in] app[table]
-- @param [in] auth_fn[function]		function(req,res)->ok; false 时已写响应
-- @return ok[boolean]
-- @return err[string|nil]
-- @stability stable
M.mount = function (app, auth_fn)
	return handler.mount(app, auth_fn)
end


return M

-- end
