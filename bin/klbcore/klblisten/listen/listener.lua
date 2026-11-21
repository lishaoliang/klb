--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   listen/listener.lua
-- @brief  共享 listen + probe 分发 (脚本入口实现)
-- @stability internal
-- @impl mature
-- @note   经 klisten; HTTP/RTSP 等按协议转各包 new_serve; **非** 独立 bind 各协议
--   \n 非公开 require 面; 请用 klbcore.klblisten
-- @history 修改历史
--  \n 2026 创建文件
--]]


local io = require("io")
local kco = require("kco")
local knetcode = require("knetcode")
local net = require("klbcore.klbcode.netcode")
local klisten = require("klisten")
local khttp = require("khttp")
local krtsp = require("krtsp")
local ksmp = require("ksmp")
local kmnp = require("kmnp")

local cfger = require("klbcore.klblisten.cfger")
local kmux_sink = require("klbcore.klblisten.listen.kmux_sink")
local httpserver = require("klbcore.klbhttp.serve.httpserver")
local rtspserver = require("klbcore.klbrtsp.serve.rtspserver")
local smpserver = require("klbcore.klbsmp.serve.smpserver")
local smpserverpcer = require("klbcore.klbsmp.serve.smpserverpcer")
local mnpserver = require("klbcore.klbmnp.serve.mnpserver")
local mnpserverpcer = require("klbcore.klbmnp.serve.mnpserverpcer")


--------------------------------------------------------------------------------------------
-- 内部


-- 与 C KLUA_KLISTEN_MAX_CONNECT_DEFAULT / klisten.listen 缺省对齐
local MUX_TLS_AS_HTTP_DEFAULT = true
local MUX_TLS_AS_MNP_DEFAULT = false
local MUX_TLS_AS_RTSP_DEFAULT = false
local MUX_MAX_CONNECT_DEFAULT = 20


-- @brief kmuxlist 模式: fork 循环 co_accept → 按 face 入队 (脚本勿再调 :co_accept)
local function StartKmuxFeed(self)
	if self._kmux_feed or not self._kmux_sink then
		return
	end

	self._kmux_feed = true
	kco.fork(function ()
		while self._mux and self._kmux_sink do
			local face, session, msg = self:co_accept()
			if "exit" == msg then
				break
			end

			if "ok" == msg and session then
				if "table" == type(session) then
					if "function" == type(session.disconnect) then
						session:disconnect()
					elseif "function" == type(session.close) then
						session:close()
					end
				elseif not self._kmux_sink.push(face, session) then
					klisten.drop(session)
				end
			elseif session and "table" ~= type(session) then
				klisten.drop(session)
			end
		end

		self._kmux_feed = false
	end)
end


-- @brief 记录上一次错误 (C 码或脚本自定义)
local function SetLast(self, code, errstr)
	self._last_code = code
	self._last_errstr = errstr
end


-- @brief new_serve / new_serve_rpc 公共 opts: TLS 材料 + 可选心跳
-- @note ping_* 来自 self._cfg; 未设则走 C 默认 (15s/30s)
-- @param [in] self[table]
-- @return [table|nil]
local function ServeConnOpts(self)
	local o = nil

	if self._http_opts then
		o = {
			cert = self._http_opts.cert,
			key = self._http_opts.key,
		}
	end

	local cfg = self._cfg
	if type(cfg) == "table" then
		if cfg.ping_interval_ms ~= nil or cfg.ping_timeout_ms ~= nil then
			if o == nil then
				o = {}
			end

			if cfg.ping_interval_ms ~= nil then
				o.ping_interval_ms = cfg.ping_interval_ms
			end

			if cfg.ping_timeout_ms ~= nil then
				o.ping_timeout_ms = cfg.ping_timeout_ms
			end
		end
	end

	return o
end


-- @brief 清除上一次错误
local function ClearLast(self)
	self._last_code = nil
	self._last_errstr = nil
end


-- @brief 组装完整 klisten mux cfg (四项均有值; 对齐 C 缺省)
-- @param [in]      use_opts[table|nil]
-- @return [table]	`{ tls_as_http, tls_as_mnp, tls_as_rtsp, max_connect [, host] [, family] [, reuseport] [, nodelay] [, keepalive] }`
local function BuildMuxOpts(use_opts)
	local o = {
		tls_as_http = MUX_TLS_AS_HTTP_DEFAULT,
		tls_as_mnp = MUX_TLS_AS_MNP_DEFAULT,
		tls_as_rtsp = MUX_TLS_AS_RTSP_DEFAULT,
		max_connect = MUX_MAX_CONNECT_DEFAULT,
	}

	if "table" ~= type(use_opts) then
		return o
	end

	if nil ~= use_opts.tls_as_http then
		o.tls_as_http = (true == use_opts.tls_as_http)
	end

	if nil ~= use_opts.tls_as_mnp then
		o.tls_as_mnp = (true == use_opts.tls_as_mnp)
	end

	if nil ~= use_opts.tls_as_rtsp then
		o.tls_as_rtsp = (true == use_opts.tls_as_rtsp)
	end

	if "number" == type(use_opts.max_connect) then
		local n = use_opts.max_connect
		if 1 > n then
			n = 1
		end
		o.max_connect = n
	end

	-- 绑定地址; 仅在有值 (非空串) 时下发, 缺省 INADDR_ANY
	if "string" == type(use_opts.host) and "" ~= use_opts.host then
		o.host = use_opts.host
	end

	-- 地址族: "ipv4"(缺省) / "ipv6" / "dual"
	if "string" == type(use_opts.family) and "" ~= use_opts.family then
		o.family = use_opts.family
	end

	if nil ~= use_opts.reuseport then
		o.reuseport = (true == use_opts.reuseport)
	end

	if nil ~= use_opts.nodelay then
		o.nodelay = (true == use_opts.nodelay)
	end

	if nil ~= use_opts.keepalive then
		o.keepalive = (true == use_opts.keepalive)
	end

	return o
end


-- @brief schemes set → 排序数组 (稳定查询)
-- @param [in]      schemes[table|nil]	set
-- @return [table]	string 数组; nil/空 → {}
local function SchemesToList(schemes)
	local list = {}
	if "table" ~= type(schemes) then
		return list
	end

	for name, on in pairs(schemes) do
		if on and "string" == type(name) then
			list[#list + 1] = name
		end
	end

	table.sort(list)
	return list
end


-- @brief 复制 mux_opts 表 (查询返回用)
local function CopyMuxOpts(src)
	local o = {
		tls_as_http = MUX_TLS_AS_HTTP_DEFAULT,
		tls_as_mnp = MUX_TLS_AS_MNP_DEFAULT,
		tls_as_rtsp = MUX_TLS_AS_RTSP_DEFAULT,
		max_connect = MUX_MAX_CONNECT_DEFAULT,
	}

	if "table" == type(src) then
		if nil ~= src.tls_as_http then
			o.tls_as_http = (true == src.tls_as_http)
		end

		if nil ~= src.tls_as_mnp then
			o.tls_as_mnp = (true == src.tls_as_mnp)
		end

		if nil ~= src.tls_as_rtsp then
			o.tls_as_rtsp = (true == src.tls_as_rtsp)
		end

		if "number" == type(src.max_connect) then
			o.max_connect = src.max_connect
		end

		if "string" == type(src.host) and "" ~= src.host then
			o.host = src.host
		end

		if "string" == type(src.family) and "" ~= src.family then
			o.family = src.family
		end

		if nil ~= src.reuseport then
			o.reuseport = (true == src.reuseport)
		end

		if nil ~= src.nodelay then
			o.nodelay = (true == src.nodelay)
		end

		if nil ~= src.keepalive then
			o.keepalive = (true == src.keepalive)
		end
	end

	return o
end


-- @brief 合并 opts 与 schemes 推导的 mux_opts / need_tls
local function BuildOpenCfg(base_cfg, spec)
	local cfg = {}
	if "table" == type(base_cfg) then
		for k, v in pairs(base_cfg) do
			cfg[k] = v
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

	return cfg
end


-- @brief 从 opts 取 schemes / scheme
local function SchemesFromOpts(opts)
	if "table" ~= type(opts) then
		return nil
	end

	if nil ~= opts.schemes then
		return opts.schemes
	end

	if "table" == type(opts.scheme) then
		return opts.scheme
	end

	if "string" == type(opts.scheme) then
		return { opts.scheme }
	end

	return nil
end


local function ResolvePem(s)
	if "string" ~= type(s) or "" == s then
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


local function ResolveHttpOpts(opts)
	if not opts then
		return nil, true
	end

	if true ~= opts.tls then
		return opts, true
	end

	local cert = ResolvePem(opts.cert)
	local key = ResolvePem(opts.key)
	if not cert or not key then
		return nil, false
	end

	local out = {}
	for k, v in pairs(opts) do
		out[k] = v
	end
	out.cert = cert
	out.key = key
	return out, true
end


-- @brief netmux sock → 对端地址表 (IPv4 / IPv6)
-- @param [in]      sock[userdata|nil]		klisten accept 所得 sock
-- @return addr[table|nil]			`{ ip, port }`; 无 sock 为 nil
local function SockAddr(sock)
	if not sock then
		return nil
	end

	local ip, port = klisten.sock_addr(sock)
	if not ip then
		return nil
	end

	return { ip = ip, port = port }
end


-- @brief schemes 是否含明文/TLS HTTP 面 (与 ws 并存时优先交 HTTP 会话)
local function WantHttpFace(schemes, is_tls)
	if nil == schemes then
		return true
	end

	if is_tls then
		return (true == schemes.https)
	end

	return (true == schemes.http)
end


-- @brief schemes 是否含 ws/wss (无 http/https 时由本模块自动 Upgrade→kws)
local function WantWsFace(schemes, is_tls)
	if nil == schemes then
		return false
	end

	if is_tls then
		return (true == schemes.wss)
	end

	return (true == schemes.ws)
end


-- @brief Connection/Upgrade 头是否含 token (逗号分隔, 忽略大小写)
local function HeaderHasToken(value, token)
	if "string" ~= type(value) or "" == value then
		return false
	end

	local want = string.lower(token)
	for part in string.gmatch(value, "[^,]+") do
		local t = string.lower((string.match(part, "^%s*(.-)%s*$") or ""))
		if want == t then
			return true
		end
	end

	return false
end


-- @brief 解析 HTTP 请求行与头 (供 ws Upgrade; 不依赖 klbweb)
-- @return method[string], path[string], ver[string], headers[table]
local function ParseHttpRequest(head)
	local method = ""
	local path = "/"
	local ver = ""
	local headers = {}

	if "string" ~= type(head) or "" == head then
		return method, path, ver, headers
	end

	local line = string.match(head, "^([^\r\n]+)")
	if line then
		local m, target, v = string.match(line, "^(%S+)%s+(%S+)%s*(%S*)")
		if m then
			method = string.upper(m)
			path = string.match(target, "^([^?]*)") or "/"
			ver = v or ""
		end
	end

	local skip = true
	for row in string.gmatch(head, "[^\r\n]+") do
		if skip then
			skip = false
		else
			local k, val = string.match(row, "^([^:]+):%s*(.*)$")
			if k then
				k = string.lower((string.match(k, "^%s*(.-)%s*$") or ""))
				headers[k] = (string.match(val or "", "^%s*(.-)%s*$") or "")
			end
		end
	end

	return method, path, ver, headers
end


-- @brief 仅 ws/wss 面: 读首包 Upgrade, 回 101, 升为 klbws 会话
-- @return ws[table|nil], code[number|nil]	成功为 wsserver; 失败 code 可空
local function CoUpgradeToWs(http_session, ws_cfg)
	local klbws = require("klbcore.klbws")
	local wsserver = require("klbcore.klbws.serve.wsserver")

	local msg, head = http_session:co_recv()
	if "text" ~= msg or "string" ~= type(head) then
		http_session:disconnect()
		return nil, net.PROTO_UNEXPECTED
	end

	local method, path, ver, headers = ParseHttpRequest(head)
	if "GET" ~= method or "HTTP/1.1" ~= ver then
		http_session:disconnect()
		return nil, net.PROTO_UNEXPECTED
	end

	if not HeaderHasToken(headers["connection"], "upgrade")
		or not HeaderHasToken(headers["upgrade"], "websocket") then
		http_session:disconnect()
		return nil, net.PROTO_UNEXPECTED
	end

	if "13" ~= (headers["sec-websocket-version"] or "") then
		http_session:disconnect()
		return nil, net.PROTO_VERSION
	end

	local key = headers["sec-websocket-key"] or ""
	if "" == key then
		http_session:disconnect()
		return nil, net.PROTO_UNEXPECTED
	end

	local want_path = nil
	local want_proto = nil
	if "table" == type(ws_cfg) then
		if "string" == type(ws_cfg.path) and "" ~= ws_cfg.path then
			want_path = ws_cfg.path
		end

		if "string" == type(ws_cfg.protocol) and "" ~= ws_cfg.protocol then
			want_proto = ws_cfg.protocol
		end
	end

	if want_path and want_path ~= path then
		http_session:disconnect()
		return nil, net.PROTO_UNEXPECTED
	end

	if want_proto and not HeaderHasToken(headers["sec-websocket-protocol"] or "", want_proto) then
		http_session:disconnect()
		return nil, net.PROTO_UNEXPECTED
	end

	local accept = klbws.accept_key(key)
	if "" == accept then
		http_session:disconnect()
		return nil, net.ERR
	end

	local switch = "HTTP/1.1 101 Switching Protocols\r\n"
		.. "Upgrade: websocket\r\n"
		.. "Connection: Upgrade\r\n"
		.. "Sec-WebSocket-Accept: " .. accept .. "\r\n"

	if want_proto then
		switch = switch .. "Sec-WebSocket-Protocol: " .. want_proto .. "\r\n"
	end

	switch = switch .. "\r\n"

	if 0 ~= http_session:send(switch) then
		http_session:disconnect()
		return nil, net.IO
	end

	local conn, code = http_session:upgrade()
	if not conn then
		http_session:disconnect()
		return nil, code or net.ERR
	end

	return wsserver.new(conn), nil
end


--------------------------------------------------------------------------------------------
-- listen 对外接口


local listen = {}


-- @brief 关闭共享监听
function listen:close()
	if self._mux then
		self._mux:close()
		self._mux = nil
	end

	self._kmux_sink = nil
	self._kmux_feed = false
end


-- @brief 是否已 open (mux 有效)
-- @return [boolean]
function listen:is_open()
	return (nil ~= self._mux)
end


-- @brief 上一次错误码 (C 调用或脚本校验失败后保留)
-- @return code[number|nil]				klb_netcode_e; 无错误为 nil
function listen:last_code()
	return self._last_code
end


-- @brief 上一次错误描述串
-- @return errstr[string|nil]			自定义串或 knetcode.strerror; 无错误为 nil
function listen:last_error()
	if nil == self._last_code then
		return nil
	end

	if nil ~= self._last_errstr then
		return self._last_errstr
	end

	return knetcode.strerror(self._last_code)
end


-- @brief 协议面白名单 (排序数组); 空表且 :info().schemes_all 表示全收
-- @return [table]	string 数组
function listen:schemes()
	return SchemesToList(self._schemes)
end


-- @brief 热更新协议面白名单 (不重 bind); 同步 C mux tls_as_*
-- @param [in]      schemes[table|nil]	协议面数组; nil 表示全收
-- @return ok[boolean]
-- @return code[number|nil]				失败时 klb_netcode_e
-- @note 含 TLS 面时须本口已装载 cert/key (open 时 tls); 多 TLS 面仍按优先序选一路
function listen:set_schemes(schemes)
	ClearLast(self)

	if not self._mux then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	if nil == schemes then
		self._schemes = nil
		self._tls_face = cfger.effective_tls_face(nil)
		self._tls_dropped = {}
		ClearLast(self)
		return true
	end

	local specs, parse_ok = cfger.parse_listen({
		port = self:port(),
		schemes = schemes,
	})
	if not parse_ok or not specs or not specs[1] then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	local spec = specs[1]
	-- raw 模式不装证书 (交裸 sock), 故不要求 _http_opts 已有 cert/key
	if spec.need_tls and not self._raw and not self._http_opts then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	local mux = spec.mux_opts
	if mux and self._mux.set_mux_opts then
		local ok, code = self._mux:set_mux_opts(mux)
		if not ok then
			SetLast(self, code or net.INVAL)
			return false, code or net.INVAL
		end
	end

	self._schemes = spec.schemes
	self._tls_face = cfger.effective_tls_face(self._schemes)
	self._tls_dropped = cfger.dropped_tls_faces(self._schemes)
	do
		local cur = CopyMuxOpts(self._mux_opts)
		if mux then
			cur.tls_as_http = (true == mux.tls_as_http)
			cur.tls_as_mnp = (true == mux.tls_as_mnp)
			cur.tls_as_rtsp = (true == mux.tls_as_rtsp)
		end
		self._mux_opts = cur
	end

	if spec.need_tls then
		self._tls = true
	end

	if self._schemes and (self._schemes.ws or self._schemes.wss) then
		require("klbcore.klbws")
	end

	ClearLast(self)
	return true
end


-- @brief 生效的 klisten mux 可选参 (三项齐全)
-- @return [table]	`{ tls_as_http, tls_as_mnp, max_connect }`
function listen:mux_opts()
	return CopyMuxOpts(self._mux_opts)
end


-- @brief 当前监听快照 (端口 / 绑定地址 / 协议面 / mux / 开关 / 上次错误)
-- @return info[table]
-- @note info = {
--   \n   open[boolean]			是否已 bind
--   \n   port[number]			当前端口; 未 open 为 0
--   \n   host[string]			配置的绑定地址; INADDR_ANY 为 ""
--   \n   local_addr[table|nil]	回读的本地地址 `{ ip, port }`; 未 open 为 nil
--   \n   family[string]		地址族 ipv4/ipv6/dual; 缺省 ipv4
--   \n   native_fd[number|nil]	监听 fd; 未 open 为 nil
--   \n   schemes[table]		协议面数组; 空且 schemes_all 为全收
--   \n   schemes_all[boolean]	无白名单 (probe 全收)
--   \n   tls_face[string]		生效 TLS 投递面 https/rtsps/mnps/smps; 无 TLS 面为 ""
--   \n   tls_face_dropped[table]	请求了但单路未投递的 TLS 面 (多 TLS 面仍只服务一路)
--   \n   tls[boolean]			是否装载证书
--   \n   plain[boolean]		与 klbhttp/web 对齐的记录字段
--   \n   mux_opts[table]		见 :mux_opts
--   \n   kmuxlist[string|table|nil]	跨线程队列 path; 分协议时为表; 未配置为 nil
--   \n   last_code[number]		上次错误码; 无错为 0
--   \n   last_error[string]	上次错误串; 无错为 ""
--   \n }
function listen:info()
	local open = (nil ~= self._mux)
	local code = self._last_code
	if nil == code then
		code = 0
	end

	local err = ""
	if 0 ~= code then
		err = self:last_error() or ""
	end

	local local_ip, local_port = nil, nil
	if open then
		local_ip, local_port = self:local_addr()
	end

	local local_addr = nil
	if local_ip then
		local_addr = { ip = local_ip, port = local_port }
	end

	local mux = CopyMuxOpts(self._mux_opts)
	local family = mux.family or "ipv4"

	return {
		open = open,
		port = open and self:port() or 0,
		host = (self._host or ""),
		local_addr = local_addr,
		family = family,
		native_fd = open and self:native_fd() or nil,
		schemes = SchemesToList(self._schemes),
		schemes_all = (nil == self._schemes),
		tls_face = (self._tls_face or ""),
		tls_face_dropped = (self._tls_dropped or {}),
		tls = (true == self._tls),
		plain = (true == self._plain),
		mux_opts = mux,
		kmuxlist = (self._kmux_sink and self._kmux_sink.info()) or nil,
		last_code = code,
		last_error = err,
	}
end


-- @brief 开始单端口监听
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] 见内联注释
-- @return ok[boolean]					true 成功; false 失败
-- @return code[number|nil]				失败时 klb_netcode_e; 描述串用 :last_error / knetcode.strerror
function listen:open(port, opts)
	-- opts = {
	--   schemes[table]			[可选] 协议面白名单 eg. { "http", "mnps" }; 与 listen_group 同形
	--   scheme[string|table]	[可选] schemes 单数写法
	--   tls[boolean]			[可选] 是否装载证书; 含 TLS 面时可由 schemes 推导为 true
	--   plain[boolean]			[可选] 与 klbhttp/web 对齐记录; 本入口靠 schemes 区分明文/TLS 面
	--   cert[string]			tls 时证书 PEM 原文或路径
	--   key[string]			tls 时私钥 PEM 原文或路径
	--   tls_as_http[boolean]	[可选] C klisten; TLS ClientHello 按 HTTP 投递; 默认 true
	--   tls_as_mnp[boolean]	[可选] C klisten; 非 HTTP/RTSP TLS 按 MNP 投递; 默认 false
	--   tls_as_rtsp[boolean]	[可选] C klisten; 非 HTTP TLS 按 RTSP 投递; 默认 false
	--   max_connect[number]	[可选] C klisten; listen backlog; 默认 20
	--   host[string]			[可选] C klisten; 绑定地址; nil/"" → 任意地址
	--   family[string]			[可选] "ipv4"(缺省) / "ipv6" / "dual"
	--   reuseport[boolean]		[可选] SO_REUSEPORT; 平台不支持则 open 失败
	--   nodelay[boolean]		[可选] 对 accept 所得连接设 TCP_NODELAY
	--   keepalive[boolean]		[可选] 对 accept 所得连接设 SO_KEEPALIVE
	--   http[table]			[可选] 交给 httpserver.new 的会话配置
	--   rtsp[table]			[可选] 交给 rtspserver.new 的会话配置
	--   smp[table]				[可选] 交给 smpserver.new 的会话配置
	--   mnp[table]				[可选] 交给 mnpserver.new 的会话配置
	--   ws[table]				[可选] 仅 ws/wss 面自动 Upgrade 时: path / protocol
	--   ping_interval_ms[number]	[可选] 服务端 Ping 间隔 ms; 默认 15000; 0=关
	--   ping_timeout_ms[number]	[可选] 等 Pong 超时 ms; 默认 30000
	-- }

	self:close()
	self._schemes = nil
	self._mux_opts = nil
	self._tls = false
	self._plain = false
	self._raw = false
	self._host = ""
	self._kmux_sink = nil
	self._kmux_feed = false
	ClearLast(self)

	local use_opts = opts
	if not use_opts then
		use_opts = self._cfg
	end

	-- 可选 kmuxlist: 可按协议分 path; 有则强制 raw
	do
		local sink, sink_err = kmux_sink.setup(use_opts and use_opts.kmuxlist)
		if sink_err then
			SetLast(self, net.INVAL, sink_err)
			return false, net.INVAL
		end

		if sink then
			self._kmux_sink = sink
			if "table" ~= type(use_opts) then
				use_opts = { raw = true }
			else
				use_opts.raw = true
			end
		end
	end

	local schemes_in = SchemesFromOpts(use_opts)
	if schemes_in then
		local specs, parse_ok = cfger.parse_listen({
			port = port,
			schemes = schemes_in,
		})
		if not parse_ok or not specs or not specs[1] then
			SetLast(self, net.INVAL, "Invalid argument")
			return false, net.INVAL
		end

		use_opts = BuildOpenCfg(use_opts, specs[1])
		self._schemes = specs[1].schemes
	end

	self._tls_face = cfger.effective_tls_face(self._schemes)
	self._tls_dropped = cfger.dropped_tls_faces(self._schemes)
	self._raw = (use_opts and true == use_opts.raw) or false

	-- raw: 只交裸 sock, 本层不建会话 → 不校验证书 (证书由消费端 new_serve 装载)
	local http_opts, ok = nil, true
	if not self._raw then
		http_opts, ok = ResolveHttpOpts(use_opts)
		if not ok then
			SetLast(self, net.INVAL, "Invalid argument")
			return false, net.INVAL
		end
	end

	-- 始终传齐 C 三项 (缺省与 klisten 一致); 便于查询生效值
	local mux_opts = BuildMuxOpts(use_opts)

	local mux, code = klisten.listen(port, mux_opts)
	if not mux then
		SetLast(self, code)
		return false, code
	end

	self._mux = mux
	self._mux_opts = mux_opts
	self._host = mux_opts.host or ""
	self._http_opts = http_opts
	self._tls = (use_opts and true == use_opts.tls) or false
	self._plain = (use_opts and true == use_opts.plain) or false
	-- 保留整表 (含 ping_interval_ms/ping_timeout_ms) 供 new_serve 下传
	self._cfg = use_opts
	if use_opts then
		self._http_cfg = use_opts.http
		self._rtsp_cfg = use_opts.rtsp
		self._smp_cfg = use_opts.smp
		self._mnp_cfg = use_opts.mnp
		self._ws_cfg = use_opts.ws
	else
		self._http_cfg = nil
		self._rtsp_cfg = nil
		self._smp_cfg = nil
		self._mnp_cfg = nil
		self._ws_cfg = nil
	end

	-- schemes 含 ws/wss 时硬依赖 klbws (无软化); raw 模式不建会话, 无需装载
	if not self._raw and self._schemes and (self._schemes.ws or self._schemes.wss) then
		require("klbcore.klbws")
	end

	-- kmuxlist 模式: 起 feed, 按 face 入队; 脚本侧用 kmuxlist.pop 消费, 勿再 :co_accept
	if self._kmux_sink then
		StartKmuxFeed(self)
	end

	ClearLast(self)
	return true
end


-- @brief 接收一条已探测连接
-- @return face[string]					协议面 http/https/mnp/mnps/...; 退出为 nil
-- @return session[table|lightuserdata]	协议会话; raw 模式/未知协议为裸 sock (已交还)
-- @return msg[string]					"ok" / "exit" / "error"
-- @return tls[boolean]					是否 TLS ClientHello
-- @return addr[table|nil]				对端地址 `{ ip, port }`; 无连接为 nil
-- @note 须在 kco 协程内; 有 schemes 时跳过不在白名单的连接
--   \n raw 模式 (open opts.raw=true): 白名单命中即返回裸 sock, 不建会话
function listen:co_accept()
	if not self._mux then
		return nil, nil, "exit", false, nil
	end

	while true do
		-- err_code: C accept 错误时第 5 值 (P1-4); ok 路径无第 5 值
		local proto, sock, msg, tls, err_code = self._mux:co_accept()
		local is_tls = (true == tls)

		if "exit" == msg then
			return nil, nil, msg, false, nil
		end

		if not sock or "ok" ~= msg then
			if sock then
				klisten.drop(sock)
			end

			if "error" == (msg or "error") then
				local code = err_code
				if nil == code then
					code = net.ACCEPT_FAIL
				end
				SetLast(self, code)
			end

			return nil, nil, msg or "error", false, nil
		end

		if not cfger.proto_allowed(self._schemes, proto, is_tls) then
			klisten.drop(sock)
		else
			local face = cfger.face_of(proto, is_tls)
			local addr = SockAddr(sock)
			local session = nil

			-- raw: 只交裸 sock (含 TLS 标记), 由调用方 klisten.wrap 后跨线程移交
			if self._raw then
				return face, sock, msg, is_tls, addr
			end

			if "http" == proto then
				-- 勿把 open 时的 tls=true 强加给明文连接; 由 probe tls 决定
				--   \n 仅下发 cert/key 供 TLS 连接用; 明文连接走 sock->tls=false
				--   \n ws/wss: probe 仍为 http; 无 http/https 白名单时自动 Upgrade→klbws
				--   \n 同时有 http+ws 时交 httpserver, 由调用方 klbws.from_http
				local serve_opts = nil
				if self._http_opts then
					serve_opts = {
						cert = self._http_opts.cert,
						key = self._http_opts.key,
					}
				end

				local conn, serve_code = khttp.new_serve(sock, serve_opts)
				if not conn then
					SetLast(self, serve_code)
					return face, nil, "error", is_tls, addr
				end

				local http_session = httpserver.new(conn, self._http_cfg)
				local want_http = WantHttpFace(self._schemes, is_tls)
				local want_ws = WantWsFace(self._schemes, is_tls)

				if want_ws and not want_http then
					-- 仅 ws/wss: 本模块读首包并升 kws; 非 Upgrade 丢弃后继续
					local ws_session, ws_code = CoUpgradeToWs(http_session, self._ws_cfg)
					if ws_session then
						return (is_tls and "wss" or "ws"), ws_session, "ok", is_tls, addr
					end

					SetLast(self, ws_code or net.PROTO_UNEXPECTED)
					if net.PROTO_UNEXPECTED ~= ws_code
						and net.PROTO_VERSION ~= ws_code then
						return (is_tls and "wss" or "ws"), nil, "error", is_tls, addr
					end
				else
					-- 含 http/https (或全收): 交 httpserver; ws 并存时由调用方 from_http
					return face, http_session, msg, is_tls, addr
				end
			elseif "rtsp" == proto then
				-- rtsps: mux tls_as_rtsp 投递 RTSP+tls; 下发 cert/key 与 khttp 对称
				local rtsp_opts = nil
				if self._http_opts then
					rtsp_opts = {
						cert = self._http_opts.cert,
						key = self._http_opts.key,
					}
				end

				local conn, serve_code = krtsp.new_serve(sock, rtsp_opts)
				if not conn then
					SetLast(self, serve_code)
					return face, nil, "error", is_tls, addr
				end

				session = rtspserver.new(conn, self._rtsp_cfg or self._cfg)
			elseif "smp" == proto then
				-- smps: tls + 可选 ping_* (与 khttp.new_serve 对称)
				local smp_opts = ServeConnOpts(self)
				local conn, serve_code = ksmp.new_serve(sock, smp_opts)
				if not conn then
					SetLast(self, serve_code)
					return face, nil, "error", is_tls, addr
				end

				session = smpserver.new(conn, self._smp_cfg or self._cfg)
			elseif "mnp" == proto then
				local mnp_opts = ServeConnOpts(self)
				local conn, serve_code = kmnp.new_serve(sock, mnp_opts)
				if not conn then
					SetLast(self, serve_code)
					return face, nil, "error", is_tls, addr
				end

				session = mnpserver.new(conn, self._mnp_cfg or self._cfg)
			elseif "smprpc" == proto then
				-- smprpcs: tls + 可选 ping_*
				local smp_rpc_opts = ServeConnOpts(self)
				local conn, serve_code = ksmp.new_serve_rpc(sock, smp_rpc_opts)
				if not conn then
					SetLast(self, serve_code)
					return face, nil, "error", is_tls, addr
				end

				session = smpserverpcer.new_rpc(conn, self._smp_cfg or self._cfg)
			elseif "mnprpc" == proto then
				-- mnprpcs: tls + 可选 ping_*
				local mnp_rpc_opts = ServeConnOpts(self)
				local conn, serve_code = kmnp.new_serve_rpc(sock, mnp_rpc_opts)
				if not conn then
					SetLast(self, serve_code)
					return face, nil, "error", is_tls, addr
				end

				session = mnpserverpcer.new_rpc(conn, self._mnp_cfg or self._cfg)
			else
				-- unknown: 交还 sock, 由调用方 drop
				return face, sock, msg, is_tls, addr
			end

			if session then
				return face, session, msg, is_tls, addr
			end

			-- session 为空: 仅 ws/wss 自动 Upgrade 可恢复失败会落到此, 继续 accept
			if "http" ~= proto then
				SetLast(self, net.ERR)
				return face, nil, "error", is_tls, addr
			end
		end
	end
end


-- @brief 监听端口号
-- @return port[number]
function listen:port()
	if not self._mux then
		return 0
	end

	return self._mux:port()
end


-- @brief 配置的绑定地址 (INADDR_ANY 为空串)
-- @return host[string]					绑定地址; INADDR_ANY / 未 open 为 ""
function listen:host()
	return self._host or ""
end


-- @brief 回读监听本地地址 (getsockname)
-- @return ip[string|nil]				本地地址文本 (IPv4 / IPv6); 未 open 为 nil
-- @return port[number|nil]				本地端口 (主机序; port=0 时由内核分配的真实口); 未 open 为 nil
function listen:local_addr()
	if not self._mux then
		return nil, nil
	end

	return self._mux:local_addr()
end


-- @brief 监听 native fd (供 setsockopt 等)
-- @return fd[number|nil]				未 open 为 nil
function listen:native_fd()
	if not self._mux or not self._mux.native_fd then
		return nil
	end

	return self._mux:native_fd()
end


-- @brief 以新绑定地址重开监听 (同端口; 既有已 accept 连接不受影响)
-- @param [in]      host[string|nil]		新绑定地址; nil/"" → INADDR_ANY
-- @return ok[boolean]
-- @return code[number|nil]				失败时 klb_netcode_e
-- @note 重开期间新连接短暂不可达; 端口沿用当前实际绑定口 (port=0 时先回读)
function listen:rebind(host)
	if not self._mux then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	if nil ~= host and "string" ~= type(host) then
		SetLast(self, net.INVAL, "Invalid argument")
		return false, net.INVAL
	end

	local ok, code = self._mux:rebind(host)
	if not ok then
		SetLast(self, code or net.INVAL)
		return false, code or net.INVAL
	end

	local new_host = ""
	if nil ~= host and "" ~= host then
		new_host = host
	end

	self._host = new_host
	do
		local cur = CopyMuxOpts(self._mux_opts)
		if "" ~= new_host then
			cur.host = new_host
		else
			cur.host = nil
		end
		self._mux_opts = cur
	end

	ClearLast(self)
	return true
end


--------------------------------------------------------------------------------------------
-- listener 工厂


local listener = {}


-- @brief 新建共享监听模块
-- @param [in]      cfg[table]			[可选] 默认 open 选项 / RTSP 会话配置
-- @return obj[table]
listener.new = function(cfg)
	local obj = {
		_mux = nil,
		_cfg = cfg,
		_schemes = nil,
		_mux_opts = nil,
		_tls_face = nil,
		_tls_dropped = nil,
		_tls = false,
		_plain = false,
		_raw = false,
		_host = "",
		_kmux_sink = nil,					-- kmux_sink; 有则按 face 写入 kmuxlist
		_kmux_feed = false,
		_http_opts = nil,
		_http_cfg = nil,
		_rtsp_cfg = nil,
		_smp_cfg = nil,
		_mnp_cfg = nil,
		_ws_cfg = nil,
		_last_code = nil,
		_last_errstr = nil,
	}

	setmetatable(obj, {
		__index = listen,
		__tostring = function(self)
			return tostring(self._mux)
		end,
	})

	return obj
end


return listener
