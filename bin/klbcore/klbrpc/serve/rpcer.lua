--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   serve/rpcer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbrpc 服务循环
-- @note   方法名是 co_recv 的首个返回值; REQUEST 的返回值经 response 发回
--   \n 同口多协议经 klblisten; 单协议仍 listen_rpc
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kco = require("kco")
local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local cfger = require("klbcore.klbrpc.cfger")
local router = require("klbcore.klbrpc.router")


--------------------------------------------------------------------------------------------
-- 前置定义

local MOD_BY_SCHEME = {
	smp = "klbcore.klbsmp",
	mnp = "klbcore.klbmnp",
	http = "klbcore.klbhttp",
	ws = "klbcore.klbws",
}


-- klblisten probe 名 → 短 scheme (同口 RPC 面; 现行仅 smp/mnp)
local SCHEME_BY_MUX_PROTO = {
	smprpc = "smp",
	mnprpc = "mnp",
}


-- 同口 mux 可投递的短 scheme
local MUX_SCHEME_OK = {
	smp = true,
	mnp = true,
}


local KIND_BY_METHOD = {
	REQUEST = "call",
	POST = "post",
	NOTIFY = "notify",
}

local api_cache = {}


-- @brief 按 scheme 加载协议包; 失败记为 false, 避免重复 require
local function LoadApi(scheme)
	local hit = api_cache[scheme]
	if false == hit then
		return nil
	end

	if nil ~= hit then
		return hit
	end

	local name = MOD_BY_SCHEME[scheme]
	if not name then
		api_cache[scheme] = false
		return nil
	end

	local ok, api = pcall(require, name)
	if not ok or "table" ~= type(api) or "function" ~= type(api.listen_rpc) then
		api_cache[scheme] = false
		return nil
	end

	api_cache[scheme] = api
	return api
end


-- @brief 保住 nil 洞与个数
local function Capture(...)
	return {
		n = select("#", ...),
		...,
	}
end


-- 内置鉴权方法名 (保留; 优先于路由表)
local AUTH_METHOD = "__auth"


-- @brief 常量时间比较
-- @param [in]  a[string|nil]
-- @param [in]  b[string]
-- @return equal[boolean]
local function TokenEq(a, b)
	if "string" ~= type(a) or "string" ~= type(b) then
		return false
	end

	local la = #a
	local lb = #b
	local n = la
	if lb > n then
		n = lb
	end

	local diff = la ~ lb
	for i = 1, n do
		local ca = (i <= la) and string.byte(a, i) or 0
		local cb = (i <= lb) and string.byte(b, i) or 0
		diff = diff | (ca ~ cb)
	end

	return 0 == diff
end


-- @brief 规范化服务端鉴权配置
-- @param [in]  auth[table|nil]		`{ mode="token"|"none", token|user+pass, expire_ms }`
-- @return [table|nil]				合法配置; 非法为 nil
-- @note mode 缺省 token; mode=none 关闭鉴权. token 优先, 否则 `user:pass`.
local function NormalizeAuth(auth)
	if "table" ~= type(auth) then
		return nil
	end

	local mode = auth.mode
	if "string" ~= type(mode) or "" == mode then
		mode = "token"
	end

	mode = string.lower(mode)
	if "none" == mode then
		return { mode = "none" }
	end

	local expect = ""
	if "string" == type(auth.token) then
		expect = auth.token
	else
		local user = tostring(auth.user or "")
		if "" == user then
			return nil
		end

		expect = user .. ":" .. tostring(auth.pass or "")
	end

	if "" == expect then
		return nil
	end

	local expire_ms = tonumber(auth.expire_ms)
	return {
		mode = "token",
		expect = expect,
		expire_ms = expire_ms,
	}
end


-- @brief 校验令牌
-- @param [in]  auth[table|nil]		NormalizeAuth 结果
-- @param [in]  token[string|nil]
-- @return ok[boolean]
-- @return [number]					失败时错误码
-- @return [string]					失败时 errstr
local function CheckToken(auth, token)
	if "table" ~= type(auth) or "none" == auth.mode then
		return true
	end

	if "string" ~= type(auth.expect) or "" == auth.expect then
		return false, base.ACCES, "auth not configured"
	end

	local expire = auth.expire_ms
	if nil ~= expire and 0 < expire and expire <= math.floor(os.time() * 1000) then
		return false, base.ACCES, "token expired"
	end

	if TokenEq(tostring(token or ""), auth.expect) then
		return true
	end

	return false, base.ACCES, "auth rejected"
end


-- @brief 新建连接鉴权状态
-- @param [in]  auth[table|nil]
-- @return [table]
local function NewAuthState(auth)
	if "table" ~= type(auth) or "none" == auth.mode then
		return { authed = true }
	end

	return { authed = false }
end


-- @brief 未知方法或处理函数出错时回应 (结构化错误码优先)
-- @param [in]  conn[table]		承载协议包 rpc 服务连接
-- @param [in]  sequence[number]	请求序号
-- @param [in]  msg[string]		错误描述
-- @param [in]  code[number|nil]	错误码; 缺省 base.ERR
-- @return ok[boolean]
local function RespondFail(conn, sequence, msg, code)
	local rc
	if "function" == type(conn.response_error) then
		rc = conn:response_error(sequence, code or base.ERR, msg)
	else
		rc = conn:response(sequence, false, msg)
	end

	if 0 ~= rc then
		conn:disconnect()
		return false
	end

	return true
end


-- @brief 一条连接上循环收包并分发
-- @param [in]  auth_state[table]		本连接鉴权状态; `{ authed }`
local function ServeConn(app, conn, scheme, auth_state)
	while true do
		local cap = Capture(conn:co_recv())
		if 0 == cap.n or nil == cap[1] then
			conn:disconnect()
			return
		end

		local state = conn:status() or {}
		local method = state.method or ""
		local sequence = state.sequence or 0
		local kind = KIND_BY_METHOD[method]
		local name = cap[1]

		if "string" ~= type(name) or "" == name or not kind then
			if "REQUEST" == method then
				if not RespondFail(conn, sequence, "method not found", base.NOENT) then
					return
				end
			end
		elseif AUTH_METHOD == name then
			-- 内置鉴权: 仅 REQUEST 回应
			if "call" == kind then
				local ok_a, code_a, msg_a = CheckToken(app._auth, cap[2])
				if not ok_a then
					if not RespondFail(conn, sequence, msg_a, code_a) then
						return
					end
				else
					auth_state.authed = true
					local rc = conn:response(sequence, true)
					if 0 ~= rc then
						conn:disconnect()
						return
					end
				end
			end
		elseif not auth_state.authed then
			-- 未鉴权: REQUEST 回错误; POST/NOTIFY 丢弃
			if "call" == kind then
				if not RespondFail(conn, sequence, "auth required", base.ACCES) then
					return
				end
			end
		else
			local fn = app._router:find(kind, name)
			if not fn then
				if "call" == kind then
					if not RespondFail(conn, sequence, "method not found", base.NOENT) then
						return
					end
				end
			else
				local args = {}
				for i = 2, cap.n do
					args[i - 1] = cap[i]
				end

				local req = {
					name = name,			-- [必须] string    方法名
					args = args,			-- [必须] array     方法名之后的参数; 无则为 {}
					method = method,		-- [必须] string    REQUEST / POST / NOTIFY
					sequence = sequence,	-- [必须] integer   请求序号
					scheme = scheme,		-- [必须] string    smp, mnp, http, ws
				}

				-- 每请求独立协程: 长方法不再阻塞本连接后续请求
				kco.fork(function ()
					local xok, xn, xr = pcall(function ()
						local got = Capture(fn(req))
						return got.n, got
					end)

					if "call" == kind then
						if not xok then
							RespondFail(conn, sequence, tostring(xn), base.ERR)
						else
							local rc = conn:response(sequence, table.unpack(xr, 1, xn))
							if 0 ~= rc then
								conn:disconnect()
							end
						end
					end
				end)
			end
		end
	end
end


-- @brief 单协议 listen_rpc 的 accept 循环
local function AcceptLoop(app, slot)
	local listener = slot.listener
	while true do
		local conn = listener:co_accept()
		if not conn then
			break
		end

		kco.fork(function ()
			ServeConn(app, conn, slot.scheme, NewAuthState(app._auth))
		end)
	end
end


-- @brief 同口 klblisten 的 accept 循环; 仅投递 smprpc / mnprpc
local function AcceptMuxLoop(app, slot)
	local listener = slot.listener
	local want = slot.schemes or {}

	while true do
		local proto, session, msg = listener:co_accept()
		if "exit" == msg then
			break
		end

		if "ok" ~= msg or not session then
			-- 探测失败或无会话; 已由 klblisten 处理 drop
		else
			local scheme = SCHEME_BY_MUX_PROTO[proto]
			if scheme and want[scheme] then
				kco.fork(function ()
					ServeConn(app, session, scheme, NewAuthState(app._auth))
				end)
			end
		end
	end
end


local function ForkAccept(app, slot)
	kco.fork(function ()
		if slot.mux then
			AcceptMuxLoop(app, slot)
		else
			AcceptLoop(app, slot)
		end
	end)
end


-- @brief 按端口归组 listen specs
-- @return [table]						`{ { port, rpctype, schemes={name=true}, list={spec...} }, ... }`
local function GroupSpecsByPort(specs)
	local order = {}
	local map = {}

	for i = 1, #specs do
		local spec = specs[i]
		local port = spec.port
		local g = map[port]
		if not g then
			g = {
				port = port,
				rpctype = spec.rpctype,
				schemes = {},
				list = {},
			}
			map[port] = g
			order[#order + 1] = g
		end

		g.schemes[spec.scheme] = true
		g.list[#g.list + 1] = spec
		if spec.rpctype then
			g.rpctype = spec.rpctype
		end
	end

	return order
end


-- @brief 一组同口规格中的协议个数
local function SchemeCount(schemes)
	local n = 0
	for _ in pairs(schemes) do
		n = n + 1
	end

	return n
end


-- @brief 同口 mux 仅支持 smp/mnp; 含 http/ws 则拒
local function MuxSchemesOk(schemes)
	for scheme, _ in pairs(schemes) do
		if not MUX_SCHEME_OK[scheme] then
			return false
		end
	end

	return true
end


--------------------------------------------------------------------------------------------
-- 服务对象

local R = {}


-- @brief 汇总监听规格; 服务中返回 false
-- @param [in]      cfg[table]			[可选] `{ listen, rpctype, auth }`
-- @return ok[boolean]					true 成功
-- @note listen 只记规格, serve 才 bind. 只传 rpctype 时覆盖已有各项.
-- @note auth `{ mode="token"|"none", token|user+pass, expire_ms }`; 仅供 serve 前设置.
function R:setup(cfg)
	if self._serving then
		return false
	end

	cfg = cfg or {}
	if nil == cfg.listen and nil == cfg.rpctype and nil == cfg.auth then
		return true
	end

	if nil ~= cfg.auth then
		local auth = NormalizeAuth(cfg.auth)
		if nil == auth then
			return false
		end

		self._auth = auth
	end

	local src = cfg.listen
	if nil == src and nil ~= cfg.rpctype then
		src = {}
		local old = self._listen_specs or {}
		for i = 1, #old do
			src[i] = {
				scheme = old[i].scheme,
				port = old[i].port,
			}
		end
	end

	if nil == src then
		self._listen_ok = true
		return true
	end

	local specs, lok = cfger.parse_listen(src, cfg.rpctype)
	if not lok or not specs then
		return false
	end

	self._listen_specs = specs
	self._listen_ok = true
	return true
end


-- @brief 按规格 bind 并 fork accept; 同口多协议走 klblisten
-- @return ok[boolean]					true 全部加载成功; 任一项失败为 false
-- @note 幂等; 已启动则直接 true. 部分失败仍对已打开的监听起循环.
--   \n 同口 mux 现行 RPC 投递 smprpc/mnprpc; http/ws 单协议仍 listen_rpc.
function R:serve()
	if not self._listen_ok then
		return false
	end

	if self._serving then
		return true
	end

	local ok = true
	local groups = GroupSpecsByPort(self._listen_specs or {})

	for gi = 1, #groups do
		local g = groups[gi]
		local nscheme = SchemeCount(g.schemes)

		if 1 < nscheme then
			-- 同口混合含 http/ws 时拒收 (mux 仅投递 smprpc/mnprpc)
			if not MuxSchemesOk(g.schemes) then
				ok = false
			else
				local lok, klblisten = pcall(require, "klbcore.klblisten")
				if not lok or "table" ~= type(klblisten) or "function" ~= type(klblisten.listen) then
					ok = false
				else
					local mux = klblisten.listen(g.port, {
						rpctype = g.rpctype,
					})
					if not mux then
						ok = false
					else
						self._listeners[#self._listeners + 1] = {
							listener = mux,
							mux = true,
							schemes = g.schemes,
							port = g.port,
							forked = false,
						}
					end
				end
			end
		else
			local spec = g.list[1]
			local api = LoadApi(spec.scheme)
			if not api then
				ok = false
			else
				local listener = api.listen_rpc(spec.port, {
					rpctype = spec.rpctype,
				})
				-- 协议包须失败回 nil; 另以 is_open 兜底(避免 bind 失败仍回表)
				if (not listener) or (listener.is_open and not listener:is_open()) then
					ok = false
				else
					self._listeners[#self._listeners + 1] = {
						listener = listener,
						scheme = spec.scheme,
						mux = false,
						forked = false,
					}
				end
			end
		end
	end

	for i = 1, #self._listeners do
		local slot = self._listeners[i]
		if slot.listener and not slot.forked then
			ForkAccept(self, slot)
			slot.forked = true
		end
	end

	if 0 < #self._listeners then
		self._serving = true
	end

	return ok
end


-- @brief 关闭全部监听; 保留方法表与 listen 规格
-- @return ok[boolean]					true
function R:stop()
	for i = 1, #self._listeners do
		local slot = self._listeners[i]
		local listener = slot and slot.listener
		if listener and listener.close then
			listener:close()
		end
	end

	self._listeners = {}
	self._serving = false
	return true
end


-- @brief 注册 REQUEST 方法
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`, 返回值作为 response 载荷
-- @return ok[boolean]
function R:call(name, fn)
	return self._router:add("call", name, fn)
end


-- @brief 注册 POST 方法; 无回应
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`
-- @return ok[boolean]
function R:post(name, fn)
	return self._router:add("post", name, fn)
end


-- @brief 注册 NOTIFY 方法; 无回应
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`
-- @return ok[boolean]
function R:notify(name, fn)
	return self._router:add("notify", name, fn)
end


--------------------------------------------------------------------------------------------
-- 工厂

local rpcer = {}


-- @brief 新建一个 RPC 服务
-- @param [in]      cfg[table]			[可选] 初始配置; 字段同 :setup
-- @return [table]						服务对象
-- @note 未写 listen 时缺省单口 3457 多协议 (smp/mnp; 同口 mux 现行 RPC 面), rpctype `LUA`
--   \n http/ws 须单协议 listen_rpc 或分口; 同口混声明会被 :serve 拒收
rpcer.new = function (cfg)
	cfg = cfg or {}
	local specs, lok = cfger.parse_listen(cfg.listen, cfg.rpctype)
	if nil == cfg.listen then
		specs = cfger.default_specs(cfg.rpctype)
		lok = nil ~= specs[1]
	end

	local auth = nil
	if nil ~= cfg.auth then
		auth = NormalizeAuth(cfg.auth)
		if nil == auth then
			-- 非法鉴权配置: fail-closed (拒绝全部 __auth)
			auth = { mode = "token" }
		end
	end

	local obj = {
		_router = router.new(),
		_listen_specs = specs or {},
		_listen_ok = lok and true or false,
		_listeners = {},
		_serving = false,
		_auth = auth,
	}

	setmetatable(obj, {
		__index = R,
	})

	return obj
end


return rpcer
