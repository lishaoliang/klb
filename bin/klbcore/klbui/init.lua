--[[
-- Copyright (c) 2022, GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  init.lua
-- @brief klbui init.lua
-- @note 参考 html5 标准: https://www.w3school.com.cn/html/html5_intro.asp
--     参考: https://www.runoob.com/html/html5-form-input-types.html
--     参考: https://www.w3school.com.cn/jquery/index.asp
--]]

local kgui = require("kgui")
local parser = require("klbcore.klbui.parser")
local selector = require("klbcore.klbui.selector")


local klbui = {
	['parse'] = parser.parse,
	
	['select'] = selector.select,
}

return klbui
