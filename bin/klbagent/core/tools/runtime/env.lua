--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   env.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  runtime.env_get / runtime.env_set
-- @note   env overlay 供 exec 继承
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local env_overlay = require("klbagent.core.config.env_overlay")
local util = require("klbagent.core.tools.runtime.util")


local M = {}


-- @brief 供 unsafe.exec 合并子进程环境
-- @return set[table]		name -> value
-- @return unset[table]		name 数组
M.child_env = function ()
	local data = env_overlay.load()
	local set = {}
	for k, v in pairs(data.set) do
		set[k] = v
	end

	local unset = {}
	for k, _ in pairs(data.unset) do
		unset[#unset + 1] = k
	end

	return set, unset
end


local function _env_get(args)
	local name = args.name
	if not util.name_ok(name) then
		error("need name")
	end

	local data = env_overlay.load()
	if data.unset[name] then
		return {
			name = name,
			value = nil,
			set = false,
			overlay = true,
		}
	end

	if data.set[name] ~= nil then
		return {
			name = name,
			value = data.set[name],
			set = true,
			overlay = true,
		}
	end

	local value = os.getenv(name)
	return {
		name = name,
		value = value,
		set = value ~= nil,
		overlay = false,
	}
end


local function _env_set(args)
	local name = args.name
	if not util.name_ok(name) then
		error("need name")
	end

	local data = env_overlay.load()
	if args.unset == true then
		data.set[name] = nil
		data.unset[name] = true
		if not env_overlay.save(data) then
			error("save overlay fail")
		end

		return {
			name = name,
			unset = true,
			set = false,
		}
	end

	if type(args.value) ~= "string" then
		error("need value string (or unset=true)")
	end

	if string.find(args.value, "[\0]") ~= nil then
		error("bad value")
	end

	data.unset[name] = nil
	data.set[name] = args.value
	if not env_overlay.save(data) then
		error("save overlay fail")
	end

	return {
		name = name,
		value = args.value,
		set = true,
		unset = false,
	}
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "runtime.env_get",
		category = "runtime",
		title = "get environment variable",
		summary = "Read one env var by name; respects process overlay from env_set.",
		args_schema = {
			name = "string env var name",
		},
		examples = {
			{ args = { name = "PATH" } },
			{ args = { name = "HOME" } },
		},
		docs = "tools/runtime.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.env_get", _env_get)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "runtime.env_set",
		category = "runtime",
		title = "set environment overlay",
		summary = "Pref overlay for env_get and unsafe.exec (env_overlay.json). Needs profile>=net.",
		min_profile = "net",
		args_schema = {
			name = "string env var name",
			value = "string optional when unset!=true",
			unset = "boolean optional clear overlay / strip for children",
		},
		examples = {
			{ args = { name = "ALUA_DEMO", value = "1" } },
			{ args = { name = "ALUA_DEMO", unset = true } },
		},
		docs = "tools/runtime.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.env_set", _env_set)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
