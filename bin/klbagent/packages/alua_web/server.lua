--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   server.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_web 起服 (klblisten 端口组 + klbweb 处理 + 静态页 + /api/v1/*)
-- @note   listen 每项独立协议面 (HTTP/HTTPS/同口双协议); 阻塞主循环直至 kenv 退出
--   \n 端口由 klblisten 组监听 (schemes http/https), HTTP 会话经 app:handle 投递
--   \n TLS 不再复用 core.serve.resolve_tls; 同口 tls+plain 时 TLS 失败降级明文并告警
--   \n 黑白名单 (web.json acl) 挂最外层中间件, 覆盖静态页与 /api/*
--   \n acl 支持热更 (1s 轮询 web.json mtime); 端口/地址/证书仍须重启
-- @history 修改历史
--  \n [2026-10] 创建文件 (P0); 改 klblisten 端口组 + app:handle (klbweb 自 bind 在 alua 运行时不投递)
--]]
local cjson = require("cjson")
local lfs = require("lfs")
local config = require("klbagent.core.config")
local web_acl = require("klbagent.core.config.web_acl")
local codes = require("klbagent.code")
local logmod = require("klbagent.core.log")
local api = require("klbagent.packages.alua_web.api")


local M = {}


-- @brief 审计/运行日志 (失败不阻断)
local function _log(level, msg, fields)
	pcall(logmod.write, level, msg, fields)
end


-- @brief 包内 web/ 静态目录绝对路径
-- @return dir[string]			取不到时为空串
local function _web_dir()
	local ok, path = pcall(package.searchpath, "klbagent.packages.alua_web.server", package.path)
	if not ok or type(path) ~= "string" or path == "" then
		return ""
	end

	local dir = string.match(path, "^(.*)[/\\][^/\\]+$")
	if type(dir) ~= "string" then
		return ""
	end

	return dir .. "/web"
end


-- @brief 读配置: 指定路径优先, 否则 pref web.json
-- @param [in] cfg_path[string|nil]
-- @return cfg[table]
-- @return err[string|nil]
local function _load_cfg(cfg_path)
	if type(cfg_path) ~= "string" or cfg_path == "" or cfg_path == config.web.path() then
		return config.web.load(), nil
	end

	local f = io.open(cfg_path, "rb")
	if f == nil then
		return nil, "read web config fail: " .. cfg_path
	end

	local raw = f:read("*a") or ""
	f:close()

	local ok, obj = pcall(cjson.decode, raw)
	if not ok or type(obj) ~= "table" then
		return nil, "bad web config json: " .. cfg_path
	end

	return config.web.normalize(obj), nil
end


-- serve TLS 解析缓存 (nil=未解析; false=失败; table=成功)
local _tls_cache = nil
local _tls_err = ""


-- @brief 复用 alua serve 的证书/私钥 (env → pref → ktls 自签)
-- @return info[table|false]
-- @return err[string]
local function _tls_info()
	if _tls_cache == nil then
		-- 包外禁止 require core.serve; TLS 须由 listen 项显式 cert/key 或后续独立解析
		_tls_cache = false
		_tls_err = "alua.core.serve.UNAVAILABLE"
	end

	return _tls_cache, _tls_err
end


-- @brief web.json listen → klblisten 组规格 + 组级 TLS 配置
-- @param [in] cfg[table]
-- @return listens[table]
-- @return gcfg[table]
-- @return err[string|nil]
local function _listen_specs(cfg)
	local listens = {}
	local tls_cert = nil
	local tls_key = nil

	for i = 1, #cfg.listen do
		local item = cfg.listen[i]
		local schemes = {}
		local want_tls = item.tls == true
		local cert = item.cert
		local key = item.key

		if item.plain == true then
			schemes[#schemes + 1] = "http"
		end

		if want_tls then
			if cert == "" or key == "" then
				local tls, err = _tls_info()
				if type(tls) == "table" then
					cert = tls.cert
					key = tls.key
				elseif item.plain == true then
					-- 同口还有明文面: 降级为纯 HTTP
					_log("warn", "web tls disabled (no cert)", {
						module = "web",
						port = item.port,
						err = tostring(err),
					})
					want_tls = false
				else
					return nil, nil, "port " .. tostring(item.port) .. " need TLS cert/key: " .. tostring(err)
				end
			end

			if want_tls then
				if type(cert) ~= "string" or cert == "" or type(key) ~= "string" or key == "" then
					return nil, nil, "port " .. tostring(item.port) .. " need TLS cert/key"
				end

				schemes[#schemes + 1] = "https"
				if tls_cert == nil then
					tls_cert = cert
					tls_key = key
				end
			end
		end

		listens[#listens + 1] = {
			port = item.port,
			host = cfg.addr,
			schemes = schemes,
		}
	end

	return listens, { cert = tls_cert, key = tls_key }, nil
end


-- @brief 端口组人读串 (8080:http+https,8443:https)
-- @param [in] listens[table]
-- @return text[string]
local function _listens_text(listens)
	local out = {}
	for i = 1, #listens do
		local it = listens[i]
		local proto = table.concat(it.schemes, "+")

		out[#out + 1] = tostring(it.port) .. ":" .. proto
	end

	return table.concat(out, ",")
end


-- @brief 读配置文件里的 acl (直读原文, 不触发配置回写)
-- @param [in] path[string]
-- @return acl[table|nil]			读/解析失败为 nil
local function _read_acl(path)
	if type(path) ~= "string" or path == "" then
		return nil
	end

	local f = io.open(path, "rb")
	if f == nil then
		return nil
	end

	local raw = f:read("*a") or ""
	f:close()

	local ok, obj = pcall(cjson.decode, raw)
	if not ok or type(obj) ~= "table" then
		return nil
	end

	return config.web.normalize(obj).acl
end


-- @brief 安装黑白名单中间件 (注册序在路由/静态之前 = 请求层最外层)
-- @param [in] app[table]			klbweb 站点
-- @param [in] state[table]		{ acl } 运行态; 热更时整体替换 acl
-- @return 无
-- @note deny 优先; 只认 socket 对端 (req.peer), 不信任 X-Forwarded-For
--   \n 每请求读 state.acl (支持热更); acl 未启用直接 next()
--   \n 拒绝回 403 envelope 体 (不静默断连) 并留审计
local function _install_acl(app, state)
	app:use(function (req, res, next)
		local acl = state.acl
		if not web_acl.enabled(acl) then
			next()
			return
		end

		local ip = web_acl.peer_ip(req.peer)
		local allow, reason = web_acl.check(acl, ip)
		if allow then
			next()
			return
		end

		_log("warn", "acl deny", {
			module = "web",
			ip = ip,
			path = tostring(req.path or ""),
			rule = reason,
		})

		res:co_json({
			ok = false,
			code = codes.alua_web.DENIED,
			message = "forbidden by acl (" .. tostring(reason) .. ")",
			name = codes.str(codes.alua_web.DENIED) or "",
			data = {},
		}, 403)
	end)
end


-- @brief 盯配置文件变更并热更 acl (端口/证书仍须重启)
-- @param [in] state[table]		{ path, acl, mtime }
-- @param [in] kenv[table]
-- @param [in] kco[table]
-- @note 1s 轮询 mtime; 解析失败不更新 mtime 以便下次重试
--   \n 须在 kco 协程内: 用 kco.co_sleep 让出 (ktime.sleep 会阻塞线程饿死 accept)
local function _co_watch_acl(state, kenv, kco)
	while not kenv.is_exit() do
		kco.co_sleep(1000)

		local mtime = lfs.attributes(state.path, "modification")
		if mtime ~= nil and mtime ~= state.mtime then
			local acl = _read_acl(state.path)
			if acl == nil then
				_log("debug", "acl reload skip", { module = "web", path = tostring(state.path) })
			else
				state.mtime = mtime
				state.acl = acl
				_log("info", "acl reload", {
					module = "web",
					path = state.path,
					acl_allow = #acl.allow,
					acl_deny = #acl.deny,
					acl_default = acl.default,
				})
			end
		end
	end
end


-- @brief 丢弃非 HTTP 面 / handle 失败的会话
local function _drop(session)
	if type(session) ~= "table" then
		return
	end

	if type(session.disconnect) == "function" then
		pcall(function ()
			session:disconnect()
		end)
	elseif type(session.close) == "function" then
		pcall(function ()
			session:close()
		end)
	end
end


-- @brief 端口组 accept: http/https → klbweb; 其余丢弃
-- @param [in] app[table]			klbweb 站点
-- @param [in] grp[table]			klblisten 组
-- @param [in] kenv[table]
local function _accept_loop(app, grp, kenv)
	while not kenv.is_exit() do
		local face, session, msg = grp:co_accept()
		if msg == "exit" then
			break
		end

		if msg == "ok" and session ~= nil then
			if face == "http" or face == "https" then
				if not app:handle(session) then
					_log("warn", "web handle fail", { module = "web", face = tostring(face) })
					_drop(session)
				end
			else
				_drop(session)
			end
		elseif session ~= nil then
			_drop(session)
		end
	end
end


-- @brief 起服并阻塞运行, 直至 kenv 退出
-- @param [in] cfg_path[string|nil]		省略=pref web.json
-- @return ok[boolean]
-- @return err[string|nil]
M.start = function (cfg_path)
	local cfg, err_c = _load_cfg(cfg_path)
	if cfg == nil then
		return false, err_c
	end

	cfg = config.web.normalize(cfg)
	local listens, gcfg, err_s = _listen_specs(cfg)
	if listens == nil then
		return false, err_s
	end

	local ok_web, klbweb = pcall(require, "klbcore.klbweb")
	if not ok_web or type(klbweb) ~= "table" or type(klbweb.new) ~= "function" then
		return false, "klbcore.klbweb unavailable (need klbhttp)"
	end

	local ok_l, klblisten = pcall(require, "klbcore.klblisten")
	if not ok_l or type(klblisten) ~= "table" or type(klblisten.listen_group) ~= "function" then
		return false, "klbcore.klblisten unavailable"
	end

	-- 不 bind: 端口由 klblisten 组占用, HTTP 会话经 app:handle 投递
	local app = klbweb.new({
		server = "alua-web",
		log = false,
		csrf = false,
		session = false,
		request_id = false,
		healthz = false,
		max_body = 1024 * 1024,
		listing = false,
	})

	-- ACL 热更运行态: 只换 acl; listen/addr/证书改动仍须重启
	local cfg_file = config.web.path()
	if type(cfg_path) == "string" and cfg_path ~= "" and cfg_path ~= cfg_file then
		cfg_file = cfg_path
	end

	local acl_state = {
		path = cfg_file,
		acl = cfg.acl,
		mtime = lfs.attributes(cfg_file, "modification"),
	}

	_install_acl(app, acl_state)
	api.register(app, { started_ms = os.time() * 1000 })

	local dir = _web_dir()
	if dir ~= "" then
		app:static("/", dir, { index = "index.html" })
	end

	local grp, code_g, err_g = klblisten.listen_group(listens, gcfg)
	if grp == nil then
		return false, "listen group fail (" .. tostring(code_g) .. "): " ..
			tostring(err_g or "port conflicts or privilege (<1024)")
	end

	io.stderr:write(string.format("alua web: listen %s @ %s\n",
		_listens_text(listens), tostring(cfg.addr)))

	_log("info", "web listen", {
		module = "web",
		addr = cfg.addr,
		listens = _listens_text(listens),
		acl_allow = #cfg.acl.allow,
		acl_deny = #cfg.acl.deny,
		acl_default = cfg.acl.default,
	})

	local ok_e, kenv = pcall(require, "kenv")
	local ok_k, kco = pcall(require, "kco")
	local ok_u, co_util = pcall(require, "klbagent.core.co_util")
	if not ok_e or type(kenv) ~= "table"
		or not ok_k or type(kco) ~= "table" or type(kco.fork) ~= "function"
		or not ok_u or type(co_util) ~= "table" or type(co_util.run_host_loop) ~= "function" then
		grp:close()
		return false, "kenv/kco/co_util unavailable"
	end

	kco.fork(function ()
		_accept_loop(app, grp, kenv)
	end)

	kco.fork(function ()
		_co_watch_acl(acl_state, kenv, kco)
	end)

	-- CLI 宿主已泵则 hold; 独立进程入口才自调 co_main_loop
	local mode = co_util.run_host_loop()
	if mode == "self" then
		grp:close()
		app:stop()
		_log("info", "web stop", { module = "web" })
	end

	return true, nil
end


return M

-- end
