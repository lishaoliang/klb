--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbsocks5 脚本封装 (SOCKS5; 内部可用 ksocks5)
-- @note   IO 编排在本层; wire 优先 ksocks5 (RFC1928/1929)
--   \n 返回码: 直接用 klbcore.klbcode.base / net; 不本地再映射 E_*
-- @history 修改历史
--  \n [2026-10] 从 proxy.socks5 迁入; is_ver / rep_for_upstream_err / probe (proxy L0-2/7/8)
--]]


local client = require("klbcore.klbsocks5.client")
local server = require("klbcore.klbsocks5.serve")


local klbsocks5 = {}


klbsocks5.client = client
klbsocks5.server = server
klbsocks5.VER = 0x05
-- RFC1928 REP: succeeded
klbsocks5.REP_SUCCEEDED = 0x00
-- @note 返回码真源 klbcore.klbcode (base/net); 不本地再映射 E_*


-- @brief C 绑定是否可用 (桩亦返回 true)
klbsocks5.available = function ()
	local ok, mod = pcall(require, "ksocks5")
	if ok and type(mod) == "table" and type(mod.available) == "function" then
		return mod.available() == true
	end
	return false
end


-- @brief 是否 SOCKS5 版本首字节
-- @param [in]  first[number|string|nil]
-- @return [boolean]
klbsocks5.is_ver = function (first)
	local b0 = nil
	if type(first) == "number" then
		b0 = math.floor(first)
	elseif type(first) == "string" and #first > 0 then
		b0 = string.byte(first, 1)
	end
	return b0 == klbsocks5.VER
end


-- @brief 上游失败映射为 SOCKS5 REP
-- @param [in]  err[string|nil]
-- @return [number] rep
klbsocks5.rep_for_upstream_err = function (err)
	local e = tostring(err or "")
	if e == "kproxy.tcp_connect unavailable"
		or e == "kproxy unavailable"
		or e == "chain tcp missing" then
		return 0x01
	end
	return 0x05
end


-- @brief 出站 CONNECT
klbsocks5.connect = function (proxy_host, proxy_port, dest_host, dest_port, auth, opts)
	return client.connect(proxy_host, proxy_port, dest_host, dest_port, auth, opts)
end


-- @brief 入站握手
klbsocks5.serve_conn = function (sock, opts)
	return server.serve_conn(sock, opts)
end


-- @brief 入站应答
-- @param [in]  bnd[table|nil]	可选真实 BND `{ host, port }`
klbsocks5.reply = function (sock, rep, bnd)
	return server.reply(sock, rep, bnd)
end


-- @brief 已连 TCP 上探活 (问候; 可选 CONNECT)
-- @param [in]  tcp[any]
-- @param [in]  probe_target[table|nil]	`{ host, port }`
-- @return [boolean] ok
-- @return [string|nil] err
klbsocks5.probe = function (tcp, probe_target)
	return client.probe(tcp, probe_target)
end


return klbsocks5

-- end
