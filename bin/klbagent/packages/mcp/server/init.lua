--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  MCP server 对外接口面
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 实现见 service.lua; 不得破坏本文件导出签名与可观察语义
-- @note   经 require("klbagent.packages.mcp").server; 实现 klbagent.packages.mcp.server.service
--   \n 公开表禁止代接: 函数须实体转发; 常量仅标量/自建表拷贝
-- @history 修改历史
--]]


local mcp_const = require("klbagent.packages.mcp.base.const")
local svc = require("klbagent.packages.mcp.server.service")


local M = {}


M.PROTOCOL_VERSION = mcp_const.PROTOCOL_VERSION
M.SUPPORTED_PROTOCOL_VERSIONS = {
	["2025-06-18"] = true,
}


-- @brief 工具列表变更通知 (runtime.reload 后可调)
-- @return sent[boolean]
-- @stability stable
M.notify_tools_list_changed = function ()
	return svc.notify_tools_list_changed()
end


-- @brief 取出 HTTP 会话 SSE 队列
-- @param [in] http_sid[string]
-- @param [in] max_n[number|nil]
-- @return events[table]
-- @stability stable
M.take_sse_events = function (http_sid, max_n)
	return svc.take_sse_events(http_sid, max_n)
end


-- @brief 会话是否仍登记
-- @param [in] http_sid[string]
-- @return ok[boolean]
-- @stability stable
M.has_http_session = function (http_sid)
	return svc.has_http_session(http_sid)
end


-- @brief 激活 HTTP 会话状态为当前
-- @param [in] http_sid[string|nil]		nil=新建
-- @return sid[string|nil]
-- @return err[string|nil]
-- @stability stable
M.activate_http_session = function (http_sid)
	return svc.activate_http_session(http_sid)
end


-- @brief 结束 HTTP 会话
-- @param [in] http_sid[string]
-- @return ok[boolean]
-- @stability stable
M.end_http_session = function (http_sid)
	return svc.end_http_session(http_sid)
end


-- @brief 串行化 HTTP MCP 请求
-- @param [in] fn[function]
-- @return ...
-- @note 须在 kco 协程内
-- @stability stable
M.co_with_http_lock = function (fn)
	return svc.co_with_http_lock(fn)
end


-- @brief 捕获写出并处理一行 JSON-RPC
-- @param [in] line[string]
-- @return frames[table]
-- @stability stable
M.handle_line_captured = function (line)
	return svc.handle_line_captured(line)
end


-- @brief stdio MCP 嵌入式入口 (fork 读循环后立即返回; 不自泵)
-- @param [in] opts[table|nil]			opts.profile: readonly|net|device|unsafe
-- @return ok[boolean]
-- @return err[string|nil]
-- @stability stable
M.serve_entry = function (opts)
	return svc.serve_entry(opts)
end


-- @brief stdio MCP 阻塞常驻 (serve_entry + co_util.run_host_loop)
-- @param [in] opts[table|nil]			同 serve_entry
-- @return ok[boolean]
-- @return err[string|nil]
-- @stability stable
-- @note CLI 下 hold 宿主泵; 独立 env 自调 co_main_loop
M.serve = function (opts)
	return svc.serve(opts)
end


return M

-- end
