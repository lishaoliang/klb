--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   trust.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid serve 协程受信标记 (明文 token 出口白名单)
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.serve.trust"); 仅 serve 内部 / 导出转发
-- @history 修改历史
--  \n [2026-11] 自 serve/init 迁出
--]]


local M = {}


-- 明文 token 出口白名单: 仅本 env 受信托协程 (LPC/XPC 派发) 可读
local g_trust_co = setmetatable({}, { __mode = "k" })


-- @brief 标记当前协程受信 (仅供 LPC/XPC 服务派发)
-- @param [in] on[boolean]
M.set_trust = function (on)
	local co = coroutine.running()
	if co == nil then
		return
	end

	if on == true then
		g_trust_co[co] = true
	else
		g_trust_co[co] = nil
	end
end


-- @brief 当前协程是否受信
-- @return trusted[boolean]
M.is_trusted = function ()
	local co = coroutine.running()
	if co == nil then
		return false
	end

	return g_trust_co[co] == true
end


-- @brief 临时提升当前协程受信并执行 fn (供 get_token_plain)
-- @param [in] fn[function]
-- @return ...
M.with_trust = function (fn)
	local co = coroutine.running()
	local prev = nil
	if co ~= nil then
		prev = g_trust_co[co]
		g_trust_co[co] = true
	end

	local ok, a, b, c = pcall(fn)

	if co ~= nil then
		if prev == nil then
			g_trust_co[co] = nil
		else
			g_trust_co[co] = prev
		end
	end

	if not ok then
		error(a)
	end

	return a, b, c
end


return M

-- end
