--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client/xpcclient.lua
-- @brief  klbxpc 一次性客户端
-- @note   post / co_call; co_call 须在 kco 协程内(本实现为阻塞 request)
-- @history 修改历史
--		[2026-11] 创建文件
--]]
local kxpc = require("kxpc")
local errcode = require("klbcore.klbxpc.errcode")


local xpcclient_co = {}
local default_timeout_ms = 5000


local function capture(...)
	return {
		n = select("#", ...),
		...,
	}
end


xpcclient_co.set_timeout = function (ms)
	local n = tonumber(ms)
	if not n or n <= 0 then
		default_timeout_ms = -1
	else
		default_timeout_ms = n
	end
end


local cfger = require("klbcore.klbxpc.cfger")


local function open_peer(mo_name, opts)
	if nil == opts then
		return kxpc.open(mo_name)
	end

	if "string" == type(opts) then
		if cfger.is_unix(opts) then
			return kxpc.open_unix(mo_name)
		end

		if cfger.is_npipe(opts) then
			return kxpc.open_npipe(mo_name)
		end

		if cfger.is_sysmq(opts) then
			return kxpc.open_sysmq(mo_name)
		end

		return kxpc.open(mo_name)
	end

	if "table" == type(opts) then
		local tr = opts.transport or cfger.DEFAULT_TRANSPORT
		if cfger.is_unix(tr) then
			return kxpc.open_unix(mo_name, cfger.path_arg(opts.path))
		end

		if cfger.is_npipe(tr) then
			return kxpc.open_npipe(mo_name, cfger.path_arg(opts.path))
		end

		if cfger.is_sysmq(tr) then
			return kxpc.open_sysmq(mo_name, cfger.path_arg(opts.path))
		end
	end

	return kxpc.open(mo_name)
end


xpcclient_co.post = function (mo_name, ...)
	if "string" ~= type(mo_name) or "" == mo_name then
		return false, errcode.E_ARG
	end

	local cli = open_peer(mo_name, nil)
	if not cli then
		return false, errcode.E_IO
	end

	local ok = cli:post(mo_name, ...)
	cli:close()
	if ok then
		return true
	end

	return false, errcode.E_IO
end


--- post_ex(mo_name, opts, ...)
xpcclient_co.post_ex = function (mo_name, opts, ...)
	if "string" ~= type(mo_name) or "" == mo_name then
		return false, errcode.E_ARG
	end

	local cli = open_peer(mo_name, opts)
	if not cli then
		return false, errcode.E_IO
	end

	local ok = cli:post(mo_name, ...)
	cli:close()
	if ok then
		return true
	end

	return false, errcode.E_IO
end


xpcclient_co.co_call = function (mo_name, ...)
	return xpcclient_co.co_call_timed(default_timeout_ms, mo_name, ...)
end


xpcclient_co.co_call_timed = function (ms, mo_name, ...)
	if "string" ~= type(mo_name) or "" == mo_name then
		return nil, errcode.E_ARG, "invalid mo_name"
	end

	local timeout = tonumber(ms) or default_timeout_ms
	if not timeout or timeout <= 0 then
		timeout = -1
	end

	local cli = open_peer(mo_name, nil)
	if not cli then
		return nil, errcode.E_IO, "open failed"
	end

	local cap = capture(cli:request(mo_name, timeout, ...))
	cli:close()

	if 0 == cap.n or nil == cap[1] and 1 < cap.n then
		-- request 失败时返回 nil, "failed"
		return nil, errcode.E_IO, tostring(cap[2] or "failed")
	end

	return table.unpack(cap, 1, cap.n)
end


--- co_call_ex(mo_name, opts, ms, ...)
xpcclient_co.co_call_ex = function (mo_name, opts, ms, ...)
	if "string" ~= type(mo_name) or "" == mo_name then
		return nil, errcode.E_ARG, "invalid mo_name"
	end

	local timeout = tonumber(ms) or default_timeout_ms
	if not timeout or timeout <= 0 then
		timeout = -1
	end

	local cli = open_peer(mo_name, opts)
	if not cli then
		return nil, errcode.E_IO, "open failed"
	end

	local cap = capture(cli:request(mo_name, timeout, ...))
	cli:close()

	if 0 == cap.n or nil == cap[1] and 1 < cap.n then
		return nil, errcode.E_IO, tostring(cap[2] or "failed")
	end

	return table.unpack(cap, 1, cap.n)
end


return xpcclient_co
