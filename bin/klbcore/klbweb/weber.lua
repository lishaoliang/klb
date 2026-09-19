--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   weber.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 站点对象
-- @note   路由 / 静态 / listen; 内部使用 klbhttp
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kco = require("kco")
local router = require("klbcore.klbweb.router")
local staticer = require("klbcore.klbweb.static")
local parser = require("klbcore.klbweb.parser")
local responser = require("klbcore.klbweb.responser")
local mw = require("klbcore.klbweb.mw")
local pathex = require("klbcore.util.pathex")
local klbhttp = require("klbcore.klbhttp")


--------------------------------------------------------------------------------------------
-- 前置定义

local KLB_SOCKET_CONNECT = 63
local KLB_NETCODE_WBUF_EMPTY = 70
local DEFAULT_MAX_BODY = 256 * 1024
local DEFAULT_MAX_KA = 100

local BODY_404 = [[<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>404</title>
</head>
<body>
<h1>Not Found</h1>
<p>404</p>
</body>
</html>
]]


--------------------------------------------------------------------------------------------
-- 内部实现

local function path_prefixed(prefix, path)
	if "/" == prefix then
		return true
	end

	if path == prefix then
		return true
	end

	local head = prefix .. "/"
	return head == string.sub(path, 1, #head)
end


local function normalize_mw_prefix(prefix)
	prefix = string.gsub(prefix, "\\", "/")
	if "/" ~= prefix then
		prefix = string.gsub(prefix, "/+$", "")
		if "" == prefix then
			prefix = "/"
		end
	end

	if "/" ~= string.sub(prefix, 1, 1) then
		prefix = "/" .. prefix
	end

	return prefix
end


local function want_keep_alive(app, ver, headers)
	if app and app._cfg and false == app._cfg.keep_alive then
		return false
	end

	local conn = string.lower(headers["connection"] or "")
	if string.find(conn, "close", 1, true) then
		return false
	end

	if string.find(conn, "keep-alive", 1, true) then
		return true
	end

	if ver and string.find(ver, "1%.1") then
		return true
	end

	return false
end


local function max_body_of(app, path)
	local paths = app._cfg.max_body_paths
	if "table" == type(paths) then
		for prefix, lim in pairs(paths) do
			if "string" == type(prefix) and "number" == type(lim) then
				if path_prefixed(prefix, path) then
					return lim
				end
			end
		end
	end

	return app._cfg.max_body or DEFAULT_MAX_BODY
end


local function co_recv_until_text(serve)
	while true do
		local msg, head, body = serve:co_recv()
		if "text" == msg then
			return "text", head or "", body or ""
		elseif "exit" == msg then
			return "exit", "", ""
		elseif "error" == msg then
			local code = tonumber(head) or 1
			if KLB_SOCKET_CONNECT ~= code and KLB_NETCODE_WBUF_EMPTY ~= code then
				return "error", code, ""
			end
		else
			return "error", 1, ""
		end
	end
end


local function co_wait_wbuf_empty(serve)
	while true do
		local msg, head, body = serve:co_recv()
		if "error" == msg then
			local code = tonumber(head) or 0
			if KLB_NETCODE_WBUF_EMPTY == code then
				return "ok", "", ""
			end
			if KLB_SOCKET_CONNECT ~= code then
				return "error", "", ""
			end
		elseif "exit" == msg then
			return "exit", "", ""
		elseif "text" == msg then
			return "text", head or "", body or ""
		else
			return "error", "", ""
		end
	end
end


local function co_reply(app, serve, status_line, mime, body, keep_alive, omit_body, extra)
	body = body or ""
	mime = mime or "text/plain"
	local payload = body
	if omit_body then
		payload = ""
	end

	local rc = serve:send(responser.pack_head(app, status_line, mime, #body, keep_alive, extra), payload)
	if 0 ~= rc then
		serve:disconnect()
		return false, nil, nil
	end

	local msg, head, pbody = co_wait_wbuf_empty(serve)
	if "text" == msg then
		if not keep_alive then
			serve:disconnect()
			return false, nil, nil
		end

		return true, head, pbody
	end

	if "ok" ~= msg then
		serve:disconnect()
		return false, nil, nil
	end

	if not keep_alive then
		serve:disconnect()
		return false, nil, nil
	end

	return true, nil, nil
end


local function make_res(app, serve, keep_alive, omit_body, out)
	local res
	local function finish(status_line, mime, body)
		if res._sent then
			return
		end

		local alive, ph, pb = co_reply(app, serve, status_line, mime, body, res._keep_alive, res._omit_body, res._headers)
		res._sent = true
		out.alive = alive
		out.pending_head = ph
		out.pending_body = pb
	end

	res = responser.make(app, finish)
	res._keep_alive = keep_alive
	res._omit_body = omit_body
	return res
end


local function send_error(app, req, res, code, mime, body)
	if res._sent then
		return
	end

	local pages = app._cfg.pages
	if "table" == type(pages) and "string" == type(pages[code]) then
		res:co_status(code, "text/html", pages[code])
		return
	end

	if "function" == type(app._cfg.on_error) then
		pcall(app._cfg.on_error, req, res, code)
		if res._sent then
			return
		end
	end

	if 404 == code and (not mime or "text/html" == mime) then
		res:co_status(404, "text/html", body or BODY_404)
		return
	end

	res:co_status(code, mime or "text/plain", body or "")
end


local function apply_extra(res, extra)
	extra = extra or {}
	for i = 1, #extra do
		res:header(extra[i][1], extra[i][2])
	end
end


local function co_run_handler(tag, url_rel, fn, req, res)
	local ok, err = pcall(fn, req, res)
	if not ok then
		print("handler error", tag, url_rel, err)
		if not res._sent then
			send_error(req._app, req, res, 500, "text/plain", "handler error")
		end

		return
	end

	if not res._sent then
		res:co_status(204, "text/plain", "")
	end
end


local function co_run_fns(app, tag, url_rel, fns, req, res, idx)
	if res._sent then
		return
	end

	local fn = fns[idx]
	if not fn then
		if not res._sent then
			res:co_status(204, "text/plain", "")
		end

		return
	end

	if idx == #fns then
		co_run_handler(tag, url_rel, fn, req, res)
		return
	end

	local proceeded = false
	local function nxt()
		if proceeded or res._sent then
			return
		end

		proceeded = true
		co_run_fns(app, tag, url_rel, fns, req, res, idx + 1)
	end

	local ok, err = pcall(fn, req, res, nxt)
	if not ok then
		print("middleware error", tag, req.path, err)
		if not res._sent then
			send_error(app, req, res, 500, "text/plain", "middleware error")
		end

		return
	end

	if not proceeded and not res._sent then
		res:co_status(204, "text/plain", "")
	end
end


local function co_run_mws(app, req, res, idx, tag, after)
	if res._sent then
		after()
		return
	end

	local item = app._mws[idx]
	if not item then
		after()
		return
	end

	if not path_prefixed(item.prefix, req.path) then
		co_run_mws(app, req, res, idx + 1, tag, after)
		return
	end

	local proceeded = false
	local function nxt()
		if proceeded or res._sent then
			return
		end

		proceeded = true
		co_run_mws(app, req, res, idx + 1, tag, after)
	end

	local ok, err = pcall(item.fn, req, res, nxt)
	if not ok then
		print("middleware error", tag, req.path, err)
		if not res._sent then
			send_error(app, req, res, 500, "text/plain", "middleware error")
		end

		return
	end

	if not proceeded and not res._sent then
		res:co_status(204, "text/plain", "")
	end
end


local function spa_accepts_html(headers)
	local accept = string.lower((headers and headers["accept"]) or "")
	if "" == accept then
		return true
	end

	if string.find(accept, "text/html", 1, true) then
		return true
	end

	if string.find(accept, "*/*", 1, true) then
		return true
	end

	return false
end


local function co_serve_static(app, req, res, tag, method, url_rel)
	local status, mime, sbody, _, extra = app._static:serve(url_rel, req.headers)
	if 400 == status then
		res:co_status(400, mime, sbody)
		return
	end

	if 413 == status then
		send_error(app, req, res, 413, mime, sbody)
		return
	end

	if 416 == status then
		apply_extra(res, extra)
		res:co_status(416, mime, sbody)
		return
	end

	if 0 == status or 404 == status then
		local spa = app._cfg.spa
		if spa and ("GET" == method or "HEAD" == method) and spa_accepts_html(req.headers) then
			local spa_path = true == spa and "/index.html" or spa
			local st, sm, sb, _, se = app._static:serve(spa_path, req.headers)
			if 200 == st or 304 == st then
				apply_extra(res, se)
				res:co_status(st, sm, sb)
				return
			end
		end

		send_error(app, req, res, 404, "text/html", BODY_404)
		return
	end

	apply_extra(res, extra)
	if 304 == status then
		res:co_status(304, mime, "")
		return
	end

	res:co_status(status, mime, sbody)
end


local function parse_body_fields(app, req, headers, body)
	req.form = parser.parse_form(headers, body)
	req.files = {}
	local json, jerr = parser.parse_json(headers, body)
	req.json = json
	req.json_error = jerr

	local ctype = string.lower(headers["content-type"] or "")
	if string.find(ctype, "multipart/form-data", 1, true) then
		local form, files, err = parser.parse_multipart(headers, body, app._cfg.upload_dir or "", app._cfg.max_file or (2 * 1024 * 1024))
		if "" ~= err then
			return err
		end

		req.form = form
		req.files = files
		req.body = ""
	end

	return ""
end


local function co_dispatch(app, serve, tag, head, body)
	local out = {
		alive = false,
		pending_head = nil,
		pending_body = nil,
	}

	local method, raw_path, qs, ver = parser.parse_request(head)
	local url_rel = pathex.filter_path(raw_path)
	if not url_rel then
		local dummy = make_res(app, serve, false, false, out)
		dummy:co_status(400, "text/plain", "bad path")
		return out
	end

	local headers = parser.parse_headers(head)
	local keep_alive = want_keep_alive(app, ver, headers)
	local omit_body = ("HEAD" == method)
	local res = make_res(app, serve, keep_alive, omit_body, out)

	local limit = max_body_of(app, url_rel)
	local cl = tonumber(headers["content-length"] or "") or 0
	if (0 < limit) and (limit < #(body or "") or limit < cl) then
		res:co_status(413, "text/plain", "Content Too Large")
		return out
	end

	local req = {
		method = method,
		path = url_rel,
		query_string = qs or "",
		query = parser.parse_query(qs),
		headers = headers,
		params = {},
		json = {},
		json_error = "",
		form = {},
		files = {},
		cookies = parser.parse_cookies(headers),
		head = head or "",
		body = body or "",
		tls = ("https" == tag),
		auth_user = "",
		token = "",
		session = {},
		session_id = "",
		_app = app,
	}

	local perr = parse_body_fields(app, req, headers, body)
	if "" ~= perr then
		send_error(app, req, res, 400, "text/plain", perr)
		return out
	end

	if "" ~= req.json_error then
		send_error(app, req, res, 400, "text/plain", "invalid json")
		return out
	end

	local function after_mw()
		if res._sent then
			return
		end

		local fns, params = app._router:match(method, url_rel)
		if not fns and "HEAD" == method then
			fns, params = app._router:match("GET", url_rel)
		end

		if fns then
			req.params = params or {}
			co_run_fns(app, tag, url_rel, fns, req, res, 1)
			return
		end

		local allows = app._router:methods_of(url_rel)
		if "GET" ~= method and "HEAD" ~= method then
			if 0 < #allows then
				res:header("Allow", table.concat(allows, ", "))
				res:co_status(405, "text/plain", "Method Not Allowed")
				return
			end

			send_error(app, req, res, 404, "text/plain", "Not Found")
			return
		end

		co_serve_static(app, req, res, tag, method, url_rel)
	end

	if 0 == #app._mws then
		after_mw()
	else
		co_run_mws(app, req, res, 1, tag, after_mw)
	end

	if not res._sent then
		res:co_status(204, "text/plain", "")
	end

	return out
end


local function co_serve_conn(app, serve, tag)
	local msg, head, body = co_recv_until_text(serve)
	if "text" ~= msg then
		serve:disconnect()
		return
	end

	local nreq = 0
	local max_ka = app._cfg.max_keep_alive or DEFAULT_MAX_KA
	while true do
		nreq = nreq + 1
		if 0 < max_ka and max_ka < nreq then
			serve:disconnect()
			return
		end

		local out = co_dispatch(app, serve, tag, head, body)
		if not out.alive then
			return
		end

		if out.pending_head then
			head = out.pending_head
			body = out.pending_body or ""
		else
			msg, head, body = co_recv_until_text(serve)
			if "text" ~= msg then
				serve:disconnect()
				return
			end
		end
	end
end


local function co_accept_loop(app, listener, tag)
	while true do
		local serve = listener:co_accept()
		if not serve then
			break
		end

		kco.fork(function ()
			co_serve_conn(app, serve, tag)
		end)
	end
end


local function fork_accept(app, listener, tag)
	kco.fork(function ()
		co_accept_loop(app, listener, tag)
	end)
end


local function listen_cfg_only(cfg)
	if not cfg then
		return nil
	end

	return {
		tls = cfg.tls,
		cert = cfg.cert,
		key = cfg.key,
	}
end


local function install_builtin_mw(obj, cfg)
	if false ~= cfg.log then
		local write = cfg.log
		if "function" ~= type(write) then
			write = nil
		end

		obj._mws[#obj._mws + 1] = {
			prefix = "/",
			fn = mw.log({ write = write }),
		}
	end

	if false ~= cfg.cors and nil ~= cfg.cors then
		obj._mws[#obj._mws + 1] = {
			prefix = "/",
			fn = mw.cors(cfg.cors),
		}
	end

	if cfg.session then
		local sopt = cfg.session
		if true == sopt then
			sopt = {}
		end

		obj._sessions = {}
		sopt = sopt or {}
		obj._mws[#obj._mws + 1] = {
			prefix = "/",
			fn = mw.session({
				store = obj._sessions,
				name = sopt.name,
				ttl = sopt.ttl,
				secret = sopt.secret or cfg.session_secret,
			}),
		}
	end
end


--------------------------------------------------------------------------------------------
-- 站点对象

local W = {}

-- @brief 注册通用路由; 可 `route(method, path, mw, handler)`
-- @param [in]      method[string]		HTTP 方法; eg. `POST`
-- @param [in]      path[string]		URL 路径; eg. `/api/v1/:id`
-- @param [in]      ...[function]		路由级 mw..., 末项 handler
-- @return ok[boolean]					true 成功; 参数非法为 false
function W:route(method, path, ...)
	local fns = { ... }
	if 0 == #fns then
		return false
	end

	return self._router:add(method, path, fns)
end


-- @brief 注册 GET 路由
function W:get(path, ...)
	return self:route("GET", path, ...)
end


-- @brief 注册 POST 路由
function W:post(path, ...)
	return self:route("POST", path, ...)
end


-- @brief 注册 PUT 路由
function W:put(path, ...)
	return self:route("PUT", path, ...)
end


-- @brief 注册 DELETE 路由
function W:delete(path, ...)
	return self:route("DELETE", path, ...)
end


-- @brief 注册 PATCH 路由
function W:patch(path, ...)
	return self:route("PATCH", path, ...)
end


-- @brief 注册 HEAD 路由
-- @note 无 HEAD 路由时回退匹配 GET; 响应不带 body
function W:head(path, ...)
	return self:route("HEAD", path, ...)
end


-- @brief 注册 OPTIONS 路由
function W:options(path, ...)
	return self:route("OPTIONS", path, ...)
end


-- @brief 注册中间件
-- @param [in]      path_or_fn[string|function]		前缀或处理函数
-- @param [in]      fn[function]					[可选] 前缀形式时的处理函数
-- @return ok[boolean]								true 成功; 参数非法为 false
-- @note `fn(req, res, next)`; 不调用 next 且未发送则 204
function W:use(path_or_fn, fn)
	if "function" == type(path_or_fn) then
		self._mws[#self._mws + 1] = {
			prefix = "/",
			fn = path_or_fn,
		}
		return true
	end

	if "string" == type(path_or_fn) and "function" == type(fn) then
		if "" == path_or_fn then
			return false
		end

		self._mws[#self._mws + 1] = {
			prefix = normalize_mw_prefix(path_or_fn),
			fn = fn,
		}
		return true
	end

	return false
end


-- @brief 挂载静态目录
-- @param [in]      prefix[string]		URL 前缀; eg. `/`
-- @param [in]      dir[string]			本地目录
-- @param [in]      opts[table]			[可选] listing/index/max_file/gzip/gzip_dynamic
-- @return ok[boolean]					true 成功; 参数非法为 false
function W:static(prefix, dir, opts)
	return self._static:mount(prefix, dir, opts)
end


-- @brief 打开监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选] 监听配置; 见内联 opts 注释
-- @return ok[boolean]					true 成功; 失败为 false
-- @note 内部使用 klbhttp.listen; 可多次调用以同时听 HTTP/HTTPS
--   \n 只 bind, 不启动 accept 循环; 须再调用 :fork_accept
--   \n cert/key 可为 PEM 原文或文件路径, 由 klbhttp 解析
--   \n 只把 tls/cert/key 传给 klbhttp; 站点字段见 new(cfg)
function W:listen(port, cfg)
	-- cfg = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM 原文或文件路径
	--   key[string]			tls 时必填, 私钥 PEM 原文或文件路径
	-- }
	if "number" ~= type(port) or port < 1 or 65535 < port then
		return false
	end

	local listener = klbhttp.listen(port, listen_cfg_only(cfg))
	if not listener then
		return false
	end

	local tag = "http"
	if cfg and true == cfg.tls then
		tag = "https"
	end

	self._listeners[#self._listeners + 1] = {
		listener = listener,
		tag = tag,
		forked = false,
	}

	return true
end


-- @brief 启动已打开端口的 accept 循环
-- @return 无
-- @note 幂等; 已启动的 listener 跳过
--   \n 须在全部 :listen 之后调用
function W:fork_accept()
	for i = 1, #self._listeners do
		local slot = self._listeners[i]
		if slot and slot.listener and not slot.forked then
			fork_accept(self, slot.listener, slot.tag)
			slot.forked = true
		end
	end
end


-- @brief 关闭监听
-- @return 无
function W:close()
	for i = 1, #self._listeners do
		local slot = self._listeners[i]
		local l = slot and slot.listener
		if l and l.close then
			l:close()
		end
	end

	self._listeners = {}
end


--------------------------------------------------------------------------------------------
-- 工厂

local weber = {}

-- @brief 新建一个站点对象
-- @param [in]      cfg[table]			[可选] 站点配置; 见内联 opts 注释
-- @return [table]	站点对象
weber.new = function (cfg)
	-- cfg = {
	--   server[string]			[可选] Server 头, 默认 `klbweb`
	--   keep_alive[boolean]	[可选] 是否允许 keep-alive, 默认 `true`
	--   max_body[number]		[可选] 请求体上限字节, 默认 256KiB; 0 不限制
	--   max_body_paths[table]	[可选] 前缀 -> 更大上限, 供升级包等
	--   max_keep_alive[number]	[可选] 每连接最大请求数, 默认 100
	--   cors[boolean|table]	[可选] 默认关; `true` 或表见 mw.cors
	--   log[boolean|function]	[可选] 默认开; `false` 关闭; 函数为写日志
	--   session[boolean|table]	[可选] 默认关; 内存 Cookie 会话
	--   healthz[boolean]		[可选] 默认关; 为 true 时注册 GET /healthz
	--   spa[boolean|string]	[可选] 静态 404 且 Accept html 时回 index
	--   pages[table]			[可选] `[404]` / `[500]` HTML
	--   on_error[function]		[可选] `fn(req, res, code)`
	--   upload_dir[string]		[可选] multipart 落盘目录
	--   listing[boolean]		[可选] 静态目录列表默认, 默认关
	--   max_file[number]		[可选] 静态单文件上限, 默认 2MiB
	--   gzip[boolean]			[可选] 静态预压 .gz, 默认开
	--   gzip_dynamic[boolean]	[可选] 动态 gzip, 默认关
	-- }
	cfg = cfg or {}
	local site = {
		server = "klbweb",
		keep_alive = true,
		max_body = DEFAULT_MAX_BODY,
		max_body_paths = cfg.max_body_paths,
		max_keep_alive = DEFAULT_MAX_KA,
		cors = cfg.cors,
		log = cfg.log,
		session = cfg.session,
		session_secret = cfg.session_secret,
		healthz = cfg.healthz,
		spa = cfg.spa,
		pages = cfg.pages,
		on_error = cfg.on_error,
		upload_dir = cfg.upload_dir,
		max_file = cfg.max_file,
	}

	if "string" == type(cfg.server) and "" ~= cfg.server then
		site.server = cfg.server
	end

	if false == cfg.keep_alive then
		site.keep_alive = false
	end

	if "number" == type(cfg.max_body) then
		site.max_body = cfg.max_body
	end

	if "number" == type(cfg.max_keep_alive) then
		site.max_keep_alive = cfg.max_keep_alive
	end

	local obj = {
		_cfg = site,
		_router = router.new(),
		_static = staticer.new({
			listing = cfg.listing,
			index = cfg.index,
			max_file = cfg.max_file,
			gzip = cfg.gzip,
			gzip_dynamic = cfg.gzip_dynamic,
		}),
		_listeners = {},
		_mws = {},
		_sessions = {},
	}

	install_builtin_mw(obj, site)

	if true == site.healthz then
		obj._router:add("GET", "/healthz", { function (req, res)
			res:co_json({ ok = true })
		end })
	end

	return setmetatable(obj, { __index = W })
end


return weber
