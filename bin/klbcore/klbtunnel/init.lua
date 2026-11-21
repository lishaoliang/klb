--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbtunnel 脚本封装 (alua-tunnel; wire 优先 ktunnel)
-- @note   IO/中继编排在本层; frame/crypto wire 优先 ktunnel
-- @history 修改历史
--  \n [2026-10] 从 proxy.tunnel 迁入; 导出 hello_connect / io_*
--]]


local frame = require("klbcore.klbtunnel.frame")
local crypto = require("klbcore.klbtunnel.crypto")
local client = require("klbcore.klbtunnel.client")
local server = require("klbcore.klbtunnel.serve")


local klbtunnel = {}


klbtunnel.client = client
klbtunnel.server = server


klbtunnel.MAGIC0 = frame.MAGIC0
klbtunnel.MAGIC1 = frame.MAGIC1
klbtunnel.VER = frame.VER


-- @brief 隧道能力是否可用 (C 绑定缺失时 Lua 回退亦可)
klbtunnel.available = function ()
	local ok, mod = pcall(require, "ktunnel")
	if ok and type(mod) == "table" and type(mod.available) == "function" then
		return mod.available() == true
	end
	return true
end


-- @brief 当前 wire 实现: "ktunnel"(C) | "lua"(回退)
klbtunnel.wire_kind = function ()
	local ok, mod = pcall(require, "ktunnel")
	if ok and type(mod) == "table" and type(mod.encode) == "function" then
		return "ktunnel"
	end
	return "lua"
end


klbtunnel.is_magic = function (first)
	return server.is_magic(first)
end


klbtunnel.connect = function (proxy_host, proxy_port, dest_host, dest_port, opts)
	return client.connect(proxy_host, proxy_port, dest_host, dest_port, opts)
end


klbtunnel.hello_connect = function (io, dest_host, dest_port, opts)
	return client.hello_connect(io, dest_host, dest_port, opts)
end


klbtunnel.io_from_tcp = function (tcp)
	return client.io_from_tcp(tcp)
end


klbtunnel.io_from_stream = function (stream)
	return client.io_from_stream(stream)
end


klbtunnel.io_from_klb_binary = function (c, head)
	return client.io_from_klb_binary(c, head)
end


klbtunnel.serve_conn = function (sock, opts)
	return server.serve_conn(sock, opts)
end


klbtunnel.listen = function (host, port, backlog)
	return server.listen(host, port, backlog)
end


klbtunnel.start = function (cfg, opts)
	return server.start(cfg, opts)
end


return klbtunnel

-- end

