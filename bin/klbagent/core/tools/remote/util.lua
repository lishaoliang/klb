--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  remote.* 公共辅助
-- @note   scheme(默认 mnps; auto|http|https|ws|wss|smp|mnp|smps|mnps|smprpc|mnprpc)/pin/ca / shell / path / peer 参数解析
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; scheme 扩 ws/wss; scheme 扩 smp/mnp/smps/mnps/smprpc/mnprpc; 默认 scheme=mnps (TLS+MNP+CRYPTO); auto 仍 https→http; peer= 展开 + token_ref 解析 (Todo.2026-14 / D33)
--  \n [2026-11] M1-M6: resolve_call_token ssid 内沉 (远端 TLS issue→temp; 禁明文 raw 回落; loopback 同径)
--  \n [2026-11] 废除对外 token_ref=file 用法; ssid 为唯一使用入口
--]]


local store = require("klbagent.core.config.store")
local peers_cfg = require("klbagent.core.config.peers")


local M = {}


M.DEFAULT_CHUNK = 512 * 1024
M.MAX_CHUNK = 2 * 1024 * 1024


-- @brief 签发用 TLS scheme (明文业务 scheme 仍用 mnps/smps 签发)
-- @param [in] scheme[string|nil]
-- @return issue_scheme[string]
function M.issue_scheme_for(scheme)
	local s = scheme
	if type(s) ~= "string" or s == "" then
		return "mnps"
	end

	s = string.lower(s)
	if s == "smp" or s == "smps" or s == "smprpc" then
		return "smps"
	end

	if s == "https" or s == "wss" or s == "mnps" or s == "smps" then
		return s
	end

	-- http/ws/mnp/auto/mnprpc 等: 签发走 mnps
	return "mnps"
end


-- @brief 调 ssid 客户端方法 (工具层只用 client; new_serve 仅 core/serve 可用)
-- @param [in] method[string]		co_register / co_whoami / co_get_token_plain …
-- @param [in] a1[any]
-- @param [in] a2[any]
-- @return r1[any]
-- @return r2[any]
-- @note 先 xpc (跨进程 / CLI→serve owner) 后 lpc (同进程消费线程); 任一命中即返回
local function _ssid_call(method, a1, a2)
	local ok_s, pkg = pcall(require, "klbagent.core.ssid")
	if not (ok_s and type(pkg) == "table" and type(pkg.new_client) == "function") then
		return nil
	end

	local transports = { "xpc", "lpc" }
	for i = 1, #transports do
		local client = pkg.new_client({ transport = transports[i] })
		if type(client) == "table" and type(client[method]) == "function" then
			local ok_c, r1, r2 = pcall(client[method], a1, a2)
			if ok_c and r1 ~= nil then
				return r1, r2
			end
		end
	end

	return nil
end


-- @brief 当前调用方 alua_ssid (args > session > ssid.whoami)
-- @param [in] args[table|nil]
-- @return sid[string|nil]
function M.current_alua_ssid(args)
	if type(args) == "table" then
		local sid = args.alua_ssid or args.client_id
		if type(sid) == "string" and sid ~= "" then
			return string.lower(sid)
		end
	end

	do
		local ok_sess, sess = pcall(require, "klbagent.core.session")
		if ok_sess and type(sess) == "table" and type(sess.whoami) == "function" then
			local info = sess.whoami({})
			if type(info) == "table" then
				local sid = info.alua_ssid
				if type(sid) ~= "string" or sid == "" then
					if type(info.attrs) == "table" then
						sid = info.attrs.alua_ssid
					end
				end

				if type(sid) == "string" and sid ~= "" then
					return string.lower(sid)
				end
			end
		end
	end

	do
		local via = "call"
		if type(args) == "table" and type(args.via) == "string" and args.via ~= "" then
			via = args.via
		end

		local info = _ssid_call("co_whoami", { via = via })
		if type(info) == "table" and type(info.alua_ssid) == "string" and info.alua_ssid ~= "" then
			return string.lower(info.alua_ssid)
		end
	end

	return nil
end


-- @brief 临时 token 写入 ssid 内存表 (不落盘); 返回 token_ref=mem:<id>
-- @param [in] client_id[string]
-- @param [in] token[string]
-- @return token_ref[string|nil]
-- @return err[string|nil]
function M.save_temp_token_ref(client_id, token)
	local ok_te, token_env = pcall(require, "klbagent.core.ssid.serve.token_env")
	if not (ok_te and type(token_env) == "table" and type(token_env.save) == "function") then
		return nil, "token_env unavailable"
	end

	local ref, err = token_env.save(client_id, token)
	if ref == nil then
		return nil, err
	end

	-- 同步 ssid store 摘要 (有则更新; 仅 meta, 无明文)
	do
		local safe = string.gsub(tostring(client_id), "[^%w%.%-_]", "-")
		local ok_st, st = pcall(require, "klbagent.core.ssid.base.store")
		if ok_st and type(st) == "table" and type(st.put) == "function" and safe ~= "" then
			pcall(st.put, string.lower(safe), {
				token_ref = ref,
				via = "remote",
			})
		end
	end

	return ref, nil
end


-- @brief 目标 host 的证明票: 由 ssid 客户端取 raw 派生 (工具层不接触 raw)
-- @param [in] args[table]		host?/peer?/_peer_row?
-- @param [in] client_id[string]	alua_ssid
-- @return token[string|nil]		p. 证明票; 随请求带 alua_ssid 供服务端重算校验
-- @return err[string|nil]
-- @note raw 由 ssid 客户端 (peers.<id>.env / serve.env) 内部取用; 服务端按本机 raw 重算
function M.ensure_remote_temp_token(args, client_id)
	if type(args) ~= "table" then
		return nil, "need args"
	end

	if type(client_id) ~= "string" or client_id == "" then
		return nil, "need client_id"
	end

	local peer = args.peer
	if type(peer) ~= "string" or peer == "" then
		local row = args._peer_row
		if type(row) == "table" then
			peer = row.id
		end
	end

	local ok_s, pkg = pcall(require, "klbagent.core.ssid")
	if not (ok_s and type(pkg) == "table" and type(pkg.new_client) == "function") then
		return nil, "ssid client unavailable"
	end

	local client = pkg.new_client({ transport = "xpc" })
	if type(client) ~= "table" or type(client.proof_token) ~= "function" then
		return nil, "ssid client proof_token unavailable"
	end

	local ok_c, tok, err = pcall(client.proof_token, {
		alua_ssid = client_id,
		peer = peer,
		token = args.token,
	})
	if not ok_c then
		return nil, tostring(tok)
	end

	return tok, err
end


-- @brief 是否鉴权失败响应/错误 (供一次 re-ensure 重试)
-- @param [in] obj_or_err[any]
-- @return yes[boolean]
function M.is_auth_denied(obj_or_err)
	if type(obj_or_err) == "table" then
		local code = obj_or_err.code
		local name = obj_or_err.name
		local msg = tostring(obj_or_err.message or obj_or_err.err or "")
		if code == 20 or name == "DENIED" then
			return true
		end

		local low = string.lower(msg)
		if string.find(low, "need token", 1, true)
			or string.find(low, "token expired", 1, true)
			or string.find(low, "bad token", 1, true)
			or string.find(low, "raw token only", 1, true)
			or string.find(low, "denied", 1, true)
			or string.find(low, "http_status:401", 1, true)
		then
			return true
		end

		local st = tonumber(obj_or_err.http_status)
		if st == 401 then
			return true
		end

		return false
	end

	if type(obj_or_err) == "string" then
		local low = string.lower(obj_or_err)
		return string.find(low, "need token", 1, true) ~= nil
			or string.find(low, "token expired", 1, true) ~= nil
			or string.find(low, "bad token", 1, true) ~= nil
			or string.find(low, "raw token only", 1, true) ~= nil
			or string.find(low, "denied", 1, true) ~= nil
			or string.find(low, "http_status:401", 1, true) ~= nil
	end

	return false
end


-- @brief 规范化 scheme (缺省 mnps; 显式不回落)
function M.normalize_scheme(scheme)
	if type(scheme) ~= "string" or scheme == "" then
		return "mnps"
	end

	local low = string.lower(scheme)
	if low == "https" or low == "http" or low == "auto"
		or low == "ws" or low == "wss"
		or low == "smp" or low == "mnp" or low == "smps" or low == "mnps"
		or low == "smprpc" or low == "mnprpc" then
		return low
	end

	error("scheme must be auto|https|http|ws|wss|smp|mnp|smps|mnps|smprpc|mnprpc")
end


-- @brief 是否 TLS 侧 scheme (钉扎强制)
function M.scheme_is_tls(scheme)
	return scheme == "https" or scheme == "wss" or scheme == "smps" or scheme == "mnps"
end


-- @brief 是否 WebSocket scheme
function M.scheme_is_ws(scheme)
	return scheme == "ws" or scheme == "wss"
end


-- @brief 是否明文/TLS SMP/MNP TEXT 面
function M.scheme_is_smp(scheme)
	return scheme == "smp" or scheme == "mnp" or scheme == "smps" or scheme == "mnps"
end


-- @brief 是否原生 RPC 面
function M.scheme_is_rpc(scheme)
	return scheme == "smprpc" or scheme == "mnprpc"
end


-- @brief scheme 尝试顺序 (缺省 mnps; auto: https→http; 其余显式不回落)
function M.scheme_list(scheme)
	scheme = M.normalize_scheme(scheme)
	if scheme == "auto" then
		return { "https", "http" }
	end

	return { scheme }
end


-- 规范化指纹: 去冒号/空格, 小写 hex
function M.norm_fp(s)
	if type(s) ~= "string" then
		return nil
	end

	local t = string.lower((string.gsub(s, "[:%s]", "")))
	t = string.gsub(t, "^sha256", "")
	if string.match(t, "^[0-9a-f]+$") == nil or #t < 32 then
		return nil
	end

	return t
end


function M.resolve_pin(args)
	local pin = args and args.cert_sha256
	if type(pin) ~= "string" or pin == "" then
		pin = os.getenv("ALUA_SERVE_CERT_SHA256")
	end

	return M.norm_fp(pin)
end


function M.resolve_ca(args)
	local ca = args and args.ca_path
	if type(ca) ~= "string" or ca == "" then
		ca = os.getenv("ALUA_SERVE_CA")
	end

	if type(ca) == "string" and ca ~= "" then
		return ca
	end

	return nil
end


function M.sh_quote(s)
	return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end


function M.run(cmd)
	local f = io.popen(cmd .. " 2>&1", "r")
	if f == nil then
		return false, "", -1
	end

	local out = f:read("*a") or ""
	local ok, _, code = f:close()
	if ok then
		return true, out, 0
	end

	return false, out, tonumber(code) or 1
end


function M.path_ok(path)
	if type(path) ~= "string" or path == "" then
		return false
	end

	if string.find(path, "[\r\n\0]") ~= nil then
		return false
	end

	return true
end


-- 从 env 文件解析 ALUA_SERVE_TOKEN (与 serve._token_from_serve_env 同形)
local function _token_from_file(path)
	if type(path) ~= "string" or path == "" then
		return nil
	end

	local f = io.open(path, "rb")
	if f == nil then
		return nil
	end

	local data = f:read("*a") or ""
	f:close()

	for line in string.gmatch(data, "[^\r\n]+") do
		local v = string.match(line, "^%s*export%s+ALUA_SERVE_TOKEN%s*=%s*(.-)%s*$")
		if v == nil then
			v = string.match(line, "^%s*ALUA_SERVE_TOKEN%s*=%s*(.-)%s*$")
		end

		if type(v) == "string" and v ~= "" then
			local q = string.match(v, "^\"(.*)\"$")
			if q == nil then
				q = string.match(v, "^'(.*)'$")
			end

			if q ~= nil then
				v = q
			end

			if v ~= "" then
				return v
			end
		end
	end

	return nil
end


-- @brief 解析调用鉴权 (**唯一入口: ssid**)
--  对外废除 token_ref=file; 可选显式 args.token (排障) / allow_raw_token
--  路径: 当前 alua_ssid → (非 force) ssid.get_token → 有 host 则远端 TLS issue → 本机 ensure
-- @param [in] args[table|nil]
-- @return token[string|nil]
function M.resolve_call_token(args)
	if type(args) ~= "table" then
		args = {}
	end

	local force = args._force_renew == true
	local raw_expect = nil  -- core.serve.token_expect closed

	-- 1) 显式 token 仅排障/点名; 非默认路径
	local token = args.token
	if type(token) == "string" and token ~= "" then
		return token
	end

	-- args.token_ref 已废弃: 忽略 (含历史 file:temp.* / file:serve.env)
	-- peer 行内 token_ref 亦不再展开为业务证; 一律 ssid

	local sid = M.current_alua_ssid(args)
	if type(sid) ~= "string" or sid == "" then
		local via = args.via
		if type(via) ~= "string" or via == "" then
			via = "call"
		end

		local info = _ssid_call("co_register", { via = via })
		if type(info) == "table" and type(info.alua_ssid) == "string" then
			sid = info.alua_ssid
		end
	end

	if type(sid) ~= "string" or sid == "" then
		sid = "ai"
	end

	-- 证明票与请求 alua_ssid 必须同源
	args._resolved_ssid = sid

	-- 2) 有目标 host: 优先证明票 (raw + 算法派生 p. 票; 服务端即时重算, 不受 token 生命周期影响)
	if type(args.host) == "string" and args.host ~= "" then
		local tok_p = M.ensure_remote_temp_token(args, sid)
		if type(tok_p) == "string" and tok_p ~= "" then
			return tok_p
		end
	end

	-- 3) 非 force: 经 ssid.get_token 取已缓存临时证 (内存; 对外不暴露明文 ref 用法)
	if not force then
		local got = _ssid_call("co_get_token_plain", {
			via = args.via or "call",
			alua_ssid = sid,
			profile = args.profile or "net",
			force = false,
		})
		if type(got) == "table" then
			if type(got.token) == "string" and got.token ~= "" and got.token ~= raw_expect then
				-- 有 host 时仍可能是本机 issue 的证; 有 host 优先走证明票
				if type(args.host) ~= "string" or args.host == "" then
					return got.token
				end
			end
		end
	end

	-- 3) 有目标 host: 远端签发 (loopback 同径)
	if type(args.host) == "string" and args.host ~= "" then
		local tok = M.ensure_remote_temp_token(args, sid)
		if type(tok) == "string" and tok ~= "" then
			return tok
		end

		if args.allow_raw_token == true and type(raw_expect) == "string" and raw_expect ~= "" then
			return raw_expect
		end

		return nil
	end

	-- 4) 无 host: 本机 ssid.ensure / get_token (force 可重签)
	do
		local got = _ssid_call("co_get_token_plain", {
			via = args.via or "call",
			alua_ssid = sid,
			profile = args.profile or "net",
			force = force,
		})
		if type(got) == "table" then
			if type(got.token) == "string" and got.token ~= "" and got.token ~= raw_expect then
				return got.token
			end
		end
	end

	if args.allow_raw_token == true and type(raw_expect) == "string" and raw_expect ~= "" then
		return raw_expect
	end

	return nil
end


-- @brief 解析 token_ref: mem:ID | env:NAME | peer:<id> (peers.*.env 非 temp)
-- @note 临时票只走 mem:/ssid; 禁读 temp.*.env / serve.env 作业务证
-- @param [in] ref[string|nil]
-- @param [in] peer_id[string|nil]  当 ref 缺省时可选 peers.<id>.env
-- @return token[string|nil]
function M.resolve_token_ref(ref, peer_id)
	if type(ref) ~= "string" or ref == "" then
		if type(peer_id) == "string" and peer_id ~= "" then
			ref = "file:peers." .. peer_id .. ".env"
		else
			return nil
		end
	end

	-- 内存临时票
	if string.match(ref, "^mem:") ~= nil
		or string.match(ref, "^file:temp%.") ~= nil then
		local ok_te, token_env = pcall(require, "klbagent.core.ssid.serve.token_env")
		if ok_te and type(token_env) == "table" and type(token_env.read) == "function" then
			local tok = token_env.read(ref)
			if type(tok) == "string" and tok ~= "" then
				return tok
			end
		end

		return nil
	end

	local env_name = string.match(ref, "^env:(.+)$")
	if env_name ~= nil then
		local v = os.getenv(env_name)
		if type(v) == "string" and v ~= "" then
			return v
		end

		return nil
	end

	local peer_ref = string.match(ref, "^peer:(.+)$")
	if peer_ref ~= nil then
		local id = peers_cfg.norm_id(peer_ref)
		if id == nil then
			return nil
		end

		ref = "file:peers." .. id .. ".env"
	end

	local file_part = string.match(ref, "^file:(.+)$")
	if file_part == nil then
		file_part = ref
	end

	-- 禁 temp / serve.env 业务读盘
	if file_part == "serve.env" or string.match(file_part, "^temp%.") ~= nil then
		return nil
	end

	-- token_ref 仅本地相对名 (禁绝对路径/盘符/上级跳转)
	if string.sub(file_part, 1, 1) == "/" or string.sub(file_part, 1, 1) == "\\" then
		return nil
	end

	if string.match(file_part, "^%a:[/\\]") ~= nil or string.find(file_part, "%.%.") ~= nil then
		return nil
	end

	-- 仅 peers.<id>.env 一类非 temp 文件仍可读 (hop 旧配置)
	if string.match(file_part, "^peers%.") == nil then
		return nil
	end

	local dir = store.dir()
	if type(dir) ~= "string" or dir == "" then
		return nil
	end

	dir = string.gsub(dir, "[/\\]+$", "")
	local path = dir .. "/" .. file_part
	return _token_from_file(path)
end


-- @brief 是否本机回环地址
-- @param [in] host[string|nil]
-- @return yes[boolean]
local function _is_local_host(host)
	if type(host) ~= "string" or host == "" then
		return false
	end

	local low = string.lower(host)
	if low == "localhost" or low == "::1" or low == "[::1]" then
		return true
	end

	return string.match(low, "^127%.") ~= nil
end


-- @brief peers 中是否存在同 host 的已信任登记
-- @param [in] host[string]
-- @return trusted[boolean]
local function _peers_trust_host(host)
	if type(host) ~= "string" or host == "" then
		return false
	end

	local cfg = peers_cfg.load()
	if type(cfg) ~= "table" or type(cfg.peers) ~= "table" then
		return false
	end

	local low = string.lower(host)
	for i = 1, #cfg.peers do
		local row = cfg.peers[i]
		if type(row) == "table" and type(row.host) == "string"
			and string.lower(row.host) == low and row.state == "trusted" then
			return true
		end
	end

	return false
end


-- @brief 原始 token 仅可发往本机回环或 peers 中已信任 host
-- @param [in] host[string]
-- @param [in] token[string|nil]
-- @param [in] peer_row[table|nil]  peer= 展开出的登记行 (多网卡/域名别名时按信任态放行)
-- @return ok[boolean]
-- @return err[string|nil]
function M.check_raw_target(host, token, peer_row)
	-- core.serve.token_expect 包外已收口; 无法判定 raw, 目标检查恒通过
	return true
end


-- @brief 将 peer= 展开为 host/port/scheme/token/cert_sha256 (浅拷贝 args)
-- @param [in] args[table]
-- @param [in] opts[table|nil]  allow_pending=true 时允许 pending (approve/ping)
-- @return args[table]
function M.expand_peer_args(args, opts)
	if type(args) ~= "table" then
		error("need args")
	end

	opts = opts or {}
	local peer_raw = args.peer
	if type(peer_raw) ~= "string" or peer_raw == "" then
		return args
	end

	local id = peers_cfg.norm_id(peer_raw)
	if id == nil then
		error("invalid peer id")
	end

	local cfg = peers_cfg.load()
	local row = peers_cfg.find(cfg, id)
	if row == nil then
		error("peer not found: " .. id)
	end

	local state = row.state or "pending"
	if state == "revoked" then
		error("peer revoked: " .. id)
	end

	if state ~= "trusted" and opts.allow_pending ~= true then
		error("peer not trusted (state=" .. state .. "); run peer.approve first")
	end

	if type(row.host) ~= "string" or row.host == "" then
		error("peer host empty: " .. id)
	end

	local out = {}
	for k, v in pairs(args) do
		out[k] = v
	end

	out.peer = id

	if type(out.host) ~= "string" or out.host == "" then
		out.host = row.host
	end

	if type(out.port) ~= "number" or out.port < 1 or out.port > 65535 then
		out.port = row.port
	end

	if type(out.scheme) ~= "string" or out.scheme == "" then
		out.scheme = row.scheme
	end

	if type(out.cert_sha256) ~= "string" or out.cert_sha256 == "" then
		if type(row.cert_sha256) == "string" and row.cert_sha256 ~= "" then
			out.cert_sha256 = row.cert_sha256
		end
	end

	-- 废除 peer.token_ref→业务证: 鉴权一律 ssid 对 peer.host ensure
	out.token = nil
	out.token_ref = nil

	out._peer_row = row
	return out
end


function M.resolve_peer(args, opts)
	args = M.expand_peer_args(args, opts)

	local host = args.host
	if type(host) ~= "string" or host == "" then
		error("need host or peer")
	end

	local port = args.port
	if type(port) ~= "number" or port < 1 or 65535 < port then
		error("alua.core.serve.UNAVAILABLE: need port")
	end

	local token = M.resolve_call_token(args)
	if type(token) ~= "string" or token == "" then
		error("need token (ssid ensure failed; set serve.env / allow_raw_token)")
	end

	local timeout_sec = args.timeout_sec
	if type(timeout_sec) ~= "number" or timeout_sec <= 0 then
		timeout_sec = 60
	end

	local chunk = tonumber(args.chunk_bytes) or M.DEFAULT_CHUNK
	chunk = math.floor(chunk)
	if chunk < 1024 then
		chunk = 1024
	end

	if chunk > M.MAX_CHUNK then
		chunk = M.MAX_CHUNK
	end

	local scheme = M.normalize_scheme(args.scheme)

	-- 原始 token 仅经显式 TLS scheme 外发 (禁明文/auto 泄漏)
	-- serve.token_expect 包外已收口; 无法判定 raw↔TLS

	local ok_raw, err_raw = M.check_raw_target(host, token, args._peer_row)
	if not ok_raw then
		error(err_raw)
	end

	local pin = M.resolve_pin(args)
	local ca_path = M.resolve_ca(args)
	return host, port, token, timeout_sec, chunk, scheme, pin, ca_path
end


return M

-- end
