--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   bench.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy.bench 工具 (全链路分阶段测速)
-- @history 修改历史
--  \n [2026-10] 创建文件 (M1 / Todo.2026-44)
--]]
local bench = require("klbagent.packages.proxy.bench")


local M = {}


local function _run(args)
	return bench.run(args or {})
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "proxy.bench",
		category = "net",
		title = "proxy full-path latency bench",
		summary = "Multi-round hop open with phases (resolve/connect/auth/pool/hello_connect/first_byte/total). Uses config default chain or args.hops/proxy. Optional compare_direct. Returns JSON + text.",
		min_profile = "net",
		args_schema = {
			host = "string target host (or target.host)",
			port = "number optional target port default 80",
			target = "table optional {host,port}",
			hops = "table optional hop array",
			proxy = "string|table optional single-hop shorthand",
			rounds = "number optional default 3 max 32",
			timeout_ms = "number optional per-round hop timeout",
			compare_direct = "boolean optional TCP direct baseline",
			text = "boolean optional include text report (default true)",
		},
		examples = {
			{ args = { host = "1.1.1.1", port = 443, rounds = 3 } },
			{ args = { host = "example.com", port = 80, compare_direct = true } },
			{ args = {
				host = "127.0.0.1",
				port = 9,
				hops = { { type = "mnps", host = "127.0.0.1", port = 12817 } },
				rounds = 2,
			} },
		},
		docs = "tools/proxy.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("proxy.bench", _run)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
