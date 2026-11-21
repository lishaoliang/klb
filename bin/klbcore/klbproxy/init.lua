--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbproxy 代理 IO 门面包 (Lua 契约入口 / 聚合)
-- @note   require("klbcore.klbproxy"); 子模块 io 提供 co_* 优先 + 同步回退
--   \n 分层: init 仅聚合与契约; 实现见 io
--   \n 宿主须具备 kproxy 绑定; 无则各 IO 调用回 nil/err
-- @history 修改历史
--  \n [2026-10] 目录化: 由 kproxy_io.lua 拆为 init + io/
--]]


local proxy_io = require("klbcore.klbproxy.io")


local klbproxy = {}


klbproxy.io = proxy_io


return klbproxy

-- end
