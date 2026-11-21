--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   session.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  net.session_begin / net.session_end
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local util = require("klbagent.core.tools.net.util")


local M = {}


local function _session_begin(args)
	local dir = "/tmp"
	if type(args) == "table" and type(args.dir) == "string" and args.dir ~= "" then
		if not util.path_ok(args.dir) then
			error("dir path invalid")
		end

		dir = args.dir
	end

	local path = string.format("%s/alua_cookie_%d_%d.jar", dir, os.time(), math.random(1000, 9999))
	if not util.write_file(path, "") then
		error("create cookie_jar fail")
	end

	return {
		cookie_jar = path,
		message = "pass cookie_jar to net.http_get / net.http_post for session login",
	}
end


local function _session_end(args)
	local jar = args and args.cookie_jar
	if not util.path_ok(jar) then
		error("cookie_jar path invalid")
	end

	local base = string.match(jar, "([^/]+)$") or ""
	local under_tmp = string.find(jar, "^/tmp/") ~= nil
	local our_name = string.find(base, "^alua_cookie_") ~= nil
	if not under_tmp and not our_name then
		error("refuse to remove cookie_jar outside /tmp or non-alua name")
	end

	os.remove(jar)
	return {
		removed = true,
		cookie_jar = jar,
	}
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "net.session_begin",
		category = "net",
		title = "Begin cookie session",
		summary = "Create empty cookie_jar path for login then authenticated GETs/POSTs.",
		min_profile = "net",
		args_schema = {
			dir = "string optional directory for jar (default /tmp)",
		},
		examples = { { args = {} } },
		docs = "README.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("net.session_begin", _session_begin)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "net.session_end",
		category = "net",
		title = "End cookie session",
		summary = "Remove cookie_jar created under /tmp (or alua_cookie_* name).",
		min_profile = "net",
		args_schema = {
			cookie_jar = "string cookie jar path from session_begin",
		},
		examples = { { args = { cookie_jar = "/tmp/alua_cookie_1.jar" } } },
		docs = "README.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("net.session_end", _session_end)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
