--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   embed.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 子模块: 宿主进程内嵌入 serve (非阻塞; 由宿主泵送)
-- @note   与前台 serve 共用路由与鉴权; 差异: 自行 app:serve bind, 不接 klblisten mux
--   \n 须宿主按 pump_embed 返回的毫秒数泵送 kenv.loop_once
--   \n @stability internal: 实现文件, 契约面见 core/serve/init.lua
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 start_embed / pump_embed / stop_embed / embed_status
--]]
local kenv = require("kenv")
local token = require("klbagent.core.serve.security.token")
local tls = require("klbagent.core.serve.security.tls")
local state = require("klbagent.core.serve.base.state")
local auth = require("klbagent.core.serve.security.auth")
local boot = require("klbagent.core.serve.base.boot")
local profile = require("klbagent.core.serve.security.profile")
local http = require("klbagent.core.serve.http")


local M = {}


-- g2de 等宿主嵌入 / 代理入口固定口 (勿进 serve 候选池)
M.EMBED_DEFAULT_PORT = 12818


-- @brief 构造带 listen 配置的 klbweb app (嵌入形态自行 bind)
-- @param [in] listen_port[number]
-- @param [in] tls_info[table]
-- @param [in] server_name[string|nil]
-- @return app[table|nil]
-- @return err[string|nil]
local function _new_serve_app(listen_port, tls_info, server_name)
	local ok_web, klbweb = pcall(require, "klbcore.klbweb")
	if not ok_web or type(klbweb) ~= "table" or type(klbweb.new) ~= "function" then
		return nil, "need klbcore.klbweb (klbhttp)"
	end

	local listen_cfg = listen_port
	if tls_info.tls then
		listen_cfg = {
			port = listen_port,
			tls = true,
			plain = true,
			cert = tls_info.cert,
			key = tls_info.key,
		}
	end

	local app = klbweb.new({
		server = server_name or "alua",
		listen = listen_cfg,
		log = false,
		csrf = false,
		session = false,
		healthz = false,
		max_body = 4 * 1024 * 1024,
	})

	http.mount_v1_routes(app)
	return app
end


-- @brief 宿主进程内启动非阻塞 serve (须 pump_embed 泵送 kenv.loop_once)
-- @param [in] opts[table|nil]			port / profile / token / service
-- @return ok[boolean]
-- @return err[string|nil]
M.start_embed = function (opts)
	opts = opts or {}

	if state.embed_app ~= nil then
		return true
	end

	local listen_port = tonumber(opts.port) or M.EMBED_DEFAULT_PORT
	if listen_port < 1 or 65535 < listen_port then
		return false, "bad port"
	end

	-- 同前台 serve: 宿主未预注册时自持注册, 否则 /v1/call 一律 TOOL_NOT_FOUND
	local ok_load, err_load = boot.ensure_tools()
	if not ok_load then
		return false, "load tools fail: " .. tostring(err_load)
	end

	if type(opts.token) == "string" and opts.token ~= "" then
		state.embed_token = opts.token
	end

	-- 嵌入式缺 token 时同样自动生成 16 位并落盘
	if auth.token_expect() == nil and token.autogen_embedded() == nil then
		return false, "ALUA_SERVE_TOKEN required"
	end

	auth.ensure_sweeper()

	local tls_info, err_tls = tls.resolve_tls()
	if tls_info == nil then
		tls_info = {
			tls = false,
			plain = true,
			cert = nil,
			key = nil,
		}
		if type(err_tls) == "string" and err_tls ~= "" then
			io.stderr:write("g2de embed serve: tls skip (" .. err_tls .. "), plain http only\n")
		end
	end

	if type(opts.profile) == "string" and opts.profile ~= "" then
		local ok_pf, err_pf = profile.apply_profile(opts.profile)
		if not ok_pf then
			state.embed_token = nil
			return false, tostring(err_pf or "set profile fail")
		end
	end

	local server_name = opts.server or "g2de-agent"
	local app, err_app = _new_serve_app(listen_port, tls_info, server_name)
	if app == nil then
		state.embed_token = nil
		return false, err_app
	end

	local ok_s = app:serve()
	if not ok_s then
		state.embed_token = nil
		return false, "bind fail port=" .. tostring(listen_port)
	end

	state.embed_app = app
	state.embed_service = opts.service or "g2de"
	state.embed_opts = {
		port = listen_port,
		profile = opts.profile,
	}

	if tls_info.tls then
		io.stderr:write(string.format(
			"g2de embed serve: listen https+http://0.0.0.0:%d  service=%s  POST /v1/call\n",
			listen_port, state.embed_service))
	else
		io.stderr:write(string.format(
			"g2de embed serve: listen http://0.0.0.0:%d  service=%s  POST /v1/call\n",
			listen_port, state.embed_service))
	end

	return true
end


-- @brief 泵送嵌入 serve 一次 (主线程; 与游戏 loop 同频)
-- @return ms[number]  kenv.loop_once 建议休眠毫秒
M.pump_embed = function ()
	if state.embed_app == nil then
		return 0
	end

	local ms = kenv.loop_once()
	if type(ms) ~= "number" then
		return 0
	end

	return ms
end


-- @brief 停止嵌入 serve
-- @return ok[boolean]
M.stop_embed = function ()
	if state.embed_app == nil then
		return true
	end

	if type(state.embed_app.stop) == "function" then
		state.embed_app:stop()
	end

	state.embed_app = nil
	state.embed_opts = nil
	state.embed_token = nil
	state.embed_service = "alua"
	return true
end


-- @brief 嵌入 serve 状态 (只读)
-- @return info[table]
M.embed_status = function ()
	if state.embed_app == nil then
		return {
			listening = false,
		}
	end

	return {
		listening = true,
		port = state.embed_opts and state.embed_opts.port,
		service = state.embed_service,
	}
end


return M

-- end
