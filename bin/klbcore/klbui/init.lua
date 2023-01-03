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


return klbui
