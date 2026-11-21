--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   xpccall.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ssid XPC 传输调用 + peer 配置 (须在 kco 协程内)
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.client.xpccall")
-- @history 修改历史
--  \n [2026-11] 自 xpcclient 迁出
--  \n [2026-11] unix 空 path 时按 serve.port 拼 /tmp/alua.ssid.xpc.unix.pxxxx
--]]
local const = require("klbagent.core.ssid.base.const")
local util = require("klbagent.core.ssid.client.util")
local xpcpath = require("klbagent.core.ssid.base.xpcpath")
local klbxpc = require("klbcore.klbxpc")
local cfger = require("klbcore.klbxpc.cfger")


-- klbcore 通用码 → 文本 (本文件私有; 原 klbcore/private/errcode.lua 下沉)
local ERR_TEXT = {
	[0] = "ok",
	[-1] = "bad argument",
	[-2] = "not found",
	[-3] = "io error",
	[-4] = "no space",
	[-5] = "busy",
	[-6] = "not implemented",
	[-7] = "auth failed",
	[-8] = "out of range",
	[-9] = "timeout",
	[-10] = "cancelled",
	[-11] = "closed",
	[-12] = "protocol error",
	[-13] = "bad state",
	[-14] = "overflow",
	[-15] = "peer error",
	[-16] = "unknown error",
}


local M = {}


-- 对端传输配置 (默认取 klbxpc.DEFAULT_TRANSPORT, 与 serve.ensure_xpc 一致)
local g_peer = {
	transport = klbxpc.DEFAULT_TRANSPORT,
}


-- @brief 解析 peer: unix 且 path 空 → xpcpath.resolve
-- @param [in] peer[table]
-- @return peer[table]
local function _peer_with_path(peer)
	local tr = peer.transport or klbxpc.DEFAULT_TRANSPORT
	local path = peer.path
	if cfger.is_unix(tr) and (type(path) ~= "string" or path == "") then
		path = select(1, xpcpath.resolve(peer))
	end

	return {
		transport = tr,
		path = path or "",
	}
end


-- @brief 设置对端传输
-- @param [in] opts[table|nil]		transport?=unix|msgq|npipe|sysmq (禁传面选型 "xpc"), path?/port?
-- @note ssid.new_client({transport="xpc"}) 的 "xpc" 是面选型, 非 kxpc 后端; 此处须忽略
--   \n 空 path 不在此拼死; co_call / get_peer 时再 resolve (可读最新 serve.port)
M.set_peer = function (opts)
	opts = util.norm_opts(opts)
	local tr = opts.transport
	if tr == nil or tr == "" or tr == "xpc" or tr == "lpc" then
		tr = klbxpc.DEFAULT_TRANSPORT
	end

	g_peer = {
		transport = tr,
		path = opts.path,
		port = opts.port,
		ports = opts.ports,
	}
end


-- @brief 当前对端配置 (unix 空 path 时现场拼装)
-- @return peer[table]
M.get_peer = function ()
	return _peer_with_path({
		transport = g_peer.transport or klbxpc.DEFAULT_TRANSPORT,
		path = g_peer.path or "",
		port = g_peer.port,
		ports = g_peer.ports,
	})
end


-- @brief 经 XPC 调 owner
-- @param [in] method[string]
-- @param [in] opts[table|nil]
-- @param [in] timeout_ms[number|nil]
-- @return result[any]
-- @return err[string|nil]
M.co_call = function (method, opts, timeout_ms)
	-- Lua 5.2+: running() 在主线程仍返回 thread; 须看 ismain
	local co, is_main = coroutine.running()
	if co == nil or is_main == true then
		return nil, "ssid xpc: need kco coroutine"
	end

	-- 每次 call 再 resolve: 覆盖 serve 后写 serve.port 的时序
	local peer = _peer_with_path({
		transport = g_peer.transport or klbxpc.DEFAULT_TRANSPORT,
		path = g_peer.path,
		port = g_peer.port,
		ports = g_peer.ports,
	})

	-- co_call_ex 失败形: nil, code[, errstr]; 须保留 errstr (如 "open failed")
	local ok_c, a1, a2, a3 = pcall(klbxpc.co_call_ex,
		const.MODULE_NAME,
		peer,
		util.norm_timeout_ms(timeout_ms),
		method,
		util.norm_opts(opts))
	if not ok_c then
		return nil, "ssid xpc fail: " .. tostring(a1)
	end

	if a1 == true then
		return a2
	end

	if type(a3) == "string" and a3 ~= "" then
		return nil, a3
	end

	if type(a2) == "string" and a2 ~= "" then
		return nil, a2
	end

	if type(a2) == "number" then
		return nil, string.format("%s (%d)", ERR_TEXT[a2] or "unknown error", a2)
	end

	return nil, a1 or "ssid xpc fail"
end


return M

-- end
