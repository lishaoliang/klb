--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 包入口: 仅 entry → 起 worker
-- @stability stable
-- @impl evolving
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   正式 --serve 走 worker; argv/token/TLS 等前置见 base.start; 业务在子目录
--   \n 起 worker 后本入口 klbenv.co_main_loop; 子线程各自 co_main_loop (has_loop_c 由 C 写入); 包外勿直 require 子路径
--   \n schedule_restart: deploy / tls 装后异步重启本机 serve (proc/restart)
-- @history 修改历史
--  \n [2026-11] 创建文件
--]]
local kenv = require("kenv")
local klbenv = require("klbcore.klbenv")
local codes = require("klbagent.code")
local log = require("klbagent.core.log")
local worker = require("klbagent.core.serve.worker")
local start = require("klbagent.core.serve.base.start")
local restart = require("klbagent.core.serve.proc.restart")


local M = {}


-- 异步重启本机 alua serve (deploy.self_install restart=true 等)
M.schedule_restart = restart.schedule_restart



local function _first_run(wopts)
	-- 3. 开启 worker (ssid/listen/http/long/rpc)
	local ok_w, err_w = worker.start(wopts)
	if not ok_w then
		io.stderr:write("alua serve: worker start fail: " .. tostring(err_w) .. "\n")
		log.write("error", "serve worker start fail", {
			module = "serve",
			err = tostring(err_w),
		})
		return false, codes.serve.TOOL_FAILED
	end

	io.stderr:write(string.format(
		"alua serve: worker started profile=%s candidates=%d first_port=%s\n",
		tostring(wopts.profile),
		#(wopts.ports or {}),
		tostring(wopts.port)))
	return true, codes.OK
end


-- @brief serve 入口: 前置组装 opts → 起 worker 线程组
-- @param [in] ...						位置参数: [port] [profile]
-- @return ok[boolean]
-- @return code[number]
M.entry = function (...)
	-- 1. 解析参数 (argv → token/TLS/profile → worker opts)
	local wopts, code, err = start.from_argv(...)
	if wopts == nil then
		io.stderr:write("alua serve: " .. tostring(err) .. "\n")
		log.write("error", "serve prepare fail", {
			module = "serve",
			err = tostring(err),
			code = code,
		})
		return false, code or codes.serve.TOOL_FAILED
	end

	-- 2. klbenv.co_main_loop; first_run 内起 worker; 失败则 exit 并回传码
	local ok_run, code_run = true, codes.OK
	klbenv.set_first_run(function ()
		local ok_f, code_f = _first_run(wopts)
		if false == ok_f then
			ok_run = false
			code_run = code_f or codes.serve.TOOL_FAILED
			kenv.exit()
		end
	end)
	klbenv.co_main_loop()

	return ok_run, code_run
end


return M

-- end
