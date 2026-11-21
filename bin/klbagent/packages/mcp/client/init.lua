--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  MCP client 对外接口面
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 实现见 service.lua; 不得破坏本文件导出签名与可观察语义
-- @note   经 require("klbagent.packages.mcp").client; 实现 klbagent.packages.mcp.client.service
--   \n 公开表禁止代接: 函数须实体转发; 常量仅标量/自建表拷贝
-- @history 修改历史
--  \n [2026-10] 创建文件 (P1-1)
--  \n [2026-11] 入口仅保留对外导出; 实现迁 service.lua
--  \n [2026-11] 按禁止代接规则改为实体函数转发
--]]


local mcp_const = require("klbagent.packages.mcp.base.const")
local svc = require("klbagent.packages.mcp.client.service")


local M = {}


M.PROTOCOL_VERSION = mcp_const.PROTOCOL_VERSION
M.SUPPORTED_PROTOCOL_VERSIONS = {
	["2025-06-18"] = true,
}


-- @brief 构造客户端上下文
-- @param [in] opts[table]			url/http(s); token/timeout_sec/max_bytes 可选
-- @return ctx[table|nil]
-- @return err[string|nil]
-- @stability stable
M.new_context = function (opts)
	return svc.new_context(opts)
end


-- @brief initialize + notifications/initialized (幂等)
-- @param [in] ctx[table]
-- @param [in] opts[table|nil]			protocol_version 可选
-- @return info[table|nil]
-- @return err[string|nil]
-- @stability stable
M.initialize = function (ctx, opts)
	return svc.initialize(ctx, opts)
end


-- @brief 列出远端工具 (初始化后 tools/list)
-- @param [in] ctx[table]
-- @return tools[table|nil]
-- @return err[string|nil]
-- @stability stable
M.list_tools = function (ctx)
	return svc.list_tools(ctx)
end


-- @brief 调用远端工具
-- @param [in] ctx[table]
-- @param [in] name[string]
-- @param [in] args[table|nil]
-- @return result[table|nil]
-- @return err[string|nil]
-- @stability stable
M.call_tool = function (ctx, name, args)
	return svc.call_tool(ctx, name, args)
end


-- @brief ping (初始化后)
-- @param [in] ctx[table]
-- @return result[table|nil]
-- @return err[string|nil]
-- @stability stable
M.ping = function (ctx)
	return svc.ping(ctx)
end


-- @brief DELETE 结束远端 MCP 会话
-- @param [in] ctx[table]
-- @return ok[boolean]
-- @return err[string|nil]
-- @stability stable
M.close = function (ctx)
	return svc.close(ctx)
end


-- @brief GET 订阅 server→client SSE
-- @param [in] ctx[table]
-- @param [in] opts[table|nil]			timeout_sec / max_events
-- @return events[table|nil]
-- @return err[string|nil]
-- @stability stable
M.listen = function (ctx, opts)
	return svc.listen(ctx, opts)
end


return M

-- end
