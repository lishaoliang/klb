--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   listen/grouper.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klblisten 一组多口监听
-- @stability internal
-- @impl mature
-- @note   每口 schemes 为协议面 (http/https/mnp/mnps...); 与 TLS 投递面组合
--   \n 非公开 require 面; 请用 klbcore.klblisten
-- @history 修改历史
--		[2026-10] 创建文件
--]]
local kco = require("kco")
local knetcode = require("knetcode")
local net = require("klbcore.klbcode.netcode")
local klisten = require("klisten")
local cfger = require("klbcore.klblisten.cfger")
local listener = require("klbcore.klblisten.listen.listener")
local kmux_sink = require("klbcore.klblisten.listen.kmux_sink")


--------------------------------------------------------------------------------------------
-- 内部


-- @brief 记录上一次错误 (C 码或脚本自定义)
local function SetLast(self, code, errstr)
	self._last_code = code
	self._last_errstr = errstr
end


-- @brief 清除上一次错误
local function ClearLast(self)
	self._last_code = nil
	self._last_errstr = nil
end


-- @brief 丢弃未消费的会话 / 裸 sock (raw 模式为 netmux sock lightuserdata)
-- @param [in]      session[table|lightuserdata|nil]
local function DropSession(session)
	if not session then
		return
	end

	-- raw: 裸 sock 无 disconnect/close, 关 fd 用 klisten.drop
	if "table" ~= type(session) then
		klisten.drop(session)
		return
	end

	if "function" == type(session.disconnect) then
		session:disconnect()
		return
	end

	if "function" == type(session.close) then
		session:close()
	end
end


-- @brief 按端口找 slot
-- @param [in]      self[table]
-- @param [in]      port[number]
-- @return slot[table|nil]
-- @return index[number|nil]
local function FindSlotByPort(self, port)
	if "number" ~= type(port) then
		return nil, nil
	end

	for i = 1, #self._slots do
		local s = self._slots[i]
		local p = s.port
		local L = s.listener
		if L and L.is_open and L:is_open() and L.port then
			p = L:port()
		end

		if port == p then
			return s, i
		end
	end

	return nil, nil
end


-- @brief 从 slot / listener 取协议面数组与是否全收
-- @return schemes[table]		排序后的协议面名数组
-- @return schemes_all[boolean]	无白名单 (probe 全收)
local function SchemesOfSlot(slot)
	local list = {}
	local schemes_all = false

	if not slot then
		return list, false
	end

	local L = slot.listener
	if L and L.schemes then
		list = L:schemes()
		if L.info then
			local li = L:info()
			schemes_all = (true == li.schemes_all)
		else
			schemes_all = (nil == L._schemes)
		end

		if 0 < #list or schemes_all then
			return list, schemes_all
		end
	end

	if "table" == type(slot.schemes) then
		for name, on in pairs(slot.schemes) do
			if on and "string" == type(name) then
				list[#list + 1] = name
			end
		end
		table.sort(list)
		return list, false
	end

	-- slot.schemes 为 nil → 本口全收
	return list, true
end


-- @brief schemes set → 数组 (交给 listener:open)
local function SchemesList(schemes)
	if "table" ~= type(schemes) then
		return nil
	end

	local list = {}
	for name, on in pairs(schemes) do
		if on and "string" == type(name) then
			list[#list + 1] = name
		end
	end

	if 0 == #list then
		return nil
	end

	return list
end


-- @brief 合并组 cfg 与本口 mux_opts / need_tls / schemes
-- @param [in]      force_raw[boolean]	配置了 kmuxlist 时强制 raw
local function BuildOpenCfg(base_cfg, spec, force_raw)
	local cfg = {}
	if "table" == type(base_cfg) then
		for k, v in pairs(base_cfg) do
			-- 组级移交配置不传给子口 listener
			if "kmuxlist" ~= k then
				cfg[k] = v
			end
		end
	end

	local mux = spec.mux_opts
	if mux then
		if nil == cfg.tls_as_http then
			cfg.tls_as_http = mux.tls_as_http
		end

		if nil == cfg.tls_as_mnp then
			cfg.tls_as_mnp = mux.tls_as_mnp
		end

		if nil == cfg.tls_as_rtsp then
			cfg.tls_as_rtsp = mux.tls_as_rtsp
		end
	end

	if spec.need_tls and nil == cfg.tls then
		cfg.tls = true
	end

	-- raw: 白名单命中后不建会话; kmuxlist 移交必须裸 sock
	if true == force_raw or true == spec.raw then
		cfg.raw = true
	end

	if nil == cfg.schemes then
		cfg.schemes = SchemesList(spec.schemes)
	end

	-- 本口绑定地址; 组 cfg 已指定 host 时以其为准
	if nil == cfg.host and nil ~= spec.host then
		cfg.host = spec.host
	end

	return cfg
end


-- @brief 为单口 fork accept 循环 (可在组已运行时增口)
local function StartSlotWorker(self, slot)
	self._alive = (self._alive or 0) + 1
	slot._worker = true

	kco.fork(function ()
		local L = slot.listener
		while true do
			-- listener 已按 schemes 过滤, 首返回值为协议面
			local face, session, msg, tls, addr = L:co_accept()
			if "exit" == msg then
				break
			end

			if "ok" == msg and session then
				-- 配置了 kmuxlist: 按 face 入对应跨线程队列; 不再进本环境 _q
				if self._kmux_sink then
					if "table" == type(session) then
						DropSession(session)
					elseif not self._kmux_sink.push(face, session) then
						DropSession(session)
					end
				else
					self._q[#self._q + 1] = {
						face = face,
						session = session,				-- raw 模式为裸 sock (lightuserdata)
						raw = ("table" ~= type(session)),
						msg = msg,
						tls = (true == tls),
						slot = slot,
						addr = addr,
					}
				end
			elseif "error" == msg then
				-- 上抛子口 new_serve / Upgrade 等错误到组 last_* 与 co_accept
				if L.last_code then
					local c = L:last_code()
					if nil ~= c then
						SetLast(self, c, L:last_error())
					end
				end

				self._q[#self._q + 1] = {
					face = face,
					session = nil,
					msg = "error",
					tls = (true == tls),
					slot = slot,
					addr = addr,
				}
			elseif session then
				DropSession(session)
			end
		end

		slot._worker = false
		self._alive = self._alive - 1
	end)
end


local function EnsureWorkers(self)
	if self._workers then
		return
	end

	self._workers = true
	self._alive = 0
	self._q = self._q or {}

	for i = 1, #self._slots do
		StartSlotWorker(self, self._slots[i])
	end
end


--------------------------------------------------------------------------------------------
-- group 对象


local group = {}


-- @brief 关闭全部子监听
function group:close()
	self._closed = true

	for i = 1, #self._slots do
		local L = self._slots[i].listener
		if L and L.close then
			L:close()
		end
	end

	-- 队列内未取走的连接: 会话(raw=false) / 裸 sock(raw=true) 一并释放, 免 fd 泄漏
	for i = 1, #self._q do
		DropSession(self._q[i].session)
	end

	self._slots = {}
	self._workers = false
	self._q = {}
	self._alive = 0
	-- kmuxlist 按 path 进程内共享, 此处只丢本地引用 (未 pop 的 sock 仍在队列中)
	self._kmux_sink = nil
end


-- @brief 上一次错误码 (C 调用或脚本校验失败后保留)
-- @return code[number|nil]				klb_netcode_e; 无错误为 nil
function group:last_code()
	return self._last_code
end


-- @brief 上一次错误描述串
-- @return errstr[string|nil]			自定义串或 knetcode.strerror; 无错误为 nil
function group:last_error()
	if nil == self._last_code then
		return nil
	end

	if nil ~= self._last_errstr then
		return self._last_errstr
	end

	return knetcode.strerror(self._last_code)
end


-- @brief 按一组规格 bind 各口
-- @param [in]      listen[table]		一条或数组; 见 cfger.parse_listen
-- @return ok[boolean]					任一项失败为 false (已 open 的会 close)
-- @return code[number|nil]				失败时 klb_netcode_e
function group:open(listen)
	ClearLast(self)

	local specs, ok = cfger.parse_listen(listen)
	if not ok or not specs then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	self:close()
	self._closed = false

	-- 可选: 监听结果写入 kmuxlist (可按协议分 path); 有则强制 raw 并在 open 后起 worker
	self._kmux_sink = nil
	do
		local sink, sink_err = kmux_sink.setup(self._cfg and self._cfg.kmuxlist)
		if sink_err then
			SetLast(self, net.INVAL, sink_err)
			return false, net.INVAL
		end

		self._kmux_sink = sink
	end

	local force_raw = (nil ~= self._kmux_sink)
	local slots = {}
	for i = 1, #specs do
		local spec = specs[i]
		if "tcp" ~= spec.transport then
			self:close()
			SetLast(self, net.INVAL, "Invalid argument")
			return false, net.INVAL
		end

		local open_cfg = BuildOpenCfg(self._cfg, spec, force_raw)
		local L = listener.new(open_cfg)
		local open_ok, code = L:open(spec.port, open_cfg)
		if not open_ok then
			local c = code or net.INVAL
			-- 透传子口 C 错误串; 无码时用子口 last_error / Invalid argument
			if nil ~= code then
				SetLast(self, c)
			else
				SetLast(self, c, L:last_error() or "Invalid argument")
			end
			self:close()
			return false, c
		end

		self._slots = slots
		slots[#slots + 1] = {
			transport = spec.transport,
			port = spec.port,
			path = spec.path,
			host = spec.host,
			schemes = spec.schemes,
			mux_opts = spec.mux_opts,
			listener = L,
		}
	end

	self._slots = slots
	self._workers = false
	self._q = {}
	self._alive = 0

	-- kmuxlist 模式: 立即起 accept worker, 结果进队列供其他线程 pop
	if self._kmux_sink then
		EnsureWorkers(self)
	end

	ClearLast(self)
	return true
end


-- @brief 组内增口 (可在已 open / 已 co_accept 后调用)
-- @param [in]      item[table]			单条 listen 规格 (同 open 元素)
-- @return ok[boolean]
-- @return code[number|nil]
function group:add_listen(item)
	ClearLast(self)

	if self._closed then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	local specs, ok = cfger.parse_listen(item)
	if not ok or not specs or 1 ~= #specs then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	local spec = specs[1]
	if "tcp" ~= spec.transport then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	if FindSlotByPort(self, spec.port) then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	local open_cfg = BuildOpenCfg(self._cfg, spec, (nil ~= self._kmux_sink))
	local L = listener.new(open_cfg)
	local open_ok, code = L:open(spec.port, open_cfg)
	if not open_ok then
		local c = code or net.INVAL
		if nil ~= code then
			SetLast(self, c)
		else
			SetLast(self, c, L:last_error() or "Invalid argument")
		end
		return false, c
	end

	local slot = {
		transport = spec.transport,
		port = spec.port,
		path = spec.path,
		host = spec.host,
		schemes = spec.schemes,
		mux_opts = spec.mux_opts,
		listener = L,
	}
	self._slots[#self._slots + 1] = slot

	if self._workers then
		StartSlotWorker(self, slot)
	end

	ClearLast(self)
	return true
end


-- @brief 组内删口 (close 该口 listener; 不整组关闭)
-- @param [in]      port[number]		端口
-- @return ok[boolean]
-- @return code[number|nil]
function group:remove_listen(port)
	ClearLast(self)

	local slot, idx = FindSlotByPort(self, port)
	if not slot then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	local L = slot.listener
	if L and L.close then
		L:close()
	end

	table.remove(self._slots, idx)
	ClearLast(self)
	return true
end


-- @brief 按端口热更新协议面 (转发子口 :set_schemes)
-- @param [in]      port[number]
-- @param [in]      schemes[table|nil]
-- @return ok[boolean]
-- @return code[number|nil]
function group:set_schemes(port, schemes)
	ClearLast(self)

	local slot = FindSlotByPort(self, port)
	if not slot or not slot.listener then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	local L = slot.listener
	local ok, code = L:set_schemes(schemes)
	if not ok then
		SetLast(self, code or net.INVAL, L:last_error())
		return false, code or net.INVAL
	end

	-- 同步 slot 快照
	if L.schemes then
		local list = L:schemes()
		local set = nil
		if 0 < #list then
			set = {}
			for i = 1, #list do
				set[list[i]] = true
			end
		end
		slot.schemes = set
	end

	if L.mux_opts then
		slot.mux_opts = L:mux_opts()
	end

	ClearLast(self)
	return true
end


-- @brief 按端口重绑绑定地址 (转发子口 :rebind; 同端口, 既有连接不受影响)
-- @param [in]      port[number]			端口
-- @param [in]      host[string|nil]		新绑定地址; nil/"" → INADDR_ANY
-- @return ok[boolean]
-- @return code[number|nil]				失败时 klb_netcode_e
function group:rebind(port, host)
	ClearLast(self)

	local slot = FindSlotByPort(self, port)
	if not slot or not slot.listener then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	if nil ~= host and "string" ~= type(host) then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	local L = slot.listener
	local ok, code = L:rebind(host)
	if not ok then
		SetLast(self, code or net.INVAL, L:last_error())
		return false, code or net.INVAL
	end

	local new_host = ""
	if nil ~= host and "" ~= host then
		new_host = host
	end

	slot.host = new_host

	ClearLast(self)
	return true
end


-- @brief 接收一条已探测连接 (多口汇聚); 须在 kco 协程内
-- @return face[string]					协议面 http/https/mnp/mnps/...
-- @return session[table|lightuserdata]	raw=false: 协议会话; raw=true: 裸 netmux sock
-- @return msg[string]					"ok" / "exit" / "error"
-- @return slot[table|nil]
-- @return tls[boolean]
-- @return addr[table|nil]				对端地址 `{ ip, port }`; 无连接为 nil
-- @note 首次调用时为各口 fork accept 循环
--   \n raw=true (spec 或组 cfg): 白名单命中即交裸 sock, 由调用方 klisten.wrap 后跨线程移交
function group:co_accept()
	if self._closed or 0 == #self._slots then
		return nil, nil, "exit", nil, false, nil
	end

	EnsureWorkers(self)

	while true do
		if 0 < #self._q then
			local e = table.remove(self._q, 1)
			return e.face, e.session, e.msg, e.slot, e.tls, e.addr
		end

		if self._closed or 0 == self._alive then
			return nil, nil, "exit", nil, false, nil
		end

		kco.co_sleep(5)
	end
end


-- @brief 当前已 open 的口列表 (只读)
-- @return [table]
function group:slots()
	return self._slots
end


-- @brief 是否仍有已 open 的口
-- @return [boolean]
function group:is_open()
	if self._closed then
		return false
	end

	return (0 < #self._slots)
end


-- @brief 当前已 open 的端口列表
-- @return [table]	number 数组
function group:ports()
	local list = {}
	for i = 1, #self._slots do
		local L = self._slots[i].listener
		if L and L.is_open and L:is_open() then
			list[#list + 1] = L:port()
		elseif L then
			list[#list + 1] = self._slots[i].port or 0
		end
	end

	return list
end


-- @brief 按端口取协议面白名单 eg. grp:schemes(80) → { "http", "mnp", "mnps", "smp" }
-- @param [in]      port[number]		端口
-- @return schemes[table]				排序后的协议面名数组; 端口不存在为 {}
-- @return schemes_all[boolean]			true 表示该口无白名单 (全收); 端口不存在为 false
-- @note 有白名单时第二返回值为 false, 数组即支持的协议面
function group:schemes(port)
	local slot = FindSlotByPort(self, port)
	if not slot then
		return {}, false
	end

	return SchemesOfSlot(slot)
end


-- @brief 组装单口全信息表 (port_info / info(port) 共用)
-- @param [in]      slot[table|nil]
-- @param [in]      want[number]		查询端口
-- @return [table]
local function BuildPortInfo(slot, want)
	if not slot then
		return {
			found = false,
			port = want or 0,
			open = false,
			transport = "",
			path = "",
			host = "",
			local_addr = nil,
			schemes = {},
			schemes_all = false,
			tls_face = "",
			tls_face_dropped = {},
			tls = false,
			plain = false,
			mux_opts = {},
			last_code = 0,
			last_error = "",
		}
	end

	local L = slot.listener
	local li = {}
	if L and L.info then
		li = L:info()
	end

	local schemes, schemes_all = SchemesOfSlot(slot)

	local host = li.host
	if nil == host then
		host = slot.host or ""
	end

	return {
		found = true,
		port = li.port or slot.port or want or 0,
		open = (true == li.open),
		transport = slot.transport or "",
		path = slot.path or "",
		host = host,
		local_addr = li.local_addr,
		schemes = schemes,
		schemes_all = schemes_all,
		tls_face = (li.tls_face or ""),
		tls_face_dropped = (li.tls_face_dropped or {}),
		tls = (true == li.tls),
		plain = (true == li.plain),
		mux_opts = li.mux_opts or (slot.mux_opts or {}),
		last_code = li.last_code or 0,
		last_error = li.last_error or "",
	}
end


-- @brief 按端口取该口全信息 (同 :info(port))
-- @param [in]      port[number]		端口
-- @return info[table]					见 :info(port) 注释
function group:port_info(port)
	local want = 0
	if "number" == type(port) then
		want = port
	end

	return BuildPortInfo(FindSlotByPort(self, want), want)
end


-- @brief 全信息快照; 无参=整组, 有 port=该口
-- @param [in]      port[number]		[可选] 指定端口则返回该口全信息 (同 port_info)
-- @return info[table]
-- @note 无参 info = {
--   \n   open[boolean]
--   \n   closed[boolean]
--   \n   alive[number]			accept worker 存活数
--   \n   queue[number]			待取连接队列长度
--   \n   ports[table]			端口数组
--   \n   slots[table]			每口全信息 (同 info(port), 无 found 字段)
--   \n   last_code[number]
--   \n   last_error[string]
--   \n }
--   \n 有 port 时 info = {
--   \n   found[boolean] / port / open / transport / path
--   \n   host[string] / local_addr[table|nil]	绑定地址与回读本地地址 `{ ip, port }`
--   \n   schemes[table] / schemes_all[boolean]
--   \n   tls_face[string] / tls_face_dropped[table]	TLS 单路投递面与未投递面
--   \n   tls[boolean] / plain[boolean] / mux_opts[table]
--   \n   last_code[number] / last_error[string]
--   \n }
--   \n eg. grp:info() 整组; grp:info(80) 仅 80 口全信息 (含 schemes)
function group:info(port)
	if "number" == type(port) then
		return BuildPortInfo(FindSlotByPort(self, port), port)
	end

	local slots = {}
	for i = 1, #self._slots do
		local s = self._slots[i]
		local one = BuildPortInfo(s, s.port or 0)
		-- 整组 slots[] 不重复 found (恒为已挂载口)
		slots[#slots + 1] = {
			port = one.port,
			open = one.open,
			transport = one.transport,
			path = one.path,
			host = one.host,
			local_addr = one.local_addr,
			schemes = one.schemes,
			schemes_all = one.schemes_all,
			tls_face = one.tls_face,
			tls_face_dropped = one.tls_face_dropped,
			tls = one.tls,
			plain = one.plain,
			mux_opts = one.mux_opts,
			last_code = one.last_code,
			last_error = one.last_error,
		}
	end

	local code = self._last_code
	if nil == code then
		code = 0
	end

	local err = ""
	if 0 ~= code then
		err = self:last_error() or ""
	end

	return {
		open = self:is_open(),
		closed = (true == self._closed),
		alive = self._alive or 0,
		queue = #(self._q or {}),
		ports = self:ports(),
		slots = slots,
		kmuxlist = (self._kmux_sink and self._kmux_sink.info()) or nil,
		last_code = code,
		last_error = err,
	}
end


--------------------------------------------------------------------------------------------
-- 工厂


local grouper = {}


-- @brief 新建未 open 的一组监听
-- @param [in]      cfg[table]			[可选] 交给各口 listener:open (tls/cert 等)
-- @return [table]
grouper.new = function (cfg)
	local obj = {
		_cfg = cfg,
		_slots = {},
		_q = {},
		_workers = false,
		_alive = 0,
		_closed = false,
		_kmux_sink = nil,					-- kmux_sink; 有则按 face 写入 kmuxlist
		_last_code = nil,
		_last_errstr = nil,
	}

	setmetatable(obj, {
		__index = group,
	})

	return obj
end


return grouper
