--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbagent 配置门面 (分模块 JSON)
-- @note   org=klua app=alua; runtime/log/proxy/serve/web/peers; 错误与未知键用默认替代
-- @history 修改历史
--  \n [2026-09] 创建文件
--  \n [2026-10] proxy/serve/peers; web (端口组; 桩)
--]]
local store = require("klbagent.core.config.store")
local runtime = require("klbagent.core.config.runtime")
local log = require("klbagent.core.config.log")
local proxy = require("klbagent.core.config.proxy")
local serve = require("klbagent.core.config.serve")
local web = require("klbagent.core.config.web")
local peers = require("klbagent.core.config.peers")


local M = {}


M.ORG = store.ORG
M.APP = store.APP
M.runtime = runtime
M.log = log
M.proxy = proxy
M.serve = serve
M.web = web
M.peers = peers


-- @brief 配置根目录
-- @return dir[string]
M.dir = function ()
	return store.dir()
end


-- @brief 缺省全集
-- @return all[table]
M.defaults = function ()
	return {
		runtime = runtime.defaults(),
		log = log.defaults(),
		proxy = proxy.defaults(),
		serve = serve.defaults(),
		web = web.defaults(),
		peers = peers.defaults(),
	}
end


-- @brief 规范全集 (未知子表用该模块默认)
-- @param [in] all[table]
-- @return all[table]
M.normalize = function (all)
	if type(all) ~= "table" then
		return M.defaults()
	end

	return {
		runtime = runtime.normalize(all.runtime),
		log = log.normalize(all.log),
		proxy = proxy.normalize(all.proxy),
		serve = serve.normalize(all.serve),
		web = web.normalize(all.web),
		peers = peers.normalize(all.peers),
	}
end


-- @brief 加载全部分模块配置
-- @return all[table]
M.load = function ()
	return {
		runtime = runtime.load(),
		log = log.load(),
		proxy = proxy.load(),
		serve = serve.load(),
		web = web.load(),
		peers = peers.load(),
	}
end


-- @brief 保存全部分模块配置
-- @param [in] all[table]
-- @return ok[boolean]
M.save = function (all)
	if type(all) ~= "table" then
		all = M.defaults()
	end

	local ok_r = runtime.save(all.runtime)
	local ok_l = log.save(all.log)
	local ok_p = proxy.save(all.proxy)
	local ok_s = serve.save(all.serve)
	local ok_w = web.save(all.web)
	local ok_e = peers.save(all.peers)
	return ok_r and ok_l and ok_p and ok_s and ok_w and ok_e
end


-- @brief 确保 pref 根缺失 JSON 已落盘 (M1)
-- @return all[table]
M.ensure_pref_json = function ()
	return M.load()
end


return M
