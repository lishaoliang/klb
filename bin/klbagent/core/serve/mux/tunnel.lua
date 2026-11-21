--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   tunnel.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve/mux 子模块: 已鉴权会话上的 BINARY alua-tunnel 出口
-- @note   与 proxy hop type=mnps 对齐; 移交后 mux 循环结束 (disconnect 由出口负责)
--   \n @stability internal: 实现文件, 勿 require 子路径
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 _mux_alua_tunnel_exit
--]]
local log = require("klbagent.core.log")


local M = {}


-- @brief 已鉴权会话上 BINARY alua-tunnel → 代理出口 (与 proxy hop type=mnps 对齐)
-- @note 须在 kco 协程内; 移交后本 mux 循环结束 (disconnect 由出口路径负责)
-- @param [in] serve[table]				会话
-- @param [in] proto[string]				mux 协议名 (mnp/mnps)
-- @param [in] prime_body[string|nil]	首帧载荷
-- @return handed[boolean]
M.co_tunnel_exit = function (serve, proto, prime_body)
	local ok_e, entry = pcall(require, "klbagent.packages.proxy.entry")
	if not ok_e or type(entry) ~= "table" or type(entry.co_mnps_exit_authed) ~= "function" then
		log.write("warn", "serve mux tunnel exit unavailable", {
			module = "serve",
			proto = proto,
		})
		return false
	end

	-- 出口在本机直连目标; 勿套本机 proxy.json 链 (避免自跳递归)
	local cfg = nil
	local ok_c, pcfg = pcall(require, "klbagent.packages.proxy.config")
	if ok_c and type(pcfg) == "table" and type(pcfg.defaults) == "function" then
		cfg = pcfg.defaults()
	end

	local opts = nil
	if type(prime_body) == "string" and prime_body ~= "" then
		opts = { prime = prime_body }
	end

	local ok_t, err_t = entry.co_mnps_exit_authed(serve, cfg, opts)
	if not ok_t then
		log.write("warn", "serve mux tunnel exit fail", {
			module = "serve",
			proto = proto,
			err = tostring(err_t or "fail"),
		})
		return false
	end

	return true
end


return M

-- end
