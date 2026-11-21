--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rpc.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  RPC 消费线程 (smprpc/mnprpc): kmuxlist.pop → rpc.co_rpc_auth_and_loop
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构, 只要不破坏本文件导出签名与可观察语义
-- @note   kthread 入口: kthread.start("klbagent.core.serve.worker.rpc", false, -1); 加载本文件即执行
--   \n LIST_RPC 单队; pop 后 klisten.sock_protocol 区分 smprpc/mnprpc
--   \n _entry 只挂接/fork 消费; has_loop_c=false 时入口后 klbenv.co_main_loop; 消费在 kco.fork 内 co_sleep 让出
-- @history 修改历史
--  \n [2026-11] 创建文件 (桩: 到消费 sock); P2 接线 new_serve_rpc + mux.rpc; RPC 单队; 补 G/kexit
--]]
local ksys = require("ksys")
local kenv = require("kenv")
local kco = require("kco")
local klisten = require("klisten")
local kmuxlist = require("kmuxlist")
local klbenv = require("klbcore.klbenv")
local log = require("klbagent.core.log")
local worker = require("klbagent.core.serve.worker")
local wboot = require("klbagent.core.serve.base.wboot")
local open_sock = require("klbagent.core.serve.base.open_sock")
local mux_rpc = require("klbagent.core.serve.mux.rpc")


local IDLE_MS = 10


-- @brief 从 RPC 单队取一只 sock, 再按探测协议分面
-- @return sock[lightuserdata|nil]
-- @return proto[string|nil]				smprpc|mnprpc
local function _pop_one()
	local sock = kmuxlist.pop(worker.LIST_RPC)
	if sock == nil then
		return nil, nil
	end

	local proto = klisten.sock_protocol(sock)
	if proto == "smprpc" or proto == "mnprpc" then
		return sock, proto
	end

	klisten.drop(sock)
	log.write("warn", "serve rpc unknown proto", {
		module = "serve",
		proto = tostring(proto),
	})
	return nil, nil
end


-- @brief 消费循环
-- @note 须在 kco 协程内
-- @param [in] serve_opts[table|nil]
local function _co_consume(serve_opts)
	while not kenv.is_exit() do
		local sock, proto = _pop_one()
		if sock ~= nil then
			local open_fn = open_sock.open_smp_rpc
			if proto == "mnprpc" then
				open_fn = open_sock.open_mnp_rpc
			end

			local sess, err = open_fn(sock, serve_opts)
			if sess == nil then
				-- sock 已转交 new_serve: 失败由其收尾 (关 fd + free 节点); 勿再 drop
				log.write("warn", "serve rpc open fail", {
					module = "serve",
					proto = tostring(proto),
					err = tostring(err),
				})
			else
				kco.fork(function ()
					mux_rpc.co_rpc_auth_and_loop(sess, proto)
				end)
			end
		else
			kco.co_sleep(IDLE_MS)
		end
	end
end


-- @brief 入口
-- @param [in] opts[table|nil]
-- @return code[number]
local function _entry(opts)
	local ok_b, err_b = wboot.ensure(opts)
	if not ok_b then
		io.stderr:write("alua serve rpc: boot fail: " .. tostring(err_b) .. "\n")
		return 1
	end

	local serve_opts = open_sock.tls_serve_opts(opts)
	io.stderr:write("alua serve rpc: consume " .. worker.LIST_RPC .. "\n")
	kco.fork(function ()
		_co_consume(serve_opts)
	end)
	return 0
end

local function _first_run()
	local arg_opts = ksys.get_args()
	if "table" ~= type(arg_opts) then
		arg_opts = nil
	end

	local ok_run, err_run = pcall(_entry, arg_opts)
	if not ok_run then
		io.stderr:write("alua.serve rpc: " .. tostring(err_run) .. "\n")
	end
end

---------------------------------------------------------------------------------------------
--- 线程入口执行 (kthread 加载本文件即在此 env 内运行)

klbenv.set_first_run(_first_run)
klbenv.co_main_loop()

-- end
