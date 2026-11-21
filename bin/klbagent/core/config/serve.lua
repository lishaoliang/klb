--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   serve.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 非密配置模块 (serve.json)
-- @note   tls_face 等; token 仍在 serve.env; env ALUA_SERVE_* 可覆盖
-- @history 修改历史
--  \n [2026-10] 创建文件 (M1 pref JSON); set_live / load live 优先
--]]
local store = require("klbagent.core.config.store")


local M = {}


M.FILE = "serve.json"


local function _norm_tls_face(v, def)
	if type(v) ~= "string" or v == "" then
		return def
	end

	local low = string.lower(v)
	if low == "mnp" or low == "mnps" then
		return "mnp"
	end

	if low == "smp" or low == "smps" then
		return "smp"
	end

	if low == "http" or low == "https" or low == "1" or low == "true" then
		return "http"
	end

	return def
end


-- @brief 缺省
-- @return cfg[table]
M.defaults = function ()
	return {
		tls_face = "mnp",
	}
end


-- @brief 规范: 只保留已知键
-- @param [in] cfg[table]
-- @return cfg[table]
M.normalize = function (cfg)
	local def = M.defaults()
	if type(cfg) ~= "table" then
		return def
	end

	return {
		tls_face = _norm_tls_face(cfg.tls_face, def.tls_face),
	}
end


-- @brief 配置文件路径
-- @return path[string]
M.path = function ()
	return store.path(M.FILE)
end


local _live = nil


-- @brief 设置进程内 serve 配置 (不写盘)
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


-- @brief 读取 serve.json (live 优先)
-- @return cfg[table]
M.load = function ()
	if _live ~= nil then
		return _live
	end

	return store.load(M)
end


-- @brief 写入 serve.json
-- @param [in] cfg[table]
-- @return ok[boolean]
M.save = function (cfg)
	return store.save(M, cfg)
end


return M
