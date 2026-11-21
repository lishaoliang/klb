--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   serve/grouper.lua
-- @brief  klbxpc 多传输 serve 组 (独立于 xpcer)
-- @note   每传输一 kco recv 协程入队; 使用者仅 co_recv; 首返回值为 transport
--   \n mode=recv(默认): 用户 co_recv / response(tr,...)
--   \n mode=route: 内置 call/post/notify; 与 recv 互斥消费
--   \n 非公开 require 面; 请用 klbcore.klbxpc.new_serve_group / serve_group
-- @history 修改历史
--		[2026-11] 创建文件
--]]
local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local kco = require("kco")
local kxpc = require("kxpc")
local cfger = require("klbcore.klbxpc.cfger")
local router = require("klbcore.klbxpc.router")


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


--- 按传输 create 服务侧模块
-- @return mo|nil
local function create_module(name, transport, path)
	if cfger.is_unix(transport) then
		return kxpc.new_module_unix(name, path)
	end

	if cfger.is_npipe(transport) then
		return kxpc.new_module_npipe(name, path)
	end

	if cfger.is_sysmq(transport) then
		return kxpc.new_module_sysmq(name, path)
	end

	return kxpc.new_module(name)
end


local function find_slot(self, transport)
	if "string" ~= type(transport) then
		return nil
	end

	return self._by_tr[transport]
end


--------------------------------------------------------------------------------------------
-- workers


local function push_event(self, transport, cap)
	local e = {
		transport = transport,
		typ = cap[1],
		src = cap[2],
		corr = cap[3],
		rest = {},
		rest_n = 0,
	}

	if 3 < cap.n then
		for i = 4, cap.n do
			e.rest[i - 3] = cap[i]
		end
		e.rest_n = cap.n - 3
	end

	self._q[#self._q + 1] = e
end


local function route_one(self, slot, cap)
	local mo = slot.mo
	local typ = cap[1]
	local src = cap[2]
	local corr = cap[3]
	local kind = kind_of_type(typ)
	local name = cap[4]

	if "string" ~= type(name) or "" == name then
		if "call" == kind then
			mo:response(src, corr, false, "unknown")
		end
		return
	end

	local fn = self._router:find(kind, name)
	if not fn and "post" == kind then
		fn = self._router:find("notify", name)
		if fn then
			kind = "notify"
		end
	end

	if not fn then
		if "call" == kind then
			mo:response(src, corr, false, "unknown")
		end
		return
	end

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
		transport = slot.transport,
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


local function start_slot_worker(self, slot)
	self._alive = (self._alive or 0) + 1
	slot._worker = true

	kco.fork(function ()
		local mo = slot.mo
		local tr = slot.transport

		while true do
			if not self._serving or not mo then
				break
			end

			local cap = capture(mo:recv(50))
			if 0 == cap.n or nil == cap[1] then
				kco.co_sleep(1)
			elseif cfger.MODE_ROUTE == self._mode then
				route_one(self, slot, cap)
			else
				push_event(self, tr, cap)
			end
		end

		slot._worker = false
		self._alive = self._alive - 1
	end)
end


local function ensure_workers(self)
	if self._workers then
		return
	end

	self._workers = true
	self._alive = 0

	for i = 1, #self._slots do
		start_slot_worker(self, self._slots[i])
	end
end


--------------------------------------------------------------------------------------------
-- group 对象


local G = {}


--- 关闭全部 slot; 清空队列
function G:stop()
	self._serving = false

	for i = 1, #self._slots do
		local slot = self._slots[i]
		local mo = slot.mo
		slot.mo = nil
		if mo and mo.close then
			mo:close()
		end
	end

	self._slots = {}
	self._by_tr = {}
	self._dropped = {}
	self._q = {}
	self._workers = false
	self._alive = 0
	self._name = ""
	self._name_ok = false

	return true
end


G.close = G.stop


--- 打开多传输 slot (未 serve 前可改; 已 serving 须先 stop)
-- @param [in] cfg[table]  见 cfger.parse_group
-- @return ok[boolean], code[number|nil]
function G:open(cfg)
	if self._serving then
		return false, base.BUSY
	end

	local parsed, ok = cfger.parse_group(cfg)
	if not ok or not parsed then
		return false, base.INVAL
	end

	self:stop()

	local slots = {}
	local by_tr = {}
	local dropped = {}

	for i = 1, #parsed.transports do
		local tr = parsed.transports[i]
		local path = cfger.path_for(parsed.paths, tr)
		local mo = create_module(parsed.name, tr, path)
		if not mo then
			if parsed.want_all then
				dropped[#dropped + 1] = tr
			else
				-- 显式列表: 任一项失败则整组失败并拆除已开
				for j = 1, #slots do
					local m = slots[j].mo
					if m and m.close then
						m:close()
					end
				end
				return false, base.IO
			end
		else
			local slot = {
				transport = tr,
				path = parsed.paths[tr] or "",
				mo = mo,
				_worker = false,
			}
			slots[#slots + 1] = slot
			by_tr[tr] = slot
		end
	end

	if 0 == #slots then
		return false, base.IO
	end

	self._name = parsed.name
	self._name_ok = true
	self._mode = parsed.mode
	self._slots = slots
	self._by_tr = by_tr
	self._dropped = dropped
	self._q = {}
	self._workers = false
	self._alive = 0
	self._serving = false

	return true
end


--- 启动各 slot recv 协程
-- @return ok[boolean], code[number|nil]
function G:serve()
	if not self._name_ok or "" == self._name then
		return false, base.STATE
	end

	if 0 == #self._slots then
		return false, base.STATE
	end

	if self._serving then
		return true
	end

	self._serving = true
	ensure_workers(self)
	return true
end


--- 汇聚读一条 (须在 kco 协程内); 仅 mode=recv
-- @return transport, typ, src, corr, name[, ...]
-- @return nil, "exit"|"busy"  已停且无排队 / mode 非 recv
-- @note 首返回值 transport 区分来源传输
function G:co_recv()
	if cfger.MODE_RECV ~= self._mode then
		return nil, "busy"
	end

	if not self._serving and 0 == #self._q then
		return nil, "exit"
	end

	ensure_workers(self)

	while true do
		if 0 < #self._q then
			local e = table.remove(self._q, 1)
			if 0 < e.rest_n then
				return e.transport, e.typ, e.src, e.corr, table.unpack(e.rest, 1, e.rest_n)
			end

			return e.transport, e.typ, e.src, e.corr
		end

		if not self._serving and 0 == (self._alive or 0) then
			return nil, "exit"
		end

		kco.co_sleep(5)
	end
end


--- 对 REQUEST 回包; 须指定来源 transport
-- @param [in] transport[string]
-- @param [in] src[string]
-- @param [in] corr[any]
-- @return ok[boolean], code[number|nil]
function G:response(transport, src, corr, ...)
	local slot = find_slot(self, transport)
	if not slot or not slot.mo then
		return false, base.NOENT
	end

	local ok = slot.mo:response(src, corr, ...)
	if ok then
		return true
	end

	return false, base.IO
end


function G:call(name, fn)
	return self._router:add("call", name, fn)
end


function G:post(name, fn)
	return self._router:add("post", name, fn)
end


function G:notify(name, fn)
	return self._router:add("notify", name, fn)
end


--- 已开传输列表
function G:transports()
	local list = {}
	for i = 1, #self._slots do
		list[i] = self._slots[i].transport
	end
	return list
end


function G:mode()
	return self._mode
end


function G:name()
	return self._name
end


function G:is_serving()
	return true == self._serving
end


--- 快照
function G:info()
	local slots = {}
	for i = 1, #self._slots do
		local s = self._slots[i]
		slots[i] = {
			transport = s.transport,
			path = s.path or "",
			open = (nil ~= s.mo),
			worker = (true == s._worker),
		}
	end

	return {
		name = self._name,
		mode = self._mode,
		serving = (true == self._serving),
		alive = self._alive or 0,
		queue = #(self._q or {}),
		transports = self:transports(),
		dropped = self._dropped,
		slots = slots,
	}
end


--------------------------------------------------------------------------------------------
-- 工厂


local grouper = {}


--- 新建未 open 的 serve 组
-- @param [in] cfg[table|nil]  保留参数; open 请显式 :open / serve_group
grouper.new = function (cfg)
	local obj = {
		_cfg = cfg,
		_router = router.new(),
		_name = "",
		_name_ok = false,
		_mode = cfger.MODE_RECV,
		_slots = {},
		_by_tr = {},
		_dropped = {},
		_q = {},
		_workers = false,
		_alive = 0,
		_serving = false,
	}

	setmetatable(obj, { __index = G })
	return obj
end


return grouper
