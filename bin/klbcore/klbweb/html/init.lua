--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 内置错误页表
-- @note   [code] = 页长字符串; 非模板引擎; 单页仍可 require html.htmlNNN
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local html = {
	[400] = require("klbcore.klbweb.html.html400"),
	[401] = require("klbcore.klbweb.html.html401"),
	[403] = require("klbcore.klbweb.html.html403"),
	[404] = require("klbcore.klbweb.html.html404"),
	[405] = require("klbcore.klbweb.html.html405"),
	[413] = require("klbcore.klbweb.html.html413"),
	[429] = require("klbcore.klbweb.html.html429"),
	[500] = require("klbcore.klbweb.html.html500"),
}


return html
