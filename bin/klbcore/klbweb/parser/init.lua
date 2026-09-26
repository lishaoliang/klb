--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 请求解析
-- @note   请求行 / headers / query / cookie / form / json / multipart / Range; 子目录可再扩
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local head = require("klbcore.klbweb.parser.head")
local query = require("klbcore.klbweb.parser.query")
local json = require("klbcore.klbweb.parser.json")
local multipart = require("klbcore.klbweb.parser.multipart")
local range = require("klbcore.klbweb.parser.range")


local parser = {}

parser.parse_request = head.parse_request
parser.parse_headers = head.parse_headers
parser.parse_query = query.parse_query
parser.parse_cookies = query.parse_cookies
parser.parse_form = query.parse_form
parser.parse_json = json.parse_json
parser.parse_multipart = multipart.parse_multipart
parser.parse_range = range.parse_range


return parser
