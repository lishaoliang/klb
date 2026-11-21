--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid 共享层常量入口 (无子模块代接)
-- @stability internal
-- @impl evolving
-- @note   勿挂 fingerprint/store/ctx 代接; 请直接 require base.* 子模块
-- @history 修改历史
--  \n [2026-11] 创建文件
--  \n [2026-11] 去掉子模块表代接
--]]
local const = require("klbagent.core.ssid.base.const")


local M = {}


M.MODULE_NAME = const.MODULE_NAME
M.SSID_LEN = const.SSID_LEN
M.DIGEST_LEN = const.DIGEST_LEN
M.SALT_LEN = const.SALT_LEN
M.RENEW_MARGIN_MS = const.RENEW_MARGIN_MS
M.TEMP_TOKEN_PREFIX = const.TEMP_TOKEN_PREFIX


return M

-- end
