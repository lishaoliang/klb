--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  svn_https 可选包: HTTPS 边缘 + 同进程 ksvnra
-- @note   alua pkg run svn_https [listen_port]
--   \n 环境: ALUA_SVN_LISTEN / ALUA_SVN_ADDR / ALUA_SVN_SVNSERVE / ALUA_SVN_EDGE_HOST /
--   \n ALUA_SVN_REPO_ROOT / ALUA_SVN_MAX_BODY / ALUA_SVN_FILE_MAX_BYTES / ALUA_SVN_TXN_MAX_BYTES /
--   \n ALUA_SVN_CAT_MAX / ALUA_SVN_TXN_TTL / ALUA_SVN_LOCK_DB / ALUA_SVN_PROXY_DEBUG
--   \n 历史 ALUA_SVN_UPSTREAM 已废止 (stderr 警告后仍走 dav+ksvnra)
--   \n 形态: M.start_embed/M.stop_embed (单进程托管) 与 M.run (阻塞) 共用同一启动路径
-- @history 修改历史
--  \n [2026-09] 创建文件 (桩); 补充实现: klbweb listen + 反代; 默认拉起 bridge→svnserve;
--  \n [2026-10] 默认同进程 dav+ksvnra 读路径;
--  \n [2026-11] ALUA_SVN_ADDR 可配监听地址; ALUA_SVN_REPO_ROOT 供 401/404 映射; DAV 能力广告与实现对齐; 锁落盘; Range/access log; 错误体 XML; 低端口 (<1024) 能力探测与 systemd 单元引导; 拆 start_embed/stop_embed; M.run 改为 start_embed + 自泵
--]]
local codes = require("klbagent.code")
local log = require("klbagent.core.log")
local dav = require("klbagent.packages.svn_https.dav")


local M = {}


-- 单进程托管状态 (每包一实例); nil = 未启动
local g_embed = nil


local DEFAULT_LISTEN = 8443
local DEFAULT_SVNSERVE = "127.0.0.1:3690"
-- 边缘请求体上限; 须 >= 单文件上限 (ALUA_SVN_FILE_MAX_BYTES, 默认 512MiB)
local DEFAULT_MAX_BODY = 512 * 1024 * 1024


local DAV_METHODS = {
	"OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE",
	"PROPFIND", "PROPPATCH", "MKCOL", "COPY", "MOVE",
	"LOCK", "UNLOCK", "REPORT", "MERGE", "CHECKOUT", "MKACTIVITY",
	"BASELINE-CONTROL", "LABEL", "UPDATE",
}


local function _parse_args(argv)
	local listen = tonumber(os.getenv("ALUA_SVN_LISTEN")) or DEFAULT_LISTEN
	local upstream = os.getenv("ALUA_SVN_UPSTREAM") or ""
	local i = 1

	if type(argv) == "table" then
		if type(argv[i]) == "string" and tonumber(argv[i]) ~= nil then
			listen = tonumber(argv[i])
			i = i + 1
		elseif type(argv[i]) == "number" then
			listen = argv[i]
			i = i + 1
		end

		if type(argv[i]) == "string" and argv[i] ~= "" then
			upstream = argv[i]
		end
	end

	return listen, upstream
end


local function _max_body()
	local raw = os.getenv("ALUA_SVN_MAX_BODY")
	if type(raw) == "string" and raw ~= "" then
		local n = tonumber(raw)
		if type(n) == "number" and n >= 1024 then
			return math.floor(n)
		end
	end

	return DEFAULT_MAX_BODY
end


local function _register_dav(app, handler)
	for i = 1, #DAV_METHODS do
		local m = DAV_METHODS[i]
		app:route(m, "/", handler)
		app:route(m, "/*", handler)
	end
end


-- @brief TLS + klbweb 依赖
-- @return tls_info[table|nil]
-- @return klbweb[table|nil]
-- @return code[number|nil]
-- @return message[string|nil]
local function _check_tls_web(opts)
	local tls_info = nil
	local err_tls = nil

	if type(opts) == "table" and type(opts.tls) == "table" then
		tls_info = opts.tls
	else
		-- 包外禁止 core.serve.resolve_tls; 须显式传入 opts.tls
		err_tls = "alua.core.serve.UNAVAILABLE"
	end

	if tls_info == nil or tls_info.tls ~= true then
		return nil, nil, codes.svn_https.TOOL_FAILED, tostring(err_tls or "TLS required for svn_https")
	end

	local klbweb = nil
	if type(opts) == "table" and type(opts.klbweb) == "table" then
		klbweb = opts.klbweb
	else
		local ok_web, mod = pcall(require, "klbcore.klbweb")
		if ok_web and type(mod) == "table" then
			klbweb = mod
		end
	end

	if type(klbweb) ~= "table" or type(klbweb.new) ~= "function" then
		return nil, nil, codes.svn_https.TOOL_FAILED, "need klbcore.klbweb"
	end

	return tls_info, klbweb, nil, nil
end


-- @brief 创建 HTTPS 边缘 klbweb app (不 bind; 端口由 klblisten 组占用)
local function _new_edge_app(klbweb, max_body)
	return klbweb.new({
		server = "alua-svn-https",
		log = false,
		csrf = false,
		session = false,
		healthz = false,
		https = false,
		max_body = max_body,
		keep_alive = true,
	})
end


-- @brief 丢弃会话 (handle 失败 / 非 HTTP 面)
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
-- @param [in] state[table]		含 stopped=true 时立即退出 (单进程 stop)
local function _accept_loop(app, grp, kenv, state)
	while not state.stopped and not kenv.is_exit() do
		local face, session, msg = grp:co_accept()
		if msg == "exit" then
			break
		end

		if msg == "ok" and session ~= nil then
			if face == "http" or face == "https" then
				if not app:handle(session) then
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


-- @brief 打印客户端/边缘提示
-- @param [in] edge_host[string|nil]	Host 重写名; 缺省 ALUA_SVN_EDGE_HOST / <Host>
local function _write_client_hints(listen, edge_host)
	local edge_hint = edge_host
	if type(edge_hint) ~= "string" or edge_hint == "" then
		edge_hint = os.getenv("ALUA_SVN_EDGE_HOST") or "<Host>"
	end
	io.stderr:write("  client: svn co https://<host>:" .. tostring(listen) .. "/svn/<repo>\n")
	io.stderr:write("  edge rewrite host: " .. tostring(edge_hint) .. " (or request Host)\n")
end


-- @brief 监听地址列表 (逗号/空白分隔; 默认 ALUA_SVN_ADDR 或 0.0.0.0)
-- @param [in] raw[string|nil]		显式地址串; nil 读 ALUA_SVN_ADDR
local function _listen_addrs(raw)
	if type(raw) ~= "string" then
		raw = os.getenv("ALUA_SVN_ADDR")
	end

	local addrs = {}

	if type(raw) == "string" then
		for a in string.gmatch(raw, "[^,%s]+") do
			addrs[#addrs + 1] = a
		end
	end

	if 0 == #addrs then
		addrs[1] = "0.0.0.0"
	end

	return addrs
end


-- @brief 非阻塞启动: 校验依赖 → 建站点 → 端口组 bind → fork accept
-- @param [in] opts[table]		单进程托管覆盖项 (缺省读 ALUA_SVN_* 环境变量)
--   listen[number]				[可选] 监听端口, 默认 ALUA_SVN_LISTEN / 8443
--   addr[string]				[可选] 监听地址串, 默认 ALUA_SVN_ADDR / 0.0.0.0
--   svnserve[string]			[可选] 上游 host:port, 默认 ALUA_SVN_SVNSERVE / 127.0.0.1:3690
--   repo_root[string]			[可选] 仓库父目录, 默认 ALUA_SVN_REPO_ROOT
--   max_body[number]			[可选] 请求体上限, 默认 ALUA_SVN_MAX_BODY / 512MiB
--   edge_host[string]			[可选] Host 重写名, 默认 ALUA_SVN_EDGE_HOST
--   tls[table]					[必填除非已注入] TLS 面; 不再经 core.serve.resolve_tls
--   klbweb[table]				[可选] klbcore.klbweb 模块; 缺省自行 require
--   manifest[table]				[可选] 包清单 (仅日志)
-- @return ok[boolean]
-- @return err[string]			失败原因; ok 时 ""
-- @note embed 须宿主泵送; 前台 M.entry 经 co_util.run_host_loop (CLI hold / 独立自泵); 停止调 M.stop_embed
M.start_embed = function (opts)
	opts = opts or {}

	if g_embed ~= nil then
		return false, "svn_https already started"
	end

	local ok_k, kco = pcall(require, "kco")
	if not ok_k or type(kco) ~= "table" or type(kco.fork) ~= "function" then
		return false, "need kco"
	end

	local ok_l, klblisten = pcall(require, "klbcore.klblisten")
	if not ok_l or type(klblisten) ~= "table" or type(klblisten.listen_group) ~= "function" then
		return false, "need klbcore.klblisten"
	end

	local listen = tonumber(opts.listen)
		or tonumber(os.getenv("ALUA_SVN_LISTEN")) or DEFAULT_LISTEN
	if listen < 1 or 65535 < listen then
		return false, "bad listen port"
	end

	local tls_info, klbweb, code, msg = _check_tls_web(opts)
	if tls_info == nil then
		return false, tostring(msg or ("tls/web init fail " .. tostring(code)))
	end

	-- ksvnra must be preloaded by alua
	local ok_ra, mod = pcall(require, "ksvnra")
	if not ok_ra or type(mod) ~= "table" or type(mod.open) ~= "function" then
		return false, "need ksvnra (alua preload)"
	end

	local max_body = tonumber(opts.max_body) or _max_body()
	local svnserve = opts.svnserve or os.getenv("ALUA_SVN_SVNSERVE") or DEFAULT_SVNSERVE
	local repo_root = opts.repo_root or os.getenv("ALUA_SVN_REPO_ROOT") or ""
	local addrs = _listen_addrs(opts.addr)
	local app = _new_edge_app(klbweb, max_body)
	local dav_cfg = {
		svnserve = svnserve,
		repo_root = repo_root,
	}

	local function handler(req, res)
		dav.handle(req, res, dav_cfg)
	end

	_register_dav(app, handler)

	local specs = {}
	for i = 1, #addrs do
		specs[#specs + 1] = { port = listen, host = addrs[i], schemes = { "http", "https" } }
	end

	local grp, code_l, msg_l = klblisten.listen_group(specs, { cert = tls_info.cert, key = tls_info.key })
	if grp == nil then
		io.stderr:write("alua svn_https: bind fail addr=" .. table.concat(addrs, ",")
			.. " port=" .. tostring(listen) .. "\n")
		log.write("error", "svn_https bind fail", {
			module = "svn_https",
			addr = table.concat(addrs, ","),
			port = listen,
		})
		return false, "bind fail (" .. tostring(code_l) .. "): " .. tostring(msg_l or "")
	end

	local kenv = require("kenv")
	local state = { stopped = false }
	state.app = app
	state.grp = grp
	state.listen = listen
	state.dav_cfg = dav_cfg
	g_embed = state

	kco.fork(function ()
		_accept_loop(app, grp, kenv, state)
	end)

	io.stderr:write(string.format(
		"alua svn_https: https+http://%s:%d -> ksvnra %s  (pkg=%s max_body=%d)\n",
		table.concat(addrs, ","), listen, svnserve,
		tostring((opts.manifest and opts.manifest.name) or "svn_https"), max_body))
	_write_client_hints(listen, opts.edge_host)

	if listen < 1024 and not _can_bind_low_port() then
		_write_low_port_hint(listen)
	end

	return true, ""
end


-- @brief 停止 (幂等): 关端口组 → 关站点 → 关 ra 会话
-- @return ok[boolean]
M.stop_embed = function ()
	if g_embed == nil then
		return true
	end

	local e = g_embed
	g_embed = nil
	e.stopped = true

	if type(e.grp) == "table" and type(e.grp.close) == "function" then
		pcall(function ()
			e.grp:close()
		end)
	end

	if type(e.app) == "table" and type(e.app.stop) == "function" then
		pcall(function ()
			e.app:stop()
		end)
	end

	local cache = nil
	if type(e.dav_cfg) == "table" then
		cache = e.dav_cfg._ra_cache
		e.dav_cfg._ra_cache = {}
	end

	if type(cache) == "table" then
		for _, ra in pairs(cache) do
			pcall(function ()
				ra:close()
			end)
		end
	end

	return true
end


-- @brief 读 /proc/self/status 字段原值 (无该字段时 "")
-- @param [in] name[string]			字段名, 如 "Uid" / "CapEff"
-- @return value[string]
local function _proc_status_field(name)
	local f = io.open("/proc/self/status", "r")
	if f == nil then
		return ""
	end

	local text = f:read("*a") or ""
	f:close()

	local v = string.match(text, "\n" .. name .. ":%s*([^\r\n]+)")
	if v == nil then
		v = string.match(text, "^" .. name .. ":%s*([^\r\n]+)")
	end

	return tostring(v or "")
end


-- @brief 能否绑定低端口 (<1024): root 或 CAP_NET_BIND_SERVICE
-- @note 非 Linux (Windows) 无该限制; 能力位 CAP_NET_BIND_SERVICE = 10
-- @return can[boolean]
local function _can_bind_low_port()
	if package.config:sub(1, 1) == "\\" then
		return true
	end

	local uid = tonumber(string.match(_proc_status_field("Uid"), "^%d+"))
	if uid == 0 then
		return true
	end

	local cap = tonumber(_proc_status_field("CapEff"), 16)
	if cap == nil then
		return false
	end

	return 1 == math.floor(cap / 1024) % 2
end


-- @brief 低端口缺能力时的部署引导 (system 作用域 + AmbientCapabilities)
-- @param [in] listen[number]
local function _write_low_port_hint(listen)
	local user = os.getenv("USER") or "<运行用户>"

	io.stderr:write("alua pkg svn_https: listen " .. tostring(listen)
		.. " <1024 but no CAP_NET_BIND_SERVICE\n")
	io.stderr:write("  use a system-scope unit (do NOT setcap alua):\n")
	io.stderr:write("  [Service]\n")
	io.stderr:write("  User=" .. user .. "\n")
	io.stderr:write("  ExecStart=/usr/local/bin/alua pkg run svn_https " .. tostring(listen) .. "\n")
	io.stderr:write("  AmbientCapabilities=CAP_NET_BIND_SERVICE\n")
	io.stderr:write("  CapabilityBoundingSet=CAP_NET_BIND_SERVICE\n")
	io.stderr:write("  NoNewPrivileges=no\n")
end


-- @brief CLI 入口缺省 TLS 面 (宿主不对 daemon 包注入; start_embed 契约仍要求 opts.tls)
-- @return info[table]			tls_info { tls, plain, cert, key }; 解析失败为空表
local function _entry_tls()
	local ok_t, tls = pcall(require, "klbagent.core.serve.security.tls")
	if not ok_t or type(tls) ~= "table" or type(tls.resolve_tls) ~= "function" then
		return {}
	end

	local info = tls.resolve_tls()
	if type(info) == "table" then
		return info
	end

	return {}
end


-- @brief 大形态: 同进程 dav.lua + ksvnra (方案 B 终态; 含 HTTPv2 写)
-- @param [in] opts[table]		见 M.start_embed
-- @return ok[boolean]
-- @return err[string]
local function main_dav_ksvnra(opts)
	io.stderr:write("alua svn_https: in-process dav+ksvnra (read+write HTTPv2)\n")
	return M.start_embed(opts)
end


-- @brief 包入口: 阻塞形态 (alua pkg run svn_https; 内部 start_embed + 自泵)
-- @param [in] argv[table]
-- @param [in] manifest[table]
-- @return ok[boolean]
-- @return code[number]
-- @return message[string|nil]
M.entry = function (argv, manifest)
	manifest = manifest or {}
	local listen, upstream_url = _parse_args(argv)

	if type(listen) ~= "number" or listen < 1 or 65535 < listen then
		return false, codes.svn_https.BAD_REQUEST, "bad listen port"
	end

	if type(upstream_url) == "string" and upstream_url ~= "" then
		io.stderr:write("alua svn_https: ALUA_SVN_UPSTREAM is deprecated; using dav+ksvnra\n")
	end

	local ok, err = main_dav_ksvnra({ listen = listen, manifest = manifest, tls = _entry_tls() })
	if not ok then
		return false, codes.svn_https.TOOL_FAILED, tostring(err)
	end

	-- CLI 宿主已泵则 hold; 独立进程入口 (--serve / 无 co_task) 才自调 co_main_loop
	local co_util = require("klbagent.core.co_util")
	local mode = co_util.run_host_loop()
	if mode == "self" then
		M.stop_embed()
	end

	return true, codes.OK
end


return M

-- end
