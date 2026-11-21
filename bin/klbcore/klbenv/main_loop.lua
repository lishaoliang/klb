--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   main_loop.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  脚本侧 env 主循环 (has_loop_c=false 时泵送; true 则立即返回)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   alua/klua 默认 has_loop_c=false; 各入口/工具自行 require 本模块并决定退出
--   \n 常驻: main_loop() 至 kenv.exit / 信号
--   \n 一次性协程: main_loop(function() if 0>=kco.count() then kenv.exit() end end)
--   \n 工具内短泵 (单次 http 等待等) 可继续手写 loop_once; 勿嵌套于已有主泵内
-- @history 修改历史
--  \n [2026-11] 创建文件; 脚本泵替代 C has_loop
--]]
local kenv = require("kenv")
local kco = require("kco")
local ktime = require("ktime")
local kthread = require("kthread")

-- 退出收尾: close_all 后 count 仍 >0 的最长等待 (毫秒); 超时打 snapshot 再强制收尸并跳出
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


local function _main_loop(cb_loop_run, cb_first_run, cb_end_run)
	local has_cb_loop_run = ("function" == type(cb_loop_run))
	local has_cb_first_run = ("function" == type(cb_first_run))
	local has_cb_end_run = ("function" == type(cb_end_run))

	local do_loop_run = function()
		if has_cb_loop_run then
			cb_loop_run()
		end
	end

	local do_first_run = function()
		if has_cb_first_run then
			cb_first_run()
		end
	end

	local do_end_run = function()
		if has_cb_end_run then
			cb_end_run()
		end
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

	-- 如果C侧没有loop处理, 则循环处理
	while true do
		local sleep_tc = kenv.loop_once() -- 建议的休眠时间

		-- 执行回调函数
		do_loop_run()

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

	-- end
	do_end_run()

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
