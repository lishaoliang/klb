--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   ssid.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid owner 线程: 独立 klua_env; 持有 auth 表 + LPC/XPC (alua.ssid)
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构, 只要不破坏本文件导出签名与可观察语义
-- @note   kthread 入口: kthread.start("klbagent.core.serve.worker.ssid", false, -1); 加载本文件即执行
--   \n 同进程其它线程经 LPC alua.ssid; 本机其它进程经 XPC
--   \n _entry 只挂接; has_loop_c=false 时文件尾 klbenv.co_main_loop; 全局 G/kexit
--   \n XPC 等 listen 写 serve.port 后再绑 (消池口漂移); LPC 先就绪供同进程消费
-- @history 修改历史
--  \n [2026-11] 创建文件; 自 core.ssid.worker 迁入; 旧路径兼容转发已删除; 补 G/kexit
--  \n [2026-11] 改 require ssid.serve (owner 端面)
--  \n [2026-11] 去掉 serve/lpcer 进程类数据挂接
--  \n [2026-11] 启动前拼 unix path=/tmp/alua.ssid.xpc.unix.pxxxx
--  \n [2026-11] M1: 等 listen 落口后再 ensure_xpc
--]]
local ksys = require("ksys")
local kco = require("kco")
local ktime = require("ktime")
local klbenv = require("klbcore.klbenv")
local ssid = require("klbagent.core.ssid.serve")
local xpcpath = require("klbagent.core.ssid.base.xpcpath")
local port_mod = require("klbagent.core.serve.port")


-- 等 serve.port 上限 (毫秒); 步进
local WAIT_PORT_MS = 30000
local WAIT_STEP_MS = 50


-- @brief 等 listen 落盘口 (worker.start 已 clear; 匹配 opts.ports 若有)
-- @param [in] opts[table]
-- @return port[number|nil]
local function _wait_listen_port(opts)
	local cand = nil
	if type(opts.ports) == "table" and #opts.ports > 0 then
		cand = {}
		for i = 1, #opts.ports do
			local n = tonumber(opts.ports[i])
			if type(n) == "number" then
				cand[n] = true
			end
		end
	end

	local t0 = ktime.tick_count()
	while (ktime.tick_count() - t0) < WAIT_PORT_MS do
		local p = port_mod.read_listen_port()
		if type(p) == "number" then
			if cand == nil or cand[p] == true then
				return p
			end
		end

		kco.co_sleep(WAIT_STEP_MS)
	end

	return nil
end


-- @brief 入口: 本 env 作 ssid owner (serve 面 + LPC/XPC)
-- @param [in] opts[table|nil]			透传 ensure_xpc (transport/path/port 等)
-- @return code[number]				0 成功
local function _entry(opts)
	if "table" ~= type(opts) then
		opts = {}
	end

	local ok_l, err_l = ssid.ensure_lpc()
	if not ok_l then
		io.stderr:write("alua serve ssid: ensure_lpc fail: " .. tostring(err_l) .. "\n")
	else
		io.stderr:write("alua serve ssid: lpc ready; wait listen port for xpc\n")
	end

	-- 等 listen 写 serve.port, 再按实口拼 path (禁用候选首口预绑)
	local port_real = _wait_listen_port(opts)
	if type(port_real) ~= "number" then
		io.stderr:write("alua serve ssid: wait listen port timeout\n")
		io.stderr:write(string.format(
			"alua serve ssid: not ready lpc=%s xpc=fail\n",
			ok_l and "ok" or "fail"))
		return 1
	end

	opts.port = port_real
	opts.path = xpcpath.make(port_real)
	if type(opts.path) ~= "string" or opts.path == "" then
		io.stderr:write("alua serve ssid: xpc path make fail port="
			.. tostring(port_real) .. "\n")
		return 1
	end

	local ok_x, err_x = ssid.ensure_xpc(opts)
	if not ok_x then
		io.stderr:write("alua serve ssid: ensure_xpc fail: " .. tostring(err_x) .. "\n")
	end

	if ok_l and ok_x then
		io.stderr:write("alua serve ssid: ready (lpc/xpc owner) path="
			.. tostring(opts.path) .. " port=" .. tostring(port_real) .. "\n")
		return 0
	end

	io.stderr:write(string.format(
		"alua serve ssid: not ready lpc=%s xpc=%s\n",
		ok_l and "ok" or "fail",
		ok_x and "ok" or "fail"))
	return 1
end

local function _first_run()
	local arg_opts = ksys.get_args()
	if "table" ~= type(arg_opts) then
		arg_opts = nil
	end

	local ok_run, err_run = pcall(_entry, arg_opts)
	if not ok_run then
		io.stderr:write("alua.serve ssid: " .. tostring(err_run) .. "\n")
	end
end

---------------------------------------------------------------------------------------------
--- 线程入口执行 (kthread 加载本文件即在此 env 内运行)

klbenv.set_first_run(_first_run)
klbenv.co_main_loop()

-- end
