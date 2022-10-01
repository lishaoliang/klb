
local sdl_dev = require("sdl_dev")
local kgui = require("kgui")
local kwnd = require("kwnd")


local WND_W = 960
local WND_H = 540


-- 打开 sdl 窗口引擎
sdl_dev.open(WND_W, WND_H, 'kgui-示例')


kgui.append('kdialog', '/home', 0, 0, 140, 540)
kgui.append('kbutton', '/home/btn1', 10, 10, 100, 24)
kgui.append('kbutton', '/home/btn2', 10, 40, 100, 24)
kgui.append('kbutton', '/home/btn3', 10, 70, 100, 24)
	
kgui.append('kdialog', '/page1', 200, 100, 960 - 200, 540 - 100)
kgui.append('kbutton', '/page1/btn1', 10, 10, 100, 24)
kgui.append('kbutton', '/page1/btn2', 10, 40, 100, 24)
	
	
local show_page1 = false;

kgui.bind_command('/home/btn2', function (obj, msg, x1, y1, x2, y2, lparam, wparam)
	if 0x0201 == msg or 0x0203 == msg then
		
		if show_page1 then
			kgui.end_model('/page1')
			show_page1 = false
		else
			kgui.do_model('/page1')
			show_page1 = true
		end
	end
		
	return 0
end)


kgui.bind_command('/home/btn3', function (obj, msg, x1, y1, x2, y2, lparam, wparam)
	if 0x0201 == msg or 0x0203 == msg then
		print('on command btn3', msg, x1, y1)
		
		kwnd.set(obj, 'title', '456')
		
		print(kwnd.get(obj, 'title'))
	end
	
	return 0
end)



kgui.do_model('/home')

