--[[
-- Copyright (c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  main_ui_css_2.lua
-- @brief 使用klbui-css配置窗口示例
--   klbui为参考H5与jQuery简化规则封装
--]]
local wsdl = require("wsdl")
local ktime = require("ktime")

-- lua 根路径
local basepath = wsdl.get_base_path()
package.path = package.path .. ';' .. basepath .. '?.lua;' .. basepath .. '?/init.lua;'


local klbui = require("klbcore.klbui")


local CONST_w = 1280
local CONST_h = 720


-- step1. 打开窗口
wsdl.open_wnd(basepath .. 'res/font/simsun.ttc', CONST_w, CONST_h, 'klbui-css示例2' )


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


home.css = {
	['type'] = {
		['kdialog'] = {
			
		},
		
		['kbutton'] = {
			['padding-top'] = 1,
			['padding-right'] = 2,
			['padding-bottom'] = 2,
			['padding-left'] = 10,
			
			['color:focus'] = {255, 10, 220, 220},
			['border-width:focus'] = {2, 2, 2, 2},
			['border-color:focus'] = {255,220,30,220},
		}
	},
	
	['class'] = {
		['class-btn-a'] = {
			['background-color'] = {255, 30, 30, 30},
			['color'] = {255, 220, 220, 10},
			['font-size'] = 20,
			
			['background-color:focus'] = {255, 80, 80, 80},
			['color:focus'] = {255, 10, 220, 10},
			['font-size:focus'] = 28,
		}
	},
	
	['name'] = {
		['pic1'] = {
			['visibility'] = 'hidden',
			['border-width'] = {1, 1, 1, 1}
		},
		
		['btn1'] = {
			['background-color'] = {255, 30, 30, 30},
			['color'] = {255, 220, 220, 10},
			['font-size'] = 20,
			
			['background-color:focus'] = {255, 80, 80, 80},
			['color:focus'] = {255, 10, 220, 10},
			['font-size:focus'] = 28,
		},
		
		['btn2'] = {
			['background-color'] = {255, 30, 220, 202},
			
			['background-image:focus'] = 'dialog_close_focus.bmp'
		},
		
		['btn3'] = {
			['background-color'] = 0xFFF010FF
		}
	},
	
	['id'] = {
		['btn1:visited'] = {
			['color'] = 'green',
			['background-color'] = 'green'
		},
		['btn1:hover'] = {
			['color'] = 'green',
			['background-color'] = 'green'
		},
		['btn1:active'] = {
			['color'] = 'green',
			['background-color'] = 'green'
		}
	}
}

home.dialog = {
	['path'] = '/home',
	['type'] = 'kdialog',
	['pos'] = {0, 0, CONST_w, CONST_h},
	['title'] = 'klbui-css示例2 - hello world!',
	['name'] = 'home1',
	
	['child'] = {
		{
			['path'] = '/home/pic1',
			['type'] = 'kpicture',
			['pos'] = {160, 64 + 10, CONST_w - 160 - 10, CONST_h - 64 - 20},
			['name'] = 'pic1',
		},
		
		{
			['path'] = '/home/btn1',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 10, 140, 32},
			['title'] = '图片111',
			['name'] = 'btn1',
		},
		
		{
			['path'] = '/home/btn2',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 50, 140, 32},
			['title'] = '图片222',
			['name'] = 'btn2',
		},
		
		{
			['path'] = '/home/btn3',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 90, 140, 32},
			['title'] = '图片333',
			['name'] = 'btn3',
		},
		
		{
			['path'] = '/home/btn4',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 140, 140, 32},
			['title'] = '图片444',
			['name'] = 'btn4',
			['class'] = 'class-btn-a'
		}
	}
}

local jq = klbui.select(home.dialog)


home.parse_tc = 0

home.commonds = {
	['/home'] = {
		['load'] = function ()
			
		end,
		
		['unload'] = function ()
			
		end
	},

	['/home/btn1'] = {
		['click'] = function ()
			jq('pic1').picture('2fab96b2be1e057c524763b7839136db.bmp')
			
			local title = jq('btn1').title()
			local c = 0
		end
	},

	['/home/btn2'] = {
		['click'] = function ()
			jq('pic1').picture('33fbb649d7dd6c721a39f8a0be989f81.bmp')
		end
	},

	['/home/btn3'] = {
		['click'] = function ()
			jq('pic1').picture('dialog_close_normal.bmp')
			
			local title = 'klbui-css示例2 - hello world!' .. ' - ' .. tostring(home.parse_tc) .. 'ms'			
			jq('home1').title(title)
		end
	},
	
	['/home/btn4'] = {
		['click'] = function ()
			jq('pic1').picture('dialog_close_focus.bmp')
			
			--jq('btn1')['background-color'](0xFFF01010)
			
			--local a1 = jq('btn1')['background-color']()
			
			--jq('btn1').visibility(false)
		end
	}
}

-- step4. 解析生成窗口
local t1 = ktime.tick_count()
klbui.parse(home.dialog, home.commonds, home.css)
local t2 = ktime.tick_count()

home.parse_tc = t2 - t1


-- step5. 显示 '/home' 窗口
klbui.do_model('/home')
