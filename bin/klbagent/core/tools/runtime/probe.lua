--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   probe.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  runtime.version / ping / selfcheck / hostinfo / cwd
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; selfcheck 报告 proxy / ALUA_PROXY_invalid; selfcheck 报告 serve.transport_caps (P3)
--]]


local version = require("klbagent.version")
local config = require("klbagent.core.config")
local profiles = require("klbagent.core.profiles")
local hostinfo = require("klbagent.core.hostinfo")
local lfs = require("lfs")


local M = {}


local function _version(_args)
	return {
		string = version.string,
		base = version.base,
		ym = version.ym,
	}
end


local function _ping(_args)
	return {
		pong = true,
	}
end


local function _hostinfo(_args)
	return hostinfo.probe()
end


local function _selfcheck(args)
	args = args or {}
	local catalog = require("klbagent.core.catalog")
	local cfg = config.runtime.load()
	local tools = catalog.list()
	local path = package.path or ""
	local has_klua = string.find(path, "klua", 1, true) ~= nil
	local ok_cjson = pcall(require, "cjson")
	local ok_kenv = pcall(require, "kenv")
	-- 旧平铺残留: 仅查与已加载 core.cli 同根的 klbagent/cli (勿因另一 path 旧安装误报)
	local stale_flat = false
	local ok_core_cli, core_cli = pcall(require, "klbagent.core.cli")
	if (not ok_core_cli) or type(core_cli) ~= "table" or type(core_cli.entry) ~= "function" then
		stale_flat = true
	else
		local core_file = package.searchpath("klbagent.core.cli", package.path)
		if type(core_file) == "string" then
			local root = string.match(core_file, "^(.-)/core/cli/init%.lua$")
			if root == nil then
				root = string.match(core_file, "^(.-)/core/cli%.lua$")
			end
			if type(root) == "string" then
				local f = io.open(root .. "/cli/init.lua", "r")
				if f ~= nil then
					f:close()
					stale_flat = true
				else
					f = io.open(root .. "/cli.lua", "r")
					if f ~= nil then
						f:close()
						stale_flat = true
					end
				end
			end
		end
	end

	local hi = hostinfo.probe()
	local warn = {}
	if cfg.profile == "unsafe" and (not hi.token_set) then
		warn[#warn + 1] = "unsafe_without_ALUA_SERVE_TOKEN"
	end

	if hi.caps.recommend_readonly and cfg.profile == "unsafe" then
		warn[#warn + 1] = "embedded_recommend_readonly"
	end

	local net_util = require("klbagent.core.tools.net.util")
	local proxy = net_util.proxy_status()
	if proxy.invalid then
		warn[#warn + 1] = "ALUA_PROXY_invalid"
	end

	local ai_load_mod = require("klbagent.core.tools.runtime.ai_load")
	local ai_load = ai_load_mod.evaluate(args)
	if type(ai_load.warn) == "table" then
		for i = 1, #ai_load.warn do
			warn[#warn + 1] = ai_load.warn[i]
		end
	end

	if ai_load.status == "missing" or ai_load.status == "stale" then
		warn[#warn + 1] = "ai_load_" .. ai_load.status
	end

	-- 空表须编成 JSON 数组, 勿落成 {}
	local ok_cj, cjson = pcall(require, "cjson")
	if ok_cj and type(cjson) == "table" and cjson.empty_array ~= nil and #warn == 0 then
		warn = cjson.empty_array
	end

	-- core.serve 包外契约已收口; selfcheck 不再探 transport_caps
	local serve_info = nil

	local ai_bad = (ai_load.status == "missing" or ai_load.status == "stale")
	local ok = ok_cjson and ok_kenv and (not stale_flat) and (#tools > 0)
		and (not proxy.invalid) and (not ai_bad)
	return {
		ok = ok,
		version = version.string,
		-- 生效档=override>pref; 勿仅报 pref (serve set_profile 后否则误导)
		profile = profiles.active(),
		io = cfg.io,
		tool_count = #tools,
		package_path_has_klua = has_klua,
		cjson = ok_cjson,
		kenv = ok_kenv,
		stale_flat_modules = stale_flat,
		host = hi,
		proxy = proxy,
		serve = serve_info,
		ai_load = ai_load,
		ai_load_gate = require("klbagent.core.ai_load_gate").status(),
		warn = warn,
	}
end


local function _cwd(_args)
	local path = lfs.currentdir()
	if type(path) ~= "string" or path == "" then
		error("cwd fail")
	end

	return {
		path = path,
	}
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "runtime.version",
		category = "runtime",
		title = "script version",
		summary = "Return klbagent script version string (base.ym).",
		args_schema = {},
		examples = {},
		docs = "tools/runtime.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.version", _version)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "runtime.ping",
		category = "runtime",
		title = "ping",
		summary = "Liveness check; returns pong=true.",
		args_schema = {},
		examples = {},
		docs = "tools/runtime.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.ping", _ping)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "runtime.selfcheck",
		category = "runtime",
		title = "environment self-check",
		summary = "Check version, profile, tools, host, proxy, and ai_load docs reconcile (prompt user if stale/missing).",
		args_schema = {
			host_docs_revision = "string optional",
			host_fingerprint = "string optional",
			host_capable = "boolean optional",
		},
		examples = {
			{ args = {} },
			{ args = { host_docs_revision = "0.1.99.202610", host_capable = true } },
		},
		docs = "tools/runtime.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.selfcheck", _selfcheck)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "runtime.hostinfo",
		category = "runtime",
		title = "host kind and capabilities",
		summary = "Report host_kind/virt/arch/network/caps for this peer (serve or local).",
		args_schema = {},
		examples = { { args = {} } },
		docs = "tools/runtime.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.hostinfo", _hostinfo)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "runtime.cwd",
		category = "runtime",
		title = "current working directory",
		summary = "Return process cwd path (Get-Location equivalent).",
		args_schema = {},
		examples = { { args = {} } },
		docs = "tools/runtime.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.cwd", _cwd)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
