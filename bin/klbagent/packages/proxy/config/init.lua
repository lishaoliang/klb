--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   config.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy 链配置读写
-- @note   默认空链; 读盘 schema 见 docs/tools/proxy.md
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); 入口鉴权 validate / apply_entry_auth; N1-2 listen.host / ALUA_ENTRY_HOST; LAN 须鉴权; N1-3 routes 目的地选链 / no_proxy; N2-3 no_proxy 与 util/ALUA_NO_PROXY 统一; 无 path/env 时读 pref/proxy.json; pref 路径优先 core.config.proxy.load (含 live); 链 id 随机6位; default/debug 角色; 禁 chains.default 字面 key; 去掉 active_chain; 临时切链用 proxy.use (proxy_use.json; 启动清); policy.mode blacklist|whitelist; 空 deny=全代; deny>allow
--]]


local M = {}


-- @brief 默认配置 (空链; 仅本机 listen; 无鉴权)
-- @return [table]
M.defaults = function ()
	return {
		listen = {
			host = "127.0.0.1",
			port = 12818,
			protocols = { "socks5", "http-connect", "alua-tunnel" },
		},
		auth = {
			mode = "none",
			-- mode=userpass: user/pass 或 users[]/{[user]=pass}
			-- mode=bearer: token=
		},
		-- 出站/hop TLS 默认 (M1); hop.tls 可覆盖
		-- verify=false 兼容历史 VERIFY_NONE; 生产 hop 宜 verify=true + ca_pem
		tls = {
			verify = false,
			sni = true,
			-- ca_pem = "...",
			-- alpn = { "http/1.1" },
			-- pin_sha256 = "64hex",
		},
		-- 全阶段超时 (ms); 业务/链/泵读取 M.timeouts(cfg)
		timeouts = {
			connect_ms = 15000,
			probe_ms = 5000,
			idle_ms = 100000,
			poll_ms = 40,
			accept_ms = 200,
			-- M2: hop 长连无入口请求后客户端自断 (默认 1h); 0=关闭暖池
			hop_idle_ms = 3600000,
		},
		-- M2: proxy↔alua 鉴权暖会话池 (每 hop key)
		hop_pool = {
			max = 2,
		},
		-- listen 强化: backlog / 端口占用回退次数 (不含 IPv6)
		-- listen.backlog / listen.port_fallback
		no_proxy = { "127.0.0.1", "localhost", ".local" },
		-- 目的地策略 (M1): 默认黑名单; deny 空=全部代理
		policy = {
			mode = "blacklist",
			allow = {},
			deny = {},
			-- deny 命中: direct | reject
			deny_action = "direct",
			-- whitelist 未命中 allow: direct | reject
			miss_action = "direct",
		},
		-- chains key = 随机 6 位 id; 角色用 default/debug 引用 (勿再用 chains.default 字面)
		chains = {},
		default = nil,
		debug = nil,
		routes = {},
		-- 探活: 多 target + 延迟分级阈值
		check = {
			probe_targets = {
				{ host = "example.com", port = 80 },
			},
			latency = {
				ok_ms = 500,
				slow_ms = 2000,
			},
		},
		-- 防被扫 / ACL
		security = {
			allow_from = nil,
			quiet_reject = true,
			auth_fail = {
				max = 8,
				window_ms = 60000,
				ban_ms = 300000,
			},
		},
	}
end


-- @brief 合并出站 TLS 选项 (cfg.tls + hop.tls; hop 覆盖)
-- @param [in]  cfg[table|nil]
-- @param [in]  hop[table|nil]
-- @param [in]  hostname[string|nil]	默认 SNI
-- @return [table] opts 可直接给 upgrade_tls
M.resolve_tls_opts = function (cfg, hop, hostname)
	local base = M.defaults().tls
	local out = {
		verify = base.verify == true,
		sni = true,
	}

	local function _merge(src)
		if type(src) ~= "table" then
			return
		end
		if src.verify ~= nil then
			out.verify = src.verify
		end
		if src.sni ~= nil then
			out.sni = src.sni
		end
		if src.hostname ~= nil then
			out.hostname = src.hostname
		end
		if src.ca_pem ~= nil then
			out.ca_pem = src.ca_pem
		end
		if src.alpn ~= nil then
			out.alpn = src.alpn
		end
		if src.pin_sha256 ~= nil then
			out.pin_sha256 = src.pin_sha256
		end
		if src.timeout_ms ~= nil then
			out.timeout_ms = src.timeout_ms
		end
	end

	if type(cfg) == "table" then
		_merge(cfg.tls)
	end
	if type(hop) == "table" then
		_merge(hop.tls)
	end

	-- sni=true → 用 hostname; sni=string → 作 hostname; sni=false → 不设
	if out.hostname == nil then
		if type(out.sni) == "string" and out.sni ~= "" then
			out.hostname = out.sni
		elseif out.sni ~= false and type(hostname) == "string" and hostname ~= "" then
			out.hostname = hostname
		end
	end

	if out.sni == false then
		out.sni = false
	elseif type(out.sni) == "string" then
		out.sni = out.hostname or out.sni
	end

	return out
end


-- @brief 合并后的超时表 (缺省填 defaults.timeouts)
-- @param [in]  cfg[table|nil]
-- @return [table]
M.timeouts = function (cfg)
	local base = M.defaults().timeouts
	local out = {
		connect_ms = base.connect_ms,
		probe_ms = base.probe_ms,
		idle_ms = base.idle_ms,
		poll_ms = base.poll_ms,
		accept_ms = base.accept_ms,
		hop_idle_ms = base.hop_idle_ms,
	}

	if type(cfg) == "table" and type(cfg.timeouts) == "table" then
		for k, _ in pairs(out) do
			if cfg.timeouts[k] ~= nil then
				local n = tonumber(cfg.timeouts[k])
				if n ~= nil and n >= 0 then
					out[k] = n
				end
			end
		end
	end

	return out
end


-- @brief 是否本机回环地址 (LAN 暴露判定)
-- @param [in]  host[string|nil]
-- @return [boolean]
M.is_loopback_host = function (host)
	if type(host) ~= "string" or host == "" then
		return true
	end

	local h = string.lower(host)
	if h == "127.0.0.1" or h == "localhost" or h == "::1" then
		return true
	end

	-- 127.0.0.0/8
	if string.match(h, "^127%.%d+%.%d+%.%d+$") then
		return true
	end

	return false
end


local _id_seeded = false


local function _ensure_id_seed()
	if _id_seeded then
		return
	end

	local n = math.floor(os.time() % 100000) * 1000
		+ math.floor((os.clock() * 1000000) % 1000)
	math.randomseed(n)
	_id_seeded = true
end


-- @brief 链 id 是否合法 (小写字母数字 6 位)
-- @param [in]  id[any]
-- @return [boolean]
M.is_chain_id = function (id)
	if type(id) ~= "string" then
		return false
	end

	return string.match(id, "^[a-z0-9][a-z0-9][a-z0-9][a-z0-9][a-z0-9][a-z0-9]$") ~= nil
end


-- @brief 新建链 id (随机 6 位; 避开 existing)
-- @param [in]  existing[table|nil]		已有 chains 表
-- @return [string]
M.gen_chain_id = function (existing)
	_ensure_id_seed()
	local alphabet = "abcdefghijklmnopqrstuvwxyz0123456789"
	for _ = 1, 128 do
		local t = {}
		for i = 1, 6 do
			local r = math.random(1, #alphabet)
			t[i] = string.sub(alphabet, r, r)
		end

		local id = table.concat(t)
		if type(existing) ~= "table" or existing[id] == nil then
			return id
		end
	end

	error("gen_chain_id: collision")
end


-- @brief 角色名 / 链 id → 真实 chains key
-- @param [in]  cfg[table]
-- @param [in]  name[string|nil]
-- @return [string|nil]
M.resolve_chain_ref = function (cfg, name)
	if name == nil or name == "" or name == "direct" or name == "none" then
		return nil
	end

	if type(cfg) ~= "table" then
		return name
	end

	if name == "default" then
		if type(cfg.default) == "string" and cfg.default ~= "" then
			return cfg.default
		end
		return nil
	end

	if name == "debug" then
		if type(cfg.debug) == "string" and cfg.debug ~= "" then
			return cfg.debug
		end
		return nil
	end

	return name
end


-- 临时切链: 进程内 + pref 旁路文件 (alua call 与 proxy 进程跨进程)
local _use_mem = nil
local _use_cache = { mtime = nil, id = nil, checked = 0 }


local function _use_file_path()
	local ok_c, cproxy = pcall(require, "klbagent.core.config.proxy")
	if not ok_c or type(cproxy) ~= "table" or type(cproxy.path) ~= "function" then
		return ""
	end

	local p = cproxy.path()
	if type(p) ~= "string" or p == "" then
		return ""
	end

	local dir = string.match(p, "^(.*)[/\\][^/\\]+$")
	if type(dir) ~= "string" or dir == "" then
		return ""
	end

	return dir .. "/proxy_use.json"
end


local function _read_use_file()
	local path = _use_file_path()
	if path == "" then
		return nil
	end

	local lfs_ok, lfs = pcall(require, "lfs")
	local mtime = nil
	if lfs_ok and type(lfs) == "table" and type(lfs.attributes) == "function" then
		mtime = lfs.attributes(path, "modification")
	end

	if mtime ~= nil and _use_cache.mtime == mtime then
		return _use_cache.id
	end

	local f = io.open(path, "rb")
	if f == nil then
		_use_cache.mtime = mtime
		_use_cache.id = nil
		return nil
	end

	local raw = f:read("*a")
	f:close()
	local ok_j, cjson = pcall(require, "cjson")
	if not ok_j or type(cjson) ~= "table" then
		return nil
	end

	local ok, obj = pcall(cjson.decode, raw or "")
	local id = nil
	if ok and type(obj) == "table" and type(obj.chain) == "string" and obj.chain ~= "" then
		id = obj.chain
	end

	_use_cache.mtime = mtime
	_use_cache.id = id
	return id
end


local function _write_use_file(id)
	local path = _use_file_path()
	if path == "" then
		return false
	end

	if id == nil or id == "" then
		os.remove(path)
		_use_cache.mtime = nil
		_use_cache.id = nil
		return true
	end

	local ok_j, cjson = pcall(require, "cjson")
	if not ok_j or type(cjson) ~= "table" then
		return false
	end

	local text = cjson.encode({ chain = id })
	if type(text) ~= "string" then
		return false
	end

	local f = io.open(path, "wb")
	if f == nil then
		return false
	end

	f:write(text)
	f:write("\n")
	f:close()
	_use_cache.mtime = nil
	_use_cache.id = id
	return true
end


-- @brief 解析切链名 → 真实 id (default/debug/链 id)
-- @param [in]  cfg[table]
-- @param [in]  name[string]
-- @return [string|nil] id
-- @return [string|nil] err
local function _resolve_use_name(cfg, name)
	if name == nil or name == "" or name == "default" then
		return nil, nil
	end

	local id = M.resolve_chain_ref(cfg, name)
	if id == nil and type(cfg) == "table" and type(cfg.chains) == "table"
		and type(cfg.chains[name]) == "table" then
		id = name
	end

	if id == nil then
		return nil, "unknown chain: " .. tostring(name)
	end

	if type(cfg) == "table" and type(cfg.default) == "string" and id == cfg.default then
		return nil, nil
	end

	return id, nil
end


-- @brief 临时选用链 (写 proxy_use.json; 重启入口会 clear)
-- @param [in]  cfg[table|nil]
-- @param [in]  name[string|nil]	default|debug|链id; 空/default=回落
-- @return [boolean] ok
-- @return [string|nil] err
-- @return [string|nil] active_id
M.use = function (cfg, name)
	if type(cfg) ~= "table" then
		local ok_c, cproxy = pcall(require, "klbagent.core.config.proxy")
		if ok_c and type(cproxy) == "table" and type(cproxy.load) == "function" then
			cfg = cproxy.load()
		else
			cfg = M.defaults()
		end
	end

	local id, err = _resolve_use_name(cfg, name)
	if err ~= nil then
		return false, err, nil
	end

	_use_mem = id
	_write_use_file(id)

	local active = id
	if active == nil and type(cfg.default) == "string" then
		active = cfg.default
	end

	return true, nil, active
end


-- @brief 清除临时切链 (启动入口调用; 回落 default)
-- @return [boolean] ok
M.clear_use = function ()
	_use_mem = nil
	return _write_use_file(nil)
end


-- @brief 当前临时切链 id (无覆盖则 nil)
-- @return [string|nil]
M.use_id = function ()
	if type(_use_mem) == "string" and _use_mem ~= "" then
		return _use_mem
	end

	return _read_use_file()
end


-- @brief 当前生效链 id (proxy.use 覆盖 → default → nil)
-- @param [in]  cfg[table]
-- @return [string|nil]
M.active_chain_id = function (cfg)
	if type(cfg) ~= "table" then
		return nil
	end

	local use_id = M.use_id()
	if type(use_id) == "string" and use_id ~= "" then
		local id = M.resolve_chain_ref(cfg, use_id) or use_id
		if type(cfg.chains) == "table" and type(cfg.chains[id]) == "table" then
			return id
		end
	end

	if type(cfg.default) == "string" and cfg.default ~= "" then
		return cfg.default
	end

	return nil
end


-- @brief 清 cjson.null / 非字符串角色字段; 剥遗留 active_chain
-- @param [in]  cfg[table]
-- @return [boolean] changed
local function _scrub_role_fields(cfg)
	local changed = false
	local null = nil
	do
		local ok_j, cjson = pcall(require, "cjson")
		if ok_j and type(cjson) == "table" then
			null = cjson.null
		end
	end

	if cfg.active_chain ~= nil then
		cfg.active_chain = nil
		changed = true
	end

	local fields = { "default", "debug" }
	for i = 1, #fields do
		local f = fields[i]
		local v = cfg[f]
		if v == null or (v ~= nil and type(v) ~= "string") then
			cfg[f] = nil
			changed = true
		elseif v == "" then
			cfg[f] = nil
			changed = true
		end
	end

	return changed
end


-- @brief 迁移字面 chains.default / chains.debug → 随机 id + 角色字段
-- @param [in]  cfg[table]
-- @return [table] cfg
-- @return [boolean] changed
M.migrate_legacy_chains = function (cfg)
	local changed = false
	if type(cfg) ~= "table" then
		return cfg, false
	end

	if _scrub_role_fields(cfg) then
		changed = true
	end

	if type(cfg.chains) ~= "table" then
		cfg.chains = {}
	end

	local chains = cfg.chains

	local function _is_hop_list(v)
		return type(v) == "table" and #v > 0 and type(v[1]) == "table"
			and (v[1].type ~= nil or v[1].host ~= nil)
	end

	if _is_hop_list(chains["default"]) then
		local id = M.gen_chain_id(chains)
		chains[id] = chains["default"]
		chains["default"] = nil
		cfg.default = id
		changed = true
	end

	if _is_hop_list(chains["debug"]) then
		local id = M.gen_chain_id(chains)
		chains[id] = chains["debug"]
		chains["debug"] = nil
		cfg.debug = id
		changed = true
	end

	return cfg, changed
end


-- @brief 落盘形态: 剥遗留 active_chain (仅 default/debug 链定义持久)
-- @param [in]  cfg[table]
-- @return [table] cfg
M.for_disk = function (cfg)
	if type(cfg) ~= "table" then
		return cfg
	end

	if cfg.active_chain ~= nil then
		cfg.active_chain = nil
	end

	return cfg
end


-- @brief 保护 default 链: 角色指针与 hops 内容不可被普通写覆盖
-- @param [in]  prev[table|nil]
-- @param [in]  cfg[table]
-- @return [table] cfg
M.protect_default_chain = function (prev, cfg)
	if type(cfg) ~= "table" or type(prev) ~= "table" then
		return cfg
	end

	local id = prev.default
	if type(id) ~= "string" or id == "" then
		return cfg
	end

	cfg.default = id
	if type(prev.chains) == "table" and type(prev.chains[id]) == "table" then
		if type(cfg.chains) ~= "table" then
			cfg.chains = {}
		end

		cfg.chains[id] = prev.chains[id]
	end

	return cfg
end


-- @brief 写入 listen.host / port
-- @param [in]  cfg[table]
-- @param [in]  host[string|nil]
-- @param [in]  port[number|string|nil]
-- @return [boolean] ok
-- @return [string|nil] err
M.apply_listen = function (cfg, host, port)
	if type(cfg) ~= "table" then
		return false, "cfg must be table"
	end

	if type(cfg.listen) ~= "table" then
		cfg.listen = {}
	end

	if host ~= nil and host ~= "" then
		if type(host) ~= "string" then
			return false, "listen.host bad"
		end
		-- `*` → 全接口 (IPv4 any)
		if host == "*" then
			host = "0.0.0.0"
		end
		cfg.listen.host = host
	end

	if port ~= nil and port ~= "" then
		local p = tonumber(port)
		if p == nil or p < 1 or p > 65535 then
			return false, "listen.port bad"
		end
		cfg.listen.port = p
	end

	return true, nil
end


-- @brief 解析 host:port (末段为端口数字; IPv6 须 [addr]:port)
-- @param [in]  s[string]
-- @return [string|nil] host
-- @return [number|nil] port
-- @return [string|nil] err
M.parse_hostport = function (s)
	if type(s) ~= "string" or s == "" then
		return nil, nil, "empty hostport"
	end

	-- [IPv6]:port
	if string.sub(s, 1, 1) == "[" then
		local close = string.find(s, "]", 2, true)
		if close == nil then
			return nil, nil, "bad ipv6 hostport"
		end
		local host = string.sub(s, 2, close - 1)
		local rest = string.sub(s, close + 1)
		if rest == "" then
			return host, nil, nil
		end
		local port = string.match(rest, "^:(%d+)$")
		if port == nil then
			return nil, nil, "bad ipv6 port"
		end
		return host, tonumber(port), nil
	end

	local host, port_s = string.match(s, "^(.+):(%d+)$")
	if host ~= nil then
		-- 与 user:pass 区分: host 含 '.' / '*' / 仅为 0.0.0.0 类, 或 port 在常见 listen 段
		-- 保守: 仅当 host 含 '.' 或 host=='*' 或 host 为十六进制冒号已在上方处理
		if string.find(host, ".", 1, true) ~= nil or host == "*" then
			return host, tonumber(port_s), nil
		end
	end

	return nil, nil, "not hostport"
end


-- @brief 从环境套 listen (ALUA_ENTRY_HOST / ALUA_ENTRY_PORT)
-- @param [in]  cfg[table]
-- @return [boolean] ok
-- @return [string|nil] err
M.apply_listen_env = function (cfg)
	local host = os.getenv("ALUA_ENTRY_HOST")
	local port = os.getenv("ALUA_ENTRY_PORT")
	if (type(host) ~= "string" or host == "") and (type(port) ~= "string" or port == "") then
		return true, nil
	end

	return M.apply_listen(cfg, host, port)
end


-- @brief 监听项是否含某协议名
-- @param [in]  listen[table|nil]
-- @param [in]  name[string]
-- @return [boolean]
M.listen_has_proto = function (listen, name)
	if type(name) ~= "string" or name == "" then
		return false
	end

	local protocols = listen and listen.protocols
	if type(protocols) ~= "table" then
		return name == "socks5"
	end

	local want = string.lower(name)
	for i = 1, #protocols do
		if string.lower(tostring(protocols[i])) == want then
			return true
		end
	end

	return false
end


-- @brief 监听项是否需要 kproxy listen (SOCKS5/HTTP/隧道/明文 smp)
-- @param [in]  listen[table|nil]
-- @return [boolean]
M.listen_need_kproxy = function (listen)
	return M.listen_has_proto(listen, "socks5")
		or M.listen_has_proto(listen, "http-connect")
		or M.listen_has_proto(listen, "http_connect")
		or M.listen_has_proto(listen, "alua-tunnel")
		or M.listen_has_proto(listen, "alua_tunnel")
		or M.listen_has_proto(listen, "smp")
		or M.listen_has_proto(listen, "mnp")
		or M.listen_has_proto(listen, "ws")
		or M.listen_has_proto(listen, "wss")
end


-- @brief 监听项是否收 mnps/smps (klblisten)
-- @param [in]  listen[table|nil]
-- @return [boolean]
M.listen_allow_mnps = function (listen)
	return M.listen_has_proto(listen, "mnps")
		or M.listen_has_proto(listen, "smps")
end


-- @brief 解析 mnps 绑定口 (显式 mnps_port; 否则与 kproxy 并存用 port+1, 独占用 port)
-- @param [in]  listen[table]
-- @return [number|nil] port
-- @return [string|nil] err
M.listen_mnps_port = function (listen)
	if type(listen) ~= "table" or not M.listen_allow_mnps(listen) then
		return nil, nil
	end

	local need_kp = M.listen_need_kproxy(listen)
	local port = tonumber(listen.port) or 12818
	local mnps_port = tonumber(listen.mnps_port)
	if mnps_port == nil then
		if need_kp then
			mnps_port = port + 1
		else
			mnps_port = port
		end
	end

	if mnps_port < 1 or mnps_port > 65535 then
		return nil, "listen.mnps_port bad"
	end

	return mnps_port, nil
end


-- @brief 监听组: 主 listen + 可选 listens[] 附加口 (去重 host:port)
-- @param [in]  cfg[table|nil]
-- @return [table] listen 表数组 (至少含主 listen; 空 cfg 则空表)
M.listen_group = function (cfg)
	local out = {}
	if type(cfg) ~= "table" then
		return out
	end

	local function _push(L)
		if type(L) ~= "table" then
			return
		end

		local host = tostring(L.host or "127.0.0.1")
		local port = tonumber(L.port)
		if port == nil then
			return
		end

		for i = 1, #out do
			local o = out[i]
			if tostring(o.host or "") == host and tonumber(o.port) == port then
				return
			end
		end

		out[#out + 1] = L
	end

	_push(cfg.listen)
	if type(cfg.listens) == "table" then
		for i = 1, #cfg.listens do
			_push(cfg.listens[i])
		end
	end

	return out
end


-- @brief 绑定面指纹 (host/port/mnps_port; 变则须重启入口)
-- @param [in]  cfg[table|nil]
-- @return [string]
M.listen_bind_fingerprint = function (cfg)
	local group = M.listen_group(cfg)
	local parts = {}
	for i = 1, #group do
		local L = group[i]
		local host = tostring(L.host or "")
		local port = tostring(tonumber(L.port) or "")
		local mp = M.listen_mnps_port(L)
		parts[i] = host .. "|" .. port .. "|" .. tostring(mp or "")
	end

	return table.concat(parts, ";")
end


-- @brief 校验并规范化单个 listen 表
-- @param [in]  L[table]
-- @param [in]  label[string|nil]		错误前缀
-- @return [boolean] ok
-- @return [string|nil] err
local function _validate_one_listen(L, label)
	local pref = label or "listen"
	if type(L) ~= "table" then
		return false, pref .. " must be table"
	end

	local host = L.host
	if host == nil or host == "" then
		L.host = "127.0.0.1"
		host = L.host
	elseif type(host) ~= "string" then
		return false, pref .. ".host bad"
	elseif host == "*" then
		L.host = "0.0.0.0"
		host = L.host
	end

	local port = tonumber(L.port)
	if port == nil or port < 0 or port > 65535 then
		return false, pref .. ".port bad"
	end
	L.port = port

	if L.backlog ~= nil then
		local bl = tonumber(L.backlog)
		if bl == nil or bl < 1 then
			return false, pref .. ".backlog bad"
		end
		L.backlog = math.floor(bl)
	end

	if L.port_fallback ~= nil then
		local pf = tonumber(L.port_fallback)
		if pf == nil or pf < 0 or pf > 64 then
			return false, pref .. ".port_fallback bad"
		end
		L.port_fallback = math.floor(pf)
	end

	if L.mnps_port ~= nil and L.mnps_port ~= "" then
		local mp = tonumber(L.mnps_port)
		if mp == nil or mp < 1 or mp > 65535 then
			return false, pref .. ".mnps_port bad"
		end
		L.mnps_port = mp
	end

	if L.protocols ~= nil then
		if type(L.protocols) ~= "table" then
			return false, pref .. ".protocols bad"
		end
	end

	if L.id ~= nil and L.id ~= "" and type(L.id) ~= "string" then
		return false, pref .. ".id bad"
	end

	if not M.listen_need_kproxy(L) and not M.listen_allow_mnps(L) then
		-- 缺 protocols 时 listen_has_proto 默认 socks5, 不会走到此
		if type(L.protocols) == "table" and #L.protocols == 0 then
			return false, pref .. ".protocols empty"
		end
	end

	local _, err_m = M.listen_mnps_port(L)
	if err_m ~= nil then
		return false, pref .. ": " .. err_m
	end

	return true, nil
end


-- @brief 解析 user:pass (首个 ':' 分割; pass 可含 ':')
-- @param [in]  s[string]
-- @return [string|nil] user
-- @return [string|nil] pass
-- @return [string|nil] err
M.parse_userpass = function (s)
	if type(s) ~= "string" or s == "" then
		return nil, nil, "empty userpass"
	end

	local colon = string.find(s, ":", 1, true)
	if colon == nil then
		return nil, nil, "need user:pass"
	end

	local user = string.sub(s, 1, colon - 1)
	local pass = string.sub(s, colon + 1)
	if user == "" then
		return nil, nil, "empty user"
	end

	return user, pass, nil
end


-- @brief 写入入口鉴权 (none | userpass/basic)
-- @param [in]  cfg[table]
-- @param [in]  src[string|table|nil]	`user:pass` / `{ mode, user, pass }` / nil=none
-- @return [boolean] ok
-- @return [string|nil] err
M.apply_entry_auth = function (cfg, src)
	if type(cfg) ~= "table" then
		return false, "cfg must be table"
	end

	if src == nil or src == false or src == "" then
		cfg.auth = { mode = "none" }
		return true, nil
	end

	if type(src) == "string" then
		local user, pass, err = M.parse_userpass(src)
		if user == nil then
			return false, err or "bad userpass"
		end
		cfg.auth = {
			mode = "userpass",
			user = user,
			pass = pass or "",
		}
		return true, nil
	end

	if type(src) ~= "table" then
		return false, "bad auth src"
	end

	local mode = src.mode
	if type(mode) ~= "string" or mode == "" then
		mode = "userpass"
	end
	mode = string.lower(mode)

	if mode == "none" or mode == "off" or mode == "disabled" then
		cfg.auth = { mode = "none" }
		return true, nil
	end

	if mode == "basic" or mode == "user" or mode == "password" then
		mode = "userpass"
	end
	if mode == "token" then
		mode = "bearer"
	end

	if mode == "bearer" then
		local token = tostring(src.token or "")
		if token == "" then
			return false, "auth.token required"
		end
		cfg.auth = {
			mode = "bearer",
			token = token,
		}
		return true, nil
	end

	if mode ~= "userpass" then
		return false, "unsupported auth.mode"
	end

	if type(src.users) == "table" then
		cfg.auth = {
			mode = "userpass",
			users = src.users,
		}
		return true, nil
	end

	local user = tostring(src.user or "")
	local pass = tostring(src.pass or "")
	if user == "" then
		return false, "auth.user required"
	end

	cfg.auth = {
		mode = "userpass",
		user = user,
		pass = pass,
	}
	return true, nil
end


-- @brief 从环境套入口鉴权 (ALUA_ENTRY_AUTH=user:pass)
-- @param [in]  cfg[table]
-- @return [boolean] ok
-- @return [string|nil] err
M.apply_entry_auth_env = function (cfg)
	local env = os.getenv("ALUA_ENTRY_AUTH")
	if type(env) ~= "string" or env == "" then
		return true, nil
	end

	return M.apply_entry_auth(cfg, env)
end


-- @brief 校验配置表
-- @param [in]  cfg[table]
-- @return [boolean] ok
-- @return [string|nil] err
M.validate = function (cfg)
	if type(cfg) ~= "table" then
		return false, "cfg must be table"
	end

	-- 目的地策略归一 (缺省 blacklist + 空 deny)
	cfg.policy = M.normalize_policy(cfg.policy)

	-- 可仅写 listens[]: 首项升为主 listen
	if type(cfg.listen) ~= "table" then
		if type(cfg.listens) == "table" and type(cfg.listens[1]) == "table" then
			cfg.listen = cfg.listens[1]
		else
			return false, "listen missing"
		end
	end

	local ok_l, err_l = _validate_one_listen(cfg.listen, "listen")
	if not ok_l then
		return false, err_l
	end

	if cfg.listens ~= nil then
		if type(cfg.listens) ~= "table" then
			return false, "listens must be table"
		end

		for i = 1, #cfg.listens do
			local ok_i, err_i = _validate_one_listen(cfg.listens[i], "listens[" .. tostring(i) .. "]")
			if not ok_i then
				return false, err_i
			end
		end
	end

	-- 绑定口唯一: kproxy port + mnps_port
	do
		local used = {}
		local group = M.listen_group(cfg)
		for i = 1, #group do
			local L = group[i]
			if M.listen_need_kproxy(L) then
				local p = tonumber(L.port)
				local key = "k:" .. tostring(p)
				if used[key] or used["m:" .. tostring(p)] then
					return false, "listen port conflict: " .. tostring(p)
				end
				used[key] = true
			end

			local mp, err_m = M.listen_mnps_port(L)
			if err_m ~= nil then
				return false, err_m
			end
			if mp ~= nil then
				local key_m = "m:" .. tostring(mp)
				if used[key_m] or used["k:" .. tostring(mp)] then
					return false, "listen mnps_port conflict: " .. tostring(mp)
				end
				used[key_m] = true
			end
		end
	end

	if cfg.auth ~= nil then
		if type(cfg.auth) ~= "table" then
			return false, "auth must be table"
		end

		local mode = cfg.auth.mode
		if mode == nil or mode == "" then
			mode = "none"
		end
		if type(mode) ~= "string" then
			return false, "auth.mode bad"
		end

		mode = string.lower(mode)
		if mode == "basic" or mode == "user" or mode == "password" then
			mode = "userpass"
			cfg.auth.mode = "userpass"
		end
		if mode == "token" then
			mode = "bearer"
			cfg.auth.mode = "bearer"
		end

		if mode == "none" or mode == "off" or mode == "disabled" then
			cfg.auth.mode = "none"
		elseif mode == "bearer" then
			cfg.auth.mode = "bearer"
			if type(cfg.auth.token) ~= "string" or cfg.auth.token == "" then
				return false, "auth.token required"
			end
		elseif mode == "userpass" then
			cfg.auth.mode = "userpass"
			if type(cfg.auth.users) == "table" then
				-- 多用户: users[] 或 map
			else
				local user = tostring(cfg.auth.user or "")
				if user == "" then
					return false, "auth.user required"
				end
				if cfg.auth.pass == nil then
					cfg.auth.pass = ""
				elseif type(cfg.auth.pass) ~= "string" then
					cfg.auth.pass = tostring(cfg.auth.pass)
				end
			end
		else
			return false, "unsupported auth.mode"
		end
	end

	-- LAN / 非回环 listen (含 listens[]) 必须开入口鉴权 (A-3 / N1-2)
	local auth_mode = "none"
	if type(cfg.auth) == "table" and type(cfg.auth.mode) == "string" then
		auth_mode = string.lower(cfg.auth.mode)
	end
	do
		local group = M.listen_group(cfg)
		for i = 1, #group do
			local h = group[i].host
			if not M.is_loopback_host(h) then
				if auth_mode ~= "userpass" and auth_mode ~= "bearer" then
					return false, "LAN listen requires auth (set ALUA_ENTRY_AUTH or serve user:pass)"
				end
			end
		end
	end

	if cfg.security ~= nil and type(cfg.security) ~= "table" then
		return false, "security must be table"
	end

	if cfg.check ~= nil and type(cfg.check) ~= "table" then
		return false, "check must be table"
	end

	if cfg.allow_from ~= nil and type(cfg.allow_from) ~= "table" then
		return false, "allow_from must be table"
	end

	if cfg.chains ~= nil and type(cfg.chains) ~= "table" then
		return false, "chains must be table"
	end

	-- 角色字段: 必须指向已有 chain id (勿写角色名字面)
	local function _check_role(field)
		local v = cfg[field]
		if v == nil or v == "" then
			return true, nil
		end
		if type(v) ~= "string" then
			return false, field .. " must be string"
		end
		if v == "default" or v == "debug" or v == "direct" or v == "none" then
			return false, field .. " must be chain id not role name"
		end
		if type(cfg.chains) ~= "table" or cfg.chains[v] == nil then
			return false, field .. " unknown chain: " .. v
		end
		return true, nil
	end

	do
		local ok_r, err_r = _check_role("default")
		if not ok_r then
			return false, err_r
		end
		ok_r, err_r = _check_role("debug")
		if not ok_r then
			return false, err_r
		end
	end

	if cfg.routes ~= nil then
		if type(cfg.routes) ~= "table" then
			return false, "routes must be table"
		end
		for i = 1, #cfg.routes do
			local r = cfg.routes[i]
			if type(r) ~= "table" then
				return false, "routes[" .. tostring(i) .. "] bad"
			end
			if type(r.match) ~= "string" or r.match == "" then
				return false, "routes[" .. tostring(i) .. "].match required"
			end
			if r.chain ~= nil and type(r.chain) ~= "string" then
				return false, "routes[" .. tostring(i) .. "].chain bad"
			end
		end
	end

	if cfg.no_proxy ~= nil and type(cfg.no_proxy) ~= "table" then
		return false, "no_proxy must be table"
	end

	return true, nil
end


local function _norm_host(host)
	if type(host) ~= "string" then
		return ""
	end
	return string.lower(host)
end


-- @brief 目的 host[:port] 是否命中 no_proxy (与 net.util.no_proxy_match 同语义)
-- @param [in]  cfg[table]
-- @param [in]  host[string]
-- @param [in]  port[number|nil]
-- @return [boolean]
M.host_in_no_proxy = function (cfg, host, port)
	if type(cfg) ~= "table" or type(cfg.no_proxy) ~= "table" then
		return false
	end

	local ok_u, util = pcall(require, "klbagent.core.tools.net.util")
	if not ok_u or type(util) ~= "table" or type(util.no_proxy_match) ~= "function" then
		return false
	end

	return util.no_proxy_match(host, port, cfg.no_proxy)
end


-- @brief 合并环境豁免到 cfg.no_proxy (ALUA_NO_PROXY / NO_PROXY / no_proxy)
-- @param [in]  cfg[table]
-- @return [boolean] ok
-- @return [string|nil] err
M.apply_no_proxy_env = function (cfg)
	if type(cfg) ~= "table" then
		return false, "cfg must be table"
	end

	local ok_u, util = pcall(require, "klbagent.core.tools.net.util")
	if not ok_u or type(util) ~= "table" then
		return true, nil
	end

	local raw = nil
	if type(util.no_proxy_env_raw) == "function" then
		raw = util.no_proxy_env_raw()
	else
		raw = os.getenv("ALUA_NO_PROXY")
	end

	if type(raw) ~= "string" or raw == "" then
		return true, nil
	end

	local extra = util.no_proxy_parse(raw)
	if type(cfg.no_proxy) ~= "table" then
		cfg.no_proxy = {}
	end

	local seen = {}
	for i = 1, #cfg.no_proxy do
		local p = string.lower(tostring(cfg.no_proxy[i]))
		seen[p] = true
	end

	for i = 1, #extra do
		local p = extra[i]
		if type(p) == "string" and p ~= "" and not seen[p] then
			cfg.no_proxy[#cfg.no_proxy + 1] = p
			seen[p] = true
		end
	end

	return true, nil
end


-- @brief 单条 route.match 是否命中
-- @param [in]  match[string]	`default`/`*` / 精确 host / `.suffix` / `*.suffix` / `host:port`
-- @param [in]  host[string]
-- @param [in]  port[number|nil]
-- @return [boolean]
M.route_match = function (match, host, port)
	if type(match) ~= "string" or match == "" then
		return false
	end

	local m = string.lower(match)
	if m == "default" or m == "*" then
		return true
	end

	local h = _norm_host(host)
	if h == "" then
		return false
	end

	-- host:port 精确
	local mh, mp = string.match(m, "^([^:]+):(%d+)$")
	if mh ~= nil then
		if h == mh and tonumber(port) == tonumber(mp) then
			return true
		end
		return false
	end

	if string.sub(m, 1, 1) == "." then
		if h == string.sub(m, 2) or string.sub(h, -#m) == m then
			return true
		end
		return false
	end

	if string.sub(m, 1, 2) == "*." then
		local suf = string.sub(m, 2)
		if h == string.sub(m, 3) or string.sub(h, -#suf) == suf then
			return true
		end
		return false
	end

	return h == m
end


-- @brief 规范 policy 子表 (缺省填默认; 非法 mode/action 回落)
-- @param [in]  pol[table|nil]
-- @return [table]
M.normalize_policy = function (pol)
	local def = M.defaults().policy
	local out = {
		mode = def.mode,
		allow = {},
		deny = {},
		deny_action = def.deny_action,
		miss_action = def.miss_action,
	}

	if type(pol) ~= "table" then
		return out
	end

	local mode = pol.mode
	if type(mode) == "string" then
		mode = string.lower(mode)
		if mode == "whitelist" or mode == "white" or mode == "allow" then
			out.mode = "whitelist"
		elseif mode == "blacklist" or mode == "black" or mode == "deny" then
			out.mode = "blacklist"
		end
	end

	local function _copy_list(src)
		local t = {}
		if type(src) ~= "table" then
			return t
		end

		for i = 1, #src do
			local s = src[i]
			if type(s) == "string" and s ~= "" then
				t[#t + 1] = s
			end
		end

		return t
	end

	out.allow = _copy_list(pol.allow)
	out.deny = _copy_list(pol.deny)

	local da = pol.deny_action
	if type(da) == "string" then
		da = string.lower(da)
		if da == "reject" or da == "deny" or da == "block" then
			out.deny_action = "reject"
		elseif da == "direct" or da == "bypass" then
			out.deny_action = "direct"
		end
	end

	local ma = pol.miss_action
	if type(ma) == "string" then
		ma = string.lower(ma)
		if ma == "reject" or ma == "deny" or ma == "block" then
			out.miss_action = "reject"
		elseif ma == "direct" or ma == "bypass" then
			out.miss_action = "direct"
		end
	end

	return out
end


-- @brief 列表任一 pattern 命中 (同 route_match)
-- @param [in]  list[table|nil]
-- @param [in]  host[string]
-- @param [in]  port[number|nil]
-- @return [boolean]
-- @return [string|nil] matched pattern
M.policy_list_match = function (list, host, port)
	if type(list) ~= "table" then
		return false, nil
	end

	for i = 1, #list do
		local pat = list[i]
		if type(pat) == "string" and M.route_match(pat, host, port) then
			return true, pat
		end
	end

	return false, nil
end


-- @brief 目的地策略判定
-- @return [string] action  `proxy` | `direct` | `reject`
-- @return [table] info `{ via, match?, mode }`
M.evaluate_policy = function (cfg, host, port)
	local pol = M.normalize_policy(type(cfg) == "table" and cfg.policy or nil)
	local info = { via = "policy", mode = pol.mode, match = nil }

	local hit_deny, pat_d = M.policy_list_match(pol.deny, host, port)
	if hit_deny then
		info.via = "policy_deny"
		info.match = pat_d
		if pol.deny_action == "reject" then
			return "reject", info
		end

		return "direct", info
	end

	if pol.mode == "whitelist" then
		local hit_allow, pat_a = M.policy_list_match(pol.allow, host, port)
		if hit_allow then
			info.via = "policy_allow"
			info.match = pat_a
			return "proxy", info
		end

		info.via = "policy_miss"
		if pol.miss_action == "reject" then
			return "reject", info
		end

		return "direct", info
	end

	-- blacklist: 未命中 deny → 代理 (空 deny = 全部代理)
	info.via = "policy_pass"
	return "proxy", info
end


-- @brief 按 chain 名/角色取 hops[]; `direct`/`""` → nil (直连)
-- @param [in]  cfg[table]
-- @param [in]  name[string|nil]	链 id 或角色 default/debug
-- @return [table|nil] hops
-- @return [string|nil] err
M.chain_hops = function (cfg, name)
	if name == nil or name == "" or name == "direct" or name == "none" then
		return nil, nil
	end

	if type(cfg) ~= "table" or type(cfg.chains) ~= "table" then
		return nil, "chains missing"
	end

	local id = M.resolve_chain_ref(cfg, name)
	if id == nil then
		return nil, "unknown chain: " .. tostring(name)
	end

	local hops = cfg.chains[id]
	if hops == nil then
		return nil, "unknown chain: " .. tostring(id)
	end

	if type(hops) ~= "table" then
		return nil, "bad chain: " .. tostring(id)
	end

	if #hops == 0 then
		return nil, nil
	end

	return hops, nil
end


-- @brief 按目的地选 hops (no_proxy → policy → routes → use/default → 直连)
-- @param [in]  cfg[table]
-- @param [in]  host[string]
-- @param [in]  port[number|nil]
-- @return [table|nil] hops	nil=直连 (或 reject 时亦 nil)
-- @return [table] info `{ via, chain, match?, reject? }`
M.select_hops = function (cfg, host, port)
	local info = { via = "direct", chain = nil, match = nil }

	if type(cfg) ~= "table" then
		return nil, info
	end

	if M.host_in_no_proxy(cfg, host, port) then
		info.via = "no_proxy"
		return nil, info
	end

	-- 目的地黑/白名单 (默认 blacklist + 空 deny = 全代)
	do
		local action, pinfo = M.evaluate_policy(cfg, host, port)
		if action == "reject" then
			info.via = pinfo.via or "policy_deny"
			info.match = pinfo.match
			info.mode = pinfo.mode
			info.reject = true
			info.err = "policy reject"
			return nil, info
		end

		if action == "direct" then
			info.via = pinfo.via or "policy"
			info.match = pinfo.match
			info.mode = pinfo.mode
			return nil, info
		end
		-- proxy: 继续选链
	end

	if type(cfg.routes) == "table" and #cfg.routes > 0 then
		for i = 1, #cfg.routes do
			local r = cfg.routes[i]
			if type(r) == "table" and M.route_match(r.match, host, port) then
				local chain_name = r.chain
				if chain_name == nil or chain_name == "" then
					chain_name = "default"
				end
				local hops, err = M.chain_hops(cfg, chain_name)
				local id = M.resolve_chain_ref(cfg, chain_name) or chain_name
				if err ~= nil then
					info.via = "route_error"
					info.chain = id
					info.match = r.match
					info.err = err
					return nil, info
				end
				info.via = "route"
				info.chain = id
				info.match = r.match
				return hops, info
			end
		end
	end

	-- 无 routes 或未命中: proxy.use 覆盖 → default 角色
	local active_id = M.active_chain_id(cfg)
	if active_id ~= nil then
		local hops_def, err_d = M.chain_hops(cfg, active_id)
		if err_d == nil and hops_def ~= nil then
			local use_id = M.use_id()
			if type(use_id) == "string" and use_id ~= "" then
				info.via = "use"
			else
				info.via = "default"
			end
			info.chain = active_id
			return hops_def, info
		end
	end

	return nil, info
end


-- @brief 将 overlay 合并进 dst (listen/auth 浅合; chains/routes/no_proxy/key 整表替换)
-- @param [in]  dst[table]
-- @param [in]  overlay[table|nil]
-- @return [table] dst
M.merge_overlay = function (dst, overlay)
	if type(dst) ~= "table" then
		dst = M.defaults()
	end
	if type(overlay) ~= "table" then
		return dst
	end

	if type(overlay.listen) == "table" then
		if type(dst.listen) ~= "table" then
			dst.listen = {}
		end
		for k, v in pairs(overlay.listen) do
			dst.listen[k] = v
		end
	end

	-- 多监听附加口 (整表替换; 非字段浅合并)
	if type(overlay.listens) == "table" then
		dst.listens = overlay.listens
	end

	if type(overlay.tls) == "table" then
		if type(dst.tls) ~= "table" then
			dst.tls = {}
		end
		for k, v in pairs(overlay.tls) do
			dst.tls[k] = v
		end
	end

	if type(overlay.auth) == "table" then
		dst.auth = overlay.auth
	end

	if type(overlay.chains) == "table" then
		dst.chains = overlay.chains
	end

	-- 角色字段: 跳过 cjson.null (当作未设置)
	do
		local null = nil
		local ok_j, cjson = pcall(require, "cjson")
		if ok_j and type(cjson) == "table" then
			null = cjson.null
		end

		local function _role_set(field)
			local v = overlay[field]
			if v == nil or v == null then
				return
			end
			dst[field] = v
		end

		_role_set("default")
		_role_set("debug")
	end

	if type(overlay.routes) == "table" then
		dst.routes = overlay.routes
	end

	if type(overlay.no_proxy) == "table" then
		dst.no_proxy = overlay.no_proxy
	end

	if type(overlay.policy) == "table" then
		dst.policy = M.normalize_policy(overlay.policy)
	end

	if type(overlay.timeouts) == "table" then
		if type(dst.timeouts) ~= "table" then
			dst.timeouts = {}
		end
		for k, v in pairs(overlay.timeouts) do
			dst.timeouts[k] = v
		end
	end

	if type(overlay.hop_pool) == "table" then
		if type(dst.hop_pool) ~= "table" then
			dst.hop_pool = {}
		end
		for k, v in pairs(overlay.hop_pool) do
			dst.hop_pool[k] = v
		end
	end

	if type(overlay.security) == "table" then
		dst.security = overlay.security
	end

	if type(overlay.check) == "table" then
		dst.check = overlay.check
	end

	if type(overlay.allow_from) == "table" then
		dst.allow_from = overlay.allow_from
	end

	if overlay.key ~= nil then
		dst.key = overlay.key
	end

	return dst
end

-- @brief 探活目标列表 (缺省 example.com:80)
-- @param [in]  cfg[table|nil]
-- @return [table] `{ {host,port}, ... }`
M.probe_targets = function (cfg)
	local base = M.defaults().check.probe_targets
	local out = {}
	local src = base
	if type(cfg) == "table" and type(cfg.check) == "table"
		and type(cfg.check.probe_targets) == "table"
		and #cfg.check.probe_targets > 0 then
		src = cfg.check.probe_targets
	end
	for i = 1, #src do
		local t = src[i]
		if type(t) == "table" and type(t.host) == "string" and t.host ~= "" then
			out[#out + 1] = {
				host = t.host,
				port = tonumber(t.port) or 80,
			}
		end
	end
	if #out == 0 then
		out[1] = { host = "example.com", port = 80 }
	end
	return out
end


-- @brief 延迟分级阈值
-- @param [in]  cfg[table|nil]
-- @return [table] `{ ok_ms, slow_ms }`
M.latency_grades = function (cfg)
	local base = M.defaults().check.latency
	local out = {
		ok_ms = base.ok_ms,
		slow_ms = base.slow_ms,
	}
	if type(cfg) == "table" and type(cfg.check) == "table"
		and type(cfg.check.latency) == "table" then
		if cfg.check.latency.ok_ms ~= nil then
			out.ok_ms = tonumber(cfg.check.latency.ok_ms) or out.ok_ms
		end
		if cfg.check.latency.slow_ms ~= nil then
			out.slow_ms = tonumber(cfg.check.latency.slow_ms) or out.slow_ms
		end
	end
	return out
end


-- @brief 读文件全文
-- @param [in]  path[string]
-- @return [string|nil] text
-- @return [string|nil] err
local function _read_all(path)
	local f, err = io.open(path, "rb")
	if f == nil then
		return nil, tostring(err or "open fail")
	end

	local text = f:read("*a")
	f:close()
	if type(text) ~= "string" then
		return nil, "read fail"
	end

	-- strip UTF-8 BOM (EF BB BF)
	if string.byte(text, 1) == 0xEF and string.byte(text, 2) == 0xBB and string.byte(text, 3) == 0xBF then
		text = string.sub(text, 4)
	end

	return text, nil
end


-- @brief 解析 Lua 配置 (return table / cfg / config)
-- @param [in]  text[string]
-- @param [in]  path[string]
-- @return [table|nil] overlay
-- @return [string|nil] err
local function _parse_lua_cfg(text, path)
	local env = {}
	local chunk, err = load(text, "@" .. path, "t", env)
	if chunk == nil then
		return nil, tostring(err or "lua load fail")
	end

	local ok, ret = pcall(chunk)
	if not ok then
		return nil, tostring(ret or "lua run fail")
	end

	if type(ret) == "table" then
		return ret, nil
	end
	if type(env.cfg) == "table" then
		return env.cfg, nil
	end
	if type(env.config) == "table" then
		return env.config, nil
	end

	return nil, "lua config must return table (or set cfg/config)"
end


-- @brief 解析 JSON 配置
-- @param [in]  text[string]
-- @return [table|nil] overlay
-- @return [string|nil] err
local function _parse_json_cfg(text)
	local ok_c, cjson = pcall(require, "cjson")
	if not ok_c or type(cjson) ~= "table" or type(cjson.decode) ~= "function" then
		return nil, "cjson missing"
	end

	local ok, obj = pcall(cjson.decode, text)
	if not ok then
		return nil, tostring(obj or "json decode fail")
	end
	if type(obj) ~= "table" then
		return nil, "json root must be object"
	end

	return obj, nil
end


-- @brief 从路径解析 overlay (.json 走 cjson; 其它按 Lua)
-- @param [in]  path[string]
-- @return [table|nil] overlay
-- @return [string|nil] err
M.read_file = function (path)
	if type(path) ~= "string" or path == "" then
		return nil, "path required"
	end

	local text, err_r = _read_all(path)
	if text == nil then
		return nil, err_r or "read fail"
	end

	local lower = string.lower(path)
	if string.match(lower, "%.json$") then
		return _parse_json_cfg(text)
	end

	return _parse_lua_cfg(text, path)
end


-- @brief 加载配置 (defaults + 可选读盘 + ENTRY_HOST/PORT/AUTH + NO_PROXY)
-- @param [in]  path[string|nil]	配置路径; 空则 ALUA_PROXY_CONFIG → pref/proxy.json
-- @return [table] cfg
-- @return [string|nil] err
M.load = function (path)
	local cfg = M.defaults()
	local cfg_path = path
	local used_pref_live = false

	if type(cfg_path) ~= "string" or cfg_path == "" then
		local env_p = os.getenv("ALUA_PROXY_CONFIG")
		if type(env_p) == "string" and env_p ~= "" then
			cfg_path = env_p
		else
			-- M1.a/M1.c: 默认 pref/proxy.json; 走 core.load 以吃 live
			local ok_c, cproxy = pcall(require, "klbagent.core.config.proxy")
			if ok_c and type(cproxy) == "table" and type(cproxy.load) == "function" then
				local ok_l, overlay = pcall(cproxy.load)
				if ok_l and type(overlay) == "table" then
					M.merge_overlay(cfg, overlay)
					used_pref_live = true
					local p = cproxy.path()
					if type(p) == "string" and p ~= "" then
						cfg._path = p
						cfg_path = p
					end
				end
			end
		end
	end

	if not used_pref_live and type(cfg_path) == "string" and cfg_path ~= "" then
		local overlay, err_o = M.read_file(cfg_path)
		if overlay == nil then
			return M.defaults(), "config " .. cfg_path .. ": " .. tostring(err_o or "bad")
		end
		M.merge_overlay(cfg, overlay)
		cfg._path = cfg_path
	end

	cfg = select(1, M.migrate_legacy_chains(cfg))

	local ok_l, err_l = M.apply_listen_env(cfg)
	if not ok_l then
		return M.defaults(), err_l
	end

	local ok_a, err_a = M.apply_entry_auth_env(cfg)
	if not ok_a then
		return M.defaults(), err_a
	end

	local ok_n, err_n = M.apply_no_proxy_env(cfg)
	if not ok_n then
		return M.defaults(), err_n
	end

	local ok, err = M.validate(cfg)
	if not ok then
		return M.defaults(), err
	end

	return cfg, nil
end


return M

-- end
