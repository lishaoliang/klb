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


home.css = {
	['type'] = {
		['kdialog'] = {
			
		},
		
		['ktab'] = {
			['background-color'] = {255, 80, 80, 80},
			['background-color:focus'] = {255, 220, 220, 30},
			
			['color:focus'] = {255, 10, 220, 220},	
			['border-width:focus'] = {2, 2, 2, 2},
			['border-color:focus'] = {255,220,30,220},
		},
		
		['kview'] = {
			['background-color'] = {255, 80, 80, 80},
			['background-color:focus'] = {255, 220, 220, 30},
			
			['color:focus'] = {255, 10, 220, 220},	
			['border-width:focus'] = {2, 2, 2, 2},
			['border-color:focus'] = {255,220,30,220},
		},
		
		['kbutton'] = {
			['padding-top'] = 1,
			['padding-right'] = 2,
			['padding-bottom'] = 2,
			['padding-left'] = 10,
			
			['color:focus'] = {255, 10, 220, 220},
			['border-width:focus'] = {2, 2, 2, 2},
			['border-color:focus'] = {255,220,30,220},
		},
		
		['kcombo'] = {
			['padding-top'] = 1,
			['padding-right'] = 2,
			['padding-bottom'] = 2,
			['padding-left'] = 10,
			
			['color:focus'] = {255, 10, 220, 220},
			['border-width:focus'] = {2, 2, 2, 2},
			['border-color:focus'] = {255,220,30,220},
			
			['botton-right.color'] = {255, 120, 120, 120},
			['botton-right.color:focus'] = {255, 20, 20, 250},
		},
	},
	
	['class'] = {
		
	},
	
	['name'] = {
		
	},
	
	['id'] = {
		
	}
}

local view1 = {
	['path'] = '/home/tab1/view1',
	['type'] = 'kview',
	['pos'] = {0, 34, CONST_w, CONST_h - 74},
	['title'] = '页面1',
	['name'] = 'view1',
	
	['child'] = {
		{
			['path'] = '/home/tab1/view1/btn1',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 10, 140, 32},
			['title'] = '按钮111',
			['name'] = 'btn1',
		},
		
		{
			['path'] = '/home/tab1/view1/btn2',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 50, 140, 32},
			['title'] = '按钮222',
			['name'] = 'btn2',
		},
		
		{
			['path'] = '/home/tab1/view1/btn3',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 90, 140, 32},
			['title'] = '按钮333',
			['name'] = 'btn3',
		},
		
		{
			['path'] = '/home/tab1/view1/static1',
			['type'] = 'kstatic',
			['pos'] = {10, 64 + 190, 140, 32},
			['title'] = '静态文本显示',
			['name'] = 'static1',
			
			['color'] = {255, 10, 220, 220},
		},
	}
}

local view2 = {
	['path'] = '/home/tab1/view2',
	['type'] = 'kview',
	['pos'] = {0, 34, CONST_w - 2, CONST_h - 74},
	['title'] = '页面2',
	['name'] = 'view2',
	
	['child'] = {
		{
			['path'] = '/home/tab1/view2/btn21',
			['type'] = 'kbutton',
			['pos'] = {10 + 240, 64 + 10, 140, 32},
			['title'] = 'AAAA',
			['name'] = 'btn21',
		},
		
		{
			['path'] = '/home/tab1/view2/btn22',
			['type'] = 'kbutton',
			['pos'] = {10 + 240, 64 + 50, 140, 32},
			['title'] = 'BBBB',
			['name'] = 'btn22',
		},
		
		{
			['path'] = '/home/tab1/view2/combo23',
			['type'] = 'kcombo',
			['pos'] = {10 + 240, 64 + 50 + 40, 140, 32},
			['name'] = 'combo23',
			
			['title'] = '1111',
			['value'] = '1',
			['append'] = {
				{['1'] = '1111'},
				{['2'] = '2222'},
				{['3'] = '3333'},
				{['4'] = '4444'},
				{['5'] = '5555'},
			}
		},
	}
}

home.dialog = {
	['path'] = '/home',
	['type'] = 'kdialog',
	['pos'] = {0, 0, CONST_w, CONST_h},
	
	['title'] = 'klbui-css示例4 - hello world!',
	['name'] = 'home1',
	
	['child'] = {	
		{
			['path'] = '/home/tab1',
			['type'] = 'ktab',
			['pos'] = {1, 40, CONST_w - 2, CONST_h - 40},
			['title'] = 'TAB111',
			['name'] = 'tab1',
			
			['child'] = {
				view1,
				view2,
			}
		},
	}
}

jq = klbui.select(home.dialog)

home.parse_tc = 0

home.commonds = {
	['/home'] = {
		['onload'] = function ()
			
		end,
		
		['onunload'] = function ()
			
		end
	}
}

-- step4. 解析生成窗口
local t1 = ktime.tick_count()
klbui.parse(home.dialog, home.commonds, home.css)
local t2 = ktime.tick_count()

home.parse_tc = t2 - t1


-- step5. 显示 '/home' 窗口
klbui.model('/home')

