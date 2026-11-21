--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   tls.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 子模块: TLS 面判定 / 证书解析 / 自签 / 主动重签
-- @note   默认路径 = pref 目录 (store.dir()) 下 serve.crt|serve.key
--   \n 缺证书或临近过期 (7 天) 自动自签; SAN 含本机 IP/DNS
--   \n 主动重签: M.resign_tls (runtime.tls_resign 薄封装调用)
--   \n worker 形态无进程内热换; 换证后须重启 serve
--   \n @stability internal: 实现文件, 契约面见 core/serve/init.lua
-- @history 修改历史
--  \n [2026-11] 从 core/serve/init.lua 拆出本文件
--]]
local store = require("klbagent.core.config.store")
local log = require("klbagent.core.log")
local util = require("klbagent.core.serve.base.util")


local M = {}


-- 默认自签距过期不足此秒数则启动时重签 (7 天)
local RENEW_BEFORE_SEC = 7 * 86400


-- @brief TLS 面模式: http(https)|smp(smps)|mnp(mnps); 三者互斥
-- @note 优先 ALUA_SERVE_TLS_AS_HTTP; 否则 pref serve.json tls_face; 默认 mnp
--   \n 1|true|http→https; 0|false|smp|smps→smps
-- @return over[string]					"http"|"smp"|"mnp"
M.tls_over_default = function ()
	local v = os.getenv("ALUA_SERVE_TLS_AS_HTTP")
	if type(v) == "string" and v ~= "" then
		local low = string.lower(v)
		if low == "0" or low == "false" or low == "smp" or low == "smps" then
			return "smp"
		end

		if low == "mnp" or low == "mnps" then
			return "mnp"
		end

		-- 1|true|http|https 等 → https 面
		return "http"
	end

	-- M1.a: serve.json
	local ok_s, scfg_mod = pcall(require, "klbagent.core.config.serve")
	if ok_s and type(scfg_mod) == "table" and type(scfg_mod.load) == "function" then
		local ok_l, scfg = pcall(scfg_mod.load)
		if ok_l and type(scfg) == "table" and type(scfg.tls_face) == "string" then
			local face = string.lower(scfg.tls_face)
			if face == "smp" or face == "smps" then
				return "smp"
			end
			if face == "http" or face == "https" then
				return "http"
			end
			if face == "mnp" or face == "mnps" then
				return "mnp"
			end
		end
	end

	return "mnp"
end


-- @brief TLS ClientHello 是否按 HTTP (https); false=smps/mnps
-- @return as_http[boolean]
M.tls_as_http_default = function ()
	return M.tls_over_default() == "http"
end


-- 与配置同目录: kenv.pref_path("klua","alua") / store.dir()
local function _default_tls_dir()
	local dir = store.dir()
	if type(dir) == "string" and dir ~= "" then
		return (string.gsub(dir, "[/\\]+$", ""))
	end

	return "/tmp/alua_tls"
end


local function _looks_pem(s)
	return type(s) == "string" and string.find(s, "-----BEGIN", 1, true) == 1
end


local function _pair_ok(cert, key)
	if type(cert) ~= "string" or cert == "" or type(key) ~= "string" or key == "" then
		return false
	end

	if _looks_pem(cert) and _looks_pem(key) then
		return true
	end

	if _looks_pem(cert) or _looks_pem(key) then
		return false
	end

	return util.file_readable(cert) and util.file_readable(key)
end


local function _backup_file(path)
	if not util.file_readable(path) then
		return
	end

	os.rename(path, path .. ".bak")
end


-- 本机非回环 IPv4 -> SAN "IP:x.x.x.x"
local function _local_ip_sans()
	local out = {}
	local seen = { ["127.0.0.1"] = true }
	if util.is_windows() then
		-- Win 无 hostname -I; 避免 2>/dev/null 刷「系统找不到指定的路径」
		return out
	end

	local f = io.popen("hostname -I 2>/dev/null")
	if f == nil then
		return out
	end

	local s = f:read("*a") or ""
	f:close()
	for ip in string.gmatch(s, "%S+") do
		if string.match(ip, "^%d+%.%d+%.%d+%.%d+$") and not seen[ip] then
			seen[ip] = true
			out[#out + 1] = "IP:" .. ip
		end
	end

	return out
end


-- 本机 hostname / 短名 / FQDN -> SAN "DNS:…"
-- (svn 访问 https://IP 时可能按反查主机名校验, 须覆盖)
local function _local_dns_sans()
	local out = {}
	local seen = {}
	local function add_dns(name)
		if type(name) ~= "string" then
			return
		end

		name = string.match(name, "^%s*(.-)%s*$") or ""
		if name == "" or seen[name] or name == "localhost" then
			return
		end

		if string.find(name, " ", 1, true) ~= nil then
			return
		end

		seen[name] = true
		out[#out + 1] = "DNS:" .. name
	end

	if util.is_windows() then
		local f = io.popen("hostname 2>NUL")
		if f ~= nil then
			local s = f:read("*a") or ""
			f:close()
			for line in string.gmatch(s, "([^\r\n]+)") do
				add_dns(line)
			end
		end
	else
		local f = io.popen("hostname -f 2>/dev/null; hostname 2>/dev/null; hostname -s 2>/dev/null")
		if f ~= nil then
			local s = f:read("*a") or ""
			f:close()
			for line in string.gmatch(s, "([^\r\n]+)") do
				add_dns(line)
			end
		end
	end

	-- /etc/hosts 里绑到本机 IP 的名字 (如 myhost)
	local ips = _local_ip_sans()
	local ipset = {}
	for i = 1, #ips do
		local ip = string.match(ips[i], "^IP:(.+)$")
		if ip ~= nil then
			ipset[ip] = true
		end
	end

	local hf = io.open("/etc/hosts", "r")
	if hf ~= nil then
		for line in hf:lines() do
			if not string.match(line, "^%s*#") then
				local ip = string.match(line, "^%s*(%d+%.%d+%.%d+%.%d+)%s+")
				if ip ~= nil and ipset[ip] then
					for name in string.gmatch(line, "%s+([%w%._%-]+)") do
						add_dns(name)
					end
				end
			end
		end

		hf:close()
	end

	return out
end


-- 合并默认 SAN + 本机 DNS/IP + 调用方额外 san (去重保序)
local function _merge_san(extra)
	local base = {
		"DNS:alua-serve",
		"DNS:localhost",
		"IP:127.0.0.1",
	}
	local seen = {}
	local out = {}

	local function add(item)
		if type(item) ~= "string" or item == "" or seen[item] then
			return
		end

		seen[item] = true
		out[#out + 1] = item
	end

	for i = 1, #base do
		add(base[i])
	end

	local dns = _local_dns_sans()
	for i = 1, #dns do
		add(dns[i])
	end

	local locals = _local_ip_sans()
	for i = 1, #locals do
		add(locals[i])
	end

	if type(extra) == "table" then
		for i = 1, #extra do
			add(extra[i])
		end
	end

	return out
end


-- 默认 CN: 本机短主机名 (svn 对 https://IP 常按反查名校验)
local function _default_cn()
	local dns = _local_dns_sans()
	if 0 < #dns then
		local name = string.match(dns[1], "^DNS:(.+)$")
		if type(name) == "string" and name ~= "" then
			return name
		end
	end

	return "alua-serve"
end


-- 自签到指定路径 (覆盖; 先备份旧文件)
-- days[number|nil]; san[table|nil] 额外 SAN; subject[string|nil]
local function _selfsign_to(cert_path, key_path, days, san, subject)
	local ok_k, ktls = pcall(require, "ktls")
	if not ok_k or type(ktls) ~= "table" then
		return false, "ktls require fail"
	end

	if type(ktls.available) ~= "function" or not ktls.available() then
		return false, "ktls unavailable"
	end

	local dir = string.match(cert_path, "^(.+)[/\\][^/\\]+$")
	if type(dir) == "string" and dir ~= "" then
		if util.is_windows() then
			os.execute(string.format('mkdir "%s" 2>NUL', string.gsub(dir, "/", "\\")))
		else
			os.execute(string.format("mkdir -p %q", dir))
		end
	end

	_backup_file(cert_path)
	_backup_file(key_path)

	local d = days
	if type(d) ~= "number" or d <= 0 then
		d = 3650
	end

	local subj = subject
	if type(subj) ~= "string" or subj == "" then
		subj = "O=klb,OU=alua,CN=" .. _default_cn()
	end

	local ok_s, err_s = ktls.selfsign_to_files({
		path_cert = cert_path,
		path_key = key_path,
		bits = 2048,
		days = d,
		subject = subj,
		san = _merge_san(san),
	})
	if not ok_s then
		return false, tostring(err_s or "selfsign fail")
	end

	return _pair_ok(cert_path, key_path), nil
end


-- 默认路径是否需要重签: force / 不可读 / 过期 / 即将过期
local function _default_need_resign(cert_path, force)
	if force then
		return true, "force"
	end

	if not util.file_readable(cert_path) then
		return true, "missing"
	end

	local ok_k, ktls = pcall(require, "ktls")
	if not ok_k or type(ktls) ~= "table" or type(ktls.not_after_file) ~= "function" then
		return false, nil
	end

	local info, err = ktls.not_after_file(cert_path)
	if type(info) ~= "table" then
		return true, "unreadable:" .. tostring(err or "")
	end

	local rem = info.remaining_sec
	if type(rem) ~= "number" then
		return true, "bad_validity"
	end

	if rem <= 0 then
		return true, "expired"
	end

	if rem < RENEW_BEFORE_SEC then
		return true, "near_expiry"
	end

	return false, nil
end


-- @brief 解析 serve TLS (环境变量 / 默认路径 / ktls 自签 / 过期重签)
-- @return info[table]  { tls, plain, cert, key }
-- @return err[string|nil]
M.resolve_tls = function ()
	local flag = os.getenv("ALUA_SERVE_TLS")
	local cert = os.getenv("ALUA_SERVE_CERT")
	local key = os.getenv("ALUA_SERVE_KEY")
	local force_off = false
	local force_on = false
	local force_resign = util.env_truthy("ALUA_SERVE_TLS_RESIGN")
	local tls_dir = _default_tls_dir()
	local def_cert = tls_dir .. "/serve.crt"
	local def_key = tls_dir .. "/serve.key"
	local using_default = false

	if type(flag) == "string" and flag ~= "" then
		local low = string.lower(flag)
		if low == "0" or low == "false" or low == "off" or low == "no" then
			force_off = true
		elseif low == "1" or low == "true" or low == "on" or low == "yes" then
			force_on = true
		end
	end

	if force_off then
		return {
			tls = false,
			plain = true,
			cert = nil,
			key = nil,
		}
	end

	-- 环境变量优先; 否则默认 pref_path 下 serve.crt|key
	if _pair_ok(cert, key) then
		using_default = (cert == def_cert and key == def_key)
	else
		using_default = true
		if _pair_ok(def_cert, def_key) then
			cert = def_cert
			key = def_key
		else
			cert = nil
			key = nil
		end
	end

	-- 仅默认路径: 缺失 / 过期 / 将过期 / ALUA_SERVE_TLS_RESIGN=1 时重签
	if using_default then
		local need, reason = _default_need_resign(def_cert, force_resign)
		if need then
			local ok_g, err_g = _selfsign_to(def_cert, def_key, nil)
			if ok_g then
				cert = def_cert
				key = def_key
				io.stderr:write("alua serve: ktls resign (" .. tostring(reason) .. ") -> " .. def_cert .. "\n")
			elseif not _pair_ok(cert, key) then
				if force_on then
					return nil, "ALUA_SERVE_TLS=1 but no cert; resign fail: " .. tostring(err_g)
				end

				return {
					tls = false,
					plain = true,
					cert = nil,
					key = nil,
				}
			else
				io.stderr:write("alua serve: resign fail (" .. tostring(err_g) .. "), keep old cert\n")
			end
		end
	end

	if not _pair_ok(cert, key) then
		return nil, "TLS cert/key not usable"
	end

	return {
		tls = true,
		plain = true,
		cert = cert,
		key = key,
	}
end


-- @brief 强制重签默认 (或指定) 路径证书; TLS 管理与重签均由本模块完成
-- @param opts[table|nil]  days / path_cert / path_key / san / subject / hot / restart
-- @note  worker 无 live 热换 API: hot/restart 仅记入 note; 调用方须重启 serve 加载新证
-- @return info[table]|nil
-- @return err[string|nil]
M.resign_tls = function (opts)
	opts = opts or {}
	local tls_dir = _default_tls_dir()
	local cert_path = opts.path_cert
	local key_path = opts.path_key

	if type(cert_path) ~= "string" or cert_path == "" then
		cert_path = tls_dir .. "/serve.crt"
	end

	if type(key_path) ~= "string" or key_path == "" then
		key_path = tls_dir .. "/serve.key"
	end

	local san_merged = _merge_san(opts.san)
	local ok_g, err_g = _selfsign_to(cert_path, key_path, opts.days, opts.san, opts.subject)
	if not ok_g then
		return nil, tostring(err_g or "resign fail")
	end

	local not_after = nil
	local remaining_sec = nil
	local ok_k, ktls = pcall(require, "ktls")
	if ok_k and type(ktls) == "table" and type(ktls.not_after_file) == "function" then
		local info = ktls.not_after_file(cert_path)
		if type(info) == "table" then
			not_after = info.not_after
			remaining_sec = info.remaining_sec
		end
	end

	-- worker: 无 reload_tls_live / schedule_restart 导出; 写盘成功后须重启 serve
	local want_hot = opts.hot == true
	local want_restart = opts.restart == true
	local note = "resign ok; restart alua serve to load new cert"
	if want_hot or want_restart then
		note = "resign ok; worker has no tls hot-reload; restart alua serve to load new cert"
	end

	log.write("info", "serve.security.tls resign_tls", {
		module = "serve.tls",
		cert = cert_path,
		key = key_path,
		hot = want_hot,
		restart = want_restart,
	})

	return {
		ok = true,
		cert = cert_path,
		key = key_path,
		san = san_merged,
		not_after = not_after,
		remaining_sec = remaining_sec,
		need_restart = true,
		restart_scheduled = false,
		hot_reloaded = false,
		note = note,
	}
end


return M

-- end
