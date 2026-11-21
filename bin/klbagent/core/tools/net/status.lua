--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   status.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  net.status
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; 报告 ALUA_PROXY / ALUA_NO_PROXY
--]]


local util = require("klbagent.core.tools.net.util")


local M = {}


local function _status(_args)
	local has_klb = util.klbhttp_available()
	local has_curl = util.curl_available()
	local available = has_klb or has_curl
	local backend = ""
	if has_klb then
		backend = "klbhttp"
	elseif has_curl then
		backend = "curl"
	end

	local proxy = util.proxy_status()

	local message = "net.http_* unavailable (need klbhttp or curl)"
	if has_klb and has_curl then
		message = "net.http_get/http_post via klbhttp (default); curl fallback"
	elseif has_klb then
		message = "net.http_get/http_post via klbhttp (no curl)"
	elseif has_curl then
		message = "net.http_get/http_post via curl (klbhttp unavailable)"
	end

	if proxy.invalid then
		message = message .. "; ALUA_PROXY invalid"
	elseif proxy.configured then
		message = message .. "; proxy configured"
	end

	return {
		available = available,
		message = message,
		backend = backend,
		backends = {
			klbhttp = has_klb,
			curl = has_curl,
		},
		proxy = proxy,
		features = {
			http_get = available,
			http_post = available,
			cookie_jar = available,
			session = available,
			proxy = true,
		},
	}
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "net.status",
		category = "net",
		title = "net capability status",
		summary = "Whether HTTP tools are available (klbhttp default, curl fallback; ALUA_PROXY; profile net).",
		min_profile = "net",
		args_schema = {},
		examples = { { args = {} } },
		docs = "categories.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("net.status", _status)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
