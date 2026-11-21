--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   demux.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  代理入口同口首字节协议分流 (薄壳; 魔数真源在 klbcore)
-- @note   SOCKS5 / alua-tunnel / SMP·MNP 判别调 klbsocks5/klbtunnel/klbsmp/klbmnp; HTTP 首字节本文件私有
-- @history 修改历史
--  \n [2026-10] 创建文件 (N3-3 同口分流打磨); N5 P2: SMP/MNP 魔数首字节 0xEB; classify 迁 klbcore 接口
--]]


local klbsocks5 = require("klbcore.klbsocks5")
local klbtunnel_server = require("klbcore.klbtunnel.serve")
local klbsmp = require("klbcore.klbsmp")
local klbmnp = require("klbcore.klbmnp")


local M = {}


M.PROTO_SOCKS5 = "socks5"
M.PROTO_HTTP = "http-connect"
M.PROTO_ALUA_TUNNEL = "alua-tunnel"
M.PROTO_SMP = "smp"
M.PROTO_UNKNOWN = "unknown"


-- @brief 取首字节
-- @param [in]  first[number|string|nil]
-- @return [number|nil] b0
local function _b0(first)
	if type(first) == "number" then
		local n = math.floor(first)
		if n < 0 then
			n = 0
		end
		if n > 255 then
			n = 255
		end
		return n
	end

	if type(first) == "string" and #first > 0 then
		return string.byte(first, 1)
	end

	return nil
end


-- @brief 是否 HTTP 请求方法首字节 (ASCII 字母)
-- @param [in]  b0[number]
-- @return [boolean]
local function _is_request_byte(b0)
	return (b0 >= 0x41 and b0 <= 0x5A) or (b0 >= 0x61 and b0 <= 0x7A)
end


-- @brief 按首字节分类协议
-- @param [in]  first[number|string|nil]	首字节或含首字节的前缀串
-- @return [string] proto	socks5|http-connect|alua-tunnel|smp|unknown
-- @return [number|nil] b0
M.classify = function (first)
	local b0 = _b0(first)
	if b0 == nil then
		return M.PROTO_UNKNOWN, nil
	end

	if klbtunnel_server.is_magic(b0) then
		return M.PROTO_ALUA_TUNNEL, b0
	end

	if type(klbsocks5.is_ver) == "function" and klbsocks5.is_ver(b0) then
		return M.PROTO_SOCKS5, b0
	end

	if klbsmp.is_magic0(b0) or klbmnp.is_magic0(b0) then
		return M.PROTO_SMP, b0
	end

	if _is_request_byte(b0) then
		return M.PROTO_HTTP, b0
	end

	return M.PROTO_UNKNOWN, b0
end


-- @brief 是否 alua-tunnel 魔数
-- @param [in]  first[number|string|nil]
-- @return [boolean]
M.is_alua_tunnel = function (first)
	local proto = M.classify(first)
	return proto == M.PROTO_ALUA_TUNNEL
end


return M

-- end
