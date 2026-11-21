--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   http.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  短链接消费线程 (http/ws): kmuxlist.pop LIST_HTTP → klbweb handle
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl evolving: 内部可重构, 只要不破坏本文件导出签名与可观察语义
-- @note   kthread 入口: kthread.start("klbagent.core.serve.worker.http", false, -1); 加载本文件即执行
--   \n 裸 sock → khttp.new_serve → httpserver → klbweb app:handle (含 /v1/ws Upgrade)
--   \n _entry 挂接/fork 后 klbenv.co_main_loop; 消费在 kco.fork 内 co_sleep 让出
--   \n has_loop_c=false 脚本泵; true 时 co_main_loop 立即返回由 C 泵
-- @history 修改历史
--  \n [2026-11] 创建文件 (桩: 到消费 sock); P2 接线 new_serve + klbweb; 补 G/kexit
--]]
local ksys = require("ksys")
local kenv = require("kenv")
local kco = require("kco")
local kmuxlist = require("kmuxlist")
local klbenv = require("klbcore.klbenv")
local log = require("klbagent.core.log")
local worker = require("klbagent.core.serve.worker")
local wboot = require("klbagent.core.serve.base.wboot")
local open_sock = require("klbagent.core.serve.base.open_sock")
local http_routes = require("klbagent.core.serve.http")


local IDLE_MS = 10


-- @brief 消费循环: LIST_HTTP pop → new_serve → app:handle
-- @note 须在 kco 协程内
-- @param [in] app[table]
-- @param [in] serve_opts[table|nil]
local function _co_consume(app, serve_opts)
	local path = worker.LIST_HTTP

	while not kenv.is_exit() do
		local sock = kmuxlist.pop(path)
		if sock ~= nil then
			local sess, err = open_sock.open_http(sock, serve_opts)
			if sess == nil then
				-- sock 已转交 new_serve: 失败由其收尾 (关 fd + free 节点); 勿再 drop
				log.write("warn", "serve http open fail", {
					module = "serve",
					err = tostring(err),
				})
			elseif not app:handle(sess) then
				log.write("warn", "serve http handle fail", { module = "serve" })
			end
		else
			kco.co_sleep(IDLE_MS)
		end
	end
end


-- @brief 入口: 本 env boot + klbweb + fork 消费
-- @param [in] opts[table|nil]			cert/key/tls_info/profile
-- @return code[number]				0 成功; 1 失败
local function _entry(opts)
	local ok_b, err_b = wboot.ensure(opts)
	if not ok_b then
		io.stderr:write("alua serve http: boot fail: " .. tostring(err_b) .. "\n")
		return 1
	end

	local ok_web, klbweb = pcall(require, "klbcore.klbweb")
	if not ok_web or type(klbweb) ~= "table" or type(klbweb.new) ~= "function" then
		io.stderr:write("alua serve http: need klbcore.klbweb\n")
		return 1
	end

	local app = klbweb.new({
		server = "alua",
		log = false,
		csrf = false,
		session = false,
		healthz = false,
		max_body = 4 * 1024 * 1024,
	})
	http_routes.mount_v1_routes(app)

	local serve_opts = open_sock.tls_serve_opts(opts)
	io.stderr:write("alua serve http: consume " .. worker.LIST_HTTP .. "\n")
	kco.fork(function ()
		_co_consume(app, serve_opts)
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
		io.stderr:write("alua.serve http: " .. tostring(err_run) .. "\n")
	end
end

---------------------------------------------------------------------------------------------
--- 线程入口执行 (kthread 加载本文件即在此 env 内运行)

klbenv.set_first_run(_first_run)
klbenv.co_main_loop()

-- end
