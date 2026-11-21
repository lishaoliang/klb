--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   page_tmpl.lua
-- @brief  klbui 页面模块模板
-- @note   本文件是 klbui 页面模板文件; 
--  \n 本页满窗 parse; 元素只写本文件; 
--  \n 只改 css / dialog / commands; 分辨率/语言/字体/外观在 其他文件定义
--  \n Lua-UI 约定本page页面仅处理与页面交互相关的业务逻辑
-- @history 修改历史
--  \n 2026 创建文件
--]]

local klbui = require("klbcore.klbui")

local M = {}

local jq = function () end
local jq_all = function () end


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
		-- 控件
	}
}

-----------------------------------------------------------------------------------
--- 内部私有函数



-----------------------------------------------------------------------------------
-- commands

M.commands = {
}


-----------------------------------------------------------------------------------
-- 处理导出

M.css = {}
M.dialog = {}
M.jq = jq
M.jq_all = jq_all


-- @brief 按分辨率选择本页布局: 装填 dialog / css, 并重绑 jq
-- @param [in] rs[string]   分辨率档, eg. 'HD' / 'Full HD'
-- @return 无
M.apply_resolution = function (rs)
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

	-- 重绑 jq
	jq = klbui.select(M.dialog)				-- 单选
	jq_all = klbui.select(M.dialog, true)	-- 多选

	M.jq = jq
	M.jq_all = jq_all
end

return M
