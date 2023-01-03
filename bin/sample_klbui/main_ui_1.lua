local wsdl = require("wsdl")

-- lua 根路径
local basepath = wsdl.GetBasePath()
package.path = package.path .. ';' .. basepath .. '?.lua;' .. basepath .. '?/init.lua;'


local klbui = require("klbcore.klbui")


local CONST_w = 1280
local CONST_h = 720


-- step1. 打开窗口
wsdl.OpenWnd(basepath .. 'res/font/simsun.ttc', CONST_w, CONST_h, 'klbui示例1' )


-- step2. 加载图片资源
local res_path = basepath .. 'res/images/'
klbui.load_image('dialog_close_normal.bmp', res_path .. 'dialog_close_normal.bmp')
klbui.load_image('dialog_close_focus.bmp', res_path .. 'dialog_close_focus.bmp')

klbui.load_image('2fab96b2be1e057c524763b7839136db.bmp', res_path .. '2fab96b2be1e057c524763b7839136db.bmp')
klbui.load_image('33fbb649d7dd6c721a39f8a0be989f81.bmp', res_path .. '33fbb649d7dd6c721a39f8a0be989f81.bmp')


-- step3. 初始全局默认设置



local home = {}

home.dialog = {
	['path'] = '/home',
	['type'] = 'kdialog',
	['pos'] = {0, 0, CONST_w, CONST_h},
	['title'] = 'klbui示例1 - hello world!',
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
		}
	}
}

local jq = klbui.select(home.dialog)

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
		end
	},
	
	['/home/btn4'] = {
		['click'] = function ()
			jq('pic1').picture('dialog_close_focus.bmp')
		end
	}
}

-- step4. 解析生成窗口
klbui.parse(home.dialog, home.commonds)


-- step5. 显示 '/home' 窗口
klbui.do_model('/home')
