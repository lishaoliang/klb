--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  g2de_agent 可选包 — g2de 引擎 AI 调试控制 (g2de.*)
-- @note   宿主经 set_host 注入; 禁止 require g2decore; alua pkg run g2de_agent 走 register_tools
-- @history 修改历史
--  \n 2026 创建文件
--]]
local M = {}


local host = nil


local function _reg(catalog, dispatch, desc, fn)
	local ok, err = catalog.register(desc)

	if not ok then
		return false, err
	end

	ok, err = dispatch.register(desc.name, fn)

	if not ok then
		return false, err
	end

	return true
end


local function _versions(_args)
	if type(host) == "table" and type(host.versions) == "function" then
		return host.versions()
	end

	local out = {}

	local ok_ver, ver = pcall(require, "klbagent.version")

	if ok_ver and type(ver) == "table" then
		out.klbagent = ver.string
	end

	return out
end


local function _selfcheck(_args)
	if type(host) ~= "table" then
		return {
			ok = false,
			reason = "host not set",
		}
	end

	if type(host.selfcheck) == "function" then
		return host.selfcheck()
	end

	return {
		ok = false,
		reason = "host selfcheck missing",
	}
end


local function _snapshot(_args)
	if type(host) ~= "table" or type(host.snapshot) ~= "function" then
		error("g2de host snapshot not available")
	end

	return host.snapshot()
end


local function _control_pause(_args)
	if type(host) ~= "table" or type(host.control_pause) ~= "function" then
		error("g2de host control_pause not available")
	end

	return host.control_pause()
end


local function _control_resume(_args)
	if type(host) ~= "table" or type(host.control_resume) ~= "function" then
		error("g2de host control_resume not available")
	end

	return host.control_resume()
end


local function _control_step(args)
	if type(host) ~= "table" or type(host.control_step) ~= "function" then
		error("g2de host control_step not available")
	end

	return host.control_step(args or {})
end


local function _control_inject_key(args)
	if type(host) ~= "table" or type(host.control_inject_key) ~= "function" then
		error("g2de host control_inject_key not available")
	end

	return host.control_inject_key(args or {})
end


local function _screenshot(args)
	if type(host) ~= "table" or type(host.screenshot) ~= "function" then
		error("g2de host screenshot not available")
	end

	return host.screenshot(args or {})
end


local function _control_inject_mouse(args)
	if type(host) ~= "table" or type(host.control_inject_mouse) ~= "function" then
		error("g2de host control_inject_mouse not available")
	end

	return host.control_inject_mouse(args or {})
end


local function _control_clear_input(_args)
	if type(host) ~= "table" or type(host.control_clear_input) ~= "function" then
		error("g2de host control_clear_input not available")
	end

	return host.control_clear_input()
end


local function _control_tap_key(args)
	if type(host) ~= "table" or type(host.control_tap_key) ~= "function" then
		error("g2de host control_tap_key not available")
	end

	return host.control_tap_key(args or {})
end


local function _control_analyze(args)
	if type(host) ~= "table" or type(host.control_analyze) ~= "function" then
		error("g2de host control_analyze not available")
	end

	return host.control_analyze(args or {})
end


local function _input_state(_args)
	if type(host) ~= "table" or type(host.input_state) ~= "function" then
		error("g2de host input_state not available")
	end

	return host.input_state()
end


--- 宿主 (g2decore.agent) 注入回调
-- @param [in] h[table]			snapshot/versions/selfcheck 函数
-- @return ok[boolean]
M.set_host = function (h)
	if type(h) ~= "table" then
		return false
	end

	host = h
	return true
end


--- 向 catalog/dispatch 注册 g2de.* 工具
-- @param [in] catalog[table]
-- @param [in] dispatch[table]
-- @return ok[boolean]
-- @return err[string|nil]
M.register_embed = function (catalog, dispatch)
	if type(catalog) ~= "table" or type(dispatch) ~= "table" then
		return false, "bad catalog or dispatch"
	end

	local ok, err = _reg(catalog, dispatch, {
		name = "g2de.versions",
		category = "g2de",
		title = "engine versions",
		summary = "Return kg2de / g2decore / klbagent version strings.",
		args_schema = {},
		examples = { { args = {} } },
		docs = "packages.md",
		side_effects = {},
	}, _versions)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.selfcheck",
		category = "g2de",
		title = "embed self-check",
		summary = "Check g2decore.agent host and g2de embed availability.",
		args_schema = {},
		examples = { { args = {} } },
		docs = "packages.md",
		side_effects = {},
	}, _selfcheck)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.snapshot",
		category = "g2de",
		title = "game snapshot",
		summary = "Read-only agent/app state snapshot from embed host.",
		args_schema = {},
		examples = { { args = {} } },
		docs = "packages.md",
		side_effects = {},
	}, _snapshot)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.screenshot",
		category = "g2de",
		title = "capture framebuffer",
		summary = "Capture current backbuffer as PNG; return base64 for AI visual check. Call after control.step while paused.",
		args_schema = {
			keep_file = "boolean optional keep .png under klua_run/tmp/g2de-agent-capture/",
		},
		examples = {
			{ args = {} },
			{ args = { keep_file = true } },
		},
		docs = "packages.md",
		side_effects = {},
	}, _screenshot)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.control.pause",
		category = "g2de",
		title = "pause game loop",
		summary = "Pause app on_frame until resume or step.",
		min_profile = "net",
		args_schema = {},
		examples = { { args = {} } },
		docs = "packages.md",
		side_effects = { write_pref = true },
	}, _control_pause)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.control.resume",
		category = "g2de",
		title = "resume game loop",
		summary = "Resume normal on_frame after pause.",
		min_profile = "net",
		args_schema = {},
		examples = { { args = {} } },
		docs = "packages.md",
		side_effects = { write_pref = true },
	}, _control_resume)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.control.step",
		category = "g2de",
		title = "step frames while paused",
		summary = "When paused, run up to n on_frame ticks (default 1, max 64).",
		min_profile = "net",
		args_schema = {
			n = "number optional step count",
		},
		examples = { { args = { n = 1 } } },
		docs = "packages.md",
		side_effects = { write_pref = true },
	}, _control_step)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.control.inject_key",
		category = "g2de",
		title = "inject key state",
		summary = "Override key down for g2decore.input.key_down (scancode or key name).",
		min_profile = "net",
		args_schema = {
			scancode = "number optional SDL scancode",
			key = "string optional up|down|left|right|space|escape|p|w|a|s|d",
			down = "boolean optional default true",
		},
		examples = {
			{ args = { key = "up", down = true } },
			{ args = { scancode = 44, down = true } },
		},
		docs = "packages.md",
		side_effects = { write_pref = true },
	}, _control_inject_key)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.control.inject_mouse",
		category = "g2de",
		title = "inject mouse state",
		summary = "Override mouse position and button for g2decore.input.mouse_*.",
		min_profile = "net",
		args_schema = {
			x = "number optional client x",
			y = "number optional client y",
			button = "number optional 1=left 2=middle 3=right",
			down = "boolean optional default true",
			clear_pos = "boolean optional clear injected position",
		},
		examples = { { args = { x = 320, y = 240, button = 1, down = true } } },
		docs = "packages.md",
		side_effects = { write_pref = true },
	}, _control_inject_mouse)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.control.clear_input",
		category = "g2de",
		title = "clear injected input",
		summary = "Clear all injected keyboard and mouse overrides.",
		min_profile = "net",
		args_schema = {},
		examples = { { args = {} } },
		docs = "packages.md",
		side_effects = { write_pref = true },
	}, _control_clear_input)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.control.tap_key",
		category = "g2de",
		title = "tap key with sync frames",
		summary = "Inject key down, run sync step frames, then key up.",
		min_profile = "net",
		args_schema = {
			key = "string optional key name",
			scancode = "number optional scancode",
			step = "number optional frames while down (default 1)",
		},
		examples = { { args = { key = "right", step = 2 } } },
		docs = "packages.md",
		side_effects = { write_pref = true },
	}, _control_tap_key)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.input.state",
		category = "g2de",
		title = "input state",
		summary = "Read merged keyboard/mouse state (inject overlay + hardware).",
		args_schema = {},
		examples = { { args = {} } },
		docs = "packages.md",
		side_effects = {},
	}, _input_state)

	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "g2de.analyze",
		category = "g2de",
		title = "pause input step capture analyze",
		summary = "One-shot AI flow: pause, inject keys/mouse, sync step, snapshot, screenshot.",
		min_profile = "net",
		args_schema = {
			pause = "boolean optional default true",
			step = "number optional sync frames (default 1, max 64)",
			keys = "array optional { key, down }",
			mouse = "table optional { x, y, button, down }",
			snapshot = "boolean optional default true",
			screenshot = "boolean optional default true",
			input_state = "boolean optional default true",
			resume = "boolean optional default false",
		},
		examples = {
			{
				args = {
					step = 2,
					keys = { { key = "right", down = true } },
					screenshot = true,
				},
			},
		},
		docs = "packages.md",
		side_effects = { write_pref = true },
	}, _control_analyze)

	if not ok then
		return false, err
	end

	return true
end


--- alua pkg run g2de_agent (tools 包入口)
-- @param [in] _argv[table|nil]
-- @param [in] _manifest[table|nil]
-- @return ok[boolean]
-- @return code[number]
-- @return message[string|nil]
M.register_tools = function (_argv, _manifest)
	local catalog = require("klbagent.core.catalog")
	local dispatch = require("klbagent.core.dispatch")
	local ok, err = M.register_embed(catalog, dispatch)

	if not ok then
		local codes = require("klbagent.code")
		return false, codes.g2de_agent.TOOL_FAILED, tostring(err)
	end

	return true, 0
end


return M

-- end
