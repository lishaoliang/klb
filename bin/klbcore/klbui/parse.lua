--[[
-- Copyright (c) 2022, GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  parse.lua
-- @brief 按一定规则将table解析成对话框
--]]
local kgui = require("kgui")


local function parse_wnd(wnd)
	if 'table' ~= type(wnd) then
		return
	end

	local path = wnd['path'] or '/'
	local t = wnd['type'] or ''
	local pos = wnd['pos'] or {}
	local title = wnd['title'] or ''
	local commond = wnd['commond'] or nil

	local x = pos[1]
	local y = pos[2]
	local w = pos[3]
	local h = pos[4]
	
	kgui.append(t, path, x, y, w, h)
		
	if '' ~= title then
		kgui.set(path, 'title', title)
	end

	if 'function' == type(commond) then
		kgui.bind_command(path, commond)
	end
	
	-- 子窗口
	for _, v in ipairs(wnd) do
		parse_wnd(v)
	end
end


local parse = function (dlg)
	parse_wnd(dlg)
end


return parse
