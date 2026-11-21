--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  config.* 工具 (pref JSON)
-- @note   config.get / config.set; 对接 klbagent.core.config
-- @history 修改历史
--  \n [2026-09] 创建文件
--  \n [2026-10] proxy/serve/peers; scope=once|permanent; live apply; restart_needed; runtime.loop_sleep_ms → kenv.set_loop_sleep 热生效
--]]
local config = require("klbagent.core.config")
local profiles = require("klbagent.core.profiles")
local log = require("klbagent.core.log")


local M = {}


local _MODULES = {
	runtime = true,
	log = true,
	proxy = true,
	serve = true,
	peers = true,
}


local function _norm_scope(v)
	-- 默认 permanent: 兼容旧 config.set 必写盘
	if type(v) ~= "string" or v == "" then
		return "permanent"
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


-- @brief 浅合补丁到当前模块配置 (partial cfg)
-- @param [in] mod[string]
-- @param [in] patch[table]
-- @return merged[table]
local function _merge_patch(mod, patch)
	local cur = config[mod].load()
	if type(patch) ~= "table" then
		return cur
	end

	if mod == "proxy" then
		local px = require("klbagent.packages.proxy.config")
		local out = px.defaults()
		px.merge_overlay(out, cur)
		px.merge_overlay(out, patch)
		out._path = nil
		return out
	end

	local out = {}
	for k, v in pairs(cur) do
		out[k] = v
	end

	for k, v in pairs(patch) do
		out[k] = v
	end

	return out
end


local function _get(args)
	local all = config.load()
	local mod = args.module

	if type(mod) == "string" and mod ~= "" then
		if _MODULES[mod] ~= true then
			error("bad module")
		end

		return {
			module = mod,
			cfg = all[mod],
			dir = config.dir(),
			path = config[mod].path(),
			live = config[mod].has_live(),
		}
	end

	return {
		runtime = all.runtime,
		log = all.log,
		proxy = all.proxy,
		serve = all.serve,
		peers = all.peers,
		dir = config.dir(),
		live = {
			runtime = config.runtime.has_live(),
			log = config.log.has_live(),
			proxy = config.proxy.has_live(),
			serve = config.serve.has_live(),
			peers = config.peers.has_live(),
		},
	}
end


-- @brief runtime 模块: 升 unsafe 须 confirm; profile / loop_sleep_ms 热生效
-- @param [in] merged[table]
-- @param [in] args[table]
-- @return restart_needed[boolean]
local function _apply_runtime(merged, args)
	local profiles = require("klbagent.core.profiles")
	local want = merged.profile
	if type(want) == "string" and want ~= "" then
		want = string.lower(want)
		local prev = profiles.active()
		local prev_rank = profiles.rank(prev)
		local want_rank = profiles.rank(want)

		if want == "unsafe" and want_rank > prev_rank then
			if args.confirm ~= true then
				error("raise to unsafe requires confirm=true")
			end
		end

		error("alua.core.serve.UNAVAILABLE")
	end

	-- loop_sleep_ms: 热改本 env; 已跑 kthread 不回溯 (新 start 继承当前值)
	local ms = tonumber(merged.loop_sleep_ms)
	if type(ms) == "number" then
		ms = math.floor(ms)
		if ms < 1 then
			ms = 1
		end

		local ok_e, kenv = pcall(require, "kenv")
		if ok_e and type(kenv) == "table" and type(kenv.set_loop_sleep) == "function" then
			kenv.set_loop_sleep(ms)
		end
	end

	return false
end


-- @brief serve: tls_face 变更须重启监听
-- @param [in] prev[table]
-- @param [in] merged[table]
-- @return restart_needed[boolean]
local function _apply_serve(prev, merged)
	local a = tostring(prev.tls_face or "")
	local b = tostring(merged.tls_face or "")
	if a ~= b then
		return true
	end

	return false
end


-- @brief proxy: 热更新入口 cfg; listen 变则 restart_needed
-- @param [in] merged[table]
-- @return restart_needed[boolean]
local function _apply_proxy(merged)
	local ok_e, entry = pcall(require, "klbagent.packages.proxy.entry")
	if not ok_e or type(entry) ~= "table" or type(entry.apply_live) ~= "function" then
		-- 包未载: 仅 pref live; 入口未跑则无需 restart
		return false
	end

	local ok_a, err_a, need_r = entry.apply_live(merged)
	if not ok_a then
		error(tostring(err_a or "proxy apply_live fail"))
	end

	return need_r == true
end


-- @brief token_ref 仅允许本地名 / env: / peer: (禁绝对路径与上级跳转)
-- @param [in] ref[string|nil]
-- @return ok[boolean]
local function _token_ref_local(ref)
	if type(ref) ~= "string" or ref == "" then
		return true
	end

	if string.match(ref, "^env:") ~= nil or string.match(ref, "^peer:") ~= nil then
		return true
	end

	local file_part = string.match(ref, "^file:(.+)$") or ref
	if string.sub(file_part, 1, 1) == "/" or string.sub(file_part, 1, 1) == "\\" then
		return false
	end

	if string.match(file_part, "^%a:[/\\]") ~= nil then
		return false
	end

	return string.find(file_part, "%.%.") == nil
end


-- @brief config.set 写入授权: peers/serve 须 device+; runtime.profile 按提档判定
-- @param [in] mod[string]
-- @param [in] cfg[table]
-- @return ok[boolean]
-- @return err[string|nil]
local function _authorize_set(mod, cfg)
	local ok_a, auth = pcall(require, "klbagent.core.auth")
	if not ok_a or type(auth) ~= "table" then
		return false, "auth unavailable"
	end

	local ra = nil
	if type(auth.request_auth) == "function" then
		ra = auth.request_auth()
	end

	local caller = "readonly"
	if type(auth.effective_profile) == "function" then
		caller = auth.effective_profile()
	end

	local trusted = (type(ra) ~= "table") or ra.kind == "raw" or ra.kind == "local"

	-- peers/serve 属管理面: 本地信任方, 或调用者档 >= device
	if mod == "peers" or mod == "serve" then
		if not trusted and profiles.rank(caller) < profiles.rank("device") then
			return false, "need device+ for " .. mod
		end
	end

	-- proxy 属网络面: 本地信任方, 或调用者档 >= net
	if mod == "proxy" and not trusted and profiles.rank(caller) < profiles.rank("net") then
		return false, "need net+ for proxy"
	end

	-- log / runtime 属运行面 (runtime.profile 另按提档判定): 本地信任方, 或调用者档 >= net
	if mod == "log" or mod == "runtime" then
		if not trusted and profiles.rank(caller) < profiles.rank("net") then
			return false, "need net+ for " .. mod
		end
	end

	-- runtime.profile 调整按提档判定 (临时 token 禁升档)
	if mod == "runtime" and type(cfg) == "table" and type(cfg.profile) == "string" and cfg.profile ~= "" then
		if type(auth.authorize_raise) == "function" then
			local ok_r, code_r = auth.authorize_raise(string.lower(cfg.profile))
			if not ok_r then
				return false, auth.auth_err_name(code_r)
			end
		end
	end

	-- peers token_ref 仅本地引用 (防任意路径/外部引用)
	if mod == "peers" and type(cfg) == "table" and type(cfg.peers) == "table" then
		for _, row in pairs(cfg.peers) do
			if type(row) == "table" and not _token_ref_local(row.token_ref) then
				return false, "bad token_ref"
			end
		end
	end

	return true
end


local function _set(args)
	args = args or {}
	local mod = args.module
	local cfg = args.cfg

	if type(mod) ~= "string" or mod == "" then
		error("need module")
	end

	if type(cfg) ~= "table" then
		error("need cfg table")
	end

	if _MODULES[mod] ~= true then
		error("bad module")
	end

	local scope = _norm_scope(args.scope)
	if scope == nil then
		error("bad scope (once|permanent)")
	end

	local ok_g, err_g = _authorize_set(mod, cfg)
	if not ok_g then
		error(tostring(err_g or "config.set denied"))
	end

	local prev = config[mod].load()
	local merged = _merge_patch(mod, cfg)
	merged = config[mod].normalize(merged)

	local pref_touched = false
	local pref_path = nil
	local restart_needed = false

	if scope == "permanent" then
		if not config[mod].save(merged) then
			error("save " .. mod .. ".json fail")
		end

		pref_touched = true
		pref_path = config[mod].path()
	end

	-- 一律 live (once 仅此; permanent 写盘后亦热生效)
	config[mod].set_live(merged)

	if mod == "runtime" then
		restart_needed = _apply_runtime(merged, args)
	elseif mod == "serve" then
		restart_needed = _apply_serve(prev, merged)
	elseif mod == "proxy" then
		restart_needed = _apply_proxy(merged)
	end
	-- log / peers: load() 已 live 优先; 无 restart

	log.write("info", "config.set", {
		module = "config",
		cfg_module = mod,
		scope = scope,
		pref_touched = pref_touched,
		restart_needed = restart_needed,
	})

	return {
		ok = true,
		module = mod,
		cfg = config[mod].load(),
		path = config[mod].path(),
		scope = scope,
		pref_touched = pref_touched,
		pref_path = pref_path,
		live = true,
		restart_needed = restart_needed,
		confirm = args.confirm == true,
	}
end


-- @brief 注册 config 工具
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "config.get",
		category = "config",
		title = "get pref config",
		summary = "Load pref JSON (live overlay preferred). Optional module=runtime|log|proxy|serve|peers. runtime includes loop_sleep_ms (kenv loop sleep max ms, default 5).",
		args_schema = {
			module = "string optional runtime|log|proxy|serve|peers",
		},
		examples = {
			{ args = {} },
			{ args = { module = "runtime" } },
			{ args = { module = "proxy" } },
			{ args = { module = "peers" } },
		},
		docs = "rules/core.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("config.get", _get)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "config.set",
		category = "config",
		title = "set pref config module",
		summary = "scope=permanent (default): write JSON + live. scope=once: live only. Partial cfg merges onto current. Raise runtime.profile to unsafe needs confirm=true. runtime.loop_sleep_ms (>=1) hot-applies kenv.set_loop_sleep. serve tls_face / proxy listen change may set restart_needed. Temporary proxy chain switch: proxy.use (not config.set).",
		min_profile = "readonly",
		args_schema = {
			module = "string runtime|log|proxy|serve|peers",
			cfg = "table module fields (partial ok; runtime: profile/io/loop_sleep_ms)",
			scope = "string optional once|permanent (default permanent)",
			confirm = "boolean optional; required true when raising runtime.profile to unsafe",
		},
		examples = {
			{ args = { module = "log", cfg = { level = "warn" } } },
			{ args = { module = "log", cfg = { level = "info" }, scope = "once" } },
			{ args = { module = "serve", cfg = { tls_face = "mnp" } } },
			{ args = { module = "runtime", cfg = { profile = "unsafe" }, confirm = true, scope = "once" } },
			{ args = { module = "runtime", cfg = { loop_sleep_ms = 1 } } },
			{ args = { module = "runtime", cfg = { loop_sleep_ms = 10 }, scope = "once" } },
			{ args = { module = "peers", cfg = { peers = {} }, scope = "once" } },
		},
		docs = "rules/core.md",
		side_effects = { "write_pref" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("config.set", _set)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
