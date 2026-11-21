--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   co_task.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  CLI 协程任务: kco.fork + klbenv.co_main_loop 至主体返回 / 常驻退出
-- @stability stable
-- @impl evolving
-- @note   供 entry_cli 包裹整段 CLI; 已在 kco 内则直接跑 fn (不嵌套泵)
--   \n 一次性 CLI: 主体协程返回即退出; 禁以 kco.count 判退出 —— 被调库/工具内部可能自开
--   \n 常驻或卡住的协程, 全局计数永不归零
--   \n 常驻 CLI (如 mcp serve / stdio): 先 M.hold(), 主体返回后泵送至协程清零或 kenv.exit / 信号
--   \n 业务: M.spawn 异步开; M.co_biz 分派内跑业务并等句柄; 工具层用 co_util.await_done
--]]
local kco = require("kco")
local kenv = require("kenv")
local klbenv = require("klbcore.klbenv")
local co_util = require("klbagent.core.co_util")


local M = {}


-- 常驻标记: 一次性 CLI 不置位; 由常驻子命令在 fork 业务协程后调 M.hold()
local g_hold = false


-- @brief 声明本次 CLI 为常驻 (主体返回后仍泵送)
-- @return 无
-- @stability stable
-- @note   只对当前进程内首次 CLI 任务有意义; 一次性命令禁调
M.hold = function ()
	g_hold = true
end


-- @brief 是否已声明常驻
-- @return hold[boolean]
-- @stability stable
M.is_hold = function ()
	return g_hold
end


-- @brief 异步开启业务协程 (不阻塞当前协程)
-- @param [in] fn[function]
-- @param [in] ...
-- @return h[userdata|nil]		kco.fork 句柄; fn 非法为 nil
-- @stability evolving
-- @note   须已在 CLI co_main_loop / 宿主泵下; 调用方可用 M.co_join 等待
M.spawn = function (fn, ...)
	if type(fn) ~= "function" then
		return nil
	end

	local packed = table.pack(...)
	return kco.fork(function ()
		fn(table.unpack(packed, 1, packed.n))
	end)
end


-- @brief 等待业务句柄结束 (须在 kco 协程内)
-- @param [in] h[userdata]
-- @return 无
-- @stability evolving
M.co_join = function (h)
	if h == nil then
		return
	end

	while not kco.is_done(h) do
		kco.co_sleep(5)
	end
end


-- @brief 分派与业务分离: fork 跑 fn 并等结束, 回传返回值
-- @param [in] fn[function]			须可返回 ok, code, ...
-- @param [in] ...
-- @return ...						fn 的返回; pcall 失败则 false, 2
-- @stability evolving
-- @note   供 cli.entry 包各子命令; 业务内可再 M.spawn 异步开
--   \n 须在 kco 内 (entry_cli / co_main_loop 主体)
M.co_biz = function (fn, ...)
	if type(fn) ~= "function" then
		return false, 2
	end

	local packed = table.pack(...)

	if not co_util.in_kco() then
		io.stderr:write("alua cli co_biz: must in kco under co_main_loop\n")
		return false, 2
	end

	local box = {
		pcall_ok = false,
	}

	local h = kco.fork(function ()
		local ok, a, b, c, d = pcall(function ()
			return fn(table.unpack(packed, 1, packed.n))
		end)
		box.pcall_ok = ok
		if ok then
			box.a, box.b, box.c, box.d = a, b, c, d
		else
			box.err = a
			io.stderr:write("alua cli co_biz: " .. tostring(a) .. "\n")
		end
	end)

	M.co_join(h)

	if not box.pcall_ok then
		return false, 2
	end

	return box.a, box.b, box.c, box.d
end


-- @brief 跑 CLI 任务
-- @param [in] fn[function]			须可返回 ok, code, ...
-- @param [in] ...					透传 fn
-- @return ...						fn 的返回; pcall 失败则 false, 2
M.run = function (fn, ...)
	if type(fn) ~= "function" then
		return false, 2
	end

	-- 已在协程: 禁止再套 co_main_loop
	if co_util.in_kco() then
		return fn(...)
	end

	local packed = table.pack(...)
	local box = {
		pcall_ok = false,
	}

	-- 主体协程句柄: 用 is_done 判定返回, 勿仅靠 count (库内可能另开常驻协程)
	local h = kco.fork(function ()
		local ok, a, b, c, d = pcall(function ()
			return fn(table.unpack(packed, 1, packed.n))
		end)
		box.pcall_ok = ok
		if ok then
			box.a, box.b, box.c, box.d = a, b, c, d
		else
			box.err = a
			io.stderr:write("alua cli co_task: " .. tostring(a) .. "\n")
		end
	end)

	-- 入口协程已返回 → 退出 (一次性); 常驻 (M.hold) 则等到业务协程清零或 kenv.exit / 信号
	--   勿用 0 >= kco.count() 作一次性判定: 被调库/工具自开的协程可能常驻或卡住
	klbenv.set_loop_run(function ()
		if not kco.is_done(h) then
			return
		end

		if not g_hold then
			kenv.exit()
			return
		end

		if 0 >= kco.count() then
			kenv.exit()
		end
	end)
	klbenv.co_main_loop()

	if not box.pcall_ok then
		return false, 2
	end

	return box.a, box.b, box.c, box.d
end


return M

-- end
