--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   const.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  mcp 共享常量 (协议版本 / JSON-RPC 码 / 分页与空闲)
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.packages.mcp.base.const"); 非对外契约面
-- @history 修改历史
--  \n [2026-11] 创建文件; 自 server/client 抽出
--]]


local M = {}


-- 锚定协议修订 (见 PROTOCOL.md)
M.PROTOCOL_VERSION = "2025-06-18"

-- 支持集: 集内回相同; 否则回锚定 PROTOCOL_VERSION
M.SUPPORTED_PROTOCOL_VERSIONS = {
	["2025-06-18"] = true,
}


-- JSON-RPC 2.0 错误码
M.PARSE_ERROR = -32700
M.INVALID_REQUEST = -32600
M.METHOD_NOT_FOUND = -32601
M.INVALID_PARAMS = -32602
M.INTERNAL_ERROR = -32603

-- 空闲分段阈值 (ms); 再次 initialize 亦切段
M.IDLE_SEGMENT_MS = 30 * 60 * 1000

-- tools/list 分页大小
M.TOOLS_PAGE_SIZE = 64


-- @brief 版本协商: 支持集内回相同, 否则锚定
-- @param [in] want[string|nil]
-- @return ver[string]
M.negotiate_protocol_version = function (want)
	if type(want) == "string" and want ~= "" and M.SUPPORTED_PROTOCOL_VERSIONS[want] == true then
		return want
	end

	return M.PROTOCOL_VERSION
end


return M

-- end
