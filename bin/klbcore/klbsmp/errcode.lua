--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   errcode.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbcore.klbsmp 私有错误码别名 (映射到 klbcore.klbcode)
-- @note   失败形状统一: nil|false, code[, errstr]
--   \n 数值对齐 basecode/netcode; 禁止再用 -1..-16 私有段
--   \n of(number|数字串) 透传; of(英文串) 粗分类
--   \n 仅本包内部使用; 禁止跨包 require
-- @stability internal
-- @impl evolving
--]]


local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local net = klbcode.net


local errcode = {}


errcode.OK			= klbcode.OK			-- 成功
errcode.E_ARG		= base.INVAL			-- 参数非法 (-22)
errcode.E_NOENT		= base.NOENT			-- 不存在 (-2)
errcode.E_IO			= base.IO			-- 读写 / 连接失败 (-5)
errcode.E_NOSPC		= base.NOSPC			-- 空间 / 容量不足 (-28)
errcode.E_BUSY		= base.BUSY			-- 占用 / 冲突 (-16)
errcode.E_NOTIMPL	= base.NOTIMPL		-- 未实现
errcode.E_AUTH		= base.ACCES			-- 认证 / 权限失败 (-13)
errcode.E_RANGE		= base.RANGE			-- 区间 / 范围非法
errcode.E_TIMEOUT	= net.TIMEOUT			-- 超时 (socket 段)
errcode.E_CANCEL		= base.CANCELED		-- 已取消
errcode.E_CLOSED		= net.CLOSED			-- 已关闭
errcode.E_PROTO		= net.ERR_PROTOCOL	-- 协议错误
errcode.E_STATE		= base.STATE			-- 状态非法
errcode.E_OVERFLOW	= base.OVERFLOW		-- 超限
errcode.E_PEER		= base.MNP_PEER		-- 对端错误
errcode.E_UNKNOWN	= base.ERR			-- 未知 / 泛化失败


local TEXT = {
	[errcode.OK]		= "ok",
	[errcode.E_ARG]		= "bad argument",
	[errcode.E_NOENT]	= "not found",
	[errcode.E_IO]		= "io error",
	[errcode.E_NOSPC]	= "no space",
	[errcode.E_BUSY]	= "busy",
	[errcode.E_NOTIMPL]	= "not implemented",
	[errcode.E_AUTH]	= "auth failed",
	[errcode.E_RANGE]	= "out of range",
	[errcode.E_TIMEOUT]	= "timeout",
	[errcode.E_CANCEL]	= "cancelled",
	[errcode.E_CLOSED]	= "closed",
	[errcode.E_PROTO]	= "protocol error",
	[errcode.E_STATE]	= "bad state",
	[errcode.E_OVERFLOW]	= "overflow",
	[errcode.E_PEER]	= "peer error",
	[errcode.E_UNKNOWN]	= "unknown error",
}


-- @brief 错误码转描述串
-- @param [in]  code[number]	错误码
-- @return [string]			静态描述串; 别名表未命中则走 klbcode.strerror
errcode.str = function (code)
	local t = TEXT[code]
	if nil ~= t then
		return t
	end

	if "number" == type(code) then
		local ok, knetcode = pcall(require, "knetcode")
		if ok and "table" == type(knetcode) and "function" == type(knetcode.strerror) then
			local s = knetcode.strerror(code)
			if "string" == type(s) and "" ~= s and nil == string.find(s, "Unknown error", 1, true) then
				return s
			end
		end
		return klbcode.strerror(code)
	end

	return "unknown error"
end


-- @brief 由描述串粗分类错误码; number / 纯数字串透传
-- @param [in]  msg[string|number|nil]
-- @return [number]			错误码
errcode.of = function (msg)
	if "number" == type(msg) then
		return msg
	end

	if "string" == type(msg) then
		local n = tonumber(msg)
		if nil ~= n and nil ~= string.match(msg, "^%-?%d+$") then
			return n
		end
	end

	local s = string.lower(tostring(msg or ""))

	if string.find(s, "timeout", 1, true) ~= nil then
		return errcode.E_TIMEOUT
	end
	if string.find(s, "cancel", 1, true) ~= nil then
		return errcode.E_CANCEL
	end
	if string.find(s, "closed", 1, true) ~= nil or string.find(s, "eof", 1, true) ~= nil then
		return errcode.E_CLOSED
	end
	if string.find(s, "auth", 1, true) ~= nil then
		return errcode.E_AUTH
	end
	if string.find(s, "bad ", 1, true) ~= nil
		or string.find(s, "need ", 1, true) ~= nil
		or string.find(s, "too long", 1, true) ~= nil
		or string.find(s, "invalid", 1, true) ~= nil then
		return errcode.E_ARG
	end
	if string.find(s, "refused", 1, true) ~= nil
		or string.find(s, "unsupported", 1, true) ~= nil
		or string.find(s, "ver", 1, true) ~= nil
		or string.find(s, "proto", 1, true) ~= nil then
		return errcode.E_PROTO
	end

	return errcode.E_IO
end


return errcode

-- end
