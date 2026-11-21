--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   page_tmpl.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_ui 页面模块模板 (对齐 lua_test klbui custom/page_tmpl)
-- @note   复制为 pages/<name>.lua; 只改 css / dialog / commands
--  \n 开窗与切页由 ui/main + ui/app; 本页满窗 parse; 元素只写本文件
-- @history 修改历史
--  \n [2026-10] 创建文件; 去掉 pages.common
--]]


local klbui = require("klbcore.klbui")
local app = require("klbagent.packages.alua_ui.ui.app")


local M = {}

local jq = function () end
local jq_all = function () end

local NAV_Y = 8
local NAV_H = 32
local Y0 = 48


-----------------------------------------------------------------------------------
-- css

local css_720p = {
	['type'] = {
	}
}


-----------------------------------------------------------------------------------
-- dialog

local dialog_720p = {
	['type'] = 'kview',
	['pos'] = {0, 0, -1, -1},
	['name'] = 'page',

	['child'] = {
		{
			['type'] = 'kbutton',
			['pos'] = {16, NAV_Y, 90, NAV_H},
			['name'] = 'btn_nav_home',
			['title'] = 'Home',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {116, NAV_Y, 90, NAV_H},
			['name'] = 'btn_nav_serve',
			['title'] = 'Serve',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {216, NAV_Y, 100, NAV_H},
			['name'] = 'btn_nav_runtime',
			['title'] = 'Runtime',
		},
		-- 本页控件 (Y 从 Y0 起)
	},
}


-----------------------------------------------------------------------------------
-- commands

M.commands = {
	['btn_nav_home'] = {
		['click'] = function ()
			app.show("home")
		end,
	},
	['btn_nav_serve'] = {
		['click'] = function ()
			app.show("serve")
		end,
	},
	['btn_nav_runtime'] = {
		['click'] = function ()
			app.show("runtime")
		end,
	},
}


-----------------------------------------------------------------------------------
-- 处理导出

M.css = {}
M.dialog = {}
M.jq = jq
M.jq_all = jq_all


-- 重定向
M.relocation = function (rs)
	if 'HD' == rs then
		M.dialog = dialog_720p
		M.css = css_720p
	elseif 'WXGA' == rs then
		M.dialog = dialog_720p
		M.css = css_720p
	elseif 'HD+' == rs then
		M.dialog = dialog_720p
		M.css = css_720p
	elseif 'Full HD' == rs then
		M.dialog = dialog_720p
		M.css = css_720p
	elseif 'QHD' == rs then
		M.dialog = dialog_720p
		M.css = css_720p
	elseif '4K UHD' == rs then
		M.dialog = dialog_720p
		M.css = css_720p
	else
		M.dialog = dialog_720p
		M.css = css_720p
	end

	-- 重定向 jq
	jq = klbui.select(M.dialog)
	jq_all = klbui.select(M.dialog, true)

	M.jq = jq
	M.jq_all = jq_all
end


return M

-- end
