--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbpfs Lua 脚本侧入口 (桩)
-- @stability experimental
-- @impl evolving
-- @attention
--   \n @stability experimental: 对外接口可随里程碑调整; 变更仍须保持可观察语义可追踪
--   \n @impl evolving: 实现未冻结; 本文件仅为契约壳
-- @note   require("klbcore.klbpfs"); C 绑定为 require("kpfs") (src_packages/klbpfs)
--   \n 本目录为纯 Lua 应用层; 与 kpfs 预加载库分层, 互不替代
-- @history 修改历史
--  \n [2026-11] 创建文件 (桩)
--]]


local klbpfs = {}


return klbpfs

-- end
