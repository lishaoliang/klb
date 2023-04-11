--[[
-- Copyright (c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  init.lua
-- @brief klbui init.lua
-- @note 参考 html5 标准: https://www.w3school.com.cn/html/html5_intro.asp
--     参考: https://www.runoob.com/html/html5-form-input-types.html
--     参考: https://www.w3school.com.cn/jquery/index.asp
--]]

local kgui = require("kgui")
local parser = require("klbcore.klbui.parser")
local selector = require("klbcore.klbui.selector")


local klbui = {}



-- @brief 解析对话框/命令, 并完成gui窗口树初始创建
-- @param [in]      dialog[table]		对话框描述table
-- @param [in]		commonds[nil,table]	[可选]命令响应集合(数组)
-- @param [in]      css[nil,table]		[可选]CSS描述table
-- @return 无
-- @note eg.
--	local dialog = {
--		['path'] = '/home',['type'] = 'kdialog',['pos'] = {0, 0, 1280, 720},
--		['title'] = '测试解析对话框!',
--		['child'] = {{...}, {...}, {...}}
--	}
--	local commonds = {
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
klbui.parse = function (dialog, commonds, css)
	return parser.parse(dialog, commonds, css)
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


-- @brief 加载资源图片
-- @param [in] key[string]			关键字
-- @param [in] path[string]			图片路径
-- @return [number(int)] 	0.成功; 非0.失败
klbui.load_image = function (key, path)
	return kgui.load_image(key, path)
end


-- @brief 模态显示窗口
-- @param [in] path[string]			窗口虚拟路径; eg. '/home'
-- @return [number(int)] 	0.成功; 非0.失败
klbui.do_model = function (path)
	return kgui.do_model(path)
end


-- @brief 弹出窗口
-- @param [in] path[string]			窗口虚拟路径; eg. '/menu1'
-- @return [number(int)] 	0.成功; 非0.失败
klbui.popup = function (path)
	return kgui.popup(path)
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


return klbui
