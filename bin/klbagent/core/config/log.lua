--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   log.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua 日志配置模块 (log.json)
-- @note   level/to_file/file_level/remote_level/path/max_bytes/ring_n; 平台默认见 host_kind
-- @history 修改历史
--  \n [2026-09] 创建文件
--  \n [2026-10] 多出口等级与平台默认; P2 ring_n; set_live / load live 优先
--]]
local store = require("klbagent.core.config.store")


local M = {}


M.FILE = "log.json"


local PC_MAX_BYTES = 1048576
local EMBEDDED_MAX_BYTES = 65536
local PC_RING_N = 200
local EMBEDDED_RING_N = 50


local function _norm_level(v, def)
	if v == "error" or v == "warn" or v == "info" or v == "debug" then
		return v
	end

	return def
end


local function _host_kind()
	local ok, hi = pcall(require, "klbagent.core.hostinfo")
	if ok and type(hi) == "table" and type(hi.probe) == "function" then
		local info = hi.probe()
		if type(info) == "table" and type(info.host_kind) == "string" and info.host_kind ~= "" then
			return info.host_kind
		end
	end

	return "linux"
end


local function _platform_defaults(host_kind)
	if host_kind == "embedded" then
		return {
			level = "warn",
			to_file = false,
			file_level = "warn",
			remote_level = "warn",
			path = "",
			max_bytes = EMBEDDED_MAX_BYTES,
			ring_n = EMBEDDED_RING_N,
		}
	end

	return {
		level = "warn",
		to_file = false,
		file_level = "warn",
		remote_level = "info",
		path = "",
		max_bytes = PC_MAX_BYTES,
		ring_n = PC_RING_N,
	}
end


local function _norm_max_bytes(v, def)
	local n = tonumber(v)
	if n == nil then
		return def
	end

	n = math.floor(n)
	if n < 4096 then
		return 4096
	end

	if n > 67108864 then
		return 67108864
	end

	return n
end


local function _norm_ring_n(v, def)
	local n = tonumber(v)
	if n == nil then
		return def
	end

	n = math.floor(n)
	if n < 0 then
		return 0
	end

	if n > 2000 then
		return 2000
	end

	return n
end


local function _norm_path(v)
	if type(v) ~= "string" then
		return ""
	end

	if v == "" then
		return ""
	end

	return v
end


-- @brief 缺省 (按 host_kind)
-- @return cfg[table]
M.defaults = function ()
	return _platform_defaults(_host_kind())
end


-- @brief 规范: 只保留已知键; 错值/缺键用默认; 未知键丢弃
-- @param [in] cfg[table]
-- @return cfg[table]
M.normalize = function (cfg)
	local def = M.defaults()
	if type(cfg) ~= "table" then
		return def
	end

	local to_file = def.to_file
	if cfg.to_file == true then
		to_file = true
	elseif cfg.to_file == false then
		to_file = false
	end

	local level = _norm_level(cfg.level, def.level)
	local file_level = _norm_level(cfg.file_level, level)
	local remote_level = _norm_level(cfg.remote_level, def.remote_level)

	return {
		level = level,
		to_file = to_file,
		file_level = file_level,
		remote_level = remote_level,
		path = _norm_path(cfg.path),
		max_bytes = _norm_max_bytes(cfg.max_bytes, def.max_bytes),
		ring_n = _norm_ring_n(cfg.ring_n, def.ring_n),
	}
end


-- @brief 配置文件路径
-- @return path[string]
M.path = function ()
	return store.path(M.FILE)
end


-- 进程内覆盖 (config.set scope=once / permanent 热生效)
local _live = nil


-- @brief 设置进程内 log 配置 (不写盘)
-- @param [in] cfg[table]
-- @return cfg[table]
M.set_live = function (cfg)
	_live = M.normalize(cfg)
	return _live
end


-- @brief 清除进程内覆盖
M.clear_live = function ()
	_live = nil
end


-- @brief 是否有 live 覆盖
-- @return yes[boolean]
M.has_live = function ()
	return _live ~= nil
end


-- @brief 读取 log.json (live 优先)
-- @return cfg[table]
M.load = function ()
	if _live ~= nil then
		return _live
	end

	return store.load(M)
end


-- @brief 写入 log.json
-- @param [in] cfg[table]
-- @return ok[boolean]
M.save = function (cfg)
	return store.save(M, cfg)
end


return M
