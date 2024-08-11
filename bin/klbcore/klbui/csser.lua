--[[
-- Copyright (c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   csser.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  CSS样式规则
-- @note 关键字等含义 参考 H5,CSS3 标准
--  1. https://www.w3school.com.cn/css/index.asp
--  2. https://www.w3school.com.cn/jquery/jquery_ref_css.asp
--]]
local kgui = require("kgui")

local csser = {}
local E = {}


local function SetAttribute(path, s, t)
	local attrs = ('table' == type(t[s]) and t[s]) or E

	-- 位置参数
	local pos = ('table' == type(attrs['pos']) and attrs['pos']) or {}
	local x = pos[1]
	local y = pos[2]
	local w = pos[3]
	local h = pos[4]
	
	-- 移动窗口至(x, y)
	if 'number' == type(x) and 'number' == type(y) then
		kgui.move(path, x, y)
	end
	
	-- 重设窗口大小(w, h)
	if 'number' == type(w) and 'number' == type(h) then
		kgui.resize(path, w, h)
	end
	
	-- 非位置参数
	for k, v in pairs(attrs) do
		if 'pos' ~= k then
			kgui.set(path, k, v)
		end
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



--[[
local dialog = {
	...
	['style'] = {
		['title'] = '按钮CSS'
	}
	...
}
--]]
function csser.css_style(wnd, path)
	local style = wnd['style'] or E

	-- css style
	for k, v in pairs(style) do
		kgui.set(path, k, v)
	end		
end


-- 默认css
function csser.css_default(wnd, path, css_def)
	-- css style
	for k, v in pairs(css_def) do
		kgui.set(path, k, v)
	end	
end


return csser
