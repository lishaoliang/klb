--[[
-- Copyright (c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  main_ui_css_3.lua
-- @brief 使用klbui-css配置窗口示例
--   klbui为参考H5与jQuery简化规则封装
--]]
local wsdl = require("wsdl")
local ktime = require("ktime")
local krand = require("krand")

-- lua 根路径
local basepath = wsdl.get_base_path()
package.path = package.path .. ';' .. basepath .. '?.lua;' .. basepath .. '?/init.lua;'


local klbui = require("klbcore.klbui")
local msgbox = require("klbcore.klbui.widgets.messagebox")


local CONST_w = 1280
local CONST_h = 720


-- step1. 打开窗口
wsdl.open_wnd(basepath .. 'res/font/simsun.ttc', CONST_w,  CONST_h, 'klbui-css示例3' )


-- step2. 加载图片资源
-- 下载来自: http://iconpark.oceanengine.com/home
-- 下载来自: https://remixicon.com/
local res_path = basepath .. 'res/images/'
klbui.load_image('dialog_close_normal.bmp', res_path .. 'dialog_close_normal.bmp')
klbui.load_image('dialog_close_focus.bmp', res_path .. 'dialog_close_focus.bmp')

klbui.load_image('2fab96b2be1e057c524763b7839136db.bmp', res_path .. '2fab96b2be1e057c524763b7839136db.bmp')
klbui.load_image('33fbb649d7dd6c721a39f8a0be989f81.bmp', res_path .. '33fbb649d7dd6c721a39f8a0be989f81.bmp')


-- step3. 初始全局默认设置
-- 可选


local home = {}

local jq = function () end
local jq0 = function () end

home.css = {
	['type'] = {
		['kdialog'] = {
			
		},
	},
	
	['class'] = {
		
	},
	
	['name'] = {
		
	},
	
	['id'] = {
		
	}
}

home.dialog = {
	['path'] = '/home',
	['type'] = 'kdialog',
	['pos'] = {0, 0, CONST_w, CONST_h},
	
	['title'] = 'klbui-css示例3 - hello world!',
	['name'] = 'home1',
	
	['child'] = {
		{
			['path'] = '/home/btn1',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 10, 140, 32},
			['title'] = '按钮111',
			['name'] = 'btn1',
		},
		
		{
			['path'] = '/home/btn2',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 50, 140, 32},
			['title'] = '按钮222',
			['name'] = 'btn2',
		},
		
		{
			['path'] = '/home/btn3',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 90, 140, 32},
			['title'] = '按钮333',
			['name'] = 'btn3',
		},
	}
}

jq = klbui.select(home.dialog)			-- 单选
jq0 = klbui.select(home.dialog, true)	-- 多选


home.parse_tc = 0

home.commonds = {
	['/home'] = {
		['onload'] = function ()
			
		end,
		
		['unload'] = function ()
			
		end
	},

	['/home/btn1'] = {
		['click'] = function ()			
			klbui.messagebox('/messagebox')
		end
	},

	['/home/btn2'] = {
		['click'] = function ()
			
		end
	},

	['/home/btn3'] = {
		['click'] = function ()
			
		end
	}
}

-- step4. 解析生成窗口
local t1 = ktime.tick_count()
klbui.parse(msgbox.dialog, msgbox.commonds, msgbox.css)
klbui.parse(home.dialog, home.commonds, home.css)
local t2 = ktime.tick_count()

home.parse_tc = t2 - t1


-- step5. 显示 '/home' 窗口
klbui.model('/home')


