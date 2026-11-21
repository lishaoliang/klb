--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   use.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy.use — 临时切链 (不写 proxy.json; 入口重启回 default)
-- @history 修改历史
--  \n [2026-10] 创建文件
--]]


local M = {}


local function _use(args)
	args = args or {}
	local chain = args.chain
	if chain == nil then
		chain = "default"
	end
	if type(chain) ~= "string" then
		error("chain must be string (default|debug|<id>)")
	end

	local px = require("klbagent.packages.proxy.config")
	local cproxy = require("klbagent.core.config.proxy")
	local cfg = cproxy.load()

	local ok, err, active = px.use(cfg, chain)
	if not ok then
		error(tostring(err or "proxy.use fail"))
	end

	return {
		ok = true,
		chain = chain,
		active = active,
		use_id = px.use_id(),
		default = cfg.default,
		debug = cfg.debug,
		note = "temporary; proxy entry restart clears to default",
	}
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "proxy.use",
		category = "net",
		title = "temporarily select proxy chain",
		summary = "Switch effective chain for running hops (default|debug|chain id). Writes proxy_use.json only; not proxy.json. Proxy entry start clears back to default.",
		min_profile = "net",
		args_schema = {
			chain = "string optional default|debug|<6-char id> (default=default)",
		},
		examples = {
			{ args = { chain = "debug" } },
			{ args = { chain = "default" } },
		},
		docs = "tools/proxy.md",
		side_effects = { "write_pref" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("proxy.use", _use)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
