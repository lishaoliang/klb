--[[
-- Copyright (c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  parser.lua
-- @brief 解析器
--   按一定规则将lua table解析成对话框
--   [2023-4] parse解析时, 加入随机唯一路径机制, 使用者可以省略['path']参数
-- @note 关键字等含义 参考 html5 标准: https://www.w3school.com.cn/html/html5_intro.asp
---   https://www.runoob.com/html/html5-form-input-types.html
--]]
local table = require("table")
local kgui = require("kgui")
local krand = require("krand")
local csser = require("klbcore.klbui.csser")
local event = require("klbcore.klbui.event")


local parser = {}
local E = {}


-- 预定义的 关键字
local CONST_keys = {
	-- 关键窗口树
	-- parse函数已加入随机唯一路径, 使用者可以省略此参数
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
	local event_str = event.transform(msg)
	if 'string' == type(event_str) then		
		if 'function' == type(cmds1[event_str]) then
			cmds1[event_str](x1, y1, x2, y2, lparam, wparam)
		elseif 'function' == type(cmds2[event_str]) then
			cmds2[event_str](x1, y1, x2, y2, lparam, wparam)
		elseif 'function' == type(cmds3[event_str]) then
			cmds3[event_str](x1, y1, x2, y2, lparam, wparam)
		end
	end
	
	return 0
end


-- 3位字符串, 最大范围为 63^3 = 250047
-- 4位字符串, 最大范围为 63^4 = 15752961
local CONST_rand_max = 4

local function ParseWnd(wnd, commonds, css, root_path, first)
	if 'table' ~= type(wnd) then
		return
	end

	-- 参考
	-- https://www.w3school.com.cn/html/html5_intro.asp	
	local t = wnd['type'] or ''
	local pos = wnd['pos'] or {}

	local x = pos[1]
	local y = pos[2]
	local w = pos[3]
	local h = pos[4]
	local path = ''
	
	if '' ~= t then
		if first then
			-- 首次运行, 是顶层对话框, 先尝试使用原始提供路径
			-- 若原始路径已被占用, 则随机分配一个路径地址, 直到无重复为止
			path = root_path
			
			local try_count = 0
			while true do
				if 0 == kgui.append(t, path, x, y, w, h) then
					break -- 添加成功
				end
				
				path = table.concat({'/', krand.rand_string(CONST_rand_max)}) -- 根路径
				
				try_count = try_count + 1
				if 99999 <= try_count then
					assert(false) -- 有错误, 类型不正确等
					path = ''
					break
				end
			end
			
			wnd['path'] = path -- 更新路径
		else
			-- 非首次运行, 则不是顶层对话框
			-- 随机分配一个路径地址, 直到无重复为止
			local try_count = 0
			while true do
				--path = root_path .. '/' .. krand.rand_string(CONST_rand_max)
				path = table.concat({root_path, '/', krand.rand_string(CONST_rand_max)})
				
				if 0 == kgui.append(t, path, x, y, w, h) then
					break -- 添加成功
				end
				
				try_count = try_count + 1
				if 99999 <= try_count then
					assert(false)	-- 有错误, 类型不正确等
					path = ''
					break
				end
			end
			
			wnd['path'] = path -- 更新路径
		end	
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
		ParseWnd(v, commonds, css, path, false)
	end
	
	-- 子窗口: 第2种表达方式
	local child = wnd['child'] or {}
	for _, v in ipairs(child) do
		ParseWnd(v, commonds, css, path, false)
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
	local param_cmds = commonds or {}
	local param_css = css or {}
	local root_path = dialog['path'] or table.concat({'/', krand.rand_string(CONST_rand_max)}) -- 根路径
	
	ParseWnd(dialog, param_cmds, param_css, root_path, true)
end


return parser
