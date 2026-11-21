--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   http.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  net.http_get / net.http_post
-- @note   有代理时 http(s):// 优先自研链 open_tunnel (+https 就地 TLS); 否则 klbhttp / curl
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; 接入 proxy_resolve (klbhttp opts.proxy / curl -x); chain_hops_resolve; socks5h 经 curl / 链; N2-1 http:// + hops 走 open_tunnel (少挂 curl -x); https + hops: open_tunnel + upgrade_tls (去 curl socks); 超时计时改墙钟 (os.clock → kenv/ktime tick_count; Bug.2026-7/10)
--]]


local util = require("klbagent.core.tools.net.util")
local log = require("klbagent.core.log")
local config = require("klbagent.packages.proxy.config")


local M = {}


local MAX_REDIRECTS = 8


-- @brief 墙钟毫秒 (禁 os.clock CPU 计时: IO 等待不推进)
-- @note  ktime 直读系统滴答; kenv 为每轮 loop 缓存, 同轮差值为 0
-- @return [number] ms
local function _now_ms()
	local ok_t, ktime = pcall(require, "ktime")
	if ok_t and type(ktime) == "table" and type(ktime.tick_count) == "function" then
		return ktime.tick_count()
	end

	local ok_e, kenv = pcall(require, "kenv")
	if ok_e and type(kenv) == "table" and type(kenv.tick_count) == "function" then
		return kenv.tick_count()
	end

	return os.time() * 1000
end


M.common_http_args = {
	url = "string http:// or https:// URL",
	timeout_sec = "number optional timeout seconds (default 15)",
	max_bytes = "number optional max response body bytes (default 1048576)",
	headers = "table optional request headers map",
	cookie_jar = "string optional Netscape cookie jar path (send+store; session)",
	cookie = "string optional Cookie header value",
	ca_path = "string optional CA PEM for curl --cacert (https)",
	proxy = "string|table optional proxy (http:// / socks5h:// or {host,port,type,tls}); else ALUA_PROXY; ALUA_NO_PROXY exempts",
	tls = "table optional https upgrade opts: verify/sni/ca_pem/alpn/pin_sha256 (see docs/tools/proxy.md Outbound TLS)",
}


local function _proxy_result_fields(hops, proxy)
	if proxy == nil then
		return nil
	end

	local out = {
		host = proxy.host,
		port = proxy.port,
		tls = proxy.tls == true,
		type = proxy.type or "http-connect",
	}
	if type(hops) == "table" then
		out.chain = { hops = #hops, via = (hops[1] and hops[1].type) or out.type }
	end

	return out
end


local function _curl_proxy_arg(hops, proxy)
	if type(hops) == "table" and hops[1] ~= nil then
		local hop = hops[1]
		local htype = hop.type or "http-connect"
		if htype == "socks5" then
			return string.format("socks5h://%s:%d", hop.host, tonumber(hop.port))
		end

		local scheme = (hop.tls == true) and "https" or "http"
		return string.format("%s://%s:%d", scheme, hop.host, tonumber(hop.port))
	end

	if proxy == nil then
		return nil
	end

	if proxy.type == "socks5" then
		return string.format("socks5h://%s:%d", proxy.host, proxy.port)
	end

	local scheme = proxy.tls and "https" or "http"
	return string.format("%s://%s:%d", scheme, proxy.host, proxy.port)
end


-- @brief URL 路径+查询 (origin-form)
local function _url_request_target(url)
	local rest = string.match(url or "", "^https?://[^/]+(.*)$")
	if rest == nil or rest == "" then
		return "/"
	end

	return rest
end


local function _close_session(sess)
	if sess ~= nil and type(sess.close) == "function" then
		pcall(function ()
			sess.close()
		end)
	end
end


-- @brief 将 klbhttp 失败码映射为可读串 (Bug.2026-29)
-- @param [in] head[any]		error 时多为 number code
-- @param [in] msg[any]			"error" / "exit" 等
-- @return detail[string]
local function _http_fail_detail(head, msg)
	local code = tonumber(head)
	if code ~= nil then
		local ok_k, knetcode = pcall(require, "knetcode")
		if ok_k and type(knetcode) == "table" and type(knetcode.strerror) == "function" then
			local s = knetcode.strerror(code)
			if type(s) == "string" and s ~= "" then
				return tostring(code) .. " " .. s
			end
		end

		return tostring(code)
	end

	if head ~= nil and head ~= "" then
		return tostring(head)
	end

	return tostring(msg or "error")
end


-- @brief 经 open_tunnel 发 HTTP; https 隧道后 upgrade_tls
-- @return [table] result
local function _http_request_chain(method, args, hops, proxy)
	local url = args.url
	if type(url) ~= "string" or string.match(url, "^https?://") == nil then
		error("chain backend needs http:// or https:// URL")
	end

	local ok_c, chain = pcall(require, "klbagent.packages.proxy.chain")
	if not ok_c or type(chain) ~= "table" or type(chain.open_tunnel) ~= "function" then
		error("proxy.chain.open_tunnel unavailable")
	end

	local timeout_sec = util.clamp_timeout(args.timeout_sec)
	local max_bytes = util.clamp_max_bytes(args.max_bytes)
	local timeout_ms = timeout_sec * 1000
	local req_body, headers = util.resolve_body(args)
	headers = util.merge_cookie_headers(headers, args)
	local cookie_jar = args.cookie_jar
	if cookie_jar ~= nil and not util.path_ok(cookie_jar) then
		error("cookie_jar path invalid")
	end

	local cur = url
	local final_url = url
	local last_head = ""
	local last_body = ""
	local status = 0
	local used_tls = false

	for _ = 1, MAX_REDIRECTS do
		local host, port = util.url_host_port(cur)
		local need_tls = (string.match(cur, "^https://") ~= nil)
		local ok_t, sess = chain.open_tunnel(
			{ host = host, port = port },
			hops,
			{ timeout_ms = timeout_ms }
		)
		if not ok_t then
			error("chain open_tunnel fail: " .. tostring(sess))
		end

		if need_tls then
			local last_hop = nil
			if type(hops) == "table" and #hops > 0 then
				last_hop = hops[#hops]
			end
			local cfg_tls = nil
			if type(args) == "table" and type(args.tls) == "table" then
				cfg_tls = { tls = args.tls }
			end
			local tls_opts = config.resolve_tls_opts(cfg_tls, last_hop, host)
			tls_opts.timeout_ms = timeout_ms
			local ok_u, err_u = sess:upgrade_tls(tls_opts, timeout_ms)
			if not ok_u then
				_close_session(sess)
				error("chain upgrade_tls fail: " .. tostring(err_u))
			end
			used_tls = true
		end

		local path = _url_request_target(cur)
		local lines = {
			string.format("%s %s HTTP/1.1", method, path),
		}
		if (not need_tls and port == 80) or (need_tls and port == 443) then
			lines[#lines + 1] = "Host: " .. host
		else
			lines[#lines + 1] = string.format("Host: %s:%d", host, port)
		end
		lines[#lines + 1] = "Connection: close"
		lines[#lines + 1] = "User-Agent: alua-chain/0.1"

		if type(headers) == "table" then
			for k, v in pairs(headers) do
				local lk = string.lower(tostring(k))
				if lk ~= "host" and lk ~= "connection" and lk ~= "content-length" then
					lines[#lines + 1] = string.format("%s: %s", tostring(k), tostring(v))
				end
			end
		end

		local body_s = ""
		if req_body ~= nil then
			body_s = tostring(req_body)
			lines[#lines + 1] = "Content-Length: " .. tostring(#body_s)
		end

		lines[#lines + 1] = ""
		local raw = table.concat(lines, "\r\n") .. "\r\n" .. body_s

		local ok_s, err_s = sess:send(raw)
		if not ok_s then
			_close_session(sess)
			error("chain send fail: " .. tostring(err_s))
		end

		-- 读响应头
		local buf = ""
		local hdr_end = nil
		local t0 = _now_ms()
		while hdr_end == nil do
			if (_now_ms() - t0) > timeout_ms then
				_close_session(sess)
				error("chain recv header timeout")
			end

			local chunk, err_r = sess:recv(4096)
			if type(chunk) == "string" and #chunk > 0 then
				buf = buf .. chunk
				hdr_end = string.find(buf, "\r\n\r\n", 1, true)
			elseif err_r ~= nil and err_r ~= "timeout" then
				_close_session(sess)
				error("chain recv fail: " .. tostring(err_r))
			end

			if #buf > 65536 and hdr_end == nil then
				_close_session(sess)
				error("chain header too large")
			end
		end

		last_head = string.sub(buf, 1, hdr_end + 3)
		local body_buf = string.sub(buf, hdr_end + 4)
		status = util.parse_status(last_head)
		final_url = cur

		local clen = tonumber(util.header_value(last_head, "Content-Length") or "")
		if clen ~= nil then
			while #body_buf < clen do
				if (_now_ms() - t0) > timeout_ms then
					break
				end
				local chunk = sess:recv(math.min(4096, clen - #body_buf))
				if type(chunk) ~= "string" or #chunk == 0 then
					break
				end
				body_buf = body_buf .. chunk
			end
			last_body = string.sub(body_buf, 1, clen)
		else
			-- 无 Content-Length: 读到关闭
			while true do
				if (_now_ms() - t0) > timeout_ms then
					break
				end
				if #body_buf >= max_bytes then
					break
				end
				local chunk = sess:recv(4096)
				if type(chunk) ~= "string" or #chunk == 0 then
					break
				end
				body_buf = body_buf .. chunk
			end
			last_body = body_buf
		end

		_close_session(sess)

		if cookie_jar ~= nil then
			util.jar_store_set_cookie(cookie_jar, last_head, final_url)
		end

		-- GET 跟随重定向 (http/https)
		if method == "GET" and status >= 300 and status < 400 then
			local loc = util.header_value(last_head, "Location")
			local next_url = util.resolve_redirect(cur, loc)
			if next_url ~= nil and next_url ~= cur and string.match(next_url, "^https?://") then
				cur = next_url
				req_body = nil
			else
				break
			end
		else
			break
		end
	end

	local truncated = false
	if #last_body > max_bytes then
		last_body = string.sub(last_body, 1, max_bytes)
		truncated = true
	end

	local result = {
		ok = true,
		status = status,
		method = method,
		url = url,
		final_url = final_url,
		bytes = #last_body,
		size_download = #last_body,
		truncated = truncated,
		max_bytes = max_bytes,
		timeout_sec = timeout_sec,
		headers = last_head,
		body = last_body,
		backend = "chain",
	}
	if cookie_jar ~= nil then
		result.cookie_jar = cookie_jar
	end

	local pref = _proxy_result_fields(hops, proxy)
	if pref ~= nil then
		if type(pref.chain) == "table" then
			pref.chain.via = used_tls and "open_tunnel+tls" or "open_tunnel"
		end
		result.proxy = pref
	end

	return result
end


local function _run_curl(exit_line)
	local ex_ok, ex_what, ex_code = os.execute(exit_line)
	local exit_code = 0
	if ex_ok == true then
		exit_code = 0
	elseif type(ex_code) == "number" then
		exit_code = ex_code
	else
		exit_code = 1
	end

	if ex_what == nil and type(ex_ok) == "number" then
		exit_code = ex_ok
	end

	return exit_code
end


local function _http_request_curl(method, args)
	local url = args.url
	if not util.url_ok(url) then
		error("url must be http:// or https://")
	end

	if not util.curl_available() then
		error("curl not found")
	end

	local timeout_sec = util.clamp_timeout(args.timeout_sec)
	local max_bytes = util.clamp_max_bytes(args.max_bytes)
	local body_path = util.tmp_path("body")
	local hdr_path = util.tmp_path("hdr")
	local meta_path = util.tmp_path("meta")
	local req_body_path = nil

	local req_body, headers = util.resolve_body(args)
	local hdr_args = util.build_header_args(headers)
	local cookie_parts, cookie_jar = util.cookie_args(args)

	local cmd = {
		"curl -sS -L",
		"-X " .. util.shell_quote(method),
		"--max-time " .. tostring(timeout_sec),
		"--max-filesize " .. tostring(max_bytes),
		"--proto " .. util.shell_quote("=http,https"),
		"--proto-redir " .. util.shell_quote("=http,https"),
		"-D " .. util.shell_quote(hdr_path),
		"-o " .. util.shell_quote(body_path),
		"-w " .. util.shell_quote("%{http_code}\n%{url_effective}\n%{size_download}"),
	}

	for i = 1, #hdr_args do
		cmd[#cmd + 1] = hdr_args[i]
	end

	for i = 1, #cookie_parts do
		cmd[#cmd + 1] = cookie_parts[i]
	end

	if req_body ~= nil then
		req_body_path = util.tmp_path("req")
		if not util.write_file(req_body_path, req_body) then
			error("write request body fail")
		end

		cmd[#cmd + 1] = "--data-binary @" .. util.shell_quote(req_body_path)
	end

	-- 可选 CA; 无则 https 自签场景由上层指纹钉扎 / klbhttp VERIFY_NONE
	local ca_path = args.ca_path
	if type(ca_path) == "string" and ca_path ~= "" then
		if not util.path_ok(ca_path) then
			error("ca_path invalid")
		end

		cmd[#cmd + 1] = "--cacert " .. util.shell_quote(ca_path)
	end

	local hops, proxy = util.chain_hops_resolve(url, args)
	local proxy_url = _curl_proxy_arg(hops, proxy)
	if proxy_url ~= nil then
		cmd[#cmd + 1] = "-x " .. util.shell_quote(proxy_url)
	end

	cmd[#cmd + 1] = "--"
	cmd[#cmd + 1] = util.shell_quote(url)
	cmd[#cmd + 1] = ">" .. util.shell_quote(meta_path)
	cmd[#cmd + 1] = "2>/dev/null"
	local line = table.concat(cmd, " ")

	local exit_code = _run_curl(line)

	local meta = util.read_file(meta_path, 4096)
	local status = 0
	local final_url = url
	local size_download = 0
	do
		local p1, p2, p3 = string.match(meta, "^(%d+)\n([^\n]*)\n([^\n]*)")
		if p1 ~= nil then
			status = tonumber(p1) or 0
			final_url = p2 or url
			size_download = tonumber(p3) or 0
		end
	end

	local body, body_n, body_trunc = util.read_file(body_path, max_bytes)
	local hdr_text, _, _ = util.read_file(hdr_path, 16384)

	os.remove(body_path)
	os.remove(hdr_path)
	os.remove(meta_path)
	if req_body_path ~= nil then
		os.remove(req_body_path)
	end

	local transport_ok = (exit_code == 0) or (status > 0 and body_n >= 0)
	if status == 0 and exit_code ~= 0 and body_n == 0 then
		error("http " .. string.lower(method) .. " failed (curl exit " .. tostring(exit_code) .. ")")
	end

	local result = {
		ok = transport_ok,
		status = status,
		method = method,
		url = url,
		final_url = final_url,
		bytes = body_n,
		size_download = size_download,
		truncated = body_trunc or (size_download > max_bytes),
		max_bytes = max_bytes,
		timeout_sec = timeout_sec,
		headers = hdr_text,
		body = body,
		backend = "curl",
	}
	if cookie_jar ~= nil then
		result.cookie_jar = cookie_jar
	end

	local pref = _proxy_result_fields(hops, proxy)
	if pref ~= nil then
		result.proxy = pref
	end

	return result
end


local function _http_request_klbhttp(method, args)
	local url = args.url
	if not util.url_ok(url) then
		error("url must be http:// or https://")
	end

	if not util.klbhttp_available() then
		error("klbhttp unavailable")
	end

	-- klbhttp 仅 co_get/co_post; DELETE/PUT/... 走 curl
	if method ~= "GET" and method ~= "POST" then
		if util.curl_available() then
			return _http_request_curl(method, args)
		end

		error("http method " .. tostring(method) .. " needs curl")
	end

	local kco = require("kco")
	local klbhttp = require("klbcore.klbhttp")
	local co_util = require("klbagent.core.co_util")

	local timeout_sec = util.clamp_timeout(args.timeout_sec)
	local max_bytes = util.clamp_max_bytes(args.max_bytes)
	local req_body, headers = util.resolve_body(args)
	headers = util.merge_cookie_headers(headers, args)
	local cookie_jar = args.cookie_jar
	if cookie_jar ~= nil and not util.path_ok(cookie_jar) then
		error("cookie_jar path invalid")
	end

	local state = {
		done = false,
		err = nil,
		msg = nil,
		head = nil,
		body = nil,
		final_url = url,
	}

	local opts = {
		headers = headers,
	}
	local ctype = util.content_type_of(headers)
	if ctype ~= nil then
		opts.content_type = ctype
	end

	local hops, proxy = util.chain_hops_resolve(url, args)
	-- socks5: klbhttp 无 SOCKS; 有 curl 则改走 curl (仍经 chain_hops_resolve)
	if proxy ~= nil and proxy.type == "socks5" then
		if util.curl_available() then
			return _http_request_curl(method, args)
		end

		error("socks5 proxy needs curl (klbhttp has no socks)")
	end

	if proxy ~= nil then
		opts.proxy = { host = proxy.host, port = proxy.port, tls = proxy.tls == true }
	end

	-- 业务在协程内 co_*; 等待走 co_util.await_done (须 co_main_loop)
	kco.fork(function ()
		local cur = url
		local msg, head, body

		for _ = 1, MAX_REDIRECTS do
			-- 每跳按当前 URL 重算代理 (尊重 ALUA_NO_PROXY; 经链简写)
			local _, hop_proxy = util.chain_hops_resolve(cur, args)
			if hop_proxy ~= nil and hop_proxy.type == "socks5" then
				state.err = "socks5 redirect needs curl"
				state.done = true
				return
			end

			if hop_proxy ~= nil then
				opts.proxy = { host = hop_proxy.host, port = hop_proxy.port, tls = hop_proxy.tls == true }
			else
				opts.proxy = nil
			end

			if method == "POST" then
				msg, head, body = klbhttp.co_post(cur, req_body or "", opts)
			else
				msg, head, body = klbhttp.co_get(cur, opts)
			end

			state.final_url = cur
			state.msg = msg
			state.head = head
			state.body = body

			if msg ~= "text" then
				break
			end

			local status = util.parse_status(head)
			if status < 300 or status >= 400 then
				break
			end

			-- POST 不自动跟随重定向; GET 跟随 Location
			if method ~= "GET" then
				break
			end

			local loc = util.header_value(head, "Location")

			local next_url = util.resolve_redirect(cur, loc)
			if next_url == nil or next_url == cur then
				break
			end

			cur = next_url
			-- 重定向下不再带 body
			req_body = nil
		end

		if cookie_jar ~= nil and type(state.head) == "string" then
			util.jar_store_set_cookie(cookie_jar, state.head, state.final_url)
		end

		state.done = true
	end)

	local ok_w, err_w = pcall(co_util.await_done, state, timeout_sec,
		"http " .. string.lower(method))
	if not ok_w then
		log.write("warn", "http timeout", {
			module = "net",
			method = method,
			url = tostring(args.url or ""),
		})
		error(tostring(err_w))
	end

	if state.err ~= nil then
		error(tostring(state.err))
	end

	if state.msg ~= "text" then
		local detail = _http_fail_detail(state.head, state.msg)
		log.write("warn", "http failed", {
			module = "net",
			method = method,
			url = tostring(args.url or ""),
			err = detail,
		})
		error("http " .. string.lower(method) .. " failed (" .. detail .. ")")
	end

	local body = state.body or ""
	local truncated = false
	if #body > max_bytes then
		body = string.sub(body, 1, max_bytes)
		truncated = true
	end

	local result = {
		ok = true,
		status = util.parse_status(state.head),
		method = method,
		url = url,
		final_url = state.final_url,
		bytes = #body,
		size_download = #(state.body or ""),
		truncated = truncated,
		max_bytes = max_bytes,
		timeout_sec = timeout_sec,
		headers = state.head or "",
		body = body,
		backend = "klbhttp",
	}
	if cookie_jar ~= nil then
		result.cookie_jar = cookie_jar
	end

	local pref = _proxy_result_fields(hops, proxy)
	if pref ~= nil then
		result.proxy = pref
	end

	return result
end


-- @brief 同步 HTTP 请求 (GET/POST)
-- @note http(s):// + hops → 自研链 (+TLS); https+http-connect 亦可 klbhttp; 再 curl
local function _http_request(method, args)
	local hops, proxy = util.chain_hops_resolve(args.url, args)
	local url = args.url

	-- SSE / 特殊方法: 调用方可 force_curl
	if args.force_curl == true then
		if util.curl_available() then
			return _http_request_curl(method, args)
		end

		error("force_curl needs curl")
	end

	-- N2-1 / 余债: 经代理的 http(s) 优先 open_tunnel; https 就地 upgrade_tls
	if type(hops) == "table" and #hops > 0 and type(url) == "string" and string.match(url, "^https?://") then
		local ok_c, chain = pcall(require, "klbagent.packages.proxy.chain")
		if ok_c and type(chain) == "table" and type(chain.open_tunnel) == "function" then
			local ok_r, res = pcall(_http_request_chain, method, args, hops, proxy)
			if ok_r then
				return res
			end

			-- 链失败: socks/多跳无 curl 可退; http-connect+https 可走 klbhttp
			log.write("warn", "chain http fail, fallback", {
				module = "net.http",
				err = tostring(res),
			})
		end
	end

	-- 单跳 http-connect + https: klbhttp CONNECT+upgrade (非 socks)
	-- klbhttp 失败 (如 https ERR_PROTOCOL=4) 回退 curl (M5 / Bug.2026-29 可用性)
	if util.klbhttp_available() then
		if proxy == nil or proxy.type ~= "socks5" then
			local ok_r, res = pcall(_http_request_klbhttp, method, args)
			if ok_r then
				return res
			end

			log.write("warn", "klbhttp fail, fallback curl", {
				module = "net.http",
				err = tostring(res),
			})
		end
	end

	-- 末路 curl (含 socks 环境仍无 TLS 升级时)
	if util.curl_available() then
		return _http_request_curl(method, args)
	end

	if proxy ~= nil and proxy.type == "socks5" then
		error("socks5 https needs kproxy.upgrade_tls or curl")
	end

	error("http backend unavailable (need chain/klbhttp/curl)")
end


local function _http_get(args)
	return _http_request("GET", args)
end


local function _http_post(args)
	return _http_request("POST", args)
end


-- @brief 同步 HTTP 请求 (供 mcp client 等内部复用; 参数同 net.http_*)
M.request = _http_request


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "net.http_get",
		category = "net",
		title = "HTTP/HTTPS GET",
		summary = "Fetch URL (http/https). proxy via chain open_tunnel (+TLS for https); else klbhttp/curl. Optional cookie_jar / ALUA_PROXY.",
		min_profile = "net",
		args_schema = M.common_http_args,
		examples = {
			{ args = { url = "http://127.0.0.1/" } },
			{ args = { url = "https://example.com/", max_bytes = 65536, cookie_jar = "/tmp/alua.jar" } },
			{ args = { url = "https://example.com/", proxy = "http://127.0.0.1:8080" } },
		},
		docs = "README.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("net.http_get", _http_get)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "net.http_post",
		category = "net",
		title = "HTTP/HTTPS POST",
		summary = "POST body or json. proxy via chain (+TLS for https); else klbhttp/curl. Optional cookie_jar / ALUA_PROXY.",
		min_profile = "net",
		args_schema = {
			url = M.common_http_args.url,
			timeout_sec = M.common_http_args.timeout_sec,
			max_bytes = M.common_http_args.max_bytes,
			headers = M.common_http_args.headers,
			cookie_jar = M.common_http_args.cookie_jar,
			cookie = M.common_http_args.cookie,
			proxy = M.common_http_args.proxy,
			body = "string optional raw request body",
			json = "any optional value encoded as JSON body (sets Content-Type)",
			content_type = "string optional Content-Type override",
		},
		examples = {
			{ args = { url = "http://127.0.0.1/api/login", json = { user = "a" }, cookie_jar = "/tmp/alua.jar" } },
			{ args = { url = "https://example.com/", body = "a=1", content_type = "application/x-www-form-urlencoded" } },
		},
		docs = "README.md",
		side_effects = { "net" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("net.http_post", _http_post)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
