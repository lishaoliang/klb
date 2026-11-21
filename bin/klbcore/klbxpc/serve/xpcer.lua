--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   serve/xpcer.lua
-- @brief  klbxpc 服务循环
-- @note   载荷经 kxpc/luaseri; 首参为方法名; REQUEST 须 response
-- @history 修改历史
--		[2026-11] 创建文件
--]]
local kco = require("kco")
local kxpc = require("kxpc")
local cfger = require("klbcore.klbxpc.cfger")
local router = require("klbcore.klbxpc.router")
local errcode = require("klbcore.klbxpc.errcode")


local function capture(...)
	return {
		n = select("#", ...),
		...,
	}
end


local function kind_of_type(t)
	if kxpc.REQUEST == t then
		return "call"
	end

	if kxpc.NOTIFY == t then
		return "notify"
	end

	return "post"
end


local function co_serve_module(app)
	local mo = app._mo

	while true do
		if not app._serving or not mo then
			return
		end

		local cap = capture(mo:recv(50))
		if 0 == cap.n or nil == cap[1] then
			kco.co_sleep(1)
		else
			local typ = cap[1]
			local src = cap[2]
			local corr = cap[3]
			local kind = kind_of_type(typ)
			local name = cap[4]

			if "string" ~= type(name) or "" == name then
				if "call" == kind then
					mo:response(src, corr, false, "unknown")
				end
			else
				local fn = app._router:find(kind, name)
				if not fn and "post" == kind then
					fn = app._router:find("notify", name)
					if fn then
						kind = "notify"
					end
				end

				if not fn then
					if "call" == kind then
						mo:response(src, corr, false, "unknown")
					end
				else
					local args = {}
					for i = 5, cap.n do
						args[i - 4] = cap[i]
					end

					local req = {
						name = name,
						args = args,
						src = src or "",
						kind = kind,
						corr = corr,
					}

					local pok, nret, rets = pcall(function ()
						local got = capture(fn(req))
						return got.n, got
					end)

					if "call" == kind then
						if not pok then
							mo:response(src, corr, false, "error")
						else
							mo:response(src, corr, table.unpack(rets, 1, nret))
						end
					end
				end
			end
		end
	end
end


local function fork_serve(app)
	kco.fork(function ()
		co_serve_module(app)
	end)
end


local L = {}


function L:setup(cfg)
	if self._serving then
		return false, errcode.E_BUSY
	end

	local parsed, ok = cfger.parse_setup(cfg)
	if not ok or not parsed then
		return false, errcode.E_ARG
	end

	if "" ~= parsed.name then
		self._name = parsed.name
		self._name_ok = true
	end

	self._transport = parsed.transport or cfger.DEFAULT_TRANSPORT
	self._path = parsed.path or ""

	return true
end


function L:serve()
	if not self._name_ok or "" == self._name then
		return false, errcode.E_STATE
	end

	if self._serving then
		return true
	end

	local mo = nil
	if cfger.is_unix(self._transport) then
		mo = kxpc.new_module_unix(self._name, cfger.path_arg(self._path))
	elseif cfger.is_npipe(self._transport) then
		mo = kxpc.new_module_npipe(self._name, cfger.path_arg(self._path))
	elseif cfger.is_sysmq(self._transport) then
		mo = kxpc.new_module_sysmq(self._name, cfger.path_arg(self._path))
	else
		mo = kxpc.new_module(self._name)
	end

	if not mo then
		return false, errcode.E_IO
	end

	self._mo = mo
	self._serving = true
	fork_serve(self)
	return true
end


function L:stop()
	local mo = self._mo
	self._serving = false
	self._mo = nil

	if mo and mo.close then
		mo:close()
	end

	return true
end


function L:call(name, fn)
	return self._router:add("call", name, fn)
end


function L:post(name, fn)
	return self._router:add("post", name, fn)
end


function L:notify(name, fn)
	return self._router:add("notify", name, fn)
end


local xpcer = {}


xpcer.new = function (cfg)
	cfg = cfg or {}
	local parsed, ok = cfger.parse_setup(cfg)
	local name = ""
	local name_ok = false
	local transport = cfger.DEFAULT_TRANSPORT
	local path = ""

	if ok and parsed then
		if "" ~= parsed.name then
			name = parsed.name
			name_ok = true
		elseif nil ~= cfg.name then
			name_ok = false
		end

		transport = parsed.transport or cfger.DEFAULT_TRANSPORT
		path = parsed.path or ""
	elseif nil ~= cfg.name then
		name_ok = false
	end

	local obj = {
		_router = router.new(),
		_name = name,
		_name_ok = name_ok,
		_transport = transport,
		_path = path,
		_mo = nil,
		_serving = false,
	}

	setmetatable(obj, { __index = L })
	return obj
end


return xpcer
