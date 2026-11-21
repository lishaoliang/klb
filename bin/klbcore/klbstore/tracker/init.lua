--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
--
-- @file   init.lua
-- @brief  klbstore 拓扑与选点 (组 / 节点)
-- @stability experimental
-- @impl evolving
-- @note   桩: 本文件仅契约; 全部接口回 nil, E_NOTIMPL
--   \n P0: tracker 与 storage 同进程, 只维护 group -> node 表, 不落盘
--   \n P1: 节点注册 / 心跳 / 主备切换; 选点按 round-robin 或剩余空间
--   \n 集群最终一致, 不做自动选主
--]]


local klbcode = require("klbcore.klbcode")
local base = klbcode.base


local mod = {}


-- @brief 创建 tracker
-- @param [in] cfg[table|nil]		{ policy = "round-robin" | "space" }
-- @return [table] tracker 对象; 失败 nil, code
-- @stability experimental
mod.new = function (cfg)
	return nil, base.NOTIMPL
end


-- @brief 注册存储节点
-- @param [in] tk[table]			tracker 对象
-- @param [in] node[table]			{ id, group, endpoint, master }
-- @return [boolean] true 成功; 失败 nil, code
-- @stability experimental
mod.add = function (tk, node)
	return nil, base.NOTIMPL
end


-- @brief 注销存储节点
-- @param [in] tk[table]			tracker 对象
-- @param [in] node_id[string]		节点标识
-- @return [boolean] true 成功; 失败 nil, code
-- @stability experimental
mod.remove = function (tk, node_id)
	return nil, base.NOTIMPL
end


-- @brief 按组选点
-- @param [in] tk[table]			tracker 对象
-- @param [in] group[string]		组名
-- @return [table] node; 失败 nil, code
-- @stability experimental
mod.choose = function (tk, group)
	return nil, base.NOTIMPL
end


-- @brief 列举组内节点
-- @param [in] tk[table]			tracker 对象
-- @param [in] group[string|nil]	组名; nil 为全部
-- @return [table] node 数组; 失败 nil, code
-- @stability experimental
mod.list = function (tk, group)
	return nil, base.NOTIMPL
end


-- @brief 心跳 / 状态上报
-- @param [in] tk[table]			tracker 对象
-- @param [in] node_id[string]		节点标识
-- @param [in] info[table]			{ free, used, conn }
-- @return [boolean] true 成功; 失败 nil, code
-- @stability experimental
mod.heartbeat = function (tk, node_id, info)
	return nil, base.NOTIMPL
end


return mod
