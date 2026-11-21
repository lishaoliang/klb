--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   unit.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua status unit — 常驻单元 (systemd / task_scheduler / init.d)
-- @note   与 host_kind 联动; 无单元时为 none
-- @history 修改历史
--  \n [2026-10] 创建文件 (由 status.lua 拆分); 嵌入式 init.d 单元 (S99_alua)
--]]


local version = require("klbagent.version")
local hostinfo = require("klbagent.core.hostinfo")
local common = require("klbagent.core.cli.status.common")


local M = {}


-- @brief unit 模块详情
-- @param [in] path_rest[table]		未用 (模块无下级)
-- @param [in] opts[table|nil]		未用
-- @return report[table]
M.collect = function (path_rest, opts)
	local hi = hostinfo.probe()
	local unit = nil
	if hi.host_kind == "windows" then
		unit = common.unit_windows()
	elseif hi.host_kind == "embedded" then
		unit = common.unit_initd()
	elseif hi.host_kind == "linux" or hi.host_kind == "wsl" then
		unit = common.unit_linux()
	end

	return {
		ok = true,
		command = "status",
		schema = common.SCHEMA,
		module = "unit",
		level = "root",
		path = { "unit" },
		version = version.string,
		host_kind = hi.host_kind,
		unit = unit,
	}
end


-- @brief 打印 unit 模块详情
-- @param [in] report[table]
M.print_text = function (report)
	if report.ok == false then
		common.print_kv("error", report.message or "fail")
		return
	end

	local unit = report.unit
	common.print_kv("host_kind", tostring(report.host_kind or ""))
	if type(unit) ~= "table" then
		common.print_kv("unit", "(none)")
		return
	end

	if unit.kind == "systemd" then
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
	else
		common.print_kv("unit", tostring(unit.kind or "?"))
	end
end


return M

-- end
