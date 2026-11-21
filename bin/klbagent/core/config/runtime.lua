--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   runtime.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua 运行时配置模块 (runtime.json)
-- @note   profile / io; 非法与未知键由 store 按默认替代
-- @history 修改历史
--  \n [2026-09] 创建文件
--  \n [2026-10] set_live / load live 优先; loop_sleep_ms (kenv.set_loop_sleep; 默认 1ms)
--]]
local store = require("klbagent.core.config.store")


local M = {}


M.FILE = "runtime.json"


local function _norm_profile(v, def)
	if v == "readonly" or v == "net" or v == "device" or v == "unsafe" then
		return v
	end

	return def
end


local function _norm_io(v, def)
	if v == "stdio" then
		return v
	end

	return def
end


-- @brief 规范 loop_sleep_ms (>=1; 非法回落默认)
local function _norm_loop_sleep_ms(v, def)
	local n = tonumber(v)
	if type(n) ~= "number" then
		return def
	end

	n = math.floor(n)
	if n < 1 then
		return def
	end

	return n
end


-- @brief 缺省
-- @return cfg[table]
M.defaults = function ()
	return {
		profile = "readonly",
		io = "stdio",
		-- alua 默认 5ms (降低空闲空转 CPU); 需低延迟可调 1; 嵌入式产品可改为 10
		loop_sleep_ms = 5,
	}
end


-- @brief 规范: 只保留已知键; 错值/缺键用默认; 未知键丢弃
-- @param [in] cfg[table]
-- @return cfg[table]
M.normalize = function (cfg)
	local def = M.defaults()
	if type(cfg) ~= "table" then
		return def
	end

	return {
		profile = _norm_profile(cfg.profile, def.profile),
		io = _norm_io(cfg.io, def.io),
		loop_sleep_ms = _norm_loop_sleep_ms(cfg.loop_sleep_ms, def.loop_sleep_ms),
	}
end


-- @brief 配置文件路径
-- @return path[string]
M.path = function ()
	return store.path(M.FILE)
end


local _live = nil


-- @brief 设置进程内 runtime 配置 (不写盘)
-- @param [in] cfg[table]
-- @return cfg[table]
M.set_live = function (cfg)
	_live = M.normalize(cfg)
	return _live
end


M.clear_live = function ()
	_live = nil
end


M.has_live = function ()
	return _live ~= nil
end


-- @brief 读取 runtime.json (live 优先; 兼容旧 alua.json 迁移)
-- @return cfg[table]
M.load = function ()
	if _live ~= nil then
		return _live
	end

	return store.load(M, "alua.json")
end


-- @brief 写入 runtime.json
-- @param [in] cfg[table]
-- @return ok[boolean]
M.save = function (cfg)
	return store.save(M, cfg)
end


return M
