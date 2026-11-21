--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   health.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve/http 子模块: GET /v1/health (服务/主机/transport 宣告)
-- @note   token 失败时透出具体原因 (raw token requires tls / token expired), 便于客户端定位
--   \n @stability internal: 实现文件, 勿 require 子路径
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 _handle_health; worker 下落盘口回读
--]]
local version = require("klbagent.version")
local profiles = require("klbagent.core.profiles")
local hostinfo = require("klbagent.core.hostinfo")
local tls = require("klbagent.core.serve.security.tls")
local caps = require("klbagent.core.serve.base.caps")
local auth = require("klbagent.core.serve.security.auth")
local state = require("klbagent.core.serve.base.state")
local port = require("klbagent.core.serve.port")


local M = {}


-- @brief GET /v1/health: 校验 token 后返回服务与主机概况
-- @param [in] req[table]
-- @param [in] res[table]
M.handle_health = function (req, res)
	local headers = req and req.headers
	local tok = auth.token_from_headers(headers)
	local sid = auth.ssid_from_headers(headers)
	local info, err_v, err_code = auth.verify_token(tok, auth.tls_of(req), sid)
	if info == nil then
		-- 透出 UNAVAILABLE / raw+TLS / expired 等, 避免一律 bad token 误导 status
		auth.deny_http(res, err_v, err_code)
		return
	end

	local hi = hostinfo.probe()
	local tls_info = tls.resolve_tls()
	local tls_listen = type(tls_info) == "table" and tls_info.tls == true
	-- worker 正式面: listen 同口多协议; embed 非 mux 面
	local mux_on = (state.embed_app == nil)
	local transport = caps.transport_caps(tls_listen, mux_on, tls.tls_over_default())

	local service = "alua"
	if state.embed_app ~= nil and type(state.embed_service) == "string" and state.embed_service ~= "" then
		service = state.embed_service
	end

	-- 实际口以 pref/serve.port 为准; serve_opts.port 可能是候选首口
	local listen_port = port.read_listen_port()
	local port_mode = nil
	if type(state.serve_opts) == "table" then
		port_mode = state.serve_opts.port_mode
		if type(listen_port) ~= "number" then
			listen_port = state.serve_opts.port
		end
	end

	res:co_json({
		ok = true,
		service = service,
		version = version.string,
		profile = profiles.active(),
		host_kind = hi.host_kind,
		virt = hi.virt,
		arch = hi.arch,
		network = hi.network,
		caps = hi.caps,
		token_set = hi.token_set,
		tls_listen = tls_listen,
		plain_listen = true,
		transport = transport,
		mux = mux_on,
		port = listen_port,
		port_mode = port_mode,
	})
end


return M

-- end
