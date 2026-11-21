--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   deadline.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbcore.klbsmp 私有单次调用 deadline 辅助 (自原 klbcore/private/deadline.lua 下沉)
-- @note   kco.timeout 无取消句柄; 用共享状态 + 到期回调唤醒阻塞调用
--   \n 典型用法: 到期关闭连接, 使阻塞的 co_call / co_recv 返回
--   \n 仅本包内部使用; 禁止跨包 require
-- @stability internal
-- @impl evolving
--]]


local deadline = {}


-- @brief 武装一次 deadline
-- @param [in]     ms[number]			毫秒; nil 或 <=0 不武装
-- @param [in]     on_expire[function]	[可选] 到期回调; 通常关闭连接以唤醒阻塞
-- @return state[table]					`{ done, expired }`; 未武装为 nil
deadline.arm = function (ms, on_expire)
	ms = tonumber(ms)
	if nil == ms or 0 >= ms then
		return nil
	end

	local ok, kco = pcall(require, "kco")
	if not ok or "table" ~= type(kco) or "function" ~= type(kco.timeout) then
		return nil
	end

	local state = { done = false, expired = false }

	kco.timeout(ms, function ()
		if state.done then
			return
		end

		state.expired = true

		if "function" == type(on_expire) then
			pcall(on_expire)
		end
	end)

	return state
end


-- @brief 结束 deadline (标记完成)
-- @param [in] state[table|nil]
-- @return expired[boolean]		true 表示武装期间已到期
deadline.finish = function (state)
	if nil == state then
		return false
	end

	state.done = true

	return state.expired
end


return deadline

-- end
