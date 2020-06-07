--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @file   kos.lua
-- @brief  C kos
--   \n require("kos")
-- @author 李绍良
-- @version 0.1
--]]

local kos = {}

-- @name   kos.os
-- @export 操作系统名称
kos.os = 'linux' -- 'linux', 'windows', 'apple', 'unix'

-- @name   kos.arch
-- @export 芯片架构
kos.arch = 'amd64'

return kos
