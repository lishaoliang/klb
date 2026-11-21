--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   main.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_ui 窗体宿主 (kg2de.klbui 主循环)
-- @note   页面只处理页面逻辑; 缺 kg2de.klbui 时尝试 g2deapp 拉起 boot.lua
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 宿主主循环; app 多页导航
--]]
local kenv = require("kenv")
local lfs = require("lfs")


local M = {}


M.DEFAULT_W = 800
M.DEFAULT_H = 600


local function _join(base, rel)
	if type(base) ~= "string" or base == "" then
		return rel
	end

	local last = base:sub(-1)
	if last == "/" or last == "\\" then
		return base .. rel
	end

	return base .. "/" .. rel
end


local function _pkg_dir()
	local src = debug.getinfo(1, "S").source
	if type(src) == "string" and src:sub(1, 1) == "@" then
		src = src:sub(2)
	end

	if type(src) ~= "string" or src == "" then
		return nil
	end

	local ui_dir = src:match("^(.*)[/\\][^/\\]+$")
	if ui_dir == nil then
		return nil
	end

	return ui_dir:match("^(.*)[/\\][^/\\]+$") or ui_dir
end


local function _has_kg2de_klbui()
	local ok, mod = pcall(require, "kg2de.klbui")
	if not ok or type(mod) ~= "table" then
		return false, nil
	end

	if type(mod.open) ~= "function" then
		return false, nil
	end

	return true, mod
end


local function _find_g2deapp()
	local base = kenv.base_path()
	local kos_ok, kos = pcall(require, "kos")
	local is_win = kos_ok and type(kos) == "table" and kos.os == "windows"
	local names = {}

	if is_win then
		names = { "g2deapp.exe", "g2deapp" }
	else
		names = { "g2deapp", "g2deapp.exe" }
	end

	for i = 1, #names do
		local p = _join(base, names[i])
		local attr = lfs.attributes(p)
		if attr ~= nil and attr.mode == "file" then
			return p
		end
	end

	return nil
end


local function _quote(s)
	s = tostring(s or "")
	local kos_ok, kos = pcall(require, "kos")
	if kos_ok and type(kos) == "table" and kos.os == "windows" then
		return '"' .. s:gsub('"', '\\"') .. '"'
	end

	return "'" .. s:gsub("'", "'\\''") .. "'"
end


-- @brief 无进程内 kg2de 时转 g2deapp 跑 boot.lua
-- @param [in] argv[table|nil]
-- @return ok[boolean]
-- @return err[string|nil]
local function _spawn_g2deapp(argv)
	local exe = _find_g2deapp()
	if exe == nil then
		return false, "g2deapp not found (need kg2de.klbui or g2deapp beside runtime root)"
	end

	local pkg = _pkg_dir()
	if pkg == nil then
		return false, "alua_ui package dir not found"
	end

	local boot = _join(pkg, "boot.lua")
	local attr = lfs.attributes(boot)
	if attr == nil then
		return false, "alua_ui boot.lua missing"
	end

	local cmd = _quote(exe) .. " " .. _quote(boot)
	if type(argv) == "table" then
		for i = 1, #argv do
			cmd = cmd .. " " .. _quote(argv[i])
		end
	end

	local rc = os.execute(cmd)
	if rc == true or rc == 0 then
		return true
	end

	return false, "g2deapp exit=" .. tostring(rc)
end


local function _try_load_font()
	local kgui = require("kgui")
	local base = kenv.base_path()
	local font_dir = _join(base, "demores/font")
	local attr = lfs.attributes(font_dir)
	if attr == nil or attr.mode ~= "directory" then
		return false
	end

	local pick = nil
	for name in lfs.dir(font_dir) do
		local low = string.lower(name)
		if string.match(low, "%.ttf$") or string.match(low, "%.otf$") or string.match(low, "%.ttc$") then
			pick = _join(font_dir, name)
			break
		end
	end

	if pick == nil then
		return false
	end

	local rc = kgui.load_font(pick)
	return (0 == rc)
end


-- @brief 进程内开窗主循环
-- @param [in] opts[table|nil]
-- @return ok[boolean]
-- @return err[string|nil]
local function _run_inproc(opts)
	opts = opts or {}
	local ok_ui, ui = _has_kg2de_klbui()
	if not ok_ui then
		return false, "kg2de.klbui not available"
	end

	local bridge = require("klbagent.packages.alua_ui.ui.bridge")
	local app = require("klbagent.packages.alua_ui.ui.app")
	local home = require("klbagent.packages.alua_ui.pages.home")
	local serve_page = require("klbagent.packages.alua_ui.pages.serve")
	local runtime_page = require("klbagent.packages.alua_ui.pages.runtime")

	local ok_tools, err_tools = bridge.ensure_tools()
	if not ok_tools then
		return false, err_tools
	end

	local w = opts.w or M.DEFAULT_W
	local h = opts.h or M.DEFAULT_H
	local title = opts.title or "alua-ui"

	local rc_open = ui.open(title, w, h)
	if type(rc_open) == "number" and 0 ~= rc_open then
		return false, "kg2de.klbui.open fail rc=" .. tostring(rc_open)
	end

	local canvas = ui.canvas_create(w, h)
	if canvas == nil then
		ui.close()
		return false, "kg2de.klbui.canvas_create fail"
	end

	local rc_att = ui.attach_canvas(canvas)
	if type(rc_att) == "number" and 0 ~= rc_att then
		ui.canvas_destroy(canvas)
		ui.close()
		return false, "kg2de.klbui.attach_canvas fail rc=" .. tostring(rc_att)
	end

	_try_load_font()

	app.register("home", home)
	app.register("serve", serve_page)
	app.register("runtime", runtime_page)

	local ok_show, err_show = app.show("home")
	if not ok_show then
		ui.canvas_destroy(canvas)
		ui.close()
		return false, err_show
	end

	-- 推荐标准: 已在 CLI 宿主 kco 内则本协程帧循环 + co_sleep;
	--   否则 set_loop_run + klbenv.co_main_loop (禁裸 while loop_once)
	local ok_u, co_util = pcall(require, "klbagent.core.co_util")
	local in_host = ok_u and type(co_util) == "table" and co_util.in_kco() == true
	local ok_c, kco = pcall(require, "kco")
	local can_co = in_host and ok_c and type(kco) == "table" and type(kco.co_sleep) == "function"

	local function _ui_frame()
		ui.pump_events()
		ui.gui_feed()
		ui.canvas_refresh(canvas)
	end

	if can_co then
		while not ui.poll_quit() do
			_ui_frame()
			kco.co_sleep(1)
		end
	else
		local klbenv = require("klbcore.klbenv")
		klbenv.set_loop_run(function ()
			if ui.poll_quit() then
				kenv.exit()
				return
			end

			_ui_frame()
		end)
		klbenv.co_main_loop()
	end

	ui.canvas_destroy(canvas)
	ui.close()
	return true
end


-- @brief 开窗并进入主循环
-- @param [in] opts[table|nil]  w/h/title/argv; force_spawn=true 强制 g2deapp
-- @return ok[boolean]
-- @return err[string|nil]
M.run = function (opts)
	opts = opts or {}

	if opts.force_spawn then
		return _spawn_g2deapp(opts.argv)
	end

	local ok_ui = _has_kg2de_klbui()
	if ok_ui then
		return _run_inproc(opts)
	end

	return _spawn_g2deapp(opts.argv)
end


return M

-- end
