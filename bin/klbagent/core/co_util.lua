--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   co_util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  kco / 主泵分流辅助 (in_kco; await_done; co_run; run_host_loop)
-- @stability evolving
-- @impl evolving
-- @note   标准入口一律 klbenv.co_main_loop
--   \n CLI: co_task 唯一泵; 包常驻: run_host_loop → hold / 自调 co_main_loop
--   \n 已在 kco 内: await 用 co_sleep; 禁嵌套 loop_once; 禁 ktime.sleep (仅主泵)
--]]
local kco = require("kco")


local M = {}


-- @brief 是否已在 kco 协程内 (非主线程)
-- @return ok[boolean]
M.in_kco = function ()
	local co, is_main = kco.running()
	return co ~= nil and is_main ~= true
end


-- @brief 等到 state.done (须在 kco 内; 用 co_sleep 让出)
-- @param [in] state[table]			须有 done 字段
-- @param [in] timeout_sec[number|nil]	默认 30
-- @param [in] err_tag[string|nil]	超时错误前缀
-- @return 无; 超时 / 非 kco error()
-- @note 禁嵌套 loop_once / ktime.sleep; 入口须已走 co_main_loop
M.await_done = function (state, timeout_sec, err_tag)
	if type(state) ~= "table" then
		error("await_done: bad state")
	end

	local tag = err_tag
	if type(tag) ~= "string" or tag == "" then
		tag = "await"
	end

	if not M.in_kco() then
		error(tag .. ": must in kco under co_main_loop")
	end

	local limit_ms = (tonumber(timeout_sec) or 30) * 1000
	if limit_ms < 1 then
		limit_ms = 1000
	end

	local ktime = require("ktime")
	local start = ktime.tick_count()

	while not state.done do
		if (ktime.tick_count() - start) > limit_ms then
			error(tag .. " timeout")
		end

		kco.co_sleep(10)
	end
end


-- @brief 常驻业务挂接后: 已在 CLI 宿主泵则 hold; 否则自调 co_main_loop
-- @param [in] opts[table|nil]		opts.until_fn()=true 时 kenv.exit (仅自泵路径)
-- @return mode[string]				"hold" | "self"
-- @note 须在 start_embed / fork 业务之后调用; 禁止在已 hold 的宿主上再套 co_main_loop
M.run_host_loop = function (opts)
	opts = opts or {}

	if M.in_kco() then
		local co_task = require("klbagent.core.cli.co_task")
		co_task.hold()
		return "hold"
	end

	local klbenv = require("klbcore.klbenv")
	if type(klbenv) ~= "table" or type(klbenv.co_main_loop) ~= "function" then
		error("klbenv.co_main_loop unavailable")
	end

	if type(opts.until_fn) == "function" then
		local kenv = require("kenv")
		klbenv.set_loop_run(function ()
			if opts.until_fn() then
				if type(kenv) == "table" and type(kenv.exit) == "function" then
					kenv.exit()
				end
			end
		end)
	end

	klbenv.co_main_loop()
	return "self"
end


-- @brief 在协程内跑 fn 并等待 (替代各处 _run_in_co 嵌套泵)
-- @param [in] timeout_ms[number|nil]	毫秒; 默认 15000
-- @param [in] fn[function]				业务 (须可 pcall)
-- @param [in] err_tag[string|nil]		超时前缀
-- @return ...							fn 成功时的返回; 失败 false, err
-- @note 须在 kco / co_main_loop 下; 非 kco 时经 co_task.run 进入标准泵再跑
M.co_run = function (timeout_ms, fn, err_tag)
	if type(fn) ~= "function" then
		return false, "bad fn"
	end

	if type(kco) ~= "table" or type(kco.fork) ~= "function" then
		return false, "kco unavailable"
	end

	if not M.in_kco() then
		local co_task = require("klbagent.core.cli.co_task")
		return co_task.run(function ()
			return M.co_run(timeout_ms, fn, err_tag)
		end)
	end

	local state = {
		done = false,
		rets = nil,
	}

	kco.fork(function ()
		state.rets = { pcall(fn) }
		state.done = true
	end)

	local timeout_sec = (tonumber(timeout_ms) or 15000) / 1000
	local tag = err_tag
	if type(tag) ~= "string" or tag == "" then
		tag = "co_run"
	end

	local ok_w, err_w = pcall(M.await_done, state, timeout_sec, tag)
	if not ok_w then
		return false, tostring(err_w)
	end

	local rets = state.rets
	if type(rets) ~= "table" or rets[1] ~= true then
		return false, tostring(rets and rets[2] or (tag .. " fail"))
	end

	return table.unpack(rets, 2)
end


return M

-- end
