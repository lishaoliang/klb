--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   serve.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_ui serve 配置页 (tls_face / scope / restart_needed)
-- @note   页面按 page_tmpl; 仅页面逻辑; 开窗由 ui/main
-- @history 修改历史
--  \n [2026-10] 创建文件 (M5); 对齐 page_tmpl; 去掉 pages.common; 导航写本页
--]]


local klbui = require("klbcore.klbui")
local bridge = require("klbagent.packages.alua_ui.ui.bridge")
local app = require("klbagent.packages.alua_ui.ui.app")


local M = {}

local jq = function () end
local jq_all = function () end

local s_scope = "permanent"
local s_face = "mnp"

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
	['title'] = 'serve',

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
		{
			['type'] = 'kstatic',
			['pos'] = {16, Y0, 768, 28},
			['name'] = 'lab_title',
			['title'] = 'Serve config',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {16, Y0 + 36, 80, 28},
			['name'] = 'lab_tgt_k',
			['title'] = 'target:',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {100, Y0 + 36, 680, 28},
			['name'] = 'lab_tgt',
			['title'] = '-',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {16, Y0 + 80, 120, 28},
			['name'] = 'lab_face_k',
			['title'] = 'tls_face:',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {140, Y0 + 80, 200, 28},
			['name'] = 'lab_face',
			['title'] = '-',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {16, Y0 + 120, 90, 32},
			['name'] = 'btn_face_mnp',
			['title'] = 'mnp',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {116, Y0 + 120, 90, 32},
			['name'] = 'btn_face_smp',
			['title'] = 'smp',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {216, Y0 + 120, 90, 32},
			['name'] = 'btn_face_http',
			['title'] = 'http',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {16, Y0 + 170, 80, 28},
			['name'] = 'lab_scope_k',
			['title'] = 'scope:',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {100, Y0 + 170, 200, 28},
			['name'] = 'lab_scope',
			['title'] = 'permanent',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {16, Y0 + 208, 100, 32},
			['name'] = 'btn_scope_once',
			['title'] = 'once',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {126, Y0 + 208, 120, 32},
			['name'] = 'btn_scope_perm',
			['title'] = 'permanent',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {16, Y0 + 260, 120, 36},
			['name'] = 'btn_apply',
			['title'] = 'Apply',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {146, Y0 + 260, 120, 36},
			['name'] = 'btn_refresh',
			['title'] = 'Refresh',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {16, Y0 + 316, 768, 80},
			['name'] = 'lab_msg',
			['title'] = '',
		},
	},
}


-----------------------------------------------------------------------------------
-- helpers

local function _set(name, title)
	pcall(function ()
		jq(name).set("title", tostring(title or "-"))
	end)
end


local function _apply()
	local ok, res, err = bridge.serve_set({ tls_face = s_face }, s_scope)
	if not ok then
		_set("lab_msg", "apply fail: " .. tostring(err and err.message or err))
		return
	end

	local msg = "applied tls_face=" .. tostring(s_face) .. " scope=" .. s_scope
	if type(res) == "table" and res.restart_needed == true then
		msg = msg .. " ; restart_needed=true"
	end

	_set("lab_msg", msg)
	M.refresh()
end


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
	['btn_refresh'] = {
		['click'] = function ()
			M.refresh()
		end,
	},
	['btn_face_mnp'] = {
		['click'] = function ()
			s_face = "mnp"
			_set("lab_face", s_face)
		end,
	},
	['btn_face_smp'] = {
		['click'] = function ()
			s_face = "smp"
			_set("lab_face", s_face)
		end,
	},
	['btn_face_http'] = {
		['click'] = function ()
			s_face = "http"
			_set("lab_face", s_face)
		end,
	},
	['btn_scope_once'] = {
		['click'] = function ()
			s_scope = "once"
			_set("lab_scope", s_scope)
		end,
	},
	['btn_scope_perm'] = {
		['click'] = function ()
			s_scope = "permanent"
			_set("lab_scope", s_scope)
		end,
	},
	['btn_apply'] = {
		['click'] = function ()
			_apply()
		end,
	},
}


-----------------------------------------------------------------------------------
-- 处理导出

M.css = {}
M.dialog = {}
M.jq = jq
M.jq_all = jq_all


-- @brief 按快照同步本页控件
-- @param [in] snap[table|nil]
M.apply_pref = function (snap)
	snap = snap or {}
	_set("lab_tgt", bridge.target_label())
	_set("lab_face", snap.tls_face or s_face or "-")
	_set("lab_scope", s_scope)
	_set("lab_msg", snap.message or "")
	if type(snap.tls_face) == "string" and snap.tls_face ~= "" then
		s_face = snap.tls_face
	end
end


-- @brief 拉 serve 配置并 apply_pref
-- @return ok[boolean]
M.refresh = function ()
	local ok, snap = bridge.serve_snap()
	if ok and type(snap) == "table" then
		M.apply_pref(snap)
		return true
	end

	M.apply_pref({
		message = (snap and snap.message) or "serve_snap fail",
	})
	return false
end


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
