--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   reload.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  runtime.reload
-- @note   热更 Lua 工具模块
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; core.config.* 非 sticky (配合 config.set live)
--]]


local M = {}


-- reload scope → 前缀列表; prefix/prefixes 可覆盖
local RELOAD_SCOPES = {
	tools = { "klbagent.core.tools." },
	packages = { "klbagent.packages." },
	klbcore = { "klbcore." },
	-- agent: 工具+可选包; klbagent: 含 core (scripts_install 热更)
	agent = { "klbagent.core.tools.", "klbagent.packages." },
	klbagent = { "klbagent." },
	all = { "klbagent.", "klbcore." },
}

-- 热更 Lua 后仍须进程重启的清单 (文档化; 返回给调用方)
local RELOAD_RESTART_NEEDED = {
	"alua serve (C bindings / TLS listen / already-accepted conns)",
	"alua pkg run * daemon (e.g. svn_https)",
	"libklb / .so / bin/alua (use deploy.self_install|pull_install + restart)",
}


-- serve/dispatch 握 upvalue 的 core 单例: 禁止换表, 否则 HTTP 仍握旧 catalog/dispatch
-- registry 除外 (须重载 PACKAGES 列表)
-- core.config.* 可热更 (set_live / schema; 无 HTTP upvalue)
-- core.tools.* 为内置工具面 (原 tools/), 可热更
local function _reload_is_sticky(name)
	if name == "klbagent.version" then
		return true
	end

	if string.sub(name, 1, #"klbagent.core.") ~= "klbagent.core." then
		return false
	end

	if name == "klbagent.core.registry" then
		return false
	end

	if name == "klbagent.core.tools"
		or string.sub(name, 1, #"klbagent.core.tools.") == "klbagent.core.tools." then
		return false
	end

	if name == "klbagent.core.config"
		or string.sub(name, 1, #"klbagent.core.config.") == "klbagent.core.config." then
		return false
	end

	return true
end


local function _reload_prefixes(args)
	local list = {}

	if type(args.prefixes) == "table" then
		for i = 1, #args.prefixes do
			local p = args.prefixes[i]
			if type(p) == "string" and p ~= "" then
				list[#list + 1] = p
			end
		end
	end

	if type(args.prefix) == "string" and args.prefix ~= "" then
		list[#list + 1] = args.prefix
	end

	if #list == 0 then
		local scope = args.scope
		if type(scope) ~= "string" or scope == "" then
			scope = "tools"
		end

		local preset = RELOAD_SCOPES[scope]
		if preset == nil then
			error("scope must be tools|packages|klbcore|agent|klbagent|all")
		end

		for i = 1, #preset do
			list[#list + 1] = preset[i]
		end

		return list, scope
	end

	return list, args.scope
end


local function _reload(args)
	args = args or {}
	local prefixes, scope = _reload_prefixes(args)
	local cleared = {}
	local preserved = {}
	local seen = {}

	for i = 1, #prefixes do
		local prefix = prefixes[i]
		for name, _ in pairs(package.loaded) do
			if type(name) == "string"
				and string.sub(name, 1, #prefix) == prefix
				and seen[name] ~= true then
				seen[name] = true
				if _reload_is_sticky(name) then
					preserved[#preserved + 1] = name
				else
					package.loaded[name] = nil
					cleared[#cleared + 1] = name
				end
			end
		end
	end

	table.sort(cleared)
	table.sort(preserved)

	local reregister = false
	for i = 1, #prefixes do
		local p = prefixes[i]
		if type(p) ~= "string" then
			-- skip
		elseif p == "klbagent." or p == "klbagent.core.tools." then
			reregister = true
			break
		elseif string.sub(p, 1, #"klbagent.core.tools.") == "klbagent.core.tools." then
			reregister = true
			break
		elseif string.sub(p, 1, #"klbagent.") == "klbagent." then
			-- 宽前缀含 tools 时须重注册
			reregister = true
			break
		end
	end

	if reregister then
		-- 同表清空再注册: serve 握的 dispatch/catalog upvalue 仍有效
		local catalog = require("klbagent.core.catalog")
		local dispatch = require("klbagent.core.dispatch")
		catalog.clear()
		dispatch.clear()

		local registry = require("klbagent.core.registry")
		package.loaded["klbagent.core.registry"] = nil
		registry = require("klbagent.core.registry")

		local ok, err = registry.load_all(catalog, dispatch)
		if not ok then
			error(tostring(err or "reload register fail"))
		end
	end

	local count = #cleared

	-- cjson 空表默认编成 object; 保持 JSON 数组
	local ok_cj, cjson = pcall(require, "cjson")
	if ok_cj and type(cjson) == "table" and cjson.empty_array ~= nil then
		if count == 0 then
			cleared = cjson.empty_array
		end

		if #preserved == 0 then
			preserved = cjson.empty_array
		end

		if #prefixes == 0 then
			prefixes = cjson.empty_array
		end
	end

	-- MCP P4-3: 工具面热更后通知宿主重拉 tools/list
	local list_changed = false
	if reregister then
		local ok_m, mcp = pcall(require, "klbagent.packages.mcp")
		local mcp_server = nil
		if ok_m and type(mcp) == "table" and type(mcp.server) == "function" then
			local ok_s, srv = pcall(mcp.server)
			if ok_s and type(srv) == "table" then
				mcp_server = srv
			end
		end

		if type(mcp_server) == "table"
			and type(mcp_server.notify_tools_list_changed) == "function" then
			list_changed = mcp_server.notify_tools_list_changed() == true
		end
	end

	return {
		ok = true,
		scope = scope,
		prefixes = prefixes,
		prefix = type(prefixes) == "table" and prefixes[1] or nil,
		cleared = cleared,
		preserved = preserved,
		count = count,
		reregister = reregister,
		tools_list_changed = list_changed,
		restart_needed = RELOAD_RESTART_NEEDED,
		note = "tools/packages cleared; core.catalog/dispatch sticky (serve-safe); C/TLS/daemons unchanged",
	}
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "runtime.reload",
		category = "runtime",
		title = "hot-reload Lua tool modules",
		summary = "Clear package.loaded by scope/prefix; sticky core.catalog/dispatch (serve-safe); clear+re-register tools. Needs profile>=net. Does not reload C/TLS/daemons.",
		min_profile = "net",
		args_schema = {
			scope = "string optional tools|packages|klbcore|agent|klbagent|all (default tools)",
			prefix = "string optional single module prefix (overrides scope when set with prefixes)",
			prefixes = "table optional list of module prefixes",
		},
		examples = {
			{ args = {} },
			{ args = { scope = "all" } },
			{ args = { prefix = "klbagent.core.tools.fs" } },
			{ args = { prefixes = { "klbagent.core.tools.", "klbcore." } } },
		},
		docs = "tools/runtime.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.reload", _reload)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
