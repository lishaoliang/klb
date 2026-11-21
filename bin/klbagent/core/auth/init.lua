--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  授权模块: 原始 token(ALUA_SERVE_TOKEN) + 临时 token(日常简称 token)
-- @note   临时表仅 serve 进程内; 仅供 ssid (serve/owner) 内调; 禁旁路直读表
--   \n 原始 token: 仅签发授权 + 自举升级; 线上必须 TLS (Todo.2026-34)
--   \n 对外工具/CLI 已废除 auth.*; 使用面唯一入口 = ssid.*
-- @history 修改历史
--  \n [2026-10] 创建文件 (Todo.2026-33 / M4-M6 P0+P1); 原始 token TLS 强制 + 工具白名单 (Todo.2026-34 / D50); 临时 token profile/caps 裁剪工具面 (Todo.2026-33 P2 / D52); embedded: 申请临时 token 直接复用原始 token (免签发/免 TLS); embedded: 原始 token 档位/业务面同临时 token
--  \n [2026-11] 删除 auth.* 直接工具白名单项; raw 面改放行 ssid.*
--  \n [2026-11] raw 白名单去掉 runtime.session_* (对外工具已删)
--]]


local profiles = require("klbagent.core.profiles")


local M = {}


-- 记名: ALUA_SERVE_TOKEN = 原始 token; 临时 token = 日常 token
M.TTL_MS = 6 * 60 * 60 * 1000
M.SWEEP_MS = 10 * 60 * 1000


-- 鉴权结果码 (导出接口第二返回值; 成功为 M.AUTH_OK)
-- 文本一律经 auth_err_name 派生; 客户端据码决定是否用原始 token 换临时 token
M.AUTH_OK = 0
M.AUTH_ERR_NEED_TOKEN = 1
M.AUTH_ERR_BAD_TOKEN = 2
M.AUTH_ERR_EXPIRED = 3
M.AUTH_ERR_RAW_TLS = 4
M.AUTH_ERR_BAD_PROFILE = 5
M.AUTH_ERR_NEED_RAW = 6
M.AUTH_ERR_RAISE_DENIED = 7
M.AUTH_ERR_CAPS_DENIED = 8
M.AUTH_ERR_BAD_TOOL = 9
M.AUTH_ERR_KCO_MISSING = 10
M.AUTH_ERR_BAD_ARGS = 11
M.AUTH_ERR_NEED_CLIENT_ID = 12
M.AUTH_ERR_NEED_TARGET = 13
M.AUTH_ERR_REVOKE_DENIED = 14
M.AUTH_ERR_CSPRNG = 15
M.AUTH_ERR_BAD_ROOT_TOKEN = 16
M.AUTH_ERR_SSID_UNAVAIL = 17


-- 内部签发授权标记: 仅 ssid.serve 构造 (已验明调用方身份)
--   \n 禁经 LPC/XPC 透传; 服务派发期剥除, 防外部伪造 auth.issue 授权
M.ISSUED_BY_SSID = "ssid.owner"


local AUTH_ERR_NAME = {
	[M.AUTH_OK] = "OK",
	[M.AUTH_ERR_NEED_TOKEN] = "need token",
	[M.AUTH_ERR_BAD_TOKEN] = "bad token",
	[M.AUTH_ERR_EXPIRED] = "token expired",
	[M.AUTH_ERR_RAW_TLS] = "raw token requires tls",
	[M.AUTH_ERR_BAD_PROFILE] = "bad profile",
	[M.AUTH_ERR_NEED_RAW] = "need raw auth (root)",
	[M.AUTH_ERR_RAISE_DENIED] = "temp token cannot raise profile",
	[M.AUTH_ERR_CAPS_DENIED] = "caps denied",
	[M.AUTH_ERR_BAD_TOOL] = "bad tool name",
	[M.AUTH_ERR_KCO_MISSING] = "kco unavailable",
	[M.AUTH_ERR_BAD_ARGS] = "bad args",
	[M.AUTH_ERR_NEED_CLIENT_ID] = "need client_id",
	[M.AUTH_ERR_NEED_TARGET] = "need token or client_id",
	[M.AUTH_ERR_REVOKE_DENIED] = "revoke denied",
	[M.AUTH_ERR_CSPRNG] = "csprng unavailable",
	[M.AUTH_ERR_BAD_ROOT_TOKEN] = "bad root_token",
	[M.AUTH_ERR_SSID_UNAVAIL] = "ssid client unavailable",
}


-- @brief 结果码转文本 (入口拒绝 message / 日志 / 工具报错共用)
-- @param [in] code[number]				auth.AUTH_OK / AUTH_ERR_*
-- @return name[string]				未知码为 "unknown"
M.auth_err_name = function (code)
	if type(code) ~= "number" then
		return "unknown"
	end

	return AUTH_ERR_NAME[code] or "unknown"
end


-- 原始 token 允许的工具 (ssid 签发面 + 自举升级 + 门禁/探活最小集)
local RAW_TOOL_ALLOW = {
	["ssid.ensure"] = true,
	["ssid.begin_ctx"] = true,
	["ssid.whoami"] = true,
	["ssid.status"] = true,
	["ssid.gc"] = true,
	["deploy.pull_install"] = true,
	["deploy.self_install"] = true,
	["deploy.source_install"] = true,
	["deploy.scripts_install"] = true,
	["deploy.win_install"] = true,
	["deploy.setup_priv"] = true,
	["runtime.ai_bootstrap"] = true,
	["runtime.ai_load_ack"] = true,
	["runtime.ping"] = true,
	["runtime.version"] = true,
	["runtime.hostinfo"] = true,
	["runtime.selfcheck"] = true,
}


-- token -> row
local g_temp = {}
-- client_id -> token (一客户端一活跃临时凭证)
local g_by_client = {}
-- coroutine.running() -> 本次请求鉴权摘要
local g_req = {}
local g_sweep_started = false


local function _now_ms()
	return math.floor(os.time() * 1000)
end


local function _token_eq(a, b)
	if type(a) ~= "string" or type(b) ~= "string" then
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

	return diff == 0
end


local function _raw_expect()
	-- core.serve.token_expect 包外已收口
	return nil
end


-- @brief 本机形态 (hostinfo.probe 有缓存, 仅首读探一次)
-- @return kind[string]			推不出时为空串
local function _host_kind()
	local ok_h, hi = pcall(require, "klbagent.core.hostinfo")
	if not ok_h or type(hi) ~= "table" or type(hi.probe) ~= "function" then
		return ""
	end

	local ok_p, info = pcall(hi.probe)
	if not ok_p or type(info) ~= "table" or type(info.host_kind) ~= "string" then
		return ""
	end

	return info.host_kind
end


-- @brief 是否嵌入式 (原始 token 直作临时 token 的条件)
-- @return ok[boolean]
M.host_is_embedded = function ()
	return _host_kind() == "embedded"
end


-- @brief 16B 随机字节转临时 token 文本 (t. + hex)
-- @param [in] raw[string|nil]
-- @return token[string|nil]
local function _hex_token(raw)
	if type(raw) ~= "string" or #raw ~= 16 then
		return nil
	end

	local parts = { "t." }
	for i = 1, 16 do
		parts[#parts + 1] = string.format("%02x", string.byte(raw, i))
	end

	return table.concat(parts)
end


-- @brief 生成临时 token (仅 CSPRNG; 不可用则拒发)
-- @return token[string|nil]
-- @return err[string|nil]	CSPRNG 不可用时说明替代方案
local function _rand_token()
	-- 优先 C 侧 CSPRNG (krand.rand_bytes: linux /dev/urandom, windows rand_s)
	local ok_k, krand = pcall(require, "krand")
	if ok_k and type(krand) == "table" and type(krand.rand_bytes) == "function" then
		local ok_b, raw = pcall(krand.rand_bytes, 16)
		if ok_b then
			local tok = _hex_token(raw)
			if tok ~= nil then
				return tok
			end
		end
	end

	-- 回退 /dev/urandom (旧绑定无 krand.rand_bytes 时); 不可用则拒发 (禁时间种子回退)
	local ok_f, f = pcall(io.open, "/dev/urandom", "rb")
	if ok_f and f ~= nil then
		local raw = f:read(16)
		f:close()
		local tok = _hex_token(raw)
		if tok ~= nil then
			return tok
		end
	end

	return nil, "csprng unavailable (krand.rand_bytes / /dev/urandom); use raw token (ALUA_SERVE_TOKEN)"
end


local function _drop_token(tok)
	local row = g_temp[tok]
	if row == nil then
		return
	end

	g_temp[tok] = nil
	if type(row.client_id) == "string" and g_by_client[row.client_id] == tok then
		g_by_client[row.client_id] = nil
	end
end


-- @brief 扫过期临时 token
-- @return removed[number]
M.sweep = function ()
	local now = _now_ms()
	local n = 0
	local dead = {}
	for tok, row in pairs(g_temp) do
		if type(row) ~= "table" or type(row.expire_ms) ~= "number" or row.expire_ms <= now then
			dead[#dead + 1] = tok
		end
	end

	for i = 1, #dead do
		_drop_token(dead[i])
		n = n + 1
	end

	return n
end


-- @brief 启动 10 分钟生命周期扫描 (幂等; 须在 kco 环境)
-- @return ok[boolean]
-- @return code[number]		AUTH_OK / AUTH_ERR_KCO_MISSING
M.ensure_sweeper = function ()
	if g_sweep_started then
		return true, M.AUTH_OK
	end

	local ok_c, kco = pcall(require, "kco")
	if not ok_c or type(kco) ~= "table" or type(kco.fork) ~= "function" then
		return false, M.AUTH_ERR_KCO_MISSING
	end

	g_sweep_started = true
	kco.fork(function ()
		local kenv = require("kenv")
		while not kenv.is_exit() do
			pcall(M.sweep)
			if type(kco.co_sleep) == "function" then
				kco.co_sleep(M.SWEEP_MS)
			else
				break
			end
		end
		g_sweep_started = false
	end)

	return true, M.AUTH_OK
end


-- @brief 原始 token (ALUA_SERVE_TOKEN / serve.env)
M.raw_token = function ()
	return _raw_expect()
end


M.is_raw = function (tok)
	local expect = _raw_expect()
	if expect == nil then
		return false
	end

	return _token_eq(tok, expect)
end


-- @brief 绑定本协程请求鉴权上下文 (serve 入口调用)
M.set_request_auth = function (info)
	local co = coroutine.running()
	if co == nil then
		return
	end

	if info == nil then
		g_req[co] = nil
		return
	end

	g_req[co] = info
end


M.clear_request_auth = function ()
	local co = coroutine.running()
	if co ~= nil then
		g_req[co] = nil
	end
end


M.request_auth = function ()
	local co = coroutine.running()
	if co == nil then
		return nil
	end

	return g_req[co]
end


-- @brief 传输面是否 TLS (proto 名或 boolean)
-- @param [in] via[string|boolean|nil]  mnps/https/true …
-- @return ok[boolean]
M.is_tls_transport = function (via)
	if via == true then
		return true
	end

	if type(via) ~= "string" or via == "" then
		return false
	end

	local p = string.lower(via)
	if p == "https" or p == "wss" then
		return true
	end

	if p == "mnps" or p == "smps" then
		return true
	end

	if p == "mnprpcs" or p == "smprpcs" then
		return true
	end

	return false
end


-- @brief 原始 token 会话是否允许调用该工具
-- @param [in] name[string]
-- @return ok[boolean]
M.allows_raw_tool = function (name)
	if type(name) ~= "string" or name == "" then
		return false
	end

	if RAW_TOOL_ALLOW[name] == true then
		return true
	end

	-- 前缀: deploy.* 自举族
	if string.sub(name, 1, 7) == "deploy." then
		return true
	end

	return false
end


-- @brief 工具名是否命中 allow 列表 (精确或 "prefix." 前缀)
-- @param [in] name[string]
-- @param [in] list[table]
-- @return ok[boolean]
local function _name_in_allow(name, list)
	if type(list) ~= "table" or #list == 0 then
		return false
	end

	for i = 1, #list do
		local pat = list[i]
		if type(pat) == "string" and pat ~= "" then
			if pat == name then
				return true
			end

			-- "fs." / "runtime." 前缀放行整族
			if string.sub(pat, -1) == "." and string.sub(name, 1, #pat) == pat then
				return true
			end
		end
	end

	return false
end


-- @brief 临时 token caps 是否允许该工具
-- @note   caps 空/nil = 不额外裁剪 (仅 profile); categories/tools 为允许列表
-- @param [in] caps[table|nil]  `{ categories={...}, tools={...} }`
-- @param [in] name[string]
-- @param [in] category[string|nil]
-- @return ok[boolean]
-- @return code[number]		AUTH_OK / AUTH_ERR_BAD_TOOL / AUTH_ERR_CAPS_DENIED
M.allows_caps = function (caps, name, category)
	if type(caps) ~= "table" then
		return true, M.AUTH_OK
	end

	if type(name) ~= "string" or name == "" then
		return false, M.AUTH_ERR_BAD_TOOL
	end

	local cats = caps.categories
	local tools = caps.tools
	local has_cats = type(cats) == "table" and #cats > 0
	local has_tools = type(tools) == "table" and #tools > 0
	if not has_cats and not has_tools then
		return true, M.AUTH_OK
	end

	if has_tools and _name_in_allow(name, tools) then
		return true, M.AUTH_OK
	end

	if has_cats and type(category) == "string" and category ~= "" then
		for i = 1, #cats do
			if cats[i] == category then
				return true, M.AUTH_OK
			end
		end
	end

	if has_tools or has_cats then
		return false, M.AUTH_ERR_CAPS_DENIED
	end

	return true, M.AUTH_OK
end


-- @brief 请求鉴权下的生效档 (temp 取 min(进程档, 签发档); 无绑定则进程档)
-- @return profile[string]
M.effective_profile = function ()
	local active = profiles.active()
	local ra = M.request_auth()
	if type(ra) ~= "table" then
		return active
	end

	if ra.kind == "temp" and type(ra.profile) == "string" and ra.profile ~= "" then
		return profiles.min_of(active, ra.profile)
	end

	-- local (stdio / lpc 桥): 按绑定档裁剪 (不高于进程档)
	if ra.kind == "local" and profiles.is_profile(ra.profile) then
		return profiles.min_of(active, ra.profile)
	end

	-- embedded: 原始 token 直作临时 token, 档位按 raw 自身 (verify 已判 unsafe)
	if ra.kind == "raw" and M.host_is_embedded() and profiles.is_profile(ra.profile) then
		return ra.profile
	end

	-- raw: 工具面另由 allows_raw_tool 裁; 档位仍用进程档 (签发面常需 unsafe)
	return active
end


-- @brief 是否允许调整生效档 (raw/local/本机 CLI; 临时 token 仅平级或降级)
-- @param [in] want[string]			目标档
-- @return ok[boolean]
-- @return code[number]		AUTH_OK / AUTH_ERR_BAD_PROFILE / AUTH_ERR_RAISE_DENIED / AUTH_ERR_NEED_RAW
M.authorize_raise = function (want)
	if not profiles.is_profile(want) then
		return false, M.AUTH_ERR_BAD_PROFILE
	end

	local ra = M.request_auth()
	if type(ra) ~= "table" then
		-- 无请求上下文: 本机 CLI 本地信任
		return true, M.AUTH_OK
	end

	if ra.kind == "raw" or ra.kind == "local" then
		return true, M.AUTH_OK
	end

	if ra.kind == "temp" then
		local caller = M.effective_profile()
		if profiles.rank(want) > profiles.rank(caller) then
			return false, M.AUTH_ERR_RAISE_DENIED
		end

		return true, M.AUTH_OK
	end

	return false, M.AUTH_ERR_NEED_RAW
end


-- @brief 校验 token; 临时 token 成功则滑动续期 6h
-- @param [in] tok[string]
-- @param [in] opts[table|nil]  `{ slide=true|false, tls=true|false|proto }` 线上 raw 须 tls
-- @return info[table|nil]  `{ ok, kind=raw|temp, tls?, client_id?, profile?, caps? }`; 失败为 nil
-- @return code[number]    `AUTH_OK` 或 `AUTH_ERR_*`; 文本用 auth_err_name(code)
M.verify = function (tok, opts)
	if type(tok) ~= "string" or tok == "" then
		return nil, M.AUTH_ERR_NEED_TOKEN
	end

	local slide = true
	local tls_hint = nil
	if type(opts) == "table" then
		if opts.slide == false then
			slide = false
		end

		tls_hint = opts.tls
	end

	if M.is_raw(tok) then
		-- 未传 tls 时不在此拒 (本机 / ssid 内调); 线上入口须显式 tls=
		-- embedded: 原始 token 直作临时 token, 不强制 TLS
		if tls_hint ~= nil and not M.is_tls_transport(tls_hint) and not M.host_is_embedded() then
			return nil, M.AUTH_ERR_RAW_TLS
		end

		local tls_ok = false
		if tls_hint ~= nil then
			tls_ok = M.is_tls_transport(tls_hint)
		end

		return {
			ok = true,
			kind = "raw",
			tls = tls_ok,
			client_id = nil,
			profile = "unsafe",
			caps = nil,
		}, M.AUTH_OK
	end

	M.sweep()
	local row = g_temp[tok]
	if row == nil then
		return nil, M.AUTH_ERR_BAD_TOKEN
	end

	local now = _now_ms()
	if type(row.expire_ms) ~= "number" or row.expire_ms <= now then
		_drop_token(tok)
		return nil, M.AUTH_ERR_EXPIRED
	end

	if slide then
		row.last_use_ms = now
		row.expire_ms = now + M.TTL_MS
	end

	local tls_ok = false
	if tls_hint ~= nil then
		tls_ok = M.is_tls_transport(tls_hint)
	end

	return {
		ok = true,
		kind = "temp",
		tls = tls_ok,
		client_id = row.client_id,
		profile = row.profile,
		caps = row.caps,
		expire_ms = row.expire_ms,
	}, M.AUTH_OK
end


-- @brief 是否允许签发/管理 (当前 raw 鉴权; 或无请求上下文且本机持有原始)
-- @return ok[boolean]
-- @return code[number]		AUTH_OK / AUTH_ERR_RAW_TLS / AUTH_ERR_BAD_ROOT_TOKEN / AUTH_ERR_NEED_RAW
local function _may_issue(args)
	-- 内部签发面: 裁决在 ssid.serve (已验明身份); 与 XPC 禁明文互不牵连
	if type(args) == "table" and args.issued_by == M.ISSUED_BY_SSID then
		return true, M.AUTH_OK
	end

	local ra = M.request_auth()
	if type(ra) == "table" then
		if ra.kind == "raw" then
			-- 线上 raw 须已 TLS (入口已拒明文); via=lpc 本机无 tls 字段时放行
			-- embedded: 原始 token 直作临时 token, 不强制 TLS
			if ra.tls == false and not M.host_is_embedded() then
				return false, M.AUTH_ERR_RAW_TLS
			end

			return true, M.AUTH_OK
		end

		-- local (stdio / lpc 桥): 本地信任
		if ra.kind == "local" then
			return true, M.AUTH_OK
		end

		-- 已绑定非 raw 请求: 仅显式 root_token (不鼓励; 且须 TLS)
		if type(args) == "table" and type(args.root_token) == "string" and args.root_token ~= "" then
			if not M.is_raw(args.root_token) then
				return false, M.AUTH_ERR_BAD_ROOT_TOKEN
			end

			if ra.tls ~= true then
				return false, M.AUTH_ERR_RAW_TLS
			end

			return true, M.AUTH_OK
		end

		return false, M.AUTH_ERR_NEED_RAW
	end

	if type(args) == "table" and type(args.root_token) == "string" and args.root_token ~= "" then
		if M.is_raw(args.root_token) then
			return true, M.AUTH_OK
		end

		return false, M.AUTH_ERR_BAD_ROOT_TOKEN
	end

	-- 无请求上下文: 本机 CLI / 同机工具 (serve 进程内已绑定请求时勿走此支)
	if _raw_expect() ~= nil then
		return true, M.AUTH_OK
	end

	return false, M.AUTH_ERR_NEED_RAW
end


-- @brief 签发临时 token
-- @param [in] args[table]  client_id, profile?, caps?, root_token?, issued_by?
-- @note issued_by = M.ISSUED_BY_SSID 为 ssid 内部裁决, 禁由 LPC/XPC 请求体传入
-- @return result[table|nil]	失败为 nil
-- @return code[number]		AUTH_OK / AUTH_ERR_*
M.issue = function (args)
	if type(args) ~= "table" then
		return nil, M.AUTH_ERR_BAD_ARGS
	end

	local client_id = args.client_id or args.client
	if type(client_id) ~= "string" or client_id == "" then
		return nil, M.AUTH_ERR_NEED_CLIENT_ID
	end

	local ok_i, code_i = _may_issue(args)
	if not ok_i then
		return nil, code_i
	end

	local profile = args.profile
	if profile == nil or profile == "" then
		profile = "net"
	end

	if not profiles.is_profile(profile) then
		return nil, M.AUTH_ERR_BAD_PROFILE
	end

	local caps = nil
	if type(args.caps) == "table" then
		caps = args.caps
	end

	-- embedded: 原始 token 直作临时 token (免签发/免过期; PC 知晓后可直接工作)
	-- 复用原始 token 时 verify 返回 kind=raw/profile=unsafe/caps=nil, 故签发档与 caps 不参与裁剪;
	-- 这里如实回报 effective_profile=unsafe, 避免调用方误以为已最小化权限
	if M.host_is_embedded() then
		local raw = _raw_expect()
		if type(raw) == "string" and raw ~= "" then
			return {
				ok = true,
				token = raw,
				client_id = client_id,
				profile = "unsafe",
				caps = nil,
				requested_profile = profile,
				effective_profile = "unsafe",
				degraded = "embedded_reuses_raw_token",
				ttl_ms = 0,				-- 0 = 不过期 (复用原始 token)
				expire_ms = 0,
				kind = "temp",
			}, M.AUTH_OK
		end
	end

	-- 同客户端替换旧临时 token
	local old = g_by_client[client_id]
	if type(old) == "string" then
		_drop_token(old)
	end

	local tok = _rand_token()
	if type(tok) ~= "string" or tok == "" then
		return nil, M.AUTH_ERR_CSPRNG
	end

	local now = _now_ms()
	local row = {
		client_id = client_id,
		profile = profile,
		caps = caps,
		issued_ms = now,
		last_use_ms = now,
		expire_ms = now + M.TTL_MS,
	}
	g_temp[tok] = row
	g_by_client[client_id] = tok

	return {
		ok = true,
		token = tok,
		client_id = client_id,
		profile = profile,
		caps = caps,
		ttl_ms = M.TTL_MS,
		expire_ms = row.expire_ms,
		kind = "temp",
	}, M.AUTH_OK
end


-- @brief 吊销: token 或 client_id
-- @param [in] args[table]  token 或 client_id (二选一)
-- @return result[table|nil]	失败为 nil
-- @return code[number]		AUTH_OK / AUTH_ERR_*
M.revoke = function (args)
	if type(args) ~= "table" then
		return nil, M.AUTH_ERR_BAD_ARGS
	end

	local ok_i, code_i = _may_issue(args)
	if not ok_i then
		-- 亦允许本人用临时 token 吊销自己
		local ra = M.request_auth()
		if type(ra) ~= "table" or ra.kind ~= "temp" then
			return nil, code_i
		end

		if type(args.token) == "string" and args.token ~= "" then
			local row = g_temp[args.token]
			if row == nil or row.client_id ~= ra.client_id then
				return nil, M.AUTH_ERR_REVOKE_DENIED
			end
		elseif type(args.client_id) == "string" and args.client_id ~= ra.client_id then
			return nil, M.AUTH_ERR_REVOKE_DENIED
		end
	end

	local n = 0
	if type(args.token) == "string" and args.token ~= "" then
		if g_temp[args.token] ~= nil then
			_drop_token(args.token)
			n = 1
		end
	elseif type(args.client_id) == "string" and args.client_id ~= "" then
		local tok = g_by_client[args.client_id]
		if type(tok) == "string" then
			_drop_token(tok)
			n = 1
		end
	else
		return nil, M.AUTH_ERR_NEED_TARGET
	end

	return { ok = true, revoked = n }, M.AUTH_OK
end


-- @brief 脱敏统计 (禁真值)
M.status = function ()
	M.sweep()
	local n = 0
	local clients = {}
	for _, row in pairs(g_temp) do
		n = n + 1
		if type(row) == "table" and type(row.client_id) == "string" then
			clients[#clients + 1] = {
				client_id = row.client_id,
				profile = row.profile,
				expire_ms = row.expire_ms,
				last_use_ms = row.last_use_ms,
			}
		end
	end

	return {
		raw_configured = (_raw_expect() ~= nil),
		temp_count = n,
		ttl_ms = M.TTL_MS,
		sweep_ms = M.SWEEP_MS,
		sweep_running = g_sweep_started,
		clients = clients,
	}
end


-- 测试/内部: 清空临时表 (禁生产路径依赖)
M._reset_for_test = function ()
	g_temp = {}
	g_by_client = {}
	g_req = {}
end


return M

-- end
