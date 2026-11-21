--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   co_main_loop.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  脚本侧 env 主循环 (loop_run 托管在 kco 协程; has_loop_c=true 则仅 first 后返回)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   first_run 经 fork 跑完再继续; loop_run 内可用 kco.co_sleep 等协程 API
--   \n 主线程只泵 loop_once + ktime.sleep; 退出后等主协程句柄 is_done 再收尾
-- @history 修改历史
--  \n [2026-11] 创建文件; 协程托管主循环
--]]
local kenv = require("kenv")
local kco = require("kco")
local ktime = require("ktime")
local kthread = require("kthread")

-- 退出收尾限时 (毫秒); 超时打 snapshot 后强制 close_all 并跳出
local CLOSE_DRAIN_MS = 3000

local function _drain_coroutines()
	local t0 = ktime.tick_count()

	while 0 < kco.count() do
		if CLOSE_DRAIN_MS <= (ktime.tick_count() - t0) then
			local snap = kco.snapshot()
			io.stderr:write(string.format("[kco] close drain timeout count=%d\n", snap.count))
			for i, it in ipairs(snap.list) do
				io.stderr:write(string.format(
					"  [%d] id=%s status=%s in_timeout=%s in_wakeup=%s has_cb=%s\n",
					i, tostring(it.id), tostring(it.status),
					tostring(it.in_timeout), tostring(it.in_wakeup), tostring(it.has_cb)))
			end
			kco.close_all()
			break
		end

		kco.close_all()
		kenv.loop_once()
		ktime.sleep(1)
	end
end


local function _call_co_func(co_func)
	local ok = true
	local err = nil

	local h = kco.fork(function()
		ok, err = pcall(co_func)
	end)

	while not kco.is_done(h) do
		kenv.loop_once()
		ktime.sleep(1)
	end

	if not ok then
		error(err)
	end
end

local function _main_loop(cb_loop_run, cb_first_run, cb_end_run)
	local has_cb_loop_run = ("function" == type(cb_loop_run))
	local has_cb_first_run = ("function" == type(cb_first_run))
	local has_cb_end_run = ("function" == type(cb_end_run))

	local do_first_run = function()
		if has_cb_first_run then
			_call_co_func(cb_first_run)
		end
	end

	local do_loop_run = function()
		if has_cb_loop_run then
			cb_loop_run()
		end
	end

	local do_end_run = function()
		if has_cb_end_run then
			cb_end_run()
		end
	end

	local co_run = 1
	local function co_main_loop(co_sleep)
		while 0 ~= co_run do
			do_loop_run()
			kco.co_sleep(co_sleep)
		end

		do_end_run()
	end

	-- 如果C侧有loop处理, 则直接退出
	local has_loop = kenv.has_loop_c()
	if has_loop then
		do_first_run()
		return 0
	end

	-- 获取env名称
	local name = kenv.get_name()

	-- 获取循环最大睡眠时间
	local sleep_max = kenv.get_loop_sleep()

	-- first
	do_first_run()

	-- main co (句柄供退出后 is_done 等待)
	local h_main = kco.fork(function()
		co_main_loop(sleep_max)
	end)

	-- 如果C侧没有loop处理, 则循环处理
	while true do
		local sleep_tc = kenv.loop_once() -- 建议的休眠时间

		-- 如果需要退出, 则退出
		if kenv.is_exit() then
			break
		end

		-- 限定范围[1, sleep_max]
		if sleep_max <= sleep_tc then
			sleep_tc = sleep_max
		end
		if sleep_tc <= 1 then
			sleep_tc = 1
		end

		-- 真实休眠, 让出CPU
		ktime.sleep(sleep_tc)
	end

	-- 等主协程退出
	co_run = 0
	while not kco.is_done(h_main) do
		kenv.loop_once()
		ktime.sleep(1)
	end

	-- 如果是主线程, 关闭所有子线程
	if "main" == name then
		kthread.stop_all() -- 关闭所有子线程
	end

	-- 协程也需要退出 (限时 drain)
	_drain_coroutines()

	return 0
end

return _main_loop

-- end
