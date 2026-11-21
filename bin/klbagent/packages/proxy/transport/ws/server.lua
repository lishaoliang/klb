--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   server.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  代理入口 WebSocket 隧道 (仅使用 klbcore.klbws 接口)
-- @note   Upgrade/101/帧/字节流在 klbws; 本层只做隧道 path 白名单
-- @history 修改历史
--  \n [2026-10] 创建文件 (N5 P1 / M1); 帧下沉 C (kws.adopt); 废弃 Lua ws.frame wire; is_upgrade/101 迁 klbcore.klbws; proxy 只调接口; accept 经 klbws.accept_tcp
--]]


local klbws = require("klbcore.klbws")


local M = {}


M.DEFAULT_PATH = "/alua-tunnel"


-- @brief 路径是否代理隧道约定 (产品路径; 非 WS 协议细节)
-- @param [in]  path[string]
-- @return [boolean]
M.is_tunnel_path = function (path)
	if type(path) ~= "string" then
		return false
	end
	if path == M.DEFAULT_PATH or path == "/v1/tunnel" then
		return true
	end
	return false
end


-- @brief 101 + adopt, 返回隧道字节流
-- @param [in]  sock[userdata]	kproxy.tcp
-- @param [in]  key[string]	Sec-WebSocket-Key
-- @return [boolean] ok
-- @return [table|string] stream 或 err
M.accept = function (sock, key)
	local stream, err = klbws.accept_tcp(sock, key)
	if stream == nil then
		return false, err or "accept_tcp fail"
	end
	return true, stream
end


return M

-- end
