--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   common.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua status — 各模块共用的探测 / 路径 / 输出小工具
-- @note   无分派逻辑; 供 status/init 与 status/<module> 复用; 禁 token 真值
-- @history 修改历史
--  \n [2026-10] 创建文件 (由 status.lua 拆分)
--]]


local cjson = require("cjson")
local config = require("klbagent.core.config")


local M = {}


-- json / 机读契约版本 (M3); 字段演进时升版
M.SCHEMA = "alua.status.v1"


local function _is_windows()
	return package.config:sub(1, 1) == "\\"
end


-- POSIX sh 单引号; Win cmd 用双引号 (单引号会被原样交给 curl 导致空响应)
local function _shell_quote(s)
	s = tostring(s)
	if _is_windows() then
		return '"' .. string.gsub(s, '"', '\\"') .. '"'
	end

	return "'" .. string.gsub(s, "'", "'\\''") .. "'"
end


local function _path_join(base, rel)
	if type(base) ~= "string" or base == "" then
		return rel or ""
	end

	base = string.gsub(base, "[/\\]+$", "")
	local sep = _is_windows() and "\\" or "/"
	return base .. sep .. tostring(rel or "")
end


local function _popen_line(cmd)
	local f = io.popen(cmd)
	if f == nil then
		return nil
	end

	local line = f:read("*l")
	f:close()
	if type(line) ~= "string" then
		return nil
	end

	line = string.gsub(line, "%s+$", "")
	if line == "" then
		return nil
	end

	return line
end


local function _popen_all(cmd)
	local f = io.popen(cmd)
	if f == nil then
		return nil
	end

	local data = f:read("*a") or ""
	f:close()
	return data
end


-- @brief 以退出码判命令成败 (只看 rc, 不取内容)
-- @param [in] cmd[string]
-- @return ok[boolean]
local function _popen_ok(cmd)
	local f = io.popen(cmd)
	if f == nil then
		return false
	end

	f:read("*a")
	local ok = f:close()
	return ok == true or ok == 0
end


local function _file_exists(path)
	if type(path) ~= "string" or path == "" then
		return false
	end

	local f = io.open(path, "rb")
	if f == nil then
		return false
	end

	f:close()
	return true
end


local function _token_source()
	local env = os.getenv("ALUA_SERVE_TOKEN")
	if type(env) == "string" and env ~= "" then
		return "env"
	end

	local dir = config.dir()
	if type(dir) == "string" and dir ~= "" then
		dir = string.gsub(dir, "[/\\]+$", "")
		if _file_exists(_path_join(dir, "serve.env")) then
			return "serve.env"
		end
	end

	-- core.serve.token_expect 包外已收口; 无法探测 other
	return "none"
end


-- @brief 脚本安装根 (…/klbagent)
local function _scripts_root()
	local src = package.searchpath("klbagent.core.cli.status", package.path)
	if type(src) ~= "string" or src == "" then
		return ""
	end

	local root = string.match(src, "^(.*)[/\\]core[/\\]cli[/\\]status[/\\]init%.lua$")
	if type(root) ~= "string" then
		root = string.match(src, "^(.*)[/\\]core[/\\]cli[/\\]status%.lua$")
	end

	if type(root) ~= "string" then
		return ""
	end

	if _is_windows() then
		root = string.gsub(root, "/", "\\")
	end

	return root
end


-- @brief 配置文件与临时目录路径表
local function _paths_info(pref)
	pref = string.gsub(tostring(pref or ""), "[/\\]+$", "")
	local paths = {
		pref = pref,
		runtime_json = _path_join(pref, "runtime.json"),
		log_json = _path_join(pref, "log.json"),
		env_overlay_json = _path_join(pref, "env_overlay.json"),
		proxy_json = _path_join(pref, "proxy.json"),
		serve_json = _path_join(pref, "serve.json"),
		peers_json = _path_join(pref, "peers.json"),
		serve_env = _path_join(pref, "serve.env"),
		serve_port = _path_join(pref, "serve.port"),
		serve_crt = _path_join(pref, "serve.crt"),
		serve_key = _path_join(pref, "serve.key"),
		alua_log = _path_join(pref, "alua.log"),
		scripts_root = _scripts_root(),
		temp = {},
	}

	-- M1: 确保缺失 pref JSON 落盘
	do
		local ok_c, cfg = pcall(require, "klbagent.core.config")
		if ok_c and type(cfg) == "table" and type(cfg.ensure_pref_json) == "function" then
			pcall(cfg.ensure_pref_json)
		end
	end

	local serve_log = os.getenv("ALUA_SERVE_LOG")
	if type(serve_log) == "string" and serve_log ~= "" then
		paths.serve_log = serve_log
	end

	if _is_windows() then
		paths.temp.TEMP = os.getenv("TEMP") or ""
		paths.temp.TMP = os.getenv("TMP") or ""
		local la = os.getenv("LOCALAPPDATA") or ""
		if la ~= "" then
			paths.temp.LocalAppData_Temp = la .. "\\Temp"
		end
		-- win alua: 对标 Linux ~/.tmp → %TEMP%\.tmp (常为 Local\Temp\.tmp)
		local base = paths.temp.TEMP
		if base == "" then
			base = paths.temp.LocalAppData_Temp
		end
		if base ~= "" then
			paths.temp.ai_tmp = base .. "\\.tmp"
			os.execute(string.format('mkdir "%s" 2>NUL', string.gsub(paths.temp.ai_tmp, "/", "\\")))
		end
	else
		paths.temp.TMPDIR = os.getenv("TMPDIR") or ""
		paths.temp.tmp = "/tmp"
		-- 工作区约定 ai 临时 (对标 win %TEMP%\.tmp)
		local home = os.getenv("HOME") or ""
		if home ~= "" then
			paths.temp.ai_tmp = home .. "/.tmp"
			os.execute(string.format("mkdir -p %q", paths.temp.ai_tmp))
		end
	end

	-- Lua os.tmpname 样例父目录 (仅展示; 立即删试探文件)
	do
		local ok_t, sample = pcall(os.tmpname)
		if ok_t and type(sample) == "string" and sample ~= "" then
			pcall(os.remove, sample)
			local parent = string.match(sample, "^(.*)[/\\][^/\\]+$")
			paths.temp.lua_tmpname_dir = parent or sample
		end
	end

	return paths
end


-- @brief 本机探活候选 host
-- Win: 优先网卡 IPv4 (loopback 常挂起/鉴权异常), 127.0.0.1 放最后
local function _probe_hosts()
	local lan = {}
	local seen = {}

	local function add(ip)
		if type(ip) ~= "string" or ip == "" or seen[ip] then
			return
		end

		if string.match(ip, "^127%.") or string.match(ip, "^169%.254%.") then
			return
		end

		if not string.match(ip, "^%d+%.%d+%.%d+%.%d+$") then
			return
		end

		seen[ip] = true
		lan[#lan + 1] = ip
	end

	if _is_windows() then
		local raw = _popen_all("ipconfig 2>NUL") or ""
		for ip in string.gmatch(raw, "(%d+%.%d+%.%d+%.%d+)") do
			add(ip)
		end
	else
		local raw = _popen_all("hostname -I 2>/dev/null") or ""
		for ip in string.gmatch(raw, "%S+") do
			add(ip)
		end
	end

	local hosts = {}
	for i = 1, #lan do
		hosts[#hosts + 1] = lan[i]
	end

	hosts[#hosts + 1] = "127.0.0.1"
	return hosts
end


-- @brief /v1/health 成功体 (须 alua 业务 ok, 非 DENIED)
local function _health_body_ok(obj)
	if type(obj) ~= "table" then
		return false
	end

	if obj.ok == true and (obj.service == "alua" or type(obj.version) == "string") then
		return true
	end

	return false
end


local function _health_err_of(obj, fallback)
	if type(obj) ~= "table" then
		return fallback or "empty"
	end

	local parts = {}
	if type(obj.name) == "string" and obj.name ~= "" then
		parts[#parts + 1] = obj.name
	end

	if type(obj.message) == "string" and obj.message ~= "" then
		parts[#parts + 1] = obj.message
	elseif type(obj.err) == "string" and obj.err ~= "" then
		parts[#parts + 1] = obj.err
	end

	if type(obj.code) == "number" then
		parts[#parts + 1] = "code=" .. tostring(math.floor(obj.code))
	end

	if #parts > 0 then
		return table.concat(parts, " ")
	end

	if obj.ok == false then
		return "health_not_ok"
	end

	return fallback or "health_not_ok"
end


-- @brief TCP/HTTP 探活 (本机口是否有监听)
-- @return listening[boolean]
-- @return how[string]
-- @return host[string|nil]		成功探到的 host
local function _tcp_listening(port)
	port = tonumber(port)
	if type(port) ~= "number" or port < 1 or 65535 < port then
		return false, "bad_port", nil
	end

	if not _is_windows() then
		-- Linux/WSL: bash /dev/tcp; 本机口 1s 超时 (mirrored/WSL 下 SYN 无回应会挂死)
		local line = _popen_line(string.format(
			"timeout 1 bash -c 'echo >/dev/tcp/127.0.0.1/%d' 2>/dev/null && echo yes || echo no",
			port))
		if line == "yes" then
			return true, "bash_tcp", "127.0.0.1"
		end
	end

	local null_dev = _is_windows() and "NUL" or "/dev/null"
	local err_redir = _is_windows() and "2>NUL" or "2>/dev/null"
	local write_fmt = _is_windows() and "-w %%{http_code}" or "-w '%{http_code}'"
	local noproxy = _is_windows() and "--noproxy *" or "--noproxy '*'"
	-- Win loopback 可能挂起: 127 用 1s; 其它 2s
	local hosts = _probe_hosts()
	for i = 1, #hosts do
		local host = hosts[i]
		local tmo = (host == "127.0.0.1") and 1 or 2
		local code = _popen_line(string.format(
			"curl -sS -m %d -o %s %s %s http://%s:%d/v1/health %s || echo 000",
			tmo, null_dev, write_fmt, noproxy, host, port, err_redir))
		if type(code) == "string" and code ~= "000" and code ~= "" then
			return true, "curl", host
		end
	end

	return false, "probe", nil
end


-- @brief 本机 serve.json tls_face (缺省 mnp; 与 core.serve.tls_over_default 对齐)
local function _serve_tls_face()
	local ok_m, mod = pcall(require, "klbagent.core.config.serve")
	if ok_m and type(mod) == "table" and type(mod.load) == "function" then
		local ok_l, cfg = pcall(mod.load)
		if ok_l and type(cfg) == "table" and type(cfg.tls_face) == "string" then
			local face = string.lower(cfg.tls_face)
			if face == "http" or face == "https" then
				return "http"
			end

			if face == "smp" or face == "smps" then
				return "smp"
			end

			if face == "mnp" or face == "mnps" then
				return "mnp"
			end
		end
	end

	return "mnp"
end


-- @brief 经 mnps/smps 探活 (raw token 须 TLS; 默认 TLS 面非 https)
-- @return ok[boolean]
-- @return body[table|nil]
-- @return err[string|nil]
-- @return host[string|nil]
local function _health_via_mux(port, token, scheme, host)
	if type(host) ~= "string" or host == "" then
		host = "127.0.0.1"
	end

	local ok_t, transport = pcall(require, "klbagent.core.tools.remote.transport")
	if not ok_t or type(transport) ~= "table" or type(transport.peer_tool) ~= "function" then
		return false, nil, "no_mux_transport", nil
	end

	-- selfcheck 可能因 ai_load 等 ok=false; 鉴权+连通成功即视为 health ok
	local ok_sc, sc_or_err = pcall(function ()
		return transport.peer_tool(
			host, port, token, 5, "runtime.selfcheck", {}, nil, scheme, nil, nil)
	end)
	if ok_sc and type(sc_or_err) == "table" and type(sc_or_err.version) == "string" then
		local body = {
			ok = true,
			service = "alua",
			version = sc_or_err.version,
			profile = sc_or_err.profile,
		}
		local si = sc_or_err.serve
		if type(si) == "table" and type(si.transport_mux_tls) == "table" then
			body.transport = si.transport_mux_tls
		end

		return true, body, nil, host
	end

	local ok_v, ver_or_err = pcall(function ()
		return transport.peer_tool(
			host, port, token, 5, "runtime.version", {}, nil, scheme, nil, nil)
	end)
	if ok_v and type(ver_or_err) == "table" and type(ver_or_err.string) == "string" then
		return true, {
			ok = true,
			service = "alua",
			version = ver_or_err.string,
		}, nil, host
	end

	local err = "mux_health_fail"
	if not ok_sc and type(sc_or_err) == "string" and sc_or_err ~= "" then
		err = sc_or_err
	elseif not ok_v and type(ver_or_err) == "string" and ver_or_err ~= "" then
		err = ver_or_err
	end

	return false, nil, err, host
end


-- @brief curl GET /v1/health (http 或 https -k; 不经 ALUA_PROXY)
local function _health_http_get(port, token, prefer_host, use_https)
	local err_redir = _is_windows() and "2>NUL" or "2>/dev/null"
	local noproxy = _is_windows() and "--noproxy *" or "--noproxy '*'"
	local hosts = {}
	-- Win: 勿优先 loopback (常先返回 DENIED/空再误判); 有 prefer 且非 127 才置顶
	if type(prefer_host) == "string" and prefer_host ~= "" and prefer_host ~= "127.0.0.1" then
		hosts[#hosts + 1] = prefer_host
	end

	local all = _probe_hosts()
	for i = 1, #all do
		local h = all[i]
		if h ~= prefer_host then
			hosts[#hosts + 1] = h
		elseif prefer_host == "127.0.0.1" then
			-- prefer 是 loopback: 仍按 all 顺序 (LAN 在前)
		end
	end

	-- prefer 为 127 时补到末尾再试
	if prefer_host == "127.0.0.1" then
		local has127 = false
		for i = 1, #hosts do
			if hosts[i] == "127.0.0.1" then
				has127 = true
				break
			end
		end

		if not has127 then
			hosts[#hosts + 1] = "127.0.0.1"
		end
	end

	local last_err = "empty"
	local last_body = nil
	local last_host = nil
	local scheme = use_https and "https" or "http"
	local curl_k = use_https and "-k" or ""
	for i = 1, #hosts do
		local host = hosts[i]
		local tmo = (host == "127.0.0.1") and 1 or 2
		local url = string.format("%s://%s:%d/v1/health", scheme, host, port)
		local cmd = string.format(
			"curl -sS %s -m %d %s -H %s %s %s",
			curl_k,
			tmo,
			noproxy,
			_shell_quote("X-Alua-Token: " .. token),
			_shell_quote(url),
			err_redir)
		local raw = _popen_all(cmd)
		if type(raw) == "string" and raw ~= "" then
			raw = string.gsub(raw, "^\239\187\191", "")
			raw = string.match(raw, "^%s*(.-)%s*$") or raw
			local ok_j, obj = pcall(cjson.decode, raw)
			if ok_j and type(obj) == "table" then
				if _health_body_ok(obj) then
					return true, obj, nil, host
				end

				last_body = obj
				last_host = host
				last_err = _health_err_of(obj, "health_not_ok")
				-- DENIED / 非 alua health: 继续试下一 host
			else
				last_err = "bad_json"
			end
		end
	end

	return false, last_body, last_err, last_host
end


-- @brief 本机 serve 探活 (按 tls_face: mnps/smps / https; 明文 http 仅回落)
-- @param [in] prefer_host[string|nil]
-- @return ok[boolean]
-- @return body[table|nil]
-- @return err[string|nil]
-- @return host[string|nil]
local function _health_get(port, token, prefer_host)
	port = tonumber(port)
	if type(port) ~= "number" then
		return false, nil, "bad_port", nil
	end

	if type(token) ~= "string" or token == "" then
		return false, nil, "no_token", nil
	end

	-- 去空白/CR (serve.env CRLF 残留会导致 bad token)
	token = string.gsub(token, "[\r\n]", "")
	token = string.match(token, "^%s*(.-)%s*$") or token
	if token == "" then
		return false, nil, "no_token", nil
	end

	local face = _serve_tls_face()
	local mux_host = "127.0.0.1"
	if type(prefer_host) == "string" and prefer_host ~= "" then
		mux_host = prefer_host
	end

	-- 默认 TLS 面 = mnps: raw token 须 TLS, 且同口无 https (见 docs/tools/remote.md)
	if face == "mnp" then
		local ok_m, body_m, err_m, host_m = _health_via_mux(port, token, "mnps", mux_host)
		if ok_m then
			return true, body_m, nil, host_m
		end

		-- 回落明文 http (临时 token 可过; raw 会 DENIED raw token requires tls)
		local ok_h, body_h, err_h, host_h = _health_http_get(port, token, prefer_host, false)
		if ok_h then
			return true, body_h, nil, host_h
		end

		return false, body_h, err_m or err_h, host_h or host_m
	end

	if face == "smp" then
		local ok_m, body_m, err_m, host_m = _health_via_mux(port, token, "smps", mux_host)
		if ok_m then
			return true, body_m, nil, host_m
		end

		local ok_h, body_h, err_h, host_h = _health_http_get(port, token, prefer_host, false)
		if ok_h then
			return true, body_h, nil, host_h
		end

		return false, body_h, err_m or err_h, host_h or host_m
	end

	-- tls_face=http: 先 https(-k), 再明文 http
	local ok_s, body_s, err_s, host_s = _health_http_get(port, token, prefer_host, true)
	if ok_s then
		return true, body_s, nil, host_s
	end

	local ok_h, body_h, err_h, host_h = _health_http_get(port, token, prefer_host, false)
	if ok_h then
		return true, body_h, nil, host_h
	end

	return false, body_h or body_s, err_h or err_s, host_h or host_s
end


local function _unit_linux(name)
	if type(name) ~= "string" or name == "" then
		name = "alua-serve.service"
	end

	local out = {
		kind = "systemd",
		active = nil,
		enabled = nil,
		scope = nil,
		name = name,
	}

	-- 开发机常为 --user unit; system 无单元时 is-active 也常回 inactive, 勿早退挡住 user
	local sys_active = _popen_line("systemctl is-active " .. name .. " 2>/dev/null")
	local sys_enabled = _popen_line("systemctl is-enabled " .. name .. " 2>/dev/null")
	if sys_active == "active" then
		out.active = sys_active
		out.enabled = sys_enabled
		out.scope = "system"
		return out
	end

	local usr_active = _popen_line("systemctl --user is-active " .. name .. " 2>/dev/null")
	local usr_enabled = _popen_line("systemctl --user is-enabled " .. name .. " 2>/dev/null")
	if usr_active == "active" or usr_active == "inactive" or usr_active == "failed" then
		out.active = usr_active
		out.enabled = usr_enabled
		out.scope = "user"
		return out
	end

	if sys_active == "inactive" or sys_active == "failed" then
		out.active = sys_active
		out.enabled = sys_enabled
		out.scope = "system"
		return out
	end

	out.active = "unknown"
	out.enabled = "unknown"
	return out
end


-- SysV init.d (嵌入式; 与 setup/serve.lua 生成脚本同名)
local INITD_DIR = "/etc/init.d"
local INITD_NAME = "S99_alua"
local INITD_MARK = "# alua-managed: generated by alua setup serve"


local function _read_text(path)
	local f = io.open(path, "rb")
	if f == nil then
		return nil
	end

	local data = f:read("*a") or ""
	f:close()
	if data == "" then
		return nil
	end

	return data
end


-- @brief 嵌入式 init.d 常驻单元 (缺省 S99_alua; web 为 S99_alua_web)
-- @param [in] name[string|nil]
-- @return unit[table]
local function _unit_initd(name)
	if type(name) ~= "string" or name == "" then
		name = INITD_NAME
	end

	local path = INITD_DIR .. "/" .. name
	local out = {
		kind = "initd",
		name = name,
		path = path,
		installed = false,
		managed = false,
		enabled = false,
		active = "unknown",
	}

	local content = _read_text(path)
	if content == nil then
		return out
	end

	out.installed = true
	out.managed = string.find(content, INITD_MARK, 1, true) ~= nil
	out.enabled = _popen_ok("test -x " .. _shell_quote(path))
	out.active = _popen_ok("sh " .. _shell_quote(path) .. " status 2>/dev/null")
		and "active" or "inactive"

	return out
end


local function _unit_windows(name)
	if type(name) ~= "string" or name == "" then
		name = "klua\\alua-serve"
	end

	local out = {
		kind = "task_scheduler",
		name = name,
		state = "unknown",
		note = "Task Scheduler",
	}

	-- Status 行常含「正在运行」/ Running; 编码可能是 GBK, 用 keywords 宽松匹配
	local raw = _popen_all('schtasks /Query /TN "' .. name .. '" /FO LIST 2>NUL')
	if type(raw) ~= "string" or raw == "" then
		out.note = "not found or schtasks unavailable"
		return out
	end

	local low = string.lower(raw)
	if string.find(raw, "正在运行", 1, true) ~= nil
		or string.find(low, "running", 1, true) ~= nil then
		out.state = "running"
	elseif string.find(raw, "就绪", 1, true) ~= nil
		or string.find(low, "ready", 1, true) ~= nil then
		out.state = "ready"
	elseif string.find(low, "disabled", 1, true) ~= nil then
		out.state = "disabled"
	else
		out.state = "present"
	end

	return out
end


local function _yn(v)
	if v == true then
		return "yes"
	end

	if v == false then
		return "no"
	end

	return tostring(v)
end


local function _print_kv(k, v)
	print(string.format("%-14s %s", k .. ":", tostring(v)))
end


-- @brief 仅测 TCP 口是否可连 (proxy 入口无 /v1/health)
-- @param [in] host[string]
-- @param [in] port[number]
-- @return ok[boolean]
-- @return how[string]
local function _tcp_port_open(host, port)
	host = tostring(host or "127.0.0.1")
	port = tonumber(port)
	if type(port) ~= "number" or port < 1 or 65535 < port then
		return false, "bad_port"
	end

	if not _is_windows() then
		-- 本机 1s / 其它 2s (与 curl 分支一致); 无超时则在丢包目标上永久阻塞
		local tmo = (host == "127.0.0.1") and 1 or 2
		local line = _popen_line(string.format(
			"timeout %d bash -c 'echo >/dev/tcp/%s/%d' 2>/dev/null && echo yes || echo no",
			tmo, host, port))
		if line == "yes" then
			return true, "bash_tcp"
		end
	end

	return false, "probe"
end


-- @brief 链名列表 (稳定排序)
-- @param [in] chains[table|nil]
-- @return names[table]
local function _chain_names(chains)
	local names = {}
	if type(chains) ~= "table" then
		return names
	end

	for k, v in pairs(chains) do
		if type(k) == "string" and type(v) == "table" then
			names[#names + 1] = k
		end
	end

	table.sort(names)
	return names
end


-- @brief 生效链 id (proxy.use → default 角色 → routes 兜底 → 首链)
-- @param [in] cfg[table]
-- @return name[string|nil]
-- @return via[string]
local function _active_chain_name(cfg)
	if type(cfg) ~= "table" then
		return nil, "none"
	end

	local ok_px, px = pcall(require, "klbagent.packages.proxy.config")
	if ok_px and type(px) == "table" and type(px.active_chain_id) == "function" then
		local id = px.active_chain_id(cfg)
		if type(id) == "string" and id ~= "" then
			if type(px.use_id) == "function" then
				local u = px.use_id()
				if type(u) == "string" and u ~= "" then
					return id, "use"
				end
			end
			return id, "default"
		end
	end

	if type(cfg.routes) == "table" then
		for i = 1, #cfg.routes do
			local r = cfg.routes[i]
			if type(r) == "table" then
				local m = string.lower(tostring(r.match or ""))
				if m == "default" or m == "*" then
					local cn = r.chain
					if cn == nil or cn == "" then
						cn = "default"
					end
					if ok_px and type(px) == "table" and type(px.resolve_chain_ref) == "function" then
						cn = px.resolve_chain_ref(cfg, cn) or cn
					end
					return tostring(cn), "route"
				end
			end
		end
	end

	local names = _chain_names(cfg.chains)
	if #names > 0 then
		return names[1], "first"
	end

	return nil, "none"
end


-- @brief 端口整数串 (cjson 常出 float)
-- @param [in] p[any]
-- @return text[string]
local function _port_str(p)
	local n = tonumber(p)
	if type(n) ~= "number" then
		return "?"
	end

	return tostring(math.floor(n + 0.5))
end


-- @brief hop 脱敏摘要 (禁 token 真值)
-- @param [in] hop[table]
-- @return row[table]
local function _hop_row(hop)
	if type(hop) ~= "table" then
		return { type = "?", host = "?", port = 0, token_set = false }
	end

	-- 已是脱敏行则直通
	if hop.token_set ~= nil and hop.token == nil and hop.token_ref == nil
		and type(hop.type) == "string" and type(hop.host) == "string" then
		return {
			type = hop.type,
			host = hop.host,
			port = tonumber(hop.port) or 0,
			token_set = hop.token_set == true,
		}
	end

	local tok = false
	if type(hop.token) == "string" and hop.token ~= "" then
		tok = true
	elseif type(hop.token_ref) == "string" and hop.token_ref ~= "" then
		tok = true
	end

	return {
		type = tostring(hop.type or "?"),
		host = tostring(hop.host or "?"),
		port = tonumber(hop.port) or 0,
		token_set = tok,
	}
end


-- @brief 单行 hop 摘要 (原始 hop 或已脱敏行均可)
-- @param [in] hops[table|nil]
-- @return text[string]
local function _hops_one_line(hops)
	if type(hops) ~= "table" or #hops == 0 then
		return "direct"
	end

	local parts = {}
	for i = 1, #hops do
		local row = _hop_row(hops[i])
		local tok = row.token_set and "token=yes" or "token=no"
		parts[#parts + 1] = string.format("%s %s:%s %s",
			row.type, row.host, _port_str(row.port), tok)
	end

	return table.concat(parts, " -> ")
end


-- @brief proxy 运行面摘要 (pref + listen 探活; ≠ 仅 ALUA_PROXY)
-- @param [in] pref[string]
-- @return proxy[table]
local function _proxy_runtime_summary(pref)
	local env_slice = {
		configured = false,
		invalid = false,
		env = "",
		no_proxy = "",
	}
	do
		local ok_u, util = pcall(require, "klbagent.core.tools.net.util")
		if ok_u and type(util) == "table" and type(util.proxy_status) == "function" then
			local st = util.proxy_status()
			if type(st) == "table" then
				env_slice.configured = st.configured == true
				env_slice.invalid = st.invalid == true
				env_slice.env = st.env or ""
				env_slice.no_proxy = st.no_proxy or ""
			end
		end
	end

	local path = _path_join(pref, "proxy.json")
	local file_ok = _file_exists(path)
	local cfg = nil
	do
		local ok_m, mod = pcall(require, "klbagent.core.config.proxy")
		if ok_m and type(mod) == "table" and type(mod.load) == "function" then
			local ok_l, c = pcall(mod.load)
			if ok_l and type(c) == "table" then
				cfg = c
			end
		end
	end

	local listen = {
		host = "127.0.0.1",
		port = 12818,
		protocols = { "socks5", "http-connect", "alua-tunnel" },
	}
	if type(cfg) == "table" and type(cfg.listen) == "table" then
		if type(cfg.listen.host) == "string" and cfg.listen.host ~= "" then
			listen.host = cfg.listen.host
		end
		local p = tonumber(cfg.listen.port)
		if type(p) == "number" and p >= 1 and p <= 65535 then
			listen.port = p
		end
		if type(cfg.listen.protocols) == "table" and #cfg.listen.protocols > 0 then
			listen.protocols = cfg.listen.protocols
		end
	end

	local listen_ok, listen_how = _tcp_port_open(listen.host, listen.port)
	local names = _chain_names(cfg and cfg.chains)
	local active, active_via = _active_chain_name(cfg or {})
	local active_hops = {}
	local hop_summary = "direct"
	if type(cfg) == "table" and type(cfg.chains) == "table"
		and type(active) == "string" and type(cfg.chains[active]) == "table" then
		local hops = cfg.chains[active]
		hop_summary = _hops_one_line(hops)
		for i = 1, #hops do
			active_hops[#active_hops + 1] = _hop_row(hops[i])
		end
	end

	local has_chains = #names > 0
	local configured = file_ok or has_chains or listen_ok or env_slice.configured
	local default_role = nil
	local debug_role = nil
	if type(cfg) == "table" then
		if type(cfg.default) == "string" and cfg.default ~= "" then
			default_role = cfg.default
		end
		if type(cfg.debug) == "string" and cfg.debug ~= "" then
			debug_role = cfg.debug
		end
	end

	return {
		configured = configured,
		running = listen_ok,
		listen = listen,
		listen_ok = listen_ok,
		listen_probe = listen_how,
		active_chain = active,
		active_via = active_via,
		default_role = default_role,
		debug_role = debug_role,
		chain_count = #names,
		chain_names = names,
		active_hops = active_hops,
		hop_summary = hop_summary,
		file = file_ok,
		pref_path = path,
		env = env_slice,
		-- 保留旧字段兼容 (env 切片; 勿再当「未配置」唯一依据)
		invalid = env_slice.invalid,
	}
end


-- @brief 读 proxy.json (normalize 后)
-- @return cfg[table|nil]
local function _load_proxy_cfg()
	local ok_m, mod = pcall(require, "klbagent.core.config.proxy")
	if not ok_m or type(mod) ~= "table" or type(mod.load) ~= "function" then
		return nil
	end

	local ok_l, c = pcall(mod.load)
	if ok_l and type(c) == "table" then
		return c
	end

	return nil
end


M.is_windows = _is_windows
M.shell_quote = _shell_quote
M.path_join = _path_join
M.popen_line = _popen_line
M.popen_all = _popen_all
M.file_exists = _file_exists
M.token_source = _token_source
M.scripts_root = _scripts_root
M.paths_info = _paths_info
M.probe_hosts = _probe_hosts
M.health_body_ok = _health_body_ok
M.health_err_of = _health_err_of
M.tcp_listening = _tcp_listening
M.health_get = _health_get
M.unit_linux = _unit_linux
M.unit_initd = _unit_initd
M.unit_windows = _unit_windows
M.yn = _yn
M.print_kv = _print_kv
M.tcp_port_open = _tcp_port_open
M.chain_names = _chain_names
M.active_chain_name = _active_chain_name
M.port_str = _port_str
M.hop_row = _hop_row
M.hops_one_line = _hops_one_line
M.proxy_runtime_summary = _proxy_runtime_summary
M.load_proxy_cfg = _load_proxy_cfg


return M

-- end
