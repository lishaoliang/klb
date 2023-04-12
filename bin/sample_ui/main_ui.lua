
local wsdl = require("wsdl")
local kgui = require("kgui")
local kwnd = require("kwnd")
local ksys = require("ksys")
local krand = require("krand")
local kco = require("kco")


package.path = package.path .. ';' .. wsdl.get_base_path() .. '?.lua'


local WND_W = 1280
local WND_H = 720

--local WND_W = 1920
--local WND_H = 1080


local a1, a2 = ksys.get_args()


-- 打开 sdl 窗口引擎
wsdl.open_wnd('./res/font/simsun.ttc', WND_W, WND_H, 'Demo对话框实例!' )


kgui.load_image('dialog_close_normal.bmp', './res/images/dialog_close_normal.bmp')
kgui.load_image('dialog_close_focus.bmp', './res/images/dialog_close_focus.bmp')

kgui.load_image('2fab96b2be1e057c524763b7839136db.bmp', './res/images/2fab96b2be1e057c524763b7839136db.bmp')
kgui.load_image('33fbb649d7dd6c721a39f8a0be989f81.bmp', './res/images/33fbb649d7dd6c721a39f8a0be989f81.bmp')


kgui.append('kdialog', '/home', 0, 0, 160, 540, 0x0002)
kgui.append('kbutton', '/home/btn1', 10, 10, 140, 32)
kgui.append('kbutton', '/home/btn2', 10, 50, 140, 32)
kgui.append('kbutton', '/home/btn3', 10, 90, 140, 32)

kgui.set('/home/btn1', 'title', '测试来着')
kgui.set('/home/btn2', 'title', '弹出窗口')
kgui.set('/home/btn3', 'title', '33333')

	
--kgui.append('kdialog', '/page1', 200, 100, 960 - 200, 540 - 100)
kgui.append('kdialog', '/page1', 0, 0, WND_W, WND_H)

kgui.append('kbutton', '/page1/btn1', 10, 60, 140, 32)
kgui.append('kbutton', '/page1/btn2', 10, 100, 140, 32)
kgui.append('kbutton', '/page1/btn3', 10, 180, 140, 32)
kgui.append('kbutton', '/page1/btn4', 10, 240, 140, 32)
kgui.append('kbutton', '/page1/btn5', 10, 300, 140, 32)

kgui.append('kpicture', '/page1/pic1', 160, 60, WND_W - 200, WND_H - 120)
kgui.append('wsdl_video', '/page1/video1', 160, 60, WND_W - 200, WND_H - 120)

kgui.set('/page1', 'title', 'DoModel弹出页面')
kgui.set('/page1/btn1', 'title', '确定')
kgui.set('/page1/btn2', 'title', '取消')
kgui.set('/page1/btn3', 'title', '随机标题')
kgui.set('/page1/btn4', 'title', '更换图片')
kgui.set('/page1/btn5', 'title', '播放')
kgui.set('/page1/pic1', 'picture', '2fab96b2be1e057c524763b7839136db.bmp')


	
kgui.bind_command('/home/btn2', function (obj, msg, x1, y1, x2, y2, lparam, wparam)
	if 0x0201 == msg or 0x0203 == msg then
		--kwnd.set(obj, 'title', 'button2')
		
		kgui.do_model('/page1')
	end
		
	return 0
end)


kgui.bind_command('/home/btn3', function (obj, msg, x1, y1, x2, y2, lparam, wparam)
	if 0x0201 == msg or 0x0203 == msg then
		print('on command btn3', msg, x1, y1)
		
		kwnd.set(obj, 'title', 'button3')
		
		print(kwnd.get(obj, 'title'))
	end
	
	return 0
end)


kgui.bind_command('/page1/btn1', function (obj, msg, x1, y1, x2, y2, lparam, wparam)
	if 0x0201 == msg or 0x0203 == msg then
		kgui.end_model('/page1')
	end
		
	return 0
end)


kgui.bind_command('/page1/btn2', function (obj, msg, x1, y1, x2, y2, lparam, wparam)
	if 0x0201 == msg or 0x0203 == msg then
		kgui.end_model('/page1')
	end
		
	return 0
end)


kgui.bind_command('/page1/btn3', function (obj, msg, x1, y1, x2, y2, lparam, wparam)
	if 0x0201 == msg or 0x0203 == msg then	
		local title = 'DoModel弹出页面-' ..  krand.rand_string(24)
		
		kgui.set('/page1', 'title', title)
	end
		
	return 0
end)

local pic1 = false
local play = false


kgui.show('/page1/pic1', true)
kgui.show('/page1/video1', false)


kgui.bind_command('/page1/btn4', function (obj, msg, x1, y1, x2, y2, lparam, wparam)
	if 0x0201 == msg or 0x0203 == msg then
		if pic1 then
			kgui.set('/page1/pic1', 'picture', '2fab96b2be1e057c524763b7839136db.bmp')			
		else
			kgui.set('/page1/pic1', 'picture', '33fbb649d7dd6c721a39f8a0be989f81.bmp')
		end
		
		pic1 = not pic1;
		
		--kgui.show('/page1/pic1', pic1)
	end		
	return 0
end)

kgui.bind_command('/page1/btn5', function (obj, msg, x1, y1, x2, y2, lparam, wparam)
	if 0x0201 == msg or 0x0203 == msg then	
		play = not play
		
		if play then
			wsdl.set_video_pos(0, 160, 60, WND_W - 200, WND_H - 120)
			
			kgui.show('/page1/pic1', false)
			kgui.show('/page1/video1', true)
			
			kgui.set('/page1/btn5', 'title', '停止')
		else
			wsdl.set_video_pos(0, 0, 0, 0, 0)
			
			kgui.show('/page1/pic1', true)
			kgui.show('/page1/video1', false)
			
			kgui.set('/page1/btn5', 'title', '播放')
		end
	end
	return 0
end)


kgui.bind_command('/page1', function (obj, msg)
	
	if 0x0418 == msg then
		-- return 1
		
	elseif 0x0419 == msg then
		-- 即将关闭对话框
		
		wsdl.set_video_pos(0, 0, 0, 0, 0)
		
		kgui.show('/page1/pic1', true)
		kgui.show('/page1/video1', false)
		
		kgui.set('/page1/btn5', 'title', '播放')
		
		play = false
	end
	
	return 0
end)

kgui.model('/home')


-- h264
local kh26x = require("kh26x")
local path_h264 = "./res/media/BGM-You KnowI'llGo Get-960x540.h264"

kco.fork(function ()
	local h264 = kh26x.load(path_h264)	
	
	while not ksys.is_exit() do
		
		if play then
			local frame1, frame2 = h264:read()
			
			wsdl.push_media(0, 0, frame1)		
			wsdl.push_media(0, 0, frame2)
		end
		
		kco.co_sleep(30)
	end
end)
