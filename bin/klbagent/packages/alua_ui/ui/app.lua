--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   app.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_ui 多页导航 (home / serve / runtime)
-- @note   上层宿主: relocation -> parse -> modal -> refresh; 页面只处理本页
-- @history 修改历史
--  \n [2026-10] 创建文件 (M4-M6); 对齐 page_tmpl 分层; 去掉 pages.common; 页内直调 app.show
--]]
local klbui = require("klbcore.klbui")
local kgui = require("kgui")


local M = {}


local s_pages = {}
local s_current = nil
local s_name = ""


-- @brief 登记页面
-- @param [in] name[string]
-- @param [in] page[table]
M.register = function (name, page)
	if type(name) == "string" and type(page) == "table" then
		s_pages[name] = page
	end
end


-- @brief 当前页名
-- @return name[string]
M.current = function ()
	return s_name
end


-- @brief 切换页面
-- @param [in] name[string]
-- @return ok[boolean]
-- @return err[string|nil]
M.show = function (name)
	local page = s_pages[name]
	if type(page) ~= "table" then
		return false, "unknown page: " .. tostring(name)
	end

	if s_current ~= nil and type(s_current.dialog) == "table" then
		local old = s_current.dialog.path or s_current.dialog.name
		if type(old) == "string" and old ~= "" then
			pcall(function ()
				klbui.modal_end(true)
			end)
			pcall(function ()
				kgui.remove(old)
			end)
		end
	end

	-- 上层: relocation -> parse -> modal -> refresh(apply_pref)
	page.relocation("HD")
	page.commands = page.commands or {}
	page.css = page.css or {}

	klbui.parse(page.dialog, page.commands, page.css)

	local path = page.dialog.path or page.dialog.name
	if type(path) == "string" and path ~= "" then
		klbui.modal(path)
	end

	if type(page.refresh) == "function" then
		page.refresh()
	elseif type(page.apply_pref) == "function" then
		page.apply_pref()
	end

	s_current = page
	s_name = name
	return true
end


return M

-- end
