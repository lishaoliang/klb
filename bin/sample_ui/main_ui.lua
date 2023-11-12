local wsdl = require("wsdl")
local ksys = require("ksys")
local krand = require("krand")
local kco = require("kco")


--------------------------------------------------------------------------------------
-- lua 路径
local basepath = wsdl.get_base_path()
package.path = package.path .. ';' .. basepath .. '?.lua;' .. basepath .. '?/init.lua;'



--------------------------------------------------------------------------------------
-- 打开窗口
local WND_W = 1280
local WND_H = 720


-- 打开 sdl 窗口
wsdl.open_wnd('./res/font/simsun.ttc', WND_W, WND_H, 'Demo对话框实例!' )



--------------------------------------------------------------------------------------
-- 解析窗口

local klbui = require("klbcore.klbui")


local function ParseWindow(page)
	local dialog = page.dialog or {}
	local commands = page.commands or {}
	local css = page.css or {}

	klbui.parse(dialog, commands, css)
	
	return dialog['path'] or ''
end


-- desktop
local desktop = require("sample_ui.desktop")
local first_page = ParseWindow(desktop)




--------------------------------------------------------------------------------------
-- model 第一个窗口
klbui.model(first_page)

