--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   cfger/init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klblisten 一组 listen 规格
-- @stability internal
-- @impl mature
-- @note   schemes 为协议面 (含 TLS 后缀); 与 tls_as_* 自由组合
--   \n transport 现行仅 tcp; udp/unix 声明会被拒 (C unix 空桩 / grouper 拒 udp)
--   \n 非公开 require 面; 请用 klbcore.klblisten
-- @history 修改历史
--		[2026-10] 创建文件
--]]


local cfger = {}


-- 明文面 / TLS 面均可单独列出
--   \n ws/wss: probe 仍为 http; 仅 ws/wss 白名单时 listener 自动 Upgrade→klbws
--   \n http+ws 并存时交 httpserver, 由调用方 klbws.from_http
local KNOWN_SCHEME = {
	http = true,
	https = true,
	ws = true,
	wss = true,
	rtsp = true,
	rtsps = true,
	smp = true,
	smps = true,
	mnp = true,
	mnps = true,
	smprpc = true,
	smprpcs = true,
	mnprpc = true,
	mnprpcs = true,
}


-- @brief 规范化 schemes 列表为 set
-- @param [in]      schemes[table]		字符串数组
-- @return [table]						`{ name = true, ... }`; 非法为 nil
local function NormalizeSchemes(schemes)
	if "table" ~= type(schemes) then
		return nil
	end

	if 0 == #schemes then
		return nil
	end

	local set = {}
	for i = 1, #schemes do
		local s = schemes[i]
		if "string" ~= type(s) then
			return nil
		end

		s = string.lower(s)
		if not KNOWN_SCHEME[s] then
			return nil
		end

		set[s] = true
	end

	return set
end


-- @brief 由 schemes 推导本口 TLS 投递面 (mux tls_as_*)
-- @param [in]      schemes[table|nil]	set; nil 用 klisten 缺省 (https 面)
-- @return [table]						`{ tls_as_http, tls_as_mnp, tls_as_rtsp }`
-- @note 同口 schemes 任意组合 (明文 + TLS 均可并存); C mux 单路 TLS 投递时优先 https/wss > rtsps > mnps > smps
--   \n eg. `{ http, mnp, mnps }` → tls_as_http=false, tls_as_mnp=true
--   \n eg. `{ rtsp, rtsps }` → tls_as_rtsp=true (TLS 投递 RTSP)
local function MuxOptsFromSchemes(schemes)
	if nil == schemes then
		return {
			tls_as_http = true,
			tls_as_mnp = false,
			tls_as_rtsp = false,
		}
	end

	local want_https = schemes.https or schemes.wss
	local want_mnps = schemes.mnps or schemes.mnprpcs
	local want_smps = schemes.smps or schemes.smprpcs
	local want_rtsps = schemes.rtsps

	if want_https then
		return {
			tls_as_http = true,
			tls_as_mnp = false,
			tls_as_rtsp = false,
		}
	end

	if want_rtsps then
		return {
			tls_as_http = false,
			tls_as_mnp = false,
			tls_as_rtsp = true,
		}
	end

	if want_mnps then
		return {
			tls_as_http = false,
			tls_as_mnp = true,
			tls_as_rtsp = false,
		}
	end

	if want_smps then
		return {
			tls_as_http = false,
			tls_as_mnp = false,
			tls_as_rtsp = false,
		}
	end

	-- 仅明文面: TLS 仍会落到默认 smps 面, 再由白名单丢掉
	return {
		tls_as_http = false,
		tls_as_mnp = false,
		tls_as_rtsp = false,
	}
end


-- @brief 是否需要装载证书 (本口含任一 TLS 面)
-- @param [in]      schemes[table|nil]
-- @return [boolean]
local function NeedsTlsCert(schemes)
	if nil == schemes then
		return false
	end

	if schemes.https or schemes.wss or schemes.mnps or schemes.smps then
		return true
	end

	if schemes.mnprpcs or schemes.smprpcs or schemes.rtsps then
		return true
	end

	return false
end


-- @brief schemes → 请求的 TLS 投递面组 (C mux 优先序; https/wss 同属一路)
-- @param [in]      schemes[table|nil]	set; nil 表示无白名单 (klisten 缺省 https 面)
-- @return [table]						组名数组 (https/rtsps/mnps/smps)
-- @note 同组视为同一路 TLS; 多组并存时仅优先序第一个被 C mux 投递 (见 :info().tls_face_dropped)
local function RequestedTlsFaces(schemes)
	if nil == schemes then
		return { "https" }
	end

	local list = {}
	if schemes.https or schemes.wss then
		list[#list + 1] = "https"
	end

	if schemes.rtsps then
		list[#list + 1] = "rtsps"
	end

	if schemes.mnps or schemes.mnprpcs then
		list[#list + 1] = "mnps"
	end

	if schemes.smps or schemes.smprpcs then
		list[#list + 1] = "smps"
	end

	return list
end


-- @brief 生效的 TLS 投递面 (C mux 实际投递的那一路)
-- @param [in]      schemes[table|nil]	set
-- @return face[string|nil]				https/rtsps/mnps/smps; 无 TLS 面为 nil
local function EffectiveTlsFace(schemes)
	local list = RequestedTlsFaces(schemes)
	return list[1]
end


-- @brief 请求了但 C mux 未投递的 TLS 面 (单路限制; 会静默丢弃)
-- @param [in]      schemes[table|nil]	set
-- @return [table]						面名数组; 无丢弃为空表
local function DroppedTlsFaces(schemes)
	local list = RequestedTlsFaces(schemes)
	local out = {}
	for i = 2, #list do
		out[#out + 1] = list[i]
	end

	return out
end


-- @brief 规范化一条 listen
-- @param [in]      item[table]			`{ port, schemes|scheme, transport, host, raw }`
-- @return [table]						规范化条目; 非法为 nil
-- @note raw=true: 白名单命中后不建会话, co_accept 直接交裸 sock (跨线程移交)
local function NormalizeItem(item)
	if "table" ~= type(item) then
		return nil
	end

	local transport = item.transport
	if nil == transport then
		transport = "tcp"
	end

	if "string" ~= type(transport) then
		return nil
	end

	transport = string.lower(transport)
	-- 现行仅 tcp; udp/unix 收口拒收
	if "tcp" ~= transport then
		return nil
	end

	-- 绑定地址; nil/"" 表示 INADDR_ANY (与 C 缺省一致)
	local host = item.host
	if nil ~= host and "string" ~= type(host) then
		return nil
	end

	if nil ~= host and "" == host then
		host = nil
	end

	local schemes_in = item.schemes
	if nil == schemes_in and "table" == type(item.scheme) then
		schemes_in = item.scheme
	elseif nil == schemes_in and "string" == type(item.scheme) then
		schemes_in = { item.scheme }
	end

	local schemes = nil
	if nil ~= schemes_in then
		schemes = NormalizeSchemes(schemes_in)
		if not schemes then
			return nil
		end
	end

	local mux_opts = MuxOptsFromSchemes(schemes)

	local port = item.port
	if "number" ~= type(port) then
		return nil
	end

	return {
		transport = "tcp",					-- [必须] string    现行仅 tcp
		port = port,						-- [必须] integer   tcp 端口
		path = "",							-- [可选] string    (unix 未实现)
		host = host,						-- [可选] string    绑定地址; nil 表示 INADDR_ANY
		schemes = schemes,					-- [可选] set       nil 表示本口全收
		mux_opts = mux_opts,				-- [必须] table     tls_as_http / tls_as_mnp / tls_as_rtsp
		need_tls = NeedsTlsCert(schemes),	-- [必须] boolean
		raw = (true == item.raw),			-- [必须] boolean   true: 交裸 sock, 不建会话
	}
end


-- @brief 解析一组 listen
-- @param [in]      listen[table]		一条或数组
-- @return specs[table]					规范化数组; 非法为 nil
-- @return ok[boolean]
-- @note eg. `{ port = 80, schemes = { "http", "mnp", "mnps" } }` 明文 http/mnp + TLS mnps
cfger.parse_listen = function (listen)
	if "table" ~= type(listen) then
		return nil, false
	end

	local list = listen
	if "table" ~= type(listen[1]) then
		list = { listen }
	end

	if 0 == #list then
		return nil, false
	end

	local specs = {}
	for i = 1, #list do
		local item = NormalizeItem(list[i])
		if not item then
			return nil, false
		end

		specs[#specs + 1] = item
	end

	return specs, true
end


-- @brief probe proto + tls → 协议面名
-- @param [in]      proto[string]		klisten 名 http/mnp/smp/...
-- @param [in]      tls[boolean]		是否 TLS ClientHello
-- @return [string]						http/https/mnp/mnps/...
cfger.face_of = function (proto, tls)
	if "string" ~= type(proto) then
		return ""
	end

	local p = string.lower(proto)
	local is_tls = (true == tls)

	if "http" == p then
		if is_tls then
			return "https"
		end

		return "http"
	end

	if "mnp" == p then
		if is_tls then
			return "mnps"
		end

		return "mnp"
	end

	if "smp" == p then
		if is_tls then
			return "smps"
		end

		return "smp"
	end

	if "mnprpc" == p then
		if is_tls then
			return "mnprpcs"
		end

		return "mnprpc"
	end

	if "smprpc" == p then
		if is_tls then
			return "smprpcs"
		end

		return "smprpc"
	end

	if "rtsp" == p then
		if is_tls then
			return "rtsps"
		end

		return "rtsp"
	end

	return p
end


-- @brief 协议面是否在本口 schemes 白名单内
-- @param [in]      schemes[table|nil]	set; nil 表示全收
-- @param [in]      proto[string]		klisten probe 名
-- @param [in]      tls[boolean]		是否 TLS
-- @return [boolean]
cfger.proto_allowed = function (schemes, proto, tls)
	if nil == schemes then
		return true
	end

	local face = cfger.face_of(proto, tls)
	if "" == face then
		return false
	end

	if schemes[face] then
		return true
	end

	-- ws/wss 现行 probe 仍为 http/https 面
	if "http" == face and schemes.ws then
		return true
	end

	if "https" == face and schemes.wss then
		return true
	end

	-- 短 scheme 覆盖同族 RPC 明文/TLS 面
	if "smprpc" == face and schemes.smp then
		return true
	end

	if "smprpcs" == face and schemes.smps then
		return true
	end

	if "mnprpc" == face and schemes.mnp then
		return true
	end

	if "mnprpcs" == face and schemes.mnps then
		return true
	end

	return false
end


cfger.mux_opts_from_schemes = MuxOptsFromSchemes
cfger.needs_tls_cert = NeedsTlsCert
cfger.effective_tls_face = EffectiveTlsFace
cfger.dropped_tls_faces = DroppedTlsFaces


return cfger
