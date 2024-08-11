--[[
-- Copyright (c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   parser.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  解析器
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
	['commands'] = true,	-- 静态命令集
	['_commands'] = true,	-- 动态命令集
}


local function OnCommand(cmds1, cmds2, cmds3, obj, msg, x1, y1, x2, y2, lparam, wparam)
	-- commond 命令 优先集次序
	-- 1. cmds1 动态命令集 
	-- 2. cmds2 由 parse 第二参数 外部静态命令集
	-- 3. cmds3 由 parse 第一参数 dialog 里面静态命令集
	local b1 = kgui.b1_event(msg)
	local b2 = kgui.b2_event(msg)
	local b3 = kgui.b3_event(msg)
	local e = kgui.to_event(msg)	
	
	local ret = 0
	
	local event_str = event.transform(e)
	if 'string' == type(event_str) then		
		if 'function' == type(cmds1[event_str]) then
			ret = cmds1[event_str](x1, y1, x2, y2, lparam, wparam, b1, b2, b3)
		elseif 'function' == type(cmds2[event_str]) then
			ret = cmds2[event_str](x1, y1, x2, y2, lparam, wparam, b1, b2, b3)
		elseif 'function' == type(cmds3[event_str]) then
			ret = cmds3[event_str](x1, y1, x2, y2, lparam, wparam, b1, b2, b3)
		end
	end
	
	-- ret 可能被 返回 非数值
	if 'number' == type(ret) then
		return ret
	end
	
	return 0
end


-- 自动对齐区域(x, y, w, h)
-- -1: 将需要自动对齐父窗口的宽高
local function AutoRect(parent_path, x, y, w, h)
	local auto_x, auto_y, auto_w, auto_h = x, y, w, h
	
	-- do. 1. 不需要处理
	if 0 <= auto_x and 0 <= auto_y and 0 <= auto_w and 0 <= auto_h then
		return auto_x, auto_y, auto_w, auto_h
	end
	
	-- 屏幕宽/高
	local sceen_w, sceen_h = kgui.wh()
	
	-- 父窗口宽/高
	local parent_rect = {}
	if parent_path then
		parent_rect = kgui.wndpos(parent_path, false)
	end
	
	local parent_w = parent_rect['w'] or sceen_w
	local parent_h = parent_rect['h'] or sceen_h
	
	-- do. 2 自动宽
	if 0 <= x and auto_w < 0 then
		auto_w = parent_w - x
		
		if auto_w < 0 then
			auto_w = 0
		end
	end
	
	-- do. 3. 自动高
	if 0 <= y and auto_h < 0 then
		auto_h = parent_h - y
		
		if auto_h < 0 then
			auto_h = 0
		end
	end
	
	-- do. 4. 自动x, 向右对齐
	if x < 0 and auto_w >= 0 then
		auto_x = parent_w - auto_w
		
		if auto_x < 0 then
			auto_x = 0
		end
	end
	
	-- do. 5. 自动y, 向下对齐
	if y < 0 and auto_h >= 0 then
		auto_y = parent_h - auto_h
		
		if auto_y < 0 then
			auto_y = 0
		end
	end
	
	return auto_x, auto_y, auto_w, auto_h
end


-- 3位字符串, 最大范围为 63^3 = 250047
-- 4位字符串, 最大范围为 63^4 = 15752961
local CONST_rand_max = 4


-- 解析窗口
local function ParseWnd(wnd, commands, css, parent_path, first)
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
			path = parent_path
			
			-- 简易自动(x,y,w,h)
			x, y, w, h = AutoRect(nil, x, y, w, h)
			
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
			
			-- 简易自动(x,y,w,h)
			x, y, w, h = AutoRect(parent_path, x, y, w, h)
			
			local try_count = 0
			while true do
				--path = parent_path .. '/' .. krand.rand_string(CONST_rand_max)
				path = table.concat({parent_path, '/', krand.rand_string(CONST_rand_max)})
				
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
	
	-- 将 cmds2 / cmds3 从bind_command绑定函数中独立提取出来, 只在解析时计算一次
	-- cmds2, 独立填写的相应命令集合
	-- 只对 有名称(name)的控件做消息绑定, 优化控件的消息响应数量
	-- 而 编号(id) 用于 检索到控件后, 做CSS操作等
	local name = wnd['name'] or ''
	
	if '' ~= name then
		local cmds2 = E		
		if 'table' == type(commands[name]) then
			cmds2 = commands[name]	-- 依据名称判定
		end
		
		-- cmds3, 内嵌在 dialog里面的
		local cmds3 = ('table' == type(wnd['commands']) and wnd['commands']) or E	
		
		-- 绑定命令函数
		kgui.bind_command(path, function (obj, msg, x1, y1, x2, y2, lparam, wparam)
			-- cmds1, 动态绑定表, 每次响应时动态计算
			local cmds1 = ('table' == type(wnd['_commands']) and wnd['_commands']) or E
			
			return OnCommand(cmds1, cmds2, cmds3, obj, msg, x1, y1, x2, y2, lparam, wparam)
		end)
	end
	
	-- 子窗口: 第1种表达方式
	for _, v in ipairs(wnd) do
		ParseWnd(v, commands, css, path, false)
	end
	
	-- 子窗口: 第2种表达方式
	local child = wnd['child'] or {}
	for _, v in ipairs(child) do
		ParseWnd(v, commands, css, path, false)
	end
	
	-- 窗口解析完成, 触发 'onparsewindow' 事件
	kgui.call_control_and_command(path, event.onparsewindow)
end

function parser.parse(dialog, commands, css)
	local param_cmds = commands or {}
	local param_css = css or {}
	local root_path = dialog['path'] or table.concat({'/', krand.rand_string(CONST_rand_max)}) -- 根路径
	
	ParseWnd(dialog, param_cmds, param_css, root_path, true)
	
	-- 对话框解析完成, 触发 'onparsedialog' 事件
	kgui.call_control_and_command(root_path, event.onparsedialog)
end


local function UpdateWndCss(wnd, css)
	-- 参考
	-- https://www.w3school.com.cn/html/html5_intro.asp
	local path = wnd['path']
	
	-- 生效 自定义CSS
	csser.css(wnd, path, css)

	-- 子窗口: 第1种表达方式
	for _, v in ipairs(wnd) do
		UpdateWndCss(v, css)
	end
	
	-- 子窗口: 第2种表达方式
	local child = wnd['child'] or {}
	for _, v in ipairs(child) do
		UpdateWndCss(v, css)
	end
end


function parser.update_css(dialog, css)
	local param_css = css or {}
	
	UpdateWndCss(dialog, param_css)
end


return parser
