--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   proxy.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua 代理配置模块 (proxy.json)
-- @note   pref 默认落盘; 密钥勿明文; 链结构对齐 packages.proxy.config
-- @history 修改历史
--  \n [2026-10] 创建文件 (M1 pref JSON); set_live / load live 优先; 链 id 随机6位; migrate legacy chains.default; 保护 default 角色; prepare_save: 剥遗留 active_chain; 临时切链见 proxy.use
--]]
local cjson = require("cjson")
local store = require("klbagent.core.config.store")


local M = {}


M.FILE = "proxy.json"


local function _px()
	return require("klbagent.packages.proxy.config")
end


-- @brief 缺省 (无入口口令)
-- @return cfg[table]
M.defaults = function ()
	local d = _px().defaults()
	d.auth = { mode = "none" }
	return d
end


local function _strip_secrets(cfg)
	if type(cfg) ~= "table" then
		return cfg
	end

	if type(cfg.auth) == "table" then
		local auth = {
			mode = cfg.auth.mode or "none",
		}
		if type(cfg.auth.user) == "string" and cfg.auth.user ~= "" then
			auth.user = cfg.auth.user
		end
		-- 口令勿落 JSON; 可用 pass_ref 指向 env 名
		if type(cfg.auth.pass_ref) == "string" and cfg.auth.pass_ref ~= "" then
			auth.pass_ref = cfg.auth.pass_ref
		end
		cfg.auth = auth
	end

	return cfg
end


-- @brief 读盘原始表 (不 normalize; 供 protect / 判断 legacy)
-- @return [table|nil]
local function _disk_raw()
	local path = store.path(M.FILE)
	if type(path) ~= "string" or path == "" then
		return nil
	end

	local f = io.open(path, "rb")
	if f == nil then
		return nil
	end

	local raw = f:read("*a")
	f:close()
	if type(raw) ~= "string" or raw == "" then
		return nil
	end

	local ok, obj = pcall(cjson.decode, raw)
	if not ok or type(obj) ~= "table" then
		return nil
	end

	return obj
end


local function _is_legacy_default_key(raw)
	if type(raw) ~= "table" or type(raw.chains) ~= "table" then
		return false
	end

	local v = raw.chains["default"]
	return type(v) == "table" and #v > 0 and type(v[1]) == "table"
		and (v[1].type ~= nil or v[1].host ~= nil)
end


-- @brief 规范: migrate 角色链; 保护 default; validate; 剥口令
-- @param [in] cfg[table]
-- @return cfg[table]
M.normalize = function (cfg)
	local def = M.defaults()
	if type(cfg) ~= "table" then
		return def
	end

	local px = _px()
	local out = px.defaults()
	px.merge_overlay(out, cfg)
	out = select(1, px.migrate_legacy_chains(out))
	out = _strip_secrets(out)

	-- 已迁格式: 锁 default 角色指针与 hops (legacy 首迁跳过)
	local raw = _disk_raw()
	if type(raw) == "table" and type(raw.default) == "string" and raw.default ~= ""
		and not _is_legacy_default_key(raw) then
		out = px.protect_default_chain({
			default = raw.default,
			chains = raw.chains,
		}, out)
	end

	local ok = px.validate(out)
	if not ok then
		return def
	end

	out._path = nil
	return _strip_secrets(out)
end


-- @brief 写盘前形态 (store.save/load); 剥遗留 active_chain 字段
-- @param [in] cfg[table]
-- @return cfg[table]
M.prepare_save = function (cfg)
	return _px().for_disk(cfg)
end


-- @brief 配置文件路径
-- @return path[string]
M.path = function ()
	return store.path(M.FILE)
end


local _live = nil


-- @brief 设置进程内 proxy 配置 (不写盘)
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


-- @brief 读取 proxy.json (live 优先; 缺则写默认)
-- @return cfg[table]
M.load = function ()
	if _live ~= nil then
		return _live
	end

	return store.load(M)
end


-- @brief 写入 proxy.json
-- @param [in] cfg[table]
-- @return ok[boolean]
M.save = function (cfg)
	return store.save(M, cfg)
end


return M

-- end
