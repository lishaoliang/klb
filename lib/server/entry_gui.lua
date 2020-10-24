--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief  GUI加载入口
-- @author 李绍良
--]]
local kgui = require("kgui")


kin = function ()

	kgui.append('kdialog', '/home', 0, 0, 960, 540)
	kgui.append('kbutton', '/home/btn1', 10, 10, 100, 24)
	kgui.append('kbutton', '/home/btn2', 10, 40, 100, 24)
	kgui.append('kbutton', '/home/btn3', 10, 70, 100, 24)
	
	kgui.append('kdialog', '/page1', 200, 100, 960 - 200, 540 - 100)
	kgui.append('kbutton', '/page1/btn1', 10, 10, 100, 24)
	kgui.append('kbutton', '/page1/btn2', 10, 40, 100, 24)
	

	kgui.bind_command('/home/btn3', function (obj, msg, x1, y1, x2, y2, lparam, wparam)
		if 0x0201 == msg then
			print('on command btn3', msg, x1, y1)
		end
		
		return 0
	end)
	
	return 0
end


kgo = function (msg, msgex, lparam, wparam)
	
	return 0
end


kexit = function ()
	return 0
end



