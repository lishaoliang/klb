--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   tls.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  runtime.tls_resign
-- @note   薄封装: 重签由 klbagent.core.serve.security.tls.resign_tls 完成
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local serve_tls = require("klbagent.core.serve.security.tls")


local M = {}


local function _tls_resign(args)
	args = args or {}
	local info, err = serve_tls.resign_tls({
		days = args.days,
		path_cert = args.path_cert,
		path_key = args.path_key,
		san = args.san,
		subject = args.subject,
		restart = args.restart == true,
		hot = args.hot == true,
	})
	if info == nil then
		error(tostring(err or "tls resign fail"))
	end

	return info
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "runtime.tls_resign",
		category = "runtime",
		title = "re-selfsign serve TLS cert",
		summary = "Regenerate self-signed cert/key via serve.security.tls; SAN includes local IPs; worker needs serve restart. Needs profile>=net.",
		min_profile = "net",
		args_schema = {
			days = "number optional validity days (default 3650)",
			path_cert = "string optional cert path (default pref_path/serve.crt)",
			path_key = "string optional key path (default pref_path/serve.key)",
			san = "array optional extra SAN (DNS:… / IP:…); local IPv4 always added",
			hot = "boolean optional (worker: no live hot-reload; noted in result)",
			restart = "boolean optional (worker: not auto-scheduled; noted in result)",
		},
		examples = {
			{ args = {} },
			{ args = { hot = true } },
			{ args = { days = 365, restart = true } },
			{ args = { san = { "IP:192.0.2.10" }, hot = true } },
		},
		docs = "tools/runtime.md",
		side_effects = { "write_fs" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("runtime.tls_resign", _tls_resign)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
