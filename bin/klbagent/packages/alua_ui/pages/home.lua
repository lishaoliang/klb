--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   home.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_ui 总览页 + 目标切换 (local / peer / host)
-- @note   页面按 page_tmpl; 仅页面逻辑; token 不展示; 开窗由 ui/main
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 总览页; 目标切换; 对齐 page_tmpl; 去掉 pages.common; 导航写本页
--]]


local klbui = require("klbcore.klbui")
local bridge = require("klbagent.packages.alua_ui.ui.bridge")
local app = require("klbagent.packages.alua_ui.ui.app")


local M = {}

local jq = function () end
local jq_all = function () end

local s_peers = {}
local s_peer_i = 0

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
	['title'] = 'alua-ui',

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
			['title'] = 'Overview',
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
			['title'] = 'local',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {16, Y0 + 72, 100, 32},
			['name'] = 'btn_tgt_local',
			['title'] = 'Local',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {126, Y0 + 72, 140, 32},
			['name'] = 'btn_tgt_loopback',
			['title'] = '127.0.0.1',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {276, Y0 + 72, 120, 32},
			['name'] = 'btn_tgt_peers',
			['title'] = 'Load peers',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {406, Y0 + 72, 120, 32},
			['name'] = 'btn_tgt_peer',
			['title'] = 'Next peer',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {16, Y0 + 120, 120, 28},
			['name'] = 'lab_ver_k',
			['title'] = 'version:',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {140, Y0 + 120, 640, 28},
			['name'] = 'lab_ver',
			['title'] = '-',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {16, Y0 + 156, 120, 28},
			['name'] = 'lab_ping_k',
			['title'] = 'ping:',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {140, Y0 + 156, 640, 28},
			['name'] = 'lab_ping',
			['title'] = '-',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {16, Y0 + 192, 120, 28},
			['name'] = 'lab_sc_k',
			['title'] = 'selfcheck:',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {140, Y0 + 192, 640, 28},
			['name'] = 'lab_sc',
			['title'] = '-',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {16, Y0 + 228, 120, 28},
			['name'] = 'lab_pf_k',
			['title'] = 'profile:',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {140, Y0 + 228, 640, 28},
			['name'] = 'lab_pf',
			['title'] = '-',
		},
		{
			['type'] = 'kstatic',
			['pos'] = {16, Y0 + 276, 768, 64},
			['name'] = 'lab_msg',
			['title'] = '',
		},
		{
			['type'] = 'kbutton',
			['pos'] = {16, Y0 + 360, 120, 36},
			['name'] = 'btn_refresh',
			['title'] = 'Refresh',
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


local function _fmt_ver(ver)
	if type(ver) ~= "table" then
		return "-"
	end

	local s = ver.string or ver.klbagent or ver.alua
	if type(s) == "string" and s ~= "" then
		return s
	end

	return tostring(ver.value or "-")
end


local function _fmt_ping(ping)
	if type(ping) ~= "table" then
		return "fail"
	end

	if ping.ok == true or ping.pong == true then
		return "ok"
	end

	if type(ping.message) == "string" and ping.message ~= "" then
		return ping.message
	end

	return "ok"
end


local function _fmt_sc(sc)
	if type(sc) ~= "table" then
		return "fail"
	end

	if sc.ok == true then
		return "ok"
	end

	if type(sc.reason) == "string" and sc.reason ~= "" then
		return sc.reason
	end

	return "not ok"
end


local function _fmt_pf(runtime)
	if type(runtime) ~= "table" then
		return "-"
	end

	local cfg = runtime.cfg or runtime
	if type(cfg) == "table" and type(cfg.profile) == "string" then
		return cfg.profile
	end

	if type(runtime.profile) == "string" then
		return runtime.profile
	end

	return "-"
end


local function _after_target(ok, err)
	if not ok then
		_set("lab_msg", tostring(err or "set_target fail"))
		_set("lab_tgt", bridge.target_label())
		return
	end

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
	['btn_tgt_local'] = {
		['click'] = function ()
			local ok, err = bridge.set_target({ mode = "local" })
			_after_target(ok, err)
		end,
	},
	['btn_tgt_loopback'] = {
		['click'] = function ()
			local ok, err = bridge.set_target({
				mode = "host",
				host = "127.0.0.1",
				port = 12817,  -- DEFAULT_PORT closed; UI 临时字面量, 宜显式配置
				scheme = "https",
			})
			_after_target(ok, err)
		end,
	},
	['btn_tgt_peers'] = {
		['click'] = function ()
			local ok, res, err = bridge.call("peer.list", {})
			if not ok then
				_set("lab_msg", "peer.list fail (need local); " .. tostring(err and err.message or err))
				return
			end

			s_peers = {}
			s_peer_i = 0
			local list = res and (res.peers or res.list or res.items) or nil
			if type(list) == "table" then
				for i = 1, #list do
					local p = list[i]
					local id = nil
					if type(p) == "string" then
						id = p
					elseif type(p) == "table" then
						id = p.id or p.name or p.peer
					end
					if type(id) == "string" and id ~= "" then
						s_peers[#s_peers + 1] = id
					end
				end
			end

			_set("lab_msg", "peers=" .. tostring(#s_peers))
			if 0 < #s_peers then
				s_peer_i = 1
				local ok2, err2 = bridge.set_target({ mode = "peer", peer = s_peers[1] })
				_after_target(ok2, err2)
			end
		end,
	},
	['btn_tgt_peer'] = {
		['click'] = function ()
			if #s_peers < 1 then
				_set("lab_msg", "Load peers first")
				return
			end

			s_peer_i = s_peer_i + 1
			if s_peer_i > #s_peers then
				s_peer_i = 1
			end

			local ok, err = bridge.set_target({ mode = "peer", peer = s_peers[s_peer_i] })
			_after_target(ok, err)
		end,
	},
}


-----------------------------------------------------------------------------------
-- 处理导出

M.css = {}
M.dialog = {}
M.jq = jq
M.jq_all = jq_all


-- @brief 按快照同步本页控件 (宿主 refresh 后调用)
-- @param [in] snap[table|nil]
M.apply_pref = function (snap)
	snap = snap or {}
	_set("lab_tgt", snap.target or bridge.target_label())
	_set("lab_ver", _fmt_ver(snap.version))
	_set("lab_ping", _fmt_ping(snap.ping))
	_set("lab_sc", _fmt_sc(snap.selfcheck))
	_set("lab_pf", _fmt_pf(snap.runtime))
	_set("lab_msg", snap.message or "")
end


-- @brief 拉总览并 apply_pref
-- @return ok[boolean]
M.refresh = function ()
	local ok, snap = bridge.overview()
	if ok and type(snap) == "table" then
		M.apply_pref(snap)
		return true
	end

	M.apply_pref({
		target = bridge.target_label(),
		message = "overview fail",
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
