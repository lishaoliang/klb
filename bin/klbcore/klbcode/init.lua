--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klb 库错码门面 (聚合 net/base; 对外统一入口)
-- @stability stable
-- @impl mature
-- @attention
--   \n @stability stable: AI 禁止擅自添加/删除/变更本文件对外接口 (含导出名, 参数, 返回与已文档含义); 须用户点名
--   \n @impl mature: 实现倾向冻结; 重构须同等谨慎, 不得破坏本文件导出签名与可观察语义
-- @note   分表挂载: M.net / M.base; 负段同名同值; 私有段按区间分域
--   \n 不扁平合并短键 (CANCELED/LIMIT 等跨表同名异值)
--   \n net 另含 HTTP status [100,599] / WS close [1000,4999] 别名
--   \n 私有: base [0x10000, +1999] / net [0x10000+2000, +3999]
--   \n 不含 alua core/code; 不含各包 internal errcode
--   \n 完整码名 M.strerror -> klb.<域>.<码名> / klb.OK / klb.UNKNOWN
--   \n require("klbcore.klbcode")
-- @history 修改历史
--  \n [2026-11] 创建文件; 并入 util/netcode.lua util/basecode.lua
--  \n [2026-11] domain/str 认 HTTP / WS 协议码段
--]]


local M = {}


M.net = require("klbcore.klbcode.netcode")
M.base = require("klbcore.klbcode.basecode")

-- 别名 (与文件名一致)
M.netcode = M.net
M.basecode = M.base

-- 全模块成功码
M.OK = 0


local function _known(s, prefix)
	return nil == string.find(s, "^" .. prefix, 1)
end


-- @brief 按数值区间判域
-- @param [in]  code[number]	错误码
-- @return [string]			"ok" / "errno" / "socket" / "http" / "ws" / "net" / "base" / "unknown"
-- @stability stable
M.domain = function (code)
	if type(code) ~= "number" then
		return "unknown"
	end

	if 0 == code then
		return "ok"
	end

	if code < 0 then
		return "errno"
	end

	if code < 64 then
		return "socket"
	end

	-- HTTP status / WS close (禁区别名; 登记名在 net 表)
	if code >= 100 and code <= 599 then
		return "http"
	end

	if code >= 1000 and code <= 4999 then
		return "ws"
	end

	if code >= M.base.PRIVATE_BASE and code <= M.base.PRIVATE_MAX then
		return "base"
	end

	if code >= M.net.PRIVATE_BASE and code <= M.net.PRIVATE_MAX then
		return "net"
	end

	return "unknown"
end


local _OK_NAME = "klb.OK"
local _UNKNOWN = "klb.UNKNOWN"


-- @brief 错误码转短键名 (带域前缀; 非完整码名)
-- @param [in]  code[number]	错误码
-- @return [string]			eg. "net.WBUF_EMPTY" / "base.STATE" / "errno.INVAL" / "OK"
-- @stability stable
-- @note 完整码名用 strerror (eg. klb.net.WBUF_EMPTY)
M.str = function (code)
	local dom = M.domain(code)

	if "ok" == dom then
		return "OK"
	end

	if "socket" == dom or "http" == dom or "ws" == dom or "net" == dom then
		local s = M.net.str(code)
		if _known(s, "netcode%-") then
			return dom .. "." .. s
		end
	elseif "base" == dom then
		local s = M.base.str(code)
		if _known(s, "basecode%-") then
			return "base." .. s
		end
	elseif "errno" == dom then
		-- 负段: base 含 NOSPC, 其余与 net 同; 先 base 再 net
		local s = M.base.str(code)
		if _known(s, "basecode%-") then
			return "errno." .. s
		end
		s = M.net.str(code)
		if _known(s, "netcode%-") then
			return "errno." .. s
		end
	end

	return "klbcode-" .. tostring(code)
end


-- @brief 码名 (完整串)
-- @param [in]  code[number]	码值
-- @return name[string]		`klb.<域>.<码名>`; 成功 `klb.OK`; 未登记 `klb.UNKNOWN`
-- @stability stable
-- @note eg. strerror(net.WBUF_EMPTY) -> "klb.net.WBUF_EMPTY"
--   \n     strerror(base.STATE) -> "klb.base.STATE"
--   \n     strerror(-22) -> "klb.errno.INVAL"
--   \n     strerror(2) -> "klb.socket.TIMEOUT"
--   \n     strerror(0) -> "klb.OK"
--   \n     strerror(70000) -> "klb.UNKNOWN"
M.strerror = function (code)
	if type(code) ~= "number" then
		return _UNKNOWN
	end

	local short = M.str(code)
	if "OK" == short then
		return _OK_NAME
	end

	if nil ~= string.find(short, "^klbcode%-", 1) then
		return _UNKNOWN
	end

	return "klb." .. short
end


return M

-- end
