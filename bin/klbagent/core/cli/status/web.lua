--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   web.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua status web — 配置页常驻单元 / 端口组 / 探活
-- @note   单元句柄随 host_kind: systemd / init.d / Task Scheduler; 禁 token 真值
--   \n 端口探活按 web.json 的 addr + 各端口项; 不解密 TLS, 仅测 TCP 可连
-- @history 修改历史
--  \n [2026-10] 创建文件 (M2: status 分派补 web)
--]]
local version = require("klbagent.version")
local hostinfo = require("klbagent.core.hostinfo")
local config = require("klbagent.core.config")
local web_acl = require("klbagent.core.config.web_acl")
local common = require("klbagent.core.cli.status.common")
local setup_common = require("klbagent.core.cli.setup.common")


local M = {}


-- @brief 端口组人读串 (8080:http+https,8443:https)
-- @param [in] listens[table]
-- @return text[string]
local function _listens_text(listens)
	local out = {}
	if type(listens) ~= "table" then
		return ""
	end

	for i = 1, #listens do
		local item = listens[i]
		local proto = "http"
		if item.tls and item.plain then
			proto = "http+https"
		elseif item.tls then
			proto = "https"
		end

		out[#out + 1] = tostring(item.port) .. ":" .. proto
	end

	return table.concat(out, ",")
end


-- @brief 黑白名单人读串 (default=.. allow=.. deny=.. [state=off])
-- @param [in] acl[table]
-- @return text[string]
local function _acl_text(acl)
	if type(acl) ~= "table" then
		return ""
	end

	local allow = acl.allow or {}
	local deny = acl.deny or {}
	local parts = {
		"default=" .. tostring(acl.default),
		"allow=" .. ((0 < #allow) and table.concat(allow, ",") or "-"),
		"deny=" .. ((0 < #deny) and table.concat(deny, ",") or "-"),
	}

	if not web_acl.enabled(acl) then
		parts[#parts + 1] = "state=off"
	end

	return table.concat(parts, " ")
end


-- @brief 取常驻单元 (按 host_kind)
-- @param [in] host_kind[string]
-- @return unit[table|nil]
local function _unit(host_kind)
	if host_kind == "windows" then
		return common.unit_windows(setup_common.WEB_TASK)
	elseif host_kind == "embedded" then
		return common.unit_initd("S99_alua_web")
	elseif host_kind == "linux" or host_kind == "wsl" then
		return common.unit_linux(setup_common.WEB_UNIT)
	end

	return nil
end


-- @brief web 模块详情
-- @param [in] path_rest[table]		未用 (模块无下级)
-- @param [in] opts[table|nil]		未用
-- @return report[table]
M.collect = function (path_rest, opts)
	local hi = hostinfo.probe()
	local cfg = config.web.normalize(config.web.load())

	local ports = {}
	for i = 1, #cfg.listen do
		local item = cfg.listen[i]
		local ok = common.tcp_port_open(cfg.addr, item.port)

		ports[#ports + 1] = {
			port = item.port,
			tls = item.tls == true,
			plain = item.plain == true,
			listening = ok == true,
		}
	end

	return {
		ok = true,
		command = "status",
		schema = common.SCHEMA,
		module = "web",
		level = "root",
		path = { "web" },
		version = version.string,
		host_kind = hi.host_kind,
		addr = cfg.addr,
		config_path = config.web.path(),
		listen = cfg.listen,
		listens_text = _listens_text(cfg.listen),
		acl = cfg.acl,
		acl_text = _acl_text(cfg.acl),
		ports = ports,
		unit = _unit(hi.host_kind),
	}
end


-- @brief 打印 web 模块详情
-- @param [in] report[table]
M.print_text = function (report)
	if report.ok == false then
		common.print_kv("error", report.message or "fail")
		return
	end

	common.print_kv("host_kind", tostring(report.host_kind or ""))
	common.print_kv("addr", tostring(report.addr or ""))
	common.print_kv("listen", tostring(report.listens_text or ""))
	common.print_kv("acl", tostring(report.acl_text or ""))

	local unit = report.unit
	if type(unit) ~= "table" then
		common.print_kv("unit", "(none)")
	elseif unit.kind == "systemd" then
		common.print_kv("unit", string.format("%s active=%s enabled=%s scope=%s",
			tostring(unit.name), tostring(unit.active),
			tostring(unit.enabled), tostring(unit.scope)))
	elseif unit.kind == "task_scheduler" then
		common.print_kv("unit", string.format("%s state=%s",
			tostring(unit.name), tostring(unit.state or "unknown")))
	elseif unit.kind == "initd" then
		common.print_kv("unit", string.format("%s active=%s enabled=%s managed=%s path=%s",
			tostring(unit.name), tostring(unit.active),
			tostring(unit.enabled), tostring(unit.managed), tostring(unit.path)))
	end

	local ports = report.ports or {}
	for i = 1, #ports do
		local one = ports[i]
		local proto = "http"
		if one.tls and one.plain then
			proto = "http+https"
		elseif one.tls then
			proto = "https"
		end

		common.print_kv("  port", string.format("%s:%s listening=%s",
			tostring(one.port), proto, common.yn(one.listening == true)))
	end
end


return M

-- end
