--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   open_sock.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  serve/base: kmuxlist 裸 sock → 协议会话 (new_serve / adopt)
-- @stability internal
-- @impl evolving
-- @note   原 worker/open_sock; 对齐 klblisten 非 raw 建会话; 证书由消费端装载
--   \n kmuxlist.pop 已内置 unwrap, 入参为 netmux sock lightuserdata
-- @history 修改历史
--  \n [2026-11] 创建文件; P0 对接 http/long/rpc 消费; 自 worker/ 迁入 base (worker 仅留线程入口)
--]]
local klisten = require("klisten")
local khttp = require("khttp")
local ksmp = require("ksmp")
local kmnp = require("kmnp")
local httpserver = require("klbcore.klbhttp.serve.httpserver")
local klbsmp = require("klbcore.klbsmp")
local klbmnp = require("klbcore.klbmnp")
local smpserverpcer = require("klbcore.klbsmp.serve.smpserverpcer")
local mnpserverpcer = require("klbcore.klbmnp.serve.mnpserverpcer")


local M = {}


-- @brief 解析 TLS PEM: 内联 PEM 原文, 或本地文件路径
-- @param [in] s[string]			PEM 原文或文件路径
-- @return pem[string|nil]			PEM 正文; 空串/不可读为 nil
-- @note 与 klbcore.klbhttp.serve.httplistener 的 resolve_pem 同语义; new_serve 只收 PEM 正文
local function _resolve_pem(s)
	if type(s) ~= "string" or "" == s then
		return nil
	end

	if string.find(s, "-----BEGIN", 1, true) then
		return s
	end

	local f = io.open(s, "rb")
	if not f then
		return nil
	end

	local body = f:read("*a")
	f:close()
	if not body or 0 == #body then
		return nil
	end

	return body
end


-- @brief 从 worker opts 取 new_serve TLS 材料 (明文连接由 C 侧 sock->tls 判定)
-- @param [in] opts[table|nil]
-- @return serve_opts[table|nil]		{ cert = PEM 正文, key = PEM 正文 } 或 nil
M.tls_serve_opts = function (opts)
	if type(opts) ~= "table" then
		return nil
	end

	if type(opts.tls_info) == "table" and opts.tls_info.tls == true then
		local cert = _resolve_pem(opts.tls_info.cert)
		local key = _resolve_pem(opts.tls_info.key)
		if cert and key then
			return {
				cert = cert,
				key = key,
			}
		end
	end

	local cert = _resolve_pem(opts.cert)
	local key = _resolve_pem(opts.key)
	if cert and key then
		return {
			cert = cert,
			key = key,
		}
	end

	return nil
end


-- @brief 丢弃未建会话的裸 sock
-- @param [in] sock[lightuserdata|nil]
M.drop = function (sock)
	if sock == nil then
		return
	end

	pcall(klisten.drop, sock)
end


-- @brief 裸 sock → HTTP httpserver 会话 (供 klbweb app:handle; 含 ws Upgrade)
-- @param [in] sock[lightuserdata]
-- @param [in] serve_opts[table|nil]
-- @return sess[table|nil]
-- @return err[string|nil]
M.open_http = function (sock, serve_opts)
	local conn, code = khttp.new_serve(sock, serve_opts)
	if conn == nil then
		return nil, "khttp.new_serve fail code=" .. tostring(code)
	end

	return httpserver.new(conn), nil
end


-- @brief 裸 sock → SMP 会话
-- @param [in] sock[lightuserdata]
-- @param [in] serve_opts[table|nil]
-- @return sess[table|nil]
-- @return err[string|nil]
M.open_smp = function (sock, serve_opts)
	local sess, code = klbsmp.adopt(sock, serve_opts)
	if sess == nil then
		return nil, "klbsmp.adopt fail code=" .. tostring(code)
	end

	return sess, nil
end


-- @brief 裸 sock → MNP 会话
-- @param [in] sock[lightuserdata]
-- @param [in] serve_opts[table|nil]
-- @return sess[table|nil]
-- @return err[string|nil]
M.open_mnp = function (sock, serve_opts)
	local sess, code = klbmnp.adopt(sock, serve_opts)
	if sess == nil then
		return nil, "klbmnp.adopt fail code=" .. tostring(code)
	end

	return sess, nil
end


-- @brief 裸 sock → SMP RPC 会话
-- @param [in] sock[lightuserdata]
-- @param [in] serve_opts[table|nil]
-- @return sess[table|nil]
-- @return err[string|nil]
M.open_smp_rpc = function (sock, serve_opts)
	local conn, code = ksmp.new_serve_rpc(sock, serve_opts)
	if conn == nil then
		return nil, "ksmp.new_serve_rpc fail code=" .. tostring(code)
	end

	return smpserverpcer.new_rpc(conn), nil
end


-- @brief 裸 sock → MNP RPC 会话
-- @param [in] sock[lightuserdata]
-- @param [in] serve_opts[table|nil]
-- @return sess[table|nil]
-- @return err[string|nil]
M.open_mnp_rpc = function (sock, serve_opts)
	local conn, code = kmnp.new_serve_rpc(sock, serve_opts)
	if conn == nil then
		return nil, "kmnp.new_serve_rpc fail code=" .. tostring(code)
	end

	return mnpserverpcer.new_rpc(conn), nil
end


return M

-- end
