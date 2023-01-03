--[[
-- Copyright (c) 2022, GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  csser.lua
-- @brief CSS样式规则
-- @note 关键字等含义 参考 html5 标准: https://www.w3school.com.cn/jquery/jquery_ref_css.asp
--]]
local kgui = require("kgui")

local csser = {}
local E = {}


local function SetAttribute(path, s, t)
	local attrs = ('table' == type(t[s]) and t[s]) or E
	
	for k, v in pairs(attrs) do
		kgui.set(path, k, v)
	end	
end


--[[
local css = {
	['type'] = {
		['kdialog'] = {
			['title'] = '测试解析对话框CSS'
		},
		
		['kbutton'] = {
			['title'] = '按钮CSS'
		}
	},
	
	['class'] = {
		
	},
	
	['name'] = {
		['btn4'] = {
			['title'] = '按钮4-CSS'
		}
	},

	['id'] = {
		
	},
}
--]]
function csser.css(wnd, path, css)
	if 'table' ~= type(css) then
		return
	end
	
	local css_type = css['type'] or E
	local css_class = css['class'] or E
	local css_name = css['name'] or E
	local css_id = css['id'] or E

	-- 1. css type
	local t = wnd['type'] or ''
	SetAttribute(path, t, css_type)
	
	-- 2. css class
	local c = wnd['class'] or ''
	SetAttribute(path, c, css_class)
	
	-- 3. css name
	local n = wnd['name'] or ''
	SetAttribute(path, n, css_name)
	
	-- 4. css id
	local id = wnd['id'] or ''
	SetAttribute(path, id, css_id)
	
end


function csser.css_style(wnd, path)
	local style = wnd['style'] or E

	-- css style
	for k, v in pairs(style) do
		kgui.set(path, k, v)
	end		
end


return csser
