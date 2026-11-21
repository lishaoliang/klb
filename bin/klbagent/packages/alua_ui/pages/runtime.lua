--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   runtime.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_ui runtime 档位页 (set_profile / unsafe 确认)
-- @note   页面按 page_tmpl; 仅页面逻辑; 开窗由 ui/main
-- @history 修改历史
--  \n [2026-10] 创建文件 (M6); 对齐 page_tmpl; 去掉 pages.common; 导航写本页
--]]


local klbui = require("klbcore.klbui")
local bridge = require("klbagent.packages.alua_ui.ui.bridge")
local app = require("klbagent.packages.alua_ui.ui.app")


local M = {}

local jq = function () end
local jq_all = function () end

local s_scope = "once"
local s_want = "readonly"
local s_unsafe_armed = false

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
	['title'] = 'runtime',

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
			['title'] = 'Runtime profile',
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
			['name'] = 'lab_active_k',
			['title'] = 'active:',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {140, Y0 + 80, 200, 28},
			['name'] = 'lab_active',
			['title'] = '-',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {360, Y0 + 80, 80, 28},
			['name'] = 'lab_pref_k',
			['title'] = 'pref:',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {440, Y0 + 80, 200, 28},
			['name'] = 'lab_pref',
			['title'] = '-',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {16, Y0 + 120, 100, 32},
			['name'] = 'btn_pf_ro',
			['title'] = 'readonly',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {126, Y0 + 120, 80, 32},
			['name'] = 'btn_pf_net',
			['title'] = 'net',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {216, Y0 + 120, 100, 32},
			['name'] = 'btn_pf_dev',
			['title'] = 'device',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {326, Y0 + 120, 100, 32},
			['name'] = 'btn_pf_unsafe',
			['title'] = 'unsafe',
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
			['title'] = 'once',
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
			['pos'] = {146, Y0 + 260, 160, 36},
			['name'] = 'btn_confirm_unsafe',
			['title'] = 'Confirm unsafe',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {316, Y0 + 260, 120, 36},
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


local function _pick(profile)
	s_want = profile
	s_unsafe_armed = false
	_set("lab_msg", "want=" .. tostring(profile))
end


local function _apply(confirm)
	local ok, res, err = bridge.set_profile(s_want, s_scope, confirm == true)
	if not ok then
		_set("lab_msg", "set_profile fail: " .. tostring(err and err.message or err))
		s_unsafe_armed = false
		return
	end

	s_unsafe_armed = false
	local msg = "profile=" .. tostring(res and res.profile or s_want) .. " scope=" .. s_scope
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
	['btn_pf_ro'] = {
		['click'] = function ()
			_pick("readonly")
		end,
	},
	['btn_pf_net'] = {
		['click'] = function ()
			_pick("net")
		end,
	},
	['btn_pf_dev'] = {
		['click'] = function ()
			_pick("device")
		end,
	},
	['btn_pf_unsafe'] = {
		['click'] = function ()
			_pick("unsafe")
			_set("lab_msg", "want=unsafe ; press Confirm unsafe then Apply")
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
	['btn_confirm_unsafe'] = {
		['click'] = function ()
			if s_want ~= "unsafe" then
				_set("lab_msg", "pick unsafe first")
				return
			end

			s_unsafe_armed = true
			_set("lab_msg", "unsafe armed ; press Apply")
		end,
	},
	['btn_apply'] = {
		['click'] = function ()
			if s_want == "unsafe" and not s_unsafe_armed then
				_set("lab_msg", "unsafe needs Confirm unsafe")
				return
			end

			_apply(s_want == "unsafe" and s_unsafe_armed)
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
	_set("lab_active", snap.active or "-")
	_set("lab_pref", snap.pref_profile or "-")
	_set("lab_scope", s_scope)
	_set("lab_msg", snap.message or "")
end


-- @brief 拉 runtime 档位并 apply_pref
-- @return ok[boolean]
M.refresh = function ()
	local ok, snap = bridge.runtime_snap()
	if ok and type(snap) == "table" then
		M.apply_pref(snap)
		return true
	end

	M.apply_pref({
		message = (snap and snap.message) or "runtime_snap fail",
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
