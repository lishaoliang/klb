--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   serve.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua status serve — 主服务端口 / 探活 / TLS 面
-- @note   复用 overview 结果; 禁 token 真值
-- @history 修改历史
--  \n [2026-10] 创建文件 (由 status.lua 拆分)
--]]


local version = require("klbagent.version")
local common = require("klbagent.core.cli.status.common")


local M = {}


-- @brief 取 overview (init 提供; 延迟 require 避免循环)
-- @param [in] opts[table|nil]
-- @return overview[table]
local function _overview(opts)
	local ok, mod = pcall(require, "klbagent.core.cli.status")
	if ok and type(mod) == "table" and type(mod.collect) == "function" then
		return mod.collect(opts)
	end

	return {}
end


-- @brief serve 模块详情
-- @param [in] path_rest[table]		未用 (模块无下级)
-- @param [in] opts[table|nil]
-- @return report[table]
M.collect = function (path_rest, opts)
	local overview = _overview(opts)
	local sv = overview.serve or {}
	local tls_face = nil
	do
		local ok_m, mod = pcall(require, "klbagent.core.config.serve")
		if ok_m and type(mod) == "table" and type(mod.load) == "function" then
			local cfg = mod.load()
			if type(cfg) == "table" then
				tls_face = cfg.tls_face
			end
		end
	end

	return {
		ok = true,
		command = "status",
		schema = common.SCHEMA,
		module = "serve",
		level = "root",
		path = { "serve" },
		version = version.string,
		serve = {
			port = sv.port,
			port_file = sv.port_file,
			port_default = sv.port_default,
			listening = sv.listening == true,
			listen_probe = sv.listen_probe,
			health = sv.health,
			transport_capable = sv.transport_capable,
			tls_face = tls_face,
		},
		token = overview.token,
		profile = overview.profile,
		files = {
			serve_env = overview.files and overview.files.serve_env,
			serve_port = overview.files and overview.files.serve_port,
			serve_crt = overview.files and overview.files.serve_crt,
			serve_key = overview.files and overview.files.serve_key,
			serve_json = overview.files and overview.files.serve_json,
		},
	}
end


-- @brief 打印 serve 模块详情
-- @param [in] report[table]
M.print_text = function (report)
	if report.ok == false then
		common.print_kv("error", report.message or "fail")
		return
	end

	local sv = report.serve or {}
	local health = sv.health or {}
	common.print_kv("listening", common.yn(sv.listening == true))
	common.print_kv("port", string.format("%s  file=%s  default=%s",
		tostring(sv.port or "?"),
		tostring(sv.port_file or "(none)"),
		tostring(sv.port_default or "")))
	common.print_kv("probe", tostring(sv.listen_probe or ""))
	common.print_kv("tls_face", tostring(sv.tls_face or "(default)"))
	if health.ok == true then
		local b = health.body or {}
		common.print_kv("health", string.format("ok  version=%s profile=%s",
			tostring(b.version or ""), tostring(b.profile or "")))
	else
		common.print_kv("health", tostring(health.err or "fail"))
	end

	local tok = report.token or {}
	common.print_kv("token", string.format("%s (source=%s)",
		common.yn(tok.set), tostring(tok.source)))
	common.print_kv("profile", tostring(report.profile or ""))
end


return M

-- end
