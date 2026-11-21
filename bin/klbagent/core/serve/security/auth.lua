--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   auth.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 子模块: 入口鉴权门面 (HTTP / WS / SMP / MNP / RPC 共用)
-- @note   token 取值 (头 / TEXT body / RPC 参) 统一在此; 校验经 ssid LPC 客户端
--   \n 消费线程禁 new_serve (owner 在 worker.ssid); 临时/证明票走 new_client; raw 本地面
--   \n via/host_tool session attrs 注入 (M2) 亦在此, 避免各入口重复
--   \n @stability internal: 实现文件, 契约面见 core/serve/init.lua
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出入口鉴权与 via 指纹
--  \n [2026-11] 接入 ssid.new_client (LPC); ssid+token 鉴权; raw 本机
--  \n [2026-11] ssid 不可用时 AUTH_ERR_SSID_UNAVAIL / serve.UNAVAILABLE
--]]
local codes = require("klbagent.code")
local token = require("klbagent.core.serve.security.token")
local state = require("klbagent.core.serve.base.state")


local M = {}


-- SMP/MNP 面首帧鉴权 TEXT 头 (P2)
M.AUTH_HEAD = "alua.auth"

-- SMP/MNP 面 TEXT 业务头 (= /v1/call 同 envelope)
M.CALL_HEAD = "alua.call"


-- @brief 懒加载授权模块 (避免与 serve 循环依赖)
-- @return mod[table|nil]
M.auth_mod = function ()
	local ok, mod = pcall(require, "klbagent.core.auth")
	if ok and type(mod) == "table" and type(mod.verify) == "function" then
		return mod
	end

	return nil
end


-- @brief 取 ssid LPC 客户端 (消费线程 → owner alua.ssid)
-- @return client[table|nil]
local function _ssid_client()
	local ok_s, pkg = pcall(require, "klbagent.core.ssid")
	if not (ok_s and type(pkg) == "table") then
		return nil
	end

	if type(pkg.new_client) == "function" then
		return pkg.new_client({ transport = "lpc" })
	end

	return nil
end


-- @brief 是否非 raw 形态 (t. 会话票 / p. 证明票)
-- @param [in] tok[string|nil]
-- @return yes[boolean]
local function _is_non_raw_token(tok)
	if type(tok) ~= "string" or tok == "" then
		return false
	end

	if string.sub(tok, 1, 2) == "t." or string.sub(tok, 1, 2) == "p." then
		return true
	end

	return false
end


-- @brief 是否 ssid owner / LPC 不可用类失败
-- @param [in] err_v[string|nil]
-- @param [in] err_code[number|nil]
-- @return yes[boolean]
M.is_ssid_unavail = function (err_v, err_code)
	local am = M.auth_mod()
	if am ~= nil and err_code == am.AUTH_ERR_SSID_UNAVAIL then
		return true
	end

	if type(err_v) ~= "string" or err_v == "" then
		return false
	end

	if string.find(err_v, "ssid client unavailable", 1, true) ~= nil then
		return true
	end

	if string.find(err_v, "ssid lpc", 1, true) ~= nil then
		return true
	end

	if string.find(err_v, "ssid xpc", 1, true) ~= nil then
		return true
	end

	if string.find(err_v, "need kco coroutine", 1, true) ~= nil then
		return true
	end

	return false
end


-- @brief ssid 不可用时的 AUTH_ERR 码 (auth 未装载则 nil)
-- @return code[number|nil]
local function _ssid_unavail_code()
	local am = M.auth_mod()
	if am ~= nil then
		return am.AUTH_ERR_SSID_UNAVAIL
	end

	return nil
end


-- @brief 解析 serve token (嵌入式传入 / 环境变量 / serve.env)
-- @return token[string|nil]
M.token_expect = function ()
	return token.token_expect(state.embed_token)
end


-- @brief 确保 token 过期清扫器已启动 (auth 未装载则忽略)
M.ensure_sweeper = function ()
	local am = M.auth_mod()
	if am ~= nil and type(am.ensure_sweeper) == "function" then
		pcall(am.ensure_sweeper)
	end
end


-- @brief serve/HTTP 入口落 via/host_tool 等同 call/stdio attrs (M2)
-- @param [in] robj[table]				dispatch 请求 (就地注入 session_id)
-- @param [in] via[string]
-- @param [in] headers[table|nil]		HTTP 头 (小写键; 可选)
M.attach_via_fingerprint = function (robj, via, headers)
	if type(robj) ~= "table" then
		return
	end

	if type(via) ~= "string" or via == "" then
		via = "http"
	end

	robj.via = via

	local env_extra = nil
	local http_headers = nil
	if type(headers) == "table" then
		local ua = headers["user-agent"] or headers["User-Agent"]
		local accept = headers["accept"] or headers["Accept"]
		local proto = headers["mcp-protocol-version"] or headers["MCP-Protocol-Version"]
		local sid = headers["mcp-session-id"] or headers["Mcp-Session-Id"]
		http_headers = {
			user_agent = type(ua) == "string" and ua or "",
			accept = type(accept) == "string" and accept or "",
			protocol_version = type(proto) == "string" and proto or "",
			session_id = type(sid) == "string" and sid or "",
		}
		if http_headers.user_agent ~= "" then
			env_extra = {
				http_ua_hash = true,
			}
		end
	end

	local ssid = _ssid_client()
	if not (type(ssid) == "table" and type(ssid.co_begin_ctx) == "function") then
		return
	end

	-- no_bind: fingerprint only; keep serve token bind (raw/temp). Else begin_ctx
	-- defaults kind=local profile=net and demotes deploy/unsafe to profile denied.
	local ctx_ok, ctx = pcall(ssid.co_begin_ctx, {
		via = via,
		skip_token = true,
		no_bind = true,
		profile = "net",
		env_extra = env_extra,
		http_headers = http_headers,
	})
	if not (ctx_ok and type(ctx) == "table") then
		return
	end

	local ok_sess, sess = pcall(require, "klbagent.core.session")
	if not (ok_sess and type(sess) == "table") then
		return
	end

	local cur = nil
	if type(sess.current_id) == "function" then
		cur = sess.current_id()
	end

	local attrs = {
		via = via,
		caller_kind = ctx.caller_kind,
		host_tool = ctx.host_tool or "",
		alua_ssid = ctx.alua_ssid,
	}
	if type(http_headers) == "table" then
		attrs.http_ua = http_headers.user_agent
		attrs.http_accept = http_headers.accept
		attrs.http_protocol_version = http_headers.protocol_version
		attrs.http_session_id = http_headers.session_id
	end

	if type(cur) ~= "string" or cur == "" then
		local info = sess.begin({
			attrs = attrs,
		})
		if type(info) == "table" then
			cur = info.id
		end
	else
		pcall(sess.set_attrs, cur, attrs)
	end

	if type(cur) == "string" and cur ~= "" then
		robj.session_id = cur
	end
end


-- @brief 从 HTTP 头取 token (x-alua-token / Bearer)
-- @param [in] headers[table|nil]
-- @return tok[string|nil]
M.token_from_headers = function (headers)
	headers = headers or {}
	local got = headers["x-alua-token"] or ""
	if type(got) == "string" and got ~= "" then
		return got
	end

	local hauth = headers["authorization"] or ""
	local bearer = string.match(hauth, "^[Bb][Ee][Aa][Rr][Ee][Rr]%s+(.+)$")
	if type(bearer) == "string" and bearer ~= "" then
		return bearer
	end

	return nil
end


-- @brief 从 HTTP 头取 alua_ssid (x-alua-ssid)
-- @param [in] headers[table|nil]
-- @return ssid[string|nil]
M.ssid_from_headers = function (headers)
	headers = headers or {}
	local got = headers["x-alua-ssid"] or headers["X-Alua-Ssid"] or ""
	if type(got) == "string" and got ~= "" then
		return got
	end

	return nil
end


-- @brief 从 SMP/MNP 鉴权 TEXT body 取 token (JSON.token 或原文)
-- @param [in] body[string|nil]
-- @return tok[string|nil]
M.token_from_mux_body = function (body)
	if type(body) ~= "string" or body == "" then
		return nil
	end

	local ok_j, obj = pcall(require("cjson").decode, body)
	if ok_j and type(obj) == "table" and type(obj.token) == "string" and obj.token ~= "" then
		return obj.token
	end

	return body
end


-- @brief 从 SMP/MNP 鉴权 TEXT body 取 alua_ssid (JSON.alua_ssid / ssid)
-- @param [in] body[string|nil]
-- @return ssid[string|nil]
M.ssid_from_mux_body = function (body)
	if type(body) ~= "string" or body == "" then
		return nil
	end

	local ok_j, obj = pcall(require("cjson").decode, body)
	if not (ok_j and type(obj) == "table") then
		return nil
	end

	local sid = obj.alua_ssid or obj.ssid
	if type(sid) == "string" and sid ~= "" then
		return sid
	end

	return nil
end


-- @brief RPC 鉴权参数取 token (string 或 {token=...})
-- @param [in] ...						首参
-- @return tok[string|nil]
M.token_from_rpc_args = function (...)
	local n = select("#", ...)
	if n < 1 then
		return nil
	end

	local a1 = select(1, ...)
	if type(a1) == "string" and a1 ~= "" then
		return a1
	end

	if type(a1) == "table" and type(a1.token) == "string" and a1.token ~= "" then
		return a1.token
	end

	return nil
end


-- @brief RPC 鉴权参数取 alua_ssid ({alua_ssid|ssid=...})
-- @param [in] ...						首参
-- @return ssid[string|nil]
M.ssid_from_rpc_args = function (...)
	local n = select("#", ...)
	if n < 1 then
		return nil
	end

	local a1 = select(1, ...)
	if type(a1) ~= "table" then
		return nil
	end

	local sid = a1.alua_ssid or a1.ssid
	if type(sid) == "string" and sid ~= "" then
		return sid
	end

	return nil
end


-- @brief proto/req 是否 TLS 面
-- @param [in] via_or_req[boolean|table|string]
-- @return is_tls[boolean]
M.tls_of = function (via_or_req)
	if type(via_or_req) == "boolean" then
		return via_or_req
	end

	if type(via_or_req) == "table" then
		if via_or_req.tls == true then
			return true
		end

		return false
	end

	local am = M.auth_mod()
	if am ~= nil and type(am.is_tls_transport) == "function" then
		return am.is_tls_transport(via_or_req)
	end

	if type(via_or_req) == "string" then
		local p = string.lower(via_or_req)
		if p == "https" or p == "wss" or p == "mnps" or p == "smps"
			or p == "mnprpcs" or p == "smprpcs" then
			return true
		end
	end

	return false
end


-- @brief 统一校验: ssid+token (LPC) 或本机 raw; 线上 raw 须 TLS
-- @param [in] tok[string]
-- @param [in] tls_hint[boolean|string|nil]  true/proto 名; 入口必传
-- @param [in] ssid[string|nil]			alua_ssid; 证明票必填, 会话票建议填
-- @return info[table|nil]
-- @return err[string|nil]
-- @return code[number|nil]		auth.AUTH_ERR_*; raw 本机分支可为 nil
M.verify_token = function (tok, tls_hint, ssid)
	local am = M.auth_mod()
	if type(tok) ~= "string" or tok == "" then
		local code = nil
		if am ~= nil then
			code = am.AUTH_ERR_NEED_TOKEN
		end

		return nil, "need token", code
	end

	if type(ssid) ~= "string" or ssid == "" then
		ssid = nil
	end

	-- 临时/证明票: 经 ssid LPC 客户端打 owner (禁本线程 new_serve / 禁传 raw)
	if _is_non_raw_token(tok) then
		local client = _ssid_client()
		if type(client) == "table" and type(client.co_verify_token) == "function" then
			local ok_v, pack = pcall(function ()
				local info, err, code = client.co_verify_token(ssid, tok, { tls = tls_hint })
				return { info, err, code }
			end)
			if not ok_v then
				return nil, "ssid client unavailable: " .. tostring(pack), _ssid_unavail_code()
			end

			local info = pack[1]
			local err = pack[2]
			local code = pack[3]
			if info ~= nil then
				return info, nil, code
			end

			if M.is_ssid_unavail(err, code) then
				if type(err) ~= "string" or err == "" then
					err = "ssid client unavailable"
				end
				if type(code) ~= "number" then
					code = _ssid_unavail_code()
				end
				return nil, err, code
			end

			if type(err) == "string" and err ~= "" then
				return nil, err, code
			end

			return nil, "bad token", code
		end

		return nil, "ssid client unavailable", _ssid_unavail_code()
	end

	-- 原始 token: 仅本机比对 (禁经 LPC/XPC)
	local expect = M.token_expect()
	if expect == nil or not token.token_eq(tok, expect) then
		local code = nil
		if am ~= nil then
			code = am.AUTH_ERR_BAD_TOKEN
		end

		return nil, "bad token", code
	end

	if tls_hint ~= nil and not M.tls_of(tls_hint) then
		local code = nil
		if am ~= nil then
			code = am.AUTH_ERR_RAW_TLS
		end

		return nil, "raw token requires tls", code
	end

	return {
		ok = true,
		kind = "raw",
		tls = M.tls_of(tls_hint),
		profile = "unsafe",
		alua_ssid = ssid,
	}, nil, am and am.AUTH_OK or nil
end


-- @brief 请求头 token 是否通过 (WS 握手门禁用)
-- @param [in] req[table|nil]
-- @return ok[boolean]
M.token_ok = function (req)
	local headers = req and req.headers
	local tok = M.token_from_headers(headers)
	local sid = M.ssid_from_headers(headers)
	local info = M.verify_token(tok, M.tls_of(req), sid)
	return info ~= nil
end


-- @brief 绑定本次请求鉴权信息 (auth 未装载则忽略)
-- @param [in] info[table|nil]
M.bind = function (info)
	local am = M.auth_mod()
	if am ~= nil and type(am.set_request_auth) == "function" then
		am.set_request_auth(info)
	end
end


-- @brief 解绑本次请求鉴权信息 (auth 未装载则忽略)
M.unbind = function ()
	local am = M.auth_mod()
	if am ~= nil and type(am.clear_request_auth) == "function" then
		am.clear_request_auth()
	end
end


-- @brief 回 401 bad token
-- @param [in] res[table]
M.deny_token = function (res)
	res:co_json({
		ok = false,
		code = codes.serve.DENIED,
		message = "bad token",
		name = codes.str(codes.serve.DENIED),
	}, 401)
end


-- @brief HTTP 鉴权失败: ssid 不可用 → UNAVAILABLE; raw+TLS → DENIED 透出; 其余 bad token
-- @param [in] res[table]
-- @param [in] err_v[string|nil]
-- @param [in] err_code[number|nil]
M.deny_http = function (res, err_v, err_code)
	if M.is_ssid_unavail(err_v, err_code) then
		local msg = err_v
		if type(msg) ~= "string" or msg == "" then
			msg = "ssid client unavailable"
		end

		res:co_json({
			ok = false,
			code = codes.serve.UNAVAILABLE,
			message = msg,
			name = codes.str(codes.serve.UNAVAILABLE),
		}, 503)
		return
	end

	local am = M.auth_mod()
	local raw_tls = false
	if am ~= nil and err_code == am.AUTH_ERR_RAW_TLS then
		raw_tls = true
	elseif err_v == "raw token requires tls" then
		raw_tls = true
	end

	if raw_tls then
		res:co_json({
			ok = false,
			code = codes.serve.DENIED,
			message = err_v or "raw token requires tls",
			name = codes.str(codes.serve.DENIED),
		}, 401)
		return
	end

	if type(err_v) == "string" and err_v ~= "" and err_v ~= "bad token" then
		local deny_plain = false
		if am ~= nil and err_code == am.AUTH_ERR_BAD_TOKEN then
			deny_plain = true
		end

		if not deny_plain then
			res:co_json({
				ok = false,
				code = codes.serve.DENIED,
				message = err_v,
				name = codes.str(codes.serve.DENIED),
			}, 401)
			return
		end
	end

	M.deny_token(res)
end


return M

-- end
