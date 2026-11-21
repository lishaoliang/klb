--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   profile.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  runtime.set_profile: 已鉴权调整权限档 (一次性 / 永久)
-- @note   scope=once (默认): 仅进程 set_override, 不写 pref
--   \n scope=permanent: 写 runtime.json + 立即生效; serve 未显式 argv 档时重启跟 pref
--   \n 升到 unsafe 须 confirm=true
--   \n 本机 (无请求上下文 / raw|local): 直接认可生效; 网络临时 token: 仅平级或降级, 升档须 raw token
--]]


local profiles = require("klbagent.core.profiles")
local config = require("klbagent.core.config")
local log = require("klbagent.core.log")
local serve_profile = require("klbagent.core.serve.security.profile")


local M = {}


local function _norm_scope(v)
	if type(v) ~= "string" or v == "" then
		return "once"
	end

	local s = string.lower(v)
	if s == "once" or s == "one" or s == "temp" or s == "temporary" then
		return "once"
	end

	if s == "permanent" or s == "persist" or s == "pref" or s == "forever" then
		return "permanent"
	end

	return nil
end


local function _set_profile(args)
	args = args or {}
	local want = args.profile
	if type(want) ~= "string" or want == "" then
		error("need profile")
	end

	want = string.lower(want)
	if not profiles.is_profile(want) then
		error("bad profile")
	end

	local scope = _norm_scope(args.scope)
	if scope == nil then
		error("bad scope (once|permanent)")
	end

	local prev = profiles.active()
	local prev_rank = profiles.rank(prev)
	local want_rank = profiles.rank(want)

	-- 授权: 本机 (无上下文 / raw|local) 直接认可; 网络 temp token 仅平级或降级
	local ok_a, auth = pcall(require, "klbagent.core.auth")
	if ok_a and type(auth) == "table" and type(auth.authorize_raise) == "function" then
		local ok_r, code_r = auth.authorize_raise(want)
		if not ok_r then
			error(auth.auth_err_name(code_r))
		end
	end

	-- 升到 unsafe 须显式确认 (token 已鉴权仍防误触)
	if want == "unsafe" and want_rank > prev_rank then
		if args.confirm ~= true then
			error("raise to unsafe requires confirm=true")
		end
	end

	local pref_touched = false
	local pref_path = nil

	if scope == "permanent" then
		local cfg = config.runtime.load()
		cfg.profile = want
		if not config.runtime.save(cfg) then
			error("save runtime.json fail")
		end

		pref_touched = true
		pref_path = config.runtime.path()
	end

	-- live: 进程 override; serve 在跑时同步 serve_opts (不依赖 serve 门面导出)
	local ok_s, err_s = serve_profile.set_live_profile(want)
	if not ok_s then
		error(tostring(err_s or "set_live_profile fail"))
	end

	local active = profiles.active()
	log.write("info", "runtime.set_profile", {
		module = "runtime",
		prev = prev,
		profile = active,
		scope = scope,
		pref_touched = pref_touched,
		confirm = args.confirm == true,
	})

	return {
		ok = true,
		prev = prev,
		profile = active,
		scope = scope,
		pref_touched = pref_touched,
		pref_path = pref_path,
		confirm = args.confirm == true,
	}
end


-- @brief 注册 runtime.set_profile
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "runtime.set_profile",
		category = "runtime",
		title = "set process profile (once or permanent)",
		summary = "scope=once (default): live override only. scope=permanent: write runtime.json + live. Raise to unsafe needs confirm=true. Serve without argv profile follows pref on restart.",
		min_profile = "readonly",
		args_schema = {
			profile = "string readonly|net|device|unsafe",
			scope = "string optional once|permanent (default once)",
			confirm = "boolean optional; required true when raising to unsafe",
		},
		examples = {
			{ args = { profile = "net" } },
			{ args = { profile = "unsafe", confirm = true } },
			{ args = { profile = "unsafe", scope = "permanent", confirm = true } },
			{ args = { profile = "readonly", scope = "permanent" } },
		},
		docs = "tools/runtime.md",
		side_effects = { "write_pref" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.set_profile", _set_profile)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
