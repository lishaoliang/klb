--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbrpc 监听 / 客户端连接配置
-- @note   缺省单端口多协议 (smp/mnp); http/ws 可声明但同口混用会被 serve 拒
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local kurl = require("kurl")


local cfger = {}


-- 缺省监听端口 (单口多协议; 各 scheme 共用)
local DEFAULT_PORT = 3457


-- 缺省启用的协议 (同 DEFAULT_PORT; 同口 mux 现行 RPC 面为 smp/mnp)
local DEFAULT_SCHEMES = {
	"smp",
	"mnp",
}


-- scheme → 缺省端口 (现行均 DEFAULT_PORT; 分口请显式写 port)
local SCHEME_PORT = {
	smp = DEFAULT_PORT,
	mnp = DEFAULT_PORT,
	http = DEFAULT_PORT,
	ws = DEFAULT_PORT,
}


-- 短 scheme / 完整 schema → 客户端 URL schema; 缺省端口按短 scheme
local CLIENT_SCHEMA = {
	smp = "smprpc",
	smprpc = "smprpc",
	smps = "smprpcs",
	smprpcs = "smprpcs",
	mnp = "mnprpc",
	mnprpc = "mnprpc",
	http = "httprpc",
	httprpc = "httprpc",
	https = "httprpcs",
	httprpcs = "httprpcs",
	ws = "wsrpc",
	wsrpc = "wsrpc",
	wss = "wsrpcs",
	wsrpcs = "wsrpcs",
}


local SCHEMA_SHORT = {
	smprpc = "smp",
	smprpcs = "smp",
	mnprpc = "mnp",
	httprpc = "http",
	httprpcs = "http",
	wsrpc = "ws",
	wsrpcs = "ws",
}


-- @brief 公开缺省端口
-- @return [number]
cfger.default_port = function ()
	return DEFAULT_PORT
end


-- @brief 公开缺省协议列表 (只读拷贝)
-- @return [table]
cfger.default_schemes = function ()
	local out = {}
	for i = 1, #DEFAULT_SCHEMES do
		out[i] = DEFAULT_SCHEMES[i]
	end

	return out
end


-- @brief 公开 rpctype 只接受字符串
-- @param [in]      v[string]			[可选] `LUA` 或 `JSON`; 省略为 `LUA`
-- @return [string]						`LUA` / `JSON`; 非法为 nil
local function NormalizeRpctype(v)
	if nil == v then
		return "LUA"
	end

	if "JSON" == v then
		return "JSON"
	end

	if "LUA" == v then
		return "LUA"
	end

	return nil
end


-- @brief 规范化单个 scheme 名
-- @param [in]      s[string]			smp / mnp / http / ws
-- @return [string]						小写; 非法为 nil
local function NormalizeSchemeName(s)
	if "string" ~= type(s) then
		return nil
	end

	local scheme = string.lower(s)
	if not SCHEME_PORT[scheme] then
		return nil
	end

	return scheme
end


-- @brief 取条目中的协议列表
-- @param [in]      item[table]			含 scheme / schemes
-- @return [table]						scheme 字符串数组; 非法为 nil
local function SchemesFromItem(item)
	if "table" == type(item.schemes) then
		if 0 == #item.schemes then
			return nil
		end

		return item.schemes
	end

	if "table" == type(item.scheme) then
		if 0 == #item.scheme then
			return nil
		end

		return item.scheme
	end

	if "string" == type(item.scheme) then
		return { item.scheme }
	end

	return nil
end


-- @brief 规范化一条或一组监听为扁平 specs
-- @param [in]      item[table]			`{ scheme|schemes, port, rpctype }`
-- @param [in]      fallback[string]	[可选] 条目未写 rpctype 时的默认
-- @return [table]						`{ { scheme, port, rpctype }, ... }`; 非法为 nil
local function NormalizeItem(item, fallback)
	if "table" ~= type(item) then
		return nil
	end

	local schemes = SchemesFromItem(item)
	if not schemes then
		return nil
	end

	local port = item.port
	if nil == port then
		port = DEFAULT_PORT
	end

	if "number" ~= type(port) then
		return nil
	end

	local rpctype = NormalizeRpctype(item.rpctype or fallback)
	if not rpctype then
		return nil
	end

	local specs = {}
	for i = 1, #schemes do
		local scheme = NormalizeSchemeName(schemes[i])
		if not scheme then
			return nil
		end

		specs[#specs + 1] = {
			scheme = scheme,				-- [必须] string    smp, mnp, http, ws
			port = port,					-- [必须] integer   监听端口
			rpctype = rpctype,				-- [必须] string    LUA 或 JSON
		}
	end

	return specs
end


-- @brief 未配置 listen 时的缺省规格 (单端口多协议)
-- @param [in]      rpctype[string]		[可选] 缺省 `LUA`
-- @return [table]						DEFAULT_SCHEMES @ DEFAULT_PORT
cfger.default_specs = function (rpctype)
	local specs = NormalizeItem({
		port = DEFAULT_PORT,
		schemes = DEFAULT_SCHEMES,
	}, rpctype)

	return specs or {}
end


-- @brief 是否为单端口多协议写法 (非数组项)
-- @param [in]      listen[table]
-- @return [boolean]
local function IsPortSchemesForm(listen)
	if "table" ~= type(listen) then
		return false
	end

	if "table" == type(listen[1]) then
		return false
	end

	if "table" == type(listen.schemes) then
		return true
	end

	if "table" == type(listen.scheme) then
		return true
	end

	if "string" == type(listen.scheme) then
		return true
	end

	if "number" == type(listen.port) and nil == listen.scheme and nil == listen.schemes then
		return true
	end

	return false
end


-- @brief 解析 listen
-- @param [in]      listen[table]		[可选] 单口多协议 / 单条 / 数组; nil 表示调用方自定缺省
-- @param [in]      rpctype[string]		[可选] 条目未写时的默认, 缺省 `LUA`
-- @return specs[table]					扁平 `{ scheme, port, rpctype }` 数组; 未提供 listen 时为 nil
-- @return ok[boolean]					true 合法或未提供; 提供了但非法为 false
-- @note 推荐 `{ port = 3457, schemes = { "smp", "mnp", "http", "ws" } }`. 亦可 `{ scheme = "smp" }` 或数组.
cfger.parse_listen = function (listen, rpctype)
	if nil == listen then
		return nil, true
	end

	if "table" ~= type(listen) then
		return nil, false
	end

	local fallback = rpctype
	local list

	if IsPortSchemesForm(listen) then
		local item = listen
		if nil == item.schemes and nil == item.scheme then
			item = {
				port = listen.port,
				schemes = DEFAULT_SCHEMES,
				rpctype = listen.rpctype,
			}
		end

		list = { item }
	else
		list = listen
		if "table" ~= type(listen[1]) then
			list = { listen }
		end
	end

	if 0 == #list then
		return nil, false
	end

	local specs = {}
	for i = 1, #list do
		local got = NormalizeItem(list[i], fallback)
		if not got then
			return nil, false
		end

		for j = 1, #got do
			specs[#specs + 1] = got[j]
		end
	end

	return specs, true
end


-- @brief 由规范化客户端规格拼 URL
-- @param [in]      spec[table]			`parse_client` 结果
-- @return [string]						目标 URL; 非法为 nil
cfger.build_client_url = function (spec)
	if "table" ~= type(spec) then
		return nil
	end

	if "string" == type(spec.url) and "" ~= spec.url then
		return spec.url
	end

	if "string" ~= type(spec.schema) or "string" ~= type(spec.host) then
		return nil
	end

	local path = spec.path
	if "string" ~= type(path) then
		path = ""
	end

	if "" ~= path and "/" ~= string.sub(path, 1, 1) then
		path = "/" .. path
	end

	if "number" == type(spec.port) then
		return string.format("%s://%s:%d%s", spec.schema, spec.host, spec.port, path)
	end

	return string.format("%s://%s%s", spec.schema, spec.host, path)
end


-- @brief 由 URL userinfo 拆出凭据
-- @param [in]      info[string|nil]	kurl 的 userinfo
-- @return [table|nil]					`{ user, pass }`; 空为 nil
local function AuthFromUserinfo(info)
	if "string" ~= type(info) or "" == info then
		return nil
	end

	local user, pass = string.match(info, "^(.-):(.*)$")
	if nil == user then
		user = info
		pass = ""
	end

	return { user = user, pass = pass }
end


-- @brief 规范化显式鉴权配置
-- @param [in]      auth[table]			`{ token, user, pass }`
-- @return [table|nil]					非法或空为 nil
local function NormalizeAuth(auth)
	if "table" ~= type(auth) then
		return nil
	end

	local token = auth.token
	if nil ~= token and "string" ~= type(token) then
		return nil
	end

	local user = auth.user
	if nil ~= user and "string" ~= type(user) then
		return nil
	end

	local pass = auth.pass
	if nil ~= pass and "string" ~= type(pass) then
		return nil
	end

	local has_token = "string" == type(token) and "" ~= token
	local has_user = "string" == type(user) and "" ~= user
	if not has_token and not has_user then
		return nil
	end

	return {
		token = token,					-- [可选] string    直接令牌
		user = user,					-- [可选] string    用户名
		pass = pass,					-- [可选] string    口令
	}
end


-- @brief 解析客户端连接规格 (一份目标)
-- @param [in]      cfg[table]			`{ url, timeout_ms, auth }` 或 `{ scheme, host|ip, port, path, rpctype, tls, timeout_ms, auth }`
-- @return spec[table]					规范化表; 非法为 nil
-- @return ok[boolean]					true 合法; 非法为 false
-- @note url 优先. host 可用 ip 别名. tls=true 时 smp/http/ws 升为 *s schema.
-- @note timeout_ms 为单次调用 deadline (毫秒); nil 表示不限.
-- @note auth `{ token|user, pass }` 优先; 未给时取 url 的 userinfo(`user:pass@`).
cfger.parse_client = function (cfg)
	if "table" ~= type(cfg) then
		return nil, false
	end

	local rpctype = NormalizeRpctype(cfg.rpctype)
	if not rpctype then
		return nil, false
	end

	local timeout_ms = cfg.timeout_ms
	if nil ~= timeout_ms and "number" ~= type(timeout_ms) then
		return nil, false
	end

	local auth = nil
	if nil ~= cfg.auth then
		auth = NormalizeAuth(cfg.auth)
		if nil == auth then
			return nil, false
		end
	end

	if "string" == type(cfg.url) and "" ~= cfg.url then
		local u = kurl.parse(cfg.url) or {}
		if nil == auth then
			auth = AuthFromUserinfo(u["userinfo"])
		end

		return {
			url = cfg.url,					-- [必须] string    完整目标 URL
			scheme = "",					-- [可选] string    短 scheme; url 模式可空
			schema = "",					-- [可选] string    URL schema; url 模式可空
			host = "",						-- [可选] string
			port = 0,						-- [可选] integer   0 表示未拆分
			path = "",						-- [可选] string
			rpctype = rpctype,				-- [必须] string    LUA 或 JSON
			timeout_ms = timeout_ms,		-- [可选] number    单次调用 deadline (毫秒); nil 不限
			auth = auth,					-- [可选] table     鉴权凭据; nil 不鉴权
		}, true
	end

	local scheme_in = cfg.scheme
	if "string" ~= type(scheme_in) then
		return nil, false
	end

	local key = string.lower(scheme_in)
	local schema = CLIENT_SCHEMA[key]
	if not schema then
		return nil, false
	end

	if true == cfg.tls then
		if "smprpc" == schema then
			schema = "smprpcs"
		elseif "httprpc" == schema then
			schema = "httprpcs"
		elseif "wsrpc" == schema then
			schema = "wsrpcs"
		end
	end

	local short = SCHEMA_SHORT[schema]
	local defport = SCHEME_PORT[short]
	if not defport then
		return nil, false
	end

	local host = cfg.host
	if "string" ~= type(host) or "" == host then
		host = cfg.ip
	end

	if "string" ~= type(host) or "" == host then
		return nil, false
	end

	local port = cfg.port
	if nil == port then
		port = defport
	end

	if "number" ~= type(port) then
		return nil, false
	end

	local path = cfg.path
	if nil == path then
		path = ""
	end

	if "string" ~= type(path) then
		return nil, false
	end

	return {
		url = "",							-- [可选] string    空表示由字段拼装
		scheme = short,						-- [必须] string    smp, mnp, http, ws
		schema = schema,					-- [必须] string    smprpc 等
		host = host,						-- [必须] string
		port = port,						-- [必须] integer
		path = path,						-- [可选] string
		rpctype = rpctype,					-- [必须] string    LUA 或 JSON
		timeout_ms = timeout_ms,			-- [可选] number    单次调用 deadline (毫秒); nil 不限
		auth = auth,						-- [可选] table     鉴权凭据; nil 不鉴权
	}, true
end


return cfger
