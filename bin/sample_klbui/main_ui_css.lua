--[[
-- Copyright (c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  main_ui_css.lua
-- @brief 使用klbui-css配置窗口示例
--   klbui为参考H5与jQuery简化规则封装
--]]
local wsdl = require("wsdl")
local ktime = require("ktime")
local krand = require("krand")

-- lua 根路径
local basepath = wsdl.get_base_path()
package.path = package.path .. ';' .. basepath .. '?.lua;' .. basepath .. '?/init.lua;'

local basepath = './'

local klbui = require("klbcore.klbui")


local CONST_w = 1280
local CONST_h = 720


-- step1. 打开窗口
wsdl.open_wnd(basepath .. 'res/font/simsun.ttc', CONST_w,  CONST_h, 'klbui-css示例' )


-- step2. 加载图片资源
-- 下载来自: http://iconpark.oceanengine.com/home
-- 下载来自: https://remixicon.com/
--local res_path = basepath .. 'res/images/'
--klbui.load_image('dialog_close_normal.bmp', res_path .. 'dialog_close_normal.bmp')
--klbui.load_image('dialog_close_focus.bmp', res_path .. 'dialog_close_focus.bmp')

--klbui.load_image('2fab96b2be1e057c524763b7839136db.bmp', res_path .. '2fab96b2be1e057c524763b7839136db.bmp')
--klbui.load_image('33fbb649d7dd6c721a39f8a0be989f81.bmp', res_path .. '33fbb649d7dd6c721a39f8a0be989f81.bmp')


-- step3. 初始全局默认设置
-- 可选


local home = {}

local jq = function () end


local tab_btn_h = 32

home.css = {
	['type'] = {
	
	},
	
	['class'] = {
		
	},
	
	['name'] = {
		
	},
	
	['id'] = {
		
	}
}

local view1 = {
	['type'] = 'kview',
	['pos'] = {0, tab_btn_h, CONST_w, CONST_h - tab_btn_h},
	['title'] = '页面1',
	
	['child'] = {
	
	}
}

local view2 = {
	['type'] = 'kview',
	['pos'] = {0, tab_btn_h, CONST_w, CONST_h - tab_btn_h},
	['title'] = '页面2',
	
	['child'] = {
	
	}
}

local view3 = {
	['type'] = 'kview',
	['pos'] = {0, tab_btn_h, CONST_w, CONST_h - tab_btn_h},
	['title'] = '页面3',
	
	['child'] = {
	
	}
}

local view4 = {
	['type'] = 'kview',
	['pos'] = {0, tab_btn_h, CONST_w, CONST_h - tab_btn_h},
	['title'] = '页面4',
	
	['child'] = {
	
	}
}

local view5 = {
	['type'] = 'kview',
	['pos'] = {0, tab_btn_h, CONST_w, CONST_h - tab_btn_h},
	['title'] = '页面5',
	
	['child'] = {
	
	}
}

local view6 = {
	['type'] = 'kview',
	['pos'] = {0, tab_btn_h, CONST_w, CONST_h - tab_btn_h},
	['title'] = '页面6',
	
	['child'] = {
	
	}
}

home.dialog = {
	['type'] = 'ktab',
	['pos'] = {0, 0, CONST_w, CONST_h},
	
	['title'] = 'klbui-css示例 - hello world!',
	['name'] = 'home1',
	
	['button.height'] = tab_btn_h,
	
	['child'] = {
		view1,
		view2,
		view3,
		view4,
		view5,
		view6,
	}
}

jq = klbui.select(home.dialog)

home.parse_tc = 0

home.commonds = {
	['home1'] = {
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
klbui.model(home.dialog['path'])

