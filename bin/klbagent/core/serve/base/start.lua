--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   start.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 主 env 启动前置: argv → token/TLS/profile → worker opts
-- @stability internal
-- @impl evolving
-- @note   供 serve.entry 调用; 勿作对外契约面
-- @history 修改历史
--  \n [2026-11] 自 serve/init.lua 迁出 prepare (init 仅留 entry+起 worker)
--]]
local codes = require("klbagent.code")
local argv_mod = require("klbagent.core.serve.base.argv")
local tls = require("klbagent.core.serve.security.tls")
local token = require("klbagent.core.serve.security.token")
local auth = require("klbagent.core.serve.security.auth")
local profile = require("klbagent.core.serve.security.profile")
local port = require("klbagent.core.serve.port")
local state = require("klbagent.core.serve.base.state")


local M = {}


-- @brief 组装 listen 候选口 (显式单口; 否则池 + 随机段)
-- @param [in] opts[table]				parse_argv 结果
-- @return ports[table]
-- @return port_mode[string]
local function _candidate_ports(opts)
	if opts.port_explicit then
		return { opts.port }, "explicit"
	end

	local ports = port.port_pool()
	do
		local t = os.time()
		local clock = os.clock()
		math.randomseed(t + math.floor((clock % 1) * 1000000))
	end

	local tried = {}
	for i = 1, #ports do
		tried[ports[i]] = true
	end

	local lo = port.RANDOM_PORT_MIN
	local hi = port.RANDOM_PORT_MAX
	local n_try = port.RANDOM_PORT_TRIES
	local guard = 0
	while n_try > 0 and guard < 64 do
		guard = guard + 1
		local p = math.random(lo, hi)
		if tried[p] ~= true then
			tried[p] = true
			ports[#ports + 1] = p
			n_try = n_try - 1
		end
	end

	return ports, "pool"
end


-- @brief 主 env 启动前置: token / TLS / profile; 组装 worker opts
-- @param [in] opts[table]				parse_argv 结果
-- @return wopts[table|nil]
-- @return code[number|nil]
-- @return err[string|nil]
local function _prepare(opts)
	local run_token = auth.token_expect()
	if run_token == nil then
		run_token = token.autogen_embedded()
	end

	if run_token == nil then
		return nil, codes.serve.TOKEN_REQUIRED, "ALUA_SERVE_TOKEN required"
	end

	auth.ensure_sweeper()

	local tls_info, err_tls = tls.resolve_tls()
	if tls_info == nil then
		return nil, codes.serve.TLS_RESOLVE_FAIL, tostring(err_tls or "tls resolve fail")
	end

	local ok_pf, err_pf = profile.apply_profile(opts.profile)
	if not ok_pf then
		return nil, codes.serve.PROFILE_FAIL, tostring(err_pf or "profile fail")
	end

	local ports, port_mode = _candidate_ports(opts)
	local wopts = {
		port = ports[1],
		ports = ports,
		port_mode = port_mode,
		profile = opts.profile,
		tls_info = tls_info,
		tls = (tls_info.tls == true),
	}

	if tls_info.tls == true then
		wopts.cert = tls_info.cert
		wopts.key = tls_info.key
	end

	state.serve_opts = {
		port = wopts.port,
		port_mode = port_mode,
		profile = opts.profile,
	}

	return wopts, nil, nil
end


-- @brief 自位置参数解析并组装 worker.start 入参
-- @param [in] ...						[port] [profile]
-- @return wopts[table|nil]
-- @return code[number|nil]
-- @return err[string|nil]
M.from_argv = function (...)
	local opts, err_p = argv_mod.parse_argv({ ... })
	if opts == nil then
		return nil, codes.serve.BAD_ARGV, tostring(err_p)
	end

	return _prepare(opts)
end


return M

-- end
