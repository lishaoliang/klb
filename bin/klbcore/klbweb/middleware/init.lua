--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 内置中间件工厂
-- @note   cors / basic / log / limit / https / bearer / session / csrf / request_id; 子目录可再扩
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local mw = {}

mw.cors = require("klbcore.klbweb.middleware.cors")
mw.basic = require("klbcore.klbweb.middleware.basic")
mw.log = require("klbcore.klbweb.middleware.log")
mw.limit = require("klbcore.klbweb.middleware.limit")
mw.https = require("klbcore.klbweb.middleware.https")
mw.bearer = require("klbcore.klbweb.middleware.bearer")
mw.session = require("klbcore.klbweb.middleware.session")
mw.csrf = require("klbcore.klbweb.middleware.csrf")
mw.request_id = require("klbcore.klbweb.middleware.request_id")


return mw
