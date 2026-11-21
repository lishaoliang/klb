--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client/lpcclient.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klblpc 一次性 LPC 客户端
-- @note   post / co_call; co_call 须在 kco 协程内
-- @history 修改历史
--		[2026-10] 创建文件
--]]
local klpc = require("klpc")
local errcode = require("klbcore.klblpc.errcode")
local deadline = require("klbcore.klblpc.deadline")


local lpcclient_co = {}

local default_timeout_ms = nil			-- 默认单次调用 deadline; nil 不限
local peer_gone_hooks = {}				-- 对端失联回调表


-- @brief 保住 nil 洞与个数
local function capture(...)
	return {
		n = select("#", ...),
		...,
	}
end


-- @brief 向模块 post; 不等待
-- @param [in]      mo_name[string]		目标模块名
-- @param [in]      ...					载荷; 首参建议为方法名
-- @return ok[boolean]
-- @return [number|nil] code			失败错误码
lpcclient_co.post = function (mo_name, ...)
	if "string" ~= type(mo_name) or "" == mo_name then
		return false, errcode.E_ARG
	end

	if klpc.post(mo_name, ...) then
		return true
	end

	return false, errcode.E_IO
end


-- @brief 设置默认单次调用 deadline
-- @param [in]      ms[number]			毫秒; nil 或 <=0 不限
lpcclient_co.set_timeout = function (ms)
	default_timeout_ms = tonumber(ms)
end


-- @brief 注册对端失联回调; 投递失败时触发
-- @param [in]      fn[function]		`fn(mo_name)`
-- @return ok[boolean]
lpcclient_co.on_peer_gone = function (fn)
	if "function" ~= type(fn) then
		return false
	end

	peer_gone_hooks[#peer_gone_hooks + 1] = fn
	return true
end


-- @brief 触发对端失联回调
local function fire_peer_gone(mo_name)
	for i = 1, #peer_gone_hooks do
		pcall(peer_gone_hooks[i], mo_name)
	end
end


-- @brief 带 deadline 的 REQUEST
-- @param [in]      ms[number|nil]		毫秒; nil 或 <=0 不限
-- @param [in]      mo_name[string]		目标模块名
-- @param [in]      ...					载荷; 首参建议为方法名
-- @return [...]						对方 response 的数据; 失败 `nil, code[, err]`
local function CallTimed(ms, mo_name, ...)
	if "string" ~= type(mo_name) or "" == mo_name then
		return nil, errcode.E_ARG, "invalid mo_name"
	end

	local lpc = klpc.new()
	if not lpc then
		return nil, errcode.E_IO, "new lpc failed"
	end

	local closed = false
	local function close_lpc()
		if closed then
			return
		end

		closed = true
		pcall(function () lpc:close() end)
	end

	local state = deadline.arm(ms, close_lpc)
	local rets = capture(lpc:co_call(mo_name, ...))
	local expired = deadline.finish(state)

	close_lpc()

	if expired then
		return nil, errcode.E_TIMEOUT, "timeout"
	end

	if 0 == rets.n then
		fire_peer_gone(mo_name)
		return nil, errcode.E_CLOSED, "closed"
	end

	if nil == rets[1] and "number" == type(rets[2]) then
		fire_peer_gone(mo_name)
		return nil, rets[2], errcode.str(rets[2])
	end

	return table.unpack(rets, 1, rets.n)
end


-- @brief 向模块 REQUEST 并等待回应; 须在 kco 协程内
-- @param [in]      mo_name[string]		目标模块名
-- @param [in]      ...					载荷; 首参建议为方法名
-- @return [...]						对方 response 的数据
-- @return [nil],[number|string]		失败时 nil, code[, err]
lpcclient_co.co_call = function (mo_name, ...)
	return CallTimed(default_timeout_ms, mo_name, ...)
end


-- @brief 带 deadline 的 REQUEST (显式毫秒)
-- @param [in]      mo_name[string]		目标模块名
-- @param [in]      ms[number]			毫秒; nil 或 <=0 不限
-- @param [in]      ...					载荷
-- @return [...]						同 `co_call`
lpcclient_co.co_call_timed = function (mo_name, ms, ...)
	return CallTimed(ms, mo_name, ...)
end


return lpcclient_co
