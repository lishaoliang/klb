--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 内置中间件工厂
-- @note   cors / basic / log / limit / https / bearer / session; 子目录可再扩
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local mw = {}

mw.cors = require("klbcore.klbweb.mw.cors")
mw.basic = require("klbcore.klbweb.mw.basic")
mw.log = require("klbcore.klbweb.mw.log")
mw.limit = require("klbcore.klbweb.mw.limit")
mw.https = require("klbcore.klbweb.mw.https")
mw.bearer = require("klbcore.klbweb.mw.bearer")
mw.session = require("klbcore.klbweb.mw.session")


return mw
