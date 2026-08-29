--[[
-- Copyright (c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbui init.lua
-- @note 参考 html5 标准: https://www.w3school.com.cn/html/html5_intro.asp
--     参考: https://www.runoob.com/html/html5-form-input-types.html
--     参考: https://www.w3school.com.cn/jquery/index.asp
-- @history 修改历史
--		[2025-1] 添加设置控件定时器时间间隔
--		[2025-2] 添加 并默认 启用 C++ 扩展控件
--		[2025-7] 添加 操作单个窗口 接口 wnder.new()
--]]
local kco = require("kco")
local kgui = require("kgui")
local parser = require("klbcore.klbui.parser")
local selector = require("klbcore.klbui.selector")
local wnder = require("klbcore.klbui.wnder")


local klbui = {}


------------------------------------------------
-- C++ 支持


-- @brief 启用 C++ 扩展控件
-- @return 无
-- @note 启用后, 支持使用 C++ 编写控件
klbui.using_cpp = function ()
	kgui.using_cpp()
end


------------------------------------------------
-- 窗口等

-- @brief 解析对话框/命令, 并完成gui窗口树初始创建
-- @param [in]      dialog[table]		对话框描述table
-- @param [in]		commands[nil,table]	[可选]命令响应集合(数组)
-- @param [in]      css[nil,table]		[可选]CSS描述table
-- @return 无
-- @note eg.
--	local dialog = {
--		['path'] = '/home',['type'] = 'kdialog',['pos'] = {0, 0, 1280, 720},
--		['title'] = '测试解析对话框!',
--		['child'] = {{...}, {...}, {...}}
--	}
--	local commands = {
--		['/home'] = {
--			['load'] = function ()
--				...
--			end
--			['unload'] = function ()
--				...
--			end
--		},
--		['/xxx'] = {...}
-- 	}
--  local css = {
--		['type'] = {
--			['kdialog'] = { ['title'] = '测试解析对话框CSS'},
--			['kbutton'] = {['title'] = '按钮CSS'}
--		}
--		['class'] = {...},
--		['name'] = {...},
--		['id'] = {...},
--	}
klbui.parse = function (dialog, commands, css)
	return parser.parse(dialog, commands, css)
end


-- @brief 更新对话框CSS
-- @param [in]      dialog[table]			对话框描述table
-- @param [in]      css[nil,table]			[可选]CSS描述table
-- @return 无
klbui.update_css = function (dialog, css)
	return parser.update_css(dialog, css)
end


-- @brief 选择器
-- @param [in]      dialog[table]		对话框描述table: 和 klbui.parse的参数1 一致
-- @param [in]      multi[nil,boolean]	[可选]true.多选; false.单选; 默认false
-- @return [function] 选择器函数
-- @note
--    @param [in]	s[string]		需要选择过滤的字符串
--    @return [table] 选择结果
--    return function(s)
--    	local t = {}
--    	...
--    	return t
--    end
--  s = 字符串规则
--    规则 : '*'		选择所有控件(含对话框本身)
--    规则 : ''			按 'name' 选择所有, eg. 'name1'
--    规则 : '#id'		按 'id' 选择所有, eg. '#aa'
--    规则 : '.class'	按 'class' 选择所有, eg. '.abc'
--    规则 : ':type'	按 'type' 选择所有, eg. ':kbutton'
klbui.select = function (dialog, multi)
	return selector.select(dialog, multi)
end


-- @brief 设置/获取默认全局CSS参数
-- @param [in] [任意]...			参数
-- @return [任意]...				值
-- @note 注意若是设置, 则必须在控件创建之前, 才能在初始化控件时生效;
--   eg. 获取 local text_color = klbui.default_css('color')
--   eg. 设置 klbui.default_css({['color']={255,220,220,220}})
klbui.default_css = function (...)
	local args = {...}
	
	if 1 < #args then
		kgui.set_default_css(...)
	elseif 1 == #args then
		if 'table' == type(args[1]) then
			for k1, v1 in pairs(args[1]) do
				-- set
				kgui.set_default_css(k1, v1)
			end
		else
			return kgui.get_default_css(...)
		end
	end
	
	return klbui
end


-- @brief 设置/获取全局CSS属性参数
-- @param [in]	t[string]			控件类型type: eg. 'kbutton'
-- @param [in]	[任意]...			参数
-- @return [任意]...				值
-- @note 注意若是设置, 则必须在控件创建之前, 才能在初始化控件时生效;
--   eg. 获取 local text_color = klbui.global_css('kbutton', 'color')
--   eg. 设置 klbui.global_css('kbutton', {['color']={255,220,220,220}})
klbui.global_css = function (t, ...)
	local args = {...}
	
	if 1 < #args then
		if 0 ~= kgui.set_global_css(t, ...) then
			return args -- 未找到设置函数, 返回参数
		end	
	elseif 1 == #args then
		if 'table' == type(args[1]) then
			local r = {}
			
			for k1, v1 in pairs(args[1]) do
				-- set
				if 0 ~= kgui.set_global_css(t, k1, v1) then
					r[k1] = v1
				end
			end
			
			return r -- 将未找到设置函数的属性,返回给调用者
		else
			return kgui.get_global_css(t, ...)
		end
	end
	
	return klbui
end


-- @brief 某个控件类型是否支持全局CSS
-- @param [in]	t[string]			控件类型type: eg. 'kbutton'
-- @param [in]	[任意]...			参数
-- @return [bool]	是否支持
klbui.has_global_css = function (t)
	if 'string' == type(t) then
		return kgui.has_global_css(t)
	end
	
	return false
end


-- @brief 设置/获取共享窗口属性; (share window css)
-- @param [in]	path[string]		共享路径: eg. '/klbui/messagebox'
-- @param [in]	[任意]...			参数
-- @return [任意]...				值
-- @note 
--   eg. 获取 local text_color = klbui.shwnd_css('/klbui/messagebox', 'color')
--   eg. 设置 klbui.shwnd_css('/klbui/messagebox', {['color']={255,220,220,220}})
klbui.shwnd_css = function (path, ...)
	local args = {...}
	
	if 1 < #args then
		kgui.set_shwnd_css(path, ...)
	elseif 1 == #args then
		if 'table' == type(args[1]) then
			for k1, v1 in pairs(args[1]) do
				-- set
				kgui.set_shwnd_css(path, k1, v1)
			end
		else
			return kgui.get_shwnd_css(path, ...)
		end
	end
	
	return klbui
end


-- @brief 获取是否为 多图层画布模式
-- @return [bool]
-- @note
--	true. 指 modal/popup/msgbox 分别使用不同的 画布
--	false. 指 modal/popup/msgbox 共享使用 主画布
klbui.is_multi_canvas_layer = function ()
	return kgui.is_multi_canvas_layer()
end


-- @brief 加载字体
-- @param [in] path[string]			字体路径
-- @return [number(int)] 	0.成功; 非0.失败
klbui.load_font = function (path)
	return kgui.load_font(path)
end


-- @brief 卸载字体
-- @return [number(int)] 	0.成功; 非0.失败
klbui.unload_font = function ()
	return kgui.unload_font()
end


-- @brief 加载资源图片
-- @param [in] key[string]			关键字
-- @param [in] path[string]			图片路径
-- @return [number(int)] 	0.成功; 非0.失败
klbui.load_image = function (key, path)
	return kgui.load_image(key, path)
end

-- @brief 清空资源图片
-- @return [number(int)] 	0.成功; 非0.失败
klbui.clear_image = function ()
	return kgui.clear_image()
end


-- @brief 清空消息事件
klbui.clear_msg = function ()
	kgui.clear_msg()
end


-- @brief 获取 窗口 的 操作 接口
-- @param [in] path[string]			窗口虚拟路径; eg. '/home'
klbui.get_wnd = function (path)
	-- 获取C提供的原始接口
	local kwnd = kgui.get_kwnd(path)
	
	-- 这里使用 wnder.new 对原始接口进行封装
	return wnder.new(kwnd)
end


-- @brief 模态显示窗口
-- @param [in] path[string]			窗口虚拟路径; eg. '/home'
-- @return [number(int)] 	0.成功; 非0.失败
--   废弃(拼写错误)
klbui.model = function (path)
	return kgui.modal(path)
end

-- @brief 结束一个model方式的对话框
-- @param [in] all[boolean]			是否关闭全部popup: 默认true
-- @param [in] path[string]			窗口路径(类unix): 默认nil
-- @return [number(int)] 	0.成功; 非0.失败
--   废弃(拼写错误)
klbui.model_end = function (all, path)
	return kgui.modal_end(all, path)
end


-- @brief 模态显示窗口
-- @param [in] path[string]			窗口虚拟路径; eg. '/home'
-- @return [number(int)] 	0.成功; 非0.失败
klbui.modal = function (path)
	return kgui.modal(path)
end

-- @brief 结束一个modal方式的对话框
-- @param [in] all[boolean]			是否关闭全部modal: 默认true
-- @param [in] path[string]			窗口路径(类unix): 默认nil
-- @return [number(int)] 	0.成功; 非0.失败
klbui.modal_end = function (all, path)
	return kgui.modal_end(all, path)
end

-- @brief 获取 modal 窗口数
-- @return [number(int)] 	modal 窗口数
klbui.modal_num = function ()
	return kgui.modal_num()
end

-- @brief 弹出窗口
-- @param [in] path[string]			窗口虚拟路径; eg. '/menu1'
-- @return [number(int)] 	0.成功; 非0.失败
klbui.popup = function (path)
	return kgui.popup(path)
end


-- @brief 结束一个popup方式的菜单
-- @param [in] all[boolean]			是否关闭全部popup: 默认true
-- @return [number(int)] 	0.成功; 非0.失败
klbui.popup_end = function (all)
	return kgui.popup_end(all)
end

-- @brief 获取 popup 窗口数
-- @return [number(int)] 	popup 窗口数
klbui.popup_num = function ()
	return kgui.popup_num()
end

-- @brief 消息框
-- @param [in] path[string]			窗口虚拟路径; eg. '/messagebox'
-- @return [number(int)] 	0.成功; 非0.失败
klbui.messagebox = function (path)
	return kgui.messagebox(path)
end


-- @brief 关闭消息框
klbui.messagebox_end = function ()
	return kgui.messagebox_end()
end

-- @brief 获取 messagebox 窗口数
-- @return [number(int)] 	messagebox 窗口数
klbui.messagebox_num = function ()
	return kgui.messagebox_num()
end


-- @brief 显示或隐藏窗口
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] show[boolean]		true.显示; false.隐藏
-- @return [number(int)] 	0.成功; 非0.失败
klbui.show = function (path, show)
	return kgui.show(path, show)
end


-- @brief 移动窗口位置
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] x[number(int)]		相对父窗口x坐标
-- @param [in] y[number(int)]		相对父窗口y坐标
-- @return [number(int)] 	0.成功; 非0.失败
klbui.move = function (path, x, y)
	return kgui.move(path, x, y)
end


-- @brief 修改窗口大小
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] w[number(int)]		宽
-- @param [in] h[number(int)]		高
-- @return [number(int)] 	0.成功; 非0.失败
klbui.resize = function (path, w, h)
	return kgui.resize(path, w, h)
end


-- @brief 获取窗口位置
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @param [in] is_in_canvas[bool]	画布坐标(默认true)
-- @return [table]		{x=0,y=0,w=1024,h=768} 或 {}
klbui.wndpos = function (path, is_in_canvas)
	return kgui.wndpos(path, is_in_canvas)
end


-- @brief 获取窗口建议的宽度
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @return [number(int)] 	宽度
klbui.suggestw = function (path)
	return kgui.suggestw(path)
end


-- @brief 获取窗口建议的高度
-- @param [in] path[string]			窗口路径(类unix): eg."/home/btn1"
-- @return [number(int)] 	高度
klbui.suggesth = function (path)
	return kgui.suggesth(path)
end


-- @brief 标记所有窗口需要刷新
-- @return 无
klbui.refresh = function ()
	kgui.refresh()
end


-- @brief 更新tip
-- @param [in] s[string]		tip字符串; 为''时, 表示清空
-- @return 无
klbui.update_tip = function (s)
	kgui.update_tip(s)
end

------------------------------------------------
-- 时间等

-- @brief 设置 聚焦延时消息 的时间(单位毫秒ms, 默认600, 范围[0, ~])
-- @return 无
-- @note 指鼠标聚焦一段时间后, 配合控件样式(KLB_WND_STYLE_FOCUS_DELAY), 会产生一个 focusdelay(KLBUI_focusdelay = 0x710)事件
-- 		同时影响 tip 弹出的时间
klbui.focusdelay = function (tc)
	kgui.focusdelay(tc)
end


-- @brief 获取系统当前 系统滴答数
-- @return [number(int)] 	系统滴答数
klbui.tick_count = function ()
	return kgui.tick_count()
end


-- @brief 设置 内部控件定时器运行间隔 (单位毫秒ms, 默认500, 范围[10, ~])
-- @return 无
-- 		调小,精度略微提高, 降低框架性能
--		调大,精度略微降低, 提高框架性能
klbui.ticker_interval = function (interval)
	kgui.ticker_interval(interval)
end


------------------------------------------------
-- 用户图层(udata layer)


-- @brief 给用户图层 绑定/解绑 窗口
-- @param [in] path[string, nil]	窗口路径(类unix): eg."/home/btn1"
-- @return [number(int)] 	0.成功; 非0.失败
-- @note 仅支持 'onload','onunload','onticker' 这几种 事件
klbui.bind_udatalayer = function (path)
	return kgui.bind_udatalayer(path)
end


-- @brief 移动 用户图层 窗口; 移动画布 和 窗口
-- @param [in] x[number(int)]		X坐标
-- @param [in] y[number(int)]		Y坐标
-- @return 无
klbui.move_udatalayer = function (x, y)
	kgui.move_udatalayer(x, y)
end


-- @brief 显示/隐藏 用户图层窗口
-- @param [in] show[boolean]		是否显示: eg. true
-- @return 无
klbui.show_udatalayer = function (show)
	kgui.show_udatalayer(show)
end


------------------------------------------------
-- 等待图层(wait layer)


-- @brief 给等待图层 绑定/解绑 窗口
-- @param [in] path[string, nil]	窗口路径(类unix): eg."/home/btn1"
-- @return [number(int)] 	0.成功; 非0.失败
-- @note 仅支持 'onload','onunload','onticker' 这几种 事件
klbui.bind_waitlayer = function (path)
	return kgui.bind_waitlayer(path)
end


-- @brief 移动 等待图层 窗口; 移动画布 和 窗口
-- @param [in] x[number(int)]		X坐标
-- @param [in] y[number(int)]		Y坐标
-- @return 无
klbui.move_waitlayer = function (x, y)
	kgui.move_waitlayer(x, y)
end


-- @brief 开启/关闭 UI等待
-- @param [in] is_wait[boolean]		是否等待: eg. true
-- @return 无
-- @note 注意:
--		1. 若绑定了 等待图层 窗口, 则显示等窗口
--		2. 启用等待后 UI框架 丢弃所有外设事件
--		3. 定时器事件 不受影响
klbui.wait = function (is_wait)
	kgui.wait(is_wait)
end


-- @brief 设置/获取 是否完整绘制 事件流程
-- @param [in] is_full[boolean]		是否完整绘制: 默认(true)
-- @return bool
-- @note is_full = true,  eg. 每个鼠标移动过程, 将会被绘制出来
--		 is_full = false, eg. 若有一串移动过程, 则只有最终状态被绘制出来
klbui.redraw_full_event = function (is_full)
	return kgui.redraw_full_event(is_full)
end


------------------------------------------------
-- W, H

local width = 0		-- 主显示宽
local height = 0	-- 主显示高

-- @brief 获取主显示宽度
-- @return [number(int)] 	宽
klbui.width = function ()
	if width <= 0 then
		width, height = kgui.wh()
	end
	
	return width
end


-- @brief 获取主显示宽度
-- @return [number(int)] 	高
klbui.height = function ()
	if height <= 0 then
		width, height = kgui.wh()
	end
	
	return height
end


------------------------------------------------
-- 清理

-- @brief 异步清理所有
-- @param [in] func[function]			清理完成之后的通知回调函数
-- @return int 0.成功; 非0.失败(错误码)
-- @note 当修改分辨率等需要将所有UI清理掉时
--     清理内容:
--       1. klb_gui_append 添加的所有窗口
--       2. klb_gui_load_image 加载的所有图片资源
--       3. klb_gui_push_shwnd 添加的所有共享窗口
--     不清理内容
--       a. klb_gui_register 注册的控件类型
--       b. klb_gui_register_extension 注册的扩展
-- @note func = function ()
--		    ...
--		 end
klbui.clear = function (func)
	width = 0
	height = 0
	
	return kgui.clear(func)
end


------------------------------------------------
-- 协程同步


-- @brief 协程同步调用
-- @param [in] func[function]			在协程中执行的函数
-- @return [function] 同步流程函数
-- @note func = function (x1, y1, x2, y2, lparam, wparam)
--		    print('->', x1, y1, x2, y2, lparam, wparam)
--		 end
--  此函数调用过程中, GUI会以阻塞方式运行(即不响应(丢弃)键鼠等消息), 一直到协程函数执行完毕 
klbui.co_sync = function (func)
	return function (...)
		--local timeout = 0
		--local func = nil
		
		-- gui 开始等待
		
		kco.fork(function (...)
			-- 执行函数流程
			func(...)
			
			-- gui 结束等待
		end, ...)
	end
end



------------------------------------------------
-- startup 启动设置


-- step1. 开启支持 C++ 扩展
-- 注意: 调用一次后, 即开启了, 后续无法关闭
--   若不需要支持 C++ 扩展, 注释掉本行
klbui.using_cpp()





return klbui
