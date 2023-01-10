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

klbui.load_image('radio-two_normal.bmp', res_path .. '单选_radio-two_1.bmp')
klbui.load_image('radio-two_focus.bmp', res_path .. '单选_radio-two_2.bmp')


-- step3. 初始全局默认设置
-- 可选


local home = {}

local jq = function () end
local jq0 = function () end

home.css = {
	['type'] = {
		['kdialog'] = {
			
		},
		
		['kstatic'] = {
			['padding-top'] = 1,
			['padding-right'] = 2,
			['padding-bottom'] = 2,
			['padding-left'] = 10,
			['color'] = {255, 220, 220, 10},
			['font-size'] = 20,
			['background-color'] = {255, 30, 30, 30},
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
		
		['kedit'] = {
			['padding-top'] = 1,
			['padding-right'] = 2,
			['padding-bottom'] = 2,
			['padding-left'] = 10,
			
			['color:focus'] = {255, 10, 220, 220},
			['border-color:focus'] = {255,160,160,160},
			['background-color:focus'] = {255,30,30,30}
		},
		
		['kcheck'] = {
			['border-width:focus'] = {2, 2, 2, 2},
			['color'] = {255, 200, 200, 200},
			['color:focus'] = {255, 220, 220, 200},
			['border-color:focus'] = {255,160,160,160},
			['background-color:focus'] = {255,30,30,30}
		},
		
		['kradio'] = {
			['border-width:focus'] = {2, 2, 2, 2},
			['color'] = {255, 200, 200, 20},
			['color:focus'] = {255, 220, 220, 200},
			['border-color:focus'] = {255,160,160,160},
			['background-color:focus'] = {255,30,30,30},
		},
		
		['kpicture'] = {
			['border-width:focus'] = {2, 2, 2, 2},
			['border-color:focus'] = {255,20,220,220},
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
			['LOAD_ptp'] = "'/home/btn1', 'kbutton', {10, 64 + 10, 140, 32}",
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
		
		{
			['path'] = '/home/btn4',
			['type'] = 'kbutton',
			['pos'] = {10, 64 + 140, 140, 32},
			['title'] = '按钮444',
			['name'] = 'btn4',
			['class'] = 'class-btn-a'
		},
		
		{
			['path'] = '/home/static1',
			['type'] = 'kstatic',
			['pos'] = {10, 64 + 190, 140, 32},
			['title'] = '静态文本显示',
			['name'] = 'static1',
		},
		
		{
			['path'] = '/home/static2',
			['type'] = 'kstatic',
			['pos'] = {10, 64 + 240, 140, 32},
			['title'] = '静态文本显示',
			['name'] = 'static2',
		},
		
		{
			['path'] = '/home/edit1',
			['type'] = 'kedit',
			['pos'] = {10 + 140 + 30, 64 + 50, 480, 32},
			['value'] = '编辑框',
			['name'] = 'edit1',
		},
		
		{
			['path'] = '/home/check1',
			['type'] = 'kcheck',
			['pos'] = {10 + 140 + 30, 64 + 100, 24, 24},
			['value'] = true,
			['name'] = 'check1',
		},
		
		{
			['path'] = '/home/radio1',
			['type'] = 'kradio',
			['pos'] = {10 + 140 + 30 + 32, 64 + 100, 24, 24},
			['value'] = true,
			['name'] = 'radio1',
		},
		
		{
			['path'] = '/home/pic1',
			['type'] = 'kpicture',
			['pos'] = {10 + 140 + 30, 64 + 100 + 40, 360, 240},
			['name'] = 'pic1',
			['image'] = '2fab96b2be1e057c524763b7839136db.bmp',
		},
		
		{
			['path'] = '/home/progress1',
			['type'] = 'kprogress',
			['pos'] = {10 + 140 + 30, 64 + 100 + 40 + 260, 360, 32},
			['value'] = 80,
			['name'] = 'progress1',
			['commonds'] = {
				['click'] = function ()
					local progress = jq('progress1').value()
					
					progress = progress + 5
					if 100 < progress then
						progress = 0
					end
					
					jq('progress1').value(progress)
				end
			}
		},
	}
}

jq = klbui.select(home.dialog)			-- 单选
jq0 = klbui.select(home.dialog, true)	-- 多选


home.parse_tc = 0

home.commonds = {
	['/home'] = {
		['load'] = function ()
			jq('edit1').value('解析耗时:' .. tostring(home.parse_tc) .. 'ms')
				
			jq('btn4').click(function ()
				jq0(':kstatic').css({
					['color'] = {255, 220, 220, 220},
					['font-size'] = 22,
					['border-color'] = {255,160,160,160},
					['background-color'] = {255, 60, 60, 60},
				})
				
				jq('static1').value('后期动态绑定')
				jq('static2').value('点击了按钮4!')
			end)
		end,
		
		['unload'] = function ()
			
		end
	},

	['/home/btn1'] = {
		['click'] = function ()
			jq('edit1').value('test!')
		end
	},

	['/home/btn2'] = {
		['click'] = function ()
			jq0(':kcheck').css({
				['border-width:focus'] = {2, 2, 2, 2},
				['color'] = {255, 160, 160, 160},
				['color:focus'] = {255, 220, 20, 20},
				['border-color:focus'] = {255,160,160,160},
				['background-color:focus'] = {255,30,30,30}
			})
		end
	},

	['/home/btn3'] = {
		['click'] = function ()
			jq0(':kcheck').css({
				['border-width:focus'] = {2, 2, 2, 2},
				['color'] = {255, 250, 250, 250},
				['color:focus'] = {255, 20, 220, 20},
				['border-color:focus'] = {255,160,160,160},
				['background-color:focus'] = {255,30,30,30}
			})
		end
	},
	
	['/home/btn4'] = {
		['click'] = function ()
			
		end
	},
	
	['/home/check1'] = {
		['click'] = function ()			
			if jq('check1').value() then
				jq('pic1')['image']('2fab96b2be1e057c524763b7839136db.bmp')
			else
				jq('pic1')['image']('33fbb649d7dd6c721a39f8a0be989f81.bmp')
			end
		end
	},
	
	['/home/radio1'] = {
		['click'] = function ()
			if jq('radio1').value() then
				jq('pic1')['image']('radio-two_normal.bmp')
			else
				jq('pic1')['image']('radio-two_focus.bmp')
			end
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


