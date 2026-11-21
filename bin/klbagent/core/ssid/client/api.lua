--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   api.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  (已弃用) 原共用 bind; 现由 lpcclient / xpcclient 显式导出 co_*
-- @stability internal
-- @impl evolving
-- @note   禁止业务方 require; 保留空壳避免旧路径崩
-- @history 修改历史
--  \n [2026-11] 创建文件; 自 client 拆出统一业务面
--  \n [2026-11] LPC/XPC 方法分区; 禁客户端触 raw
--  \n [2026-11] 导出改为 co_whoami 等
--  \n [2026-11] 逻辑迁入 lpcclient/xpcclient 显式导出; 本文件停用 bind
--]]


local M = {}


-- @brief 已弃用: 请用 lpcclient / xpcclient 的 M.co_* 
M.bind = function (call_fn, transport)
	call_fn = call_fn
	error("ssid.client.api.bind removed; use lpcclient/xpcclient co_* exports (" .. tostring(transport) .. ")")
end


return M

-- end
