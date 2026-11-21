--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   config.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua status config — pref 路径 / 配置文件是否存在 / runtime
-- @note   只出路径与存在性, 禁密钥; 脚本安装根随 status 目录化
-- @history 修改历史
--  \n [2026-10] 创建文件 (由 status.lua 拆分)
--]]


local version = require("klbagent.version")
local config = require("klbagent.core.config")
local profiles = require("klbagent.core.profiles")
local common = require("klbagent.core.cli.status.common")


local M = {}


-- @brief config 模块详情 (路径 + 文件是否存在; 禁密钥)
-- @param [in] path_rest[table]		未用 (模块无下级)
-- @param [in] opts[table|nil]		未用
-- @return report[table]
M.collect = function (path_rest, opts)
	local pref = config.dir() or ""
	pref = string.gsub(pref, "[/\\]+$", "")
	local paths = common.paths_info(pref)
	local files = {}
	local keys = {
		"serve_env", "serve_port", "serve_crt", "serve_key",
		"runtime_json", "log_json", "env_overlay_json",
		"proxy_json", "serve_json", "peers_json",
	}
	for i = 1, #keys do
		local k = keys[i]
		files[k] = common.file_exists(paths[k])
	end

	local rt = config.runtime.load()
	return {
		ok = true,
		command = "status",
		schema = common.SCHEMA,
		module = "config",
		level = "root",
		path = { "config" },
		version = version.string,
		pref = pref,
		paths = paths,
		files = files,
		runtime = {
			profile = (rt and rt.profile) or profiles.active(),
			io = (rt and rt.io) or "stdio",
		},
	}
end


-- @brief 打印 config 模块详情
-- @param [in] report[table]
M.print_text = function (report)
	if report.ok == false then
		common.print_kv("error", report.message or "fail")
		return
	end

	common.print_kv("pref", tostring(report.pref or ""))
	local rt = report.runtime or {}
	common.print_kv("runtime", string.format("profile=%s io=%s",
		tostring(rt.profile), tostring(rt.io)))
	local files = report.files or {}
	local names = {
		"runtime_json", "log_json", "proxy_json", "serve_json",
		"peers_json", "serve_env", "env_overlay_json",
	}
	for i = 1, #names do
		local k = names[i]
		common.print_kv(k, common.yn(files[k] == true))
	end
end


return M

-- end
