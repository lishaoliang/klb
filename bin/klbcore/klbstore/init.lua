--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
--
-- @file   init.lua
-- @brief  klbstore 轻量应用层文件存储 (Lua 契约入口 / 聚合)
-- @stability experimental
-- @impl evolving
-- @note   require("klbcore.klbstore"); C 包 klbstore (绑定 kstore, 桩)
--   \n 定位: 嵌入式 / 中小型站点文件存储; HTTP REST + 分片上传; P0 单机 (tracker 与 storage 同进程)
--   \n 分层: init 仅聚合与契约; 实现见 id / index / storage / tracker / client / chunk
--   \n 文件标识: group/M00/xx/yy/<name>[.<ext>]; 下载由 klbweb static 承担 (Range/304)
--   \n 失败形状: 统一 nil, code; code 真源 klbcore.klbcode
--   \n P0 状态: index / storage / chunk / server 已可用; tracker / client 仍为桩
--]]


local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local id = require("klbcore.klbstore.id")
local index = require("klbcore.klbstore.index")
local storage = require("klbcore.klbstore.storage")
local tracker = require("klbcore.klbstore.tracker")
local client = require("klbcore.klbstore.client")
local chunk = require("klbcore.klbstore.chunk")
local server = require("klbcore.klbstore.server")
local token = require("klbcore.klbstore.token")


local klbstore = {}


--------------------------------------------------------------------------------------------
-- 返回码 (真源 klbcore.klbcode)

klbstore.OK = klbcode.OK
klbstore.E_ARG = base.INVAL
klbstore.E_NOENT = base.NOENT
klbstore.E_IO = base.IO
klbstore.E_NOSPC = base.NOSPC
klbstore.E_BUSY = base.BUSY
klbstore.E_NOTIMPL = base.NOTIMPL
klbstore.E_AUTH = base.ACCES
klbstore.E_RANGE = base.RANGE


--------------------------------------------------------------------------------------------
-- 子模块

klbstore.id = id
klbstore.index = index
klbstore.storage = storage
klbstore.tracker = tracker
klbstore.client = client
klbstore.chunk = chunk
klbstore.server = server
klbstore.token = token


return klbstore
