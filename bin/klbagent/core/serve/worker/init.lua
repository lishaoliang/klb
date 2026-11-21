--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve/worker: 起停 ssid/listen/消费 kthread 入口 (独立 klua_env)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义; AI 禁止随意改本文件, 须用户点名
-- @note   主 env 调 M.start / M.stop; ssid owner + listen 占口入队; http/long/rpc 按 LIST_* 消费
--   \n 本目录除 init.lua 外均为 kthread 入口脚本 (线程源头)
--   \n 队列名见 LIST_*; 消费端 kmuxlist.pop → base.open_sock (new_serve|adopt)
--   \n 本目录仅 init + 线程入口; boot/open_sock 在 base/
-- @history 修改历史
--  \n [2026-11] 创建文件; 补 http/long/rpc 三消费线程起停; P2 消费接线; RPC 单队
--  \n [2026-11] boot/open_sock 迁 base; 目录仅留线程源头
--  \n [2026-11] 接入 serve/lpcer (M.lpcer); 后删除 lpcer
--  \n [2026-11] M1: 起线程前 clear serve.port; ssid XPC 等 listen 落口
--]]
local kthread = require("kthread")
local port_mod = require("klbagent.core.serve.port")


local M = {}


-- listen 线程名 (kthread.start 返回值); nil=未起
M._listen_thread = nil

-- ssid owner 线程名; nil=未起
M._ssid_thread = nil

-- 消费线程名: 短链接 / 长连接 / rpc; nil=未起
M._http_thread = nil
M._long_thread = nil
M._rpc_thread = nil

-- 跨线程 kmuxlist 队列 path (listen 线程写入; 消费端本 env pop)
--   \n RPC 单队: sock 内含探测协议; 消费端 klisten.sock_protocol 再分 smprpc/mnprpc
M.LIST_HTTP = "/klbagent/http"
M.LIST_SMP = "/klbagent/smp"
M.LIST_MNP = "/klbagent/mnp"
M.LIST_RPC = "/klbagent/rpc"

-- kthread 入口模块名 (require 语义, 不带 .lua 后缀; 后缀会被解析成 xxx/lua 路径)
local SSID_ENTRY = "klbagent.core.serve.worker.ssid"
local LISTEN_ENTRY = "klbagent.core.serve.worker.listen"
local HTTP_ENTRY = "klbagent.core.serve.worker.http"
local LONG_ENTRY = "klbagent.core.serve.worker.long"
local RPC_ENTRY = "klbagent.core.serve.worker.rpc"


-- @brief 是否已起 listen 线程
-- @return running[boolean]
M.is_running = function ()
	return type(M._listen_thread) == "string" and M._listen_thread ~= ""
end


-- @brief 停一只 kthread (幂等); 清空字段
-- @param [in] field[string]			M 上线程名字段
local function _stop_one(field)
	local name = M[field]
	M[field] = nil
	if type(name) ~= "string" or name == "" then
		return
	end

	pcall(kthread.stop, name)
end


-- @brief 起一只消费 kthread; 失败不改字段
-- @param [in] entry[string]
-- @param [in] opts[table]
-- @return name[string|nil]
-- @return err[string|nil]
local function _start_one(entry, opts)
	local name = kthread.start(entry, false, -1, opts)
	if type(name) ~= "string" or name == "" then
		return nil, "kthread.start fail: " .. entry
	end

	return name, nil
end


-- @brief 起 ssid + listen + http/long/rpc 消费线程 (独立 env)
-- @param [in] opts[table|nil]			透传给各线程: port / host / schemes / max_connect /
--   \n   cert / key / tls / tls_info / tls_face; ssid 另透传 ensure_xpc 项
-- @return ok[boolean]
-- @return err[string|nil]
M.start = function (opts)
	if M.is_running() then
		return false, "already started"
	end

	if type(opts) ~= "table" then
		opts = {}
	end

	-- 清脏口, 供 ssid 等 listen 写入的实口再绑 XPC
	port_mod.clear_listen_port()

	-- 先起 ssid owner (LPC 先; XPC 等落口), 再起消费端, 最后 listen 占口
	local ssid_name, err_s = _start_one(SSID_ENTRY, opts)
	if ssid_name == nil then
		return false, err_s
	end

	local http_name, err_h = _start_one(HTTP_ENTRY, opts)
	if http_name == nil then
		pcall(kthread.stop, ssid_name)
		return false, err_h
	end

	local long_name, err_l = _start_one(LONG_ENTRY, opts)
	if long_name == nil then
		pcall(kthread.stop, http_name)
		pcall(kthread.stop, ssid_name)
		return false, err_l
	end

	local rpc_name, err_r = _start_one(RPC_ENTRY, opts)
	if rpc_name == nil then
		pcall(kthread.stop, long_name)
		pcall(kthread.stop, http_name)
		pcall(kthread.stop, ssid_name)
		return false, err_r
	end

	local listen_name = kthread.start(LISTEN_ENTRY, false, -1, opts)
	if type(listen_name) ~= "string" or listen_name == "" then
		pcall(kthread.stop, rpc_name)
		pcall(kthread.stop, long_name)
		pcall(kthread.stop, http_name)
		pcall(kthread.stop, ssid_name)
		return false, "kthread.start fail"
	end

	M._ssid_thread = ssid_name
	M._http_thread = http_name
	M._long_thread = long_name
	M._rpc_thread = rpc_name
	M._listen_thread = listen_name
	return true
end


-- @brief 清空本 serve 用到的 kmuxlist 队列 (停线程前, 防未消费 sock / 残留)
local function _clear_lists()
	local ok_m, kmuxlist = pcall(require, "kmuxlist")
	if not ok_m or type(kmuxlist) ~= "table" or type(kmuxlist.new) ~= "function" then
		return
	end

	local paths = {
		M.LIST_HTTP,
		M.LIST_SMP,
		M.LIST_MNP,
		M.LIST_RPC,
	}

	for i = 1, #paths do
		local q = kmuxlist.new(paths[i], { max = 0 })
		if type(q) == "table" and type(q.clear) == "function" then
			pcall(q.clear)
		end
	end
end


-- @brief 停 listen + 三消费 + ssid (幂等)
-- @return ok[boolean]
M.stop = function ()
	-- 先停 listen (停 accept 入队), 再清队列, 再停消费, 最后 ssid owner
	_stop_one("_listen_thread")
	_clear_lists()
	_stop_one("_http_thread")
	_stop_one("_long_thread")
	_stop_one("_rpc_thread")
	_stop_one("_ssid_thread")
	return true
end


return M

-- end
