--[[
-- Copyright (c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  parser.lua
-- @brief 解析器
---   按一定规则将lua table解析成对话框
-- @note 关键字等含义 参考 html5 标准: https://www.w3school.com.cn/html/html5_intro.asp
---   https://www.runoob.com/html/html5-form-input-types.html
--]]
local kgui = require("kgui")
local csser = require("klbcore.klbui.csser")


local parser = {}
local E = {}


-- 预定义的 关键字
local CONST_keys = {
	-- 关键窗口树
	['path'] = true,		-- 虚拟路径(仿路径系统), 建立窗口树
	
	-- 可检索选择属性
	['type'] = true,		-- 控件类型
	['class'] = true,		-- 类
	['name'] = true,		-- 名称
	['id'] = true,			-- id编号
	
	-- css 样式
	['style'] = true,		-- css样式
	
	-- 显示区域
	['pos'] = true,			-- 位置,大小
	
	-- 子窗口
	['child'] = true,		-- 包含的子窗口
	
	-- 命令集
	['commonds'] = true,	-- 静态命令集
	['_commonds'] = true,	-- 动态命令集
}


local function OnCommond(cmds1, cmds2, cmds3, obj, msg, x1, y1, x2, y2, lparam, wparam)
	-- commond 命令 优先集次序
	-- 1. cmds1 动态命令集 
	-- 2. cmds2 由 parse 第二参数 外部静态命令集
	-- 3. cmds3 由 parse 第一参数 静态命令集
	if 0x0201 == msg or 0x0203 == msg then
		if 'function' == type(cmds1['click']) then
			cmds1['click'](x1, y1, x2, y2, lparam, wparam)
		elseif 'function' == type(cmds2['click']) then
			cmds2['click'](x1, y1, x2, y2, lparam, wparam)
		elseif 'function' == type(cmds3['click']) then
			cmds3['click'](x1, y1, x2, y2, lparam, wparam)
		end
	end
	
	return 0
end


local function ParseWnd(wnd, commonds, css)
	if 'table' ~= type(wnd) then
		return
	end

	-- 参考
	-- https://www.w3school.com.cn/html/html5_intro.asp
	local path = wnd['path'] or '/'
	local t = wnd['type'] or ''
	local pos = wnd['pos'] or {}

	local x = pos[1]
	local y = pos[2]
	local w = pos[3]
	local h = pos[4]
	
	if '' ~= t then
		kgui.append(t, path, x, y, w, h)
	end
	
	-- 生效 css
	csser.css(wnd, path, css)

	-- 生效	css-style 样式
	csser.css_style(wnd, path)
	
	-- set 其他属性
	for k, v in pairs(wnd) do
		if 'function' ~= type(v) and not CONST_keys[k] then
			kgui.set(path, k, v)
		end
	end

	-- 绑定命令函数
	kgui.bind_command(path, function (obj, msg, x1, y1, x2, y2, lparam, wparam)
		local cmds1 = ('table' == type(wnd['_commonds']) and wnd['_commonds']) or E
		local cmds2 = ('table' == type(commonds[path]) and commonds[path]) or E
		local cmds3 = ('table' == type(wnd['commonds']) and wnd['commonds']) or E
		return OnCommond(cmds1, cmds2, cmds3, obj, msg, x1, y1, x2, y2, lparam, wparam)
	end)
	
	-- 子窗口: 第1种表达方式
	for _, v in ipairs(wnd) do
		ParseWnd(v, commonds, css)
	end
	
	-- 子窗口: 第2种表达方式
	local child = wnd['child'] or {}
	for _, v in ipairs(child) do
		ParseWnd(v, commonds, css)
	end
end


local function CopyCommonds(src)
	local t = {}
	
	if 'table' ~= type(src) then
		return t
	end
	
	for k, v in pairs(src) do
		t[k] = v
	end
	
	return t
end


function parser.parse(dialog, commonds, css)
	--local tmp_cmds = CopyCommonds(commonds)
	
	ParseWnd(dialog, commonds, css)
end


return parser
