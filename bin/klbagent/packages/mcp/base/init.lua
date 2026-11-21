--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  mcp 共享层入口 (常量; 无子模块表代接)
-- @stability internal
-- @impl evolving
-- @note   请直接 require base.const; 本文件仅导出常用常量别名 (标量/自建表/实体转发)
-- @history 修改历史
--  \n [2026-11] 创建文件
--  \n [2026-11] 禁止函数代接; negotiate 改为实体转发
--]]


local const = require("klbagent.packages.mcp.base.const")


local M = {}


M.PROTOCOL_VERSION = const.PROTOCOL_VERSION
M.PARSE_ERROR = const.PARSE_ERROR
M.INVALID_REQUEST = const.INVALID_REQUEST
M.METHOD_NOT_FOUND = const.METHOD_NOT_FOUND
M.INVALID_PARAMS = const.INVALID_PARAMS
M.INTERNAL_ERROR = const.INTERNAL_ERROR
M.IDLE_SEGMENT_MS = const.IDLE_SEGMENT_MS
M.TOOLS_PAGE_SIZE = const.TOOLS_PAGE_SIZE

M.SUPPORTED_PROTOCOL_VERSIONS = {
	["2025-06-18"] = true,
}


-- @brief 版本协商: 支持集内回相同, 否则锚定
-- @param [in] want[string|nil]
-- @return ver[string]
M.negotiate_protocol_version = function (want)
	return const.negotiate_protocol_version(want)
end


return M

-- end
