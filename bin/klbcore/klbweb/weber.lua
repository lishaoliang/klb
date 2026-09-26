--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   weber.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 站点对象
-- @note   路由 / 静态 / listen; 内部使用 klbhttp; :use 与路由/静态按注册序
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kco = require("kco")
local router = require("klbcore.klbweb.router")
local compile = require("klbcore.klbweb.router.compile")
local staticer = require("klbcore.klbweb.static")
local parser = require("klbcore.klbweb.parser")
local responser = require("klbcore.klbweb.responser")
local mw = require("klbcore.klbweb.middleware")
local html = {
	[404] = require("klbcore.klbweb.html.html404"),
}
local pathex = require("klbcore.util.pathex")
local klbhttp = require("klbcore.klbhttp")
local cfger = require("klbcore.klbweb.cfger")


--------------------------------------------------------------------------------------------
-- 前置定义

local KLB_SOCKET_CONNECT = 63
local KLB_NETCODE_WBUF_EMPTY = 70


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

	return app._cfg.max_body or cfger.defaults.max_body
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


local function co_send_part(serve, head, body)
	local rc = serve:send(head or "", body or "")
	if 0 ~= rc then
		serve:disconnect()
		return "error", nil, nil
	end

	return co_wait_wbuf_empty(serve)
end


local function co_reply(app, serve, status_line, mime, body, keep_alive, omit_body, extra, file)
	body = body or ""
	mime = mime or "text/plain"
	file = file or { path = "", offset = 0, length = 0 }
	local payload = body
	if omit_body then
		payload = ""
	end

	local size = #body
	if "string" == type(file.path) and "" ~= file.path then
		size = file.length or 0
	end

	local head = responser.pack_head(app, status_line, mime, size, keep_alive, extra)
	local rc
	if omit_body or "" == (file.path or "") then
		rc = serve:send(head, payload)
	elseif 0 < (file.length or 0) then
		rc = serve:send_file(head, file.path, { offset = file.offset or 0, length = file.length })
	else
		rc = serve:send(head, "")
	end
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
	local function finish(status_line, mime, body, file)
		if res._sent then
			return
		end

		local alive, ph, pb = co_reply(app, serve, status_line, mime, body, res._keep_alive, res._omit_body, res._headers, file)
		res._sent = true
		out.alive = alive
		out.pending_head = ph
		out.pending_body = pb
	end

	local function start_stream(status_line, mime, mode)
		if res._sent then
			return false
		end

		local ka = res._keep_alive
		if "stream" == mode then
			ka = true
		end

		local head = responser.pack_head(app, status_line, mime, 0, ka, res._headers, mode)
		local msg = co_send_part(serve, head, "")
		if "ok" ~= msg then
			serve:disconnect()
			out.alive = false
			res._sent = true
			return false
		end

		res._sent = true
		res._streaming = true
		res._stream_mode = mode
		if "stream" == mode then
			res._keep_alive = false
		end

		return true
	end

	local function write_stream(data)
		if not res._streaming or res._stream_closed then
			return false
		end

		local msg = co_send_part(serve, "", data or "")
		if "ok" ~= msg then
			serve:disconnect()
			out.alive = false
			res._stream_closed = true
			return false
		end

		return true
	end

	local function stop_stream()
		if res._stream_closed then
			return
		end

		res._stream_closed = true
		if "stream" == res._stream_mode or not res._keep_alive then
			serve:disconnect()
			out.alive = false
			return
		end

		local msg, h, b = co_wait_wbuf_empty(serve)
		if "text" == msg then
			out.alive = true
			out.pending_head = h
			out.pending_body = b
			return
		end

		if "ok" ~= msg then
			serve:disconnect()
			out.alive = false
			return
		end

		out.alive = true
	end

	res = responser.make(app, finish, {
		start = start_stream,
		write = write_stream,
		stop = stop_stream,
	})
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

	local page = html[code]
	if "string" == type(page) and "" ~= page and (not mime or "text/html" == mime) then
		res:co_status(code, "text/html", body or page)
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


local function match_route_layer(layer, method, path)
	if layer.method ~= method then
		if not ("HEAD" == method and "GET" == layer.method) then
			return nil, {}
		end
	end

	if layer.pat then
		local caps = { string.match(path, layer.pat) }
		if 0 == #caps then
			return nil, {}
		end

		local params = {}
		for j = 1, #layer.names do
			params[layer.names[j]] = caps[j] or ""
		end

		return layer.fns, params
	end

	if layer.path == path then
		return layer.fns, {}
	end

	return nil, {}
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


local function apply_static_hit(app, req, res, status, mime, sbody, extra, sfile)
	if 400 == status then
		res:co_status(400, mime, sbody)
		return true
	end

	if 413 == status then
		send_error(app, req, res, 413, mime, sbody)
		return true
	end

	if 416 == status then
		apply_extra(res, extra)
		res:co_status(416, mime, sbody)
		return true
	end

	if 0 == status or 404 == status then
		return false
	end

	apply_extra(res, extra)
	if 304 == status then
		res:co_status(304, mime, "")
		return true
	end

	res:co_status(status, mime, sbody, sfile)
	return true
end


local function after_stack(app, req, res, method, url_rel)
	if res._sent then
		return
	end

	if "GET" == method or "HEAD" == method then
		local spa = app._cfg.spa
		if spa and spa_accepts_html(req.headers) then
			local spa_path = true == spa and "/index.html" or spa
			local st, sm, sb, _, se, sf = app._static:serve(spa_path, req.headers)
			if 200 == st or 304 == st then
				apply_extra(res, se)
				res:co_status(st, sm, sb, sf)
				return
			end
		end

		send_error(app, req, res, 404, "text/html")
		return
	end

	local allows = app._router:methods_of(url_rel)
	if 0 < #allows then
		res:header("Allow", table.concat(allows, ", "))
		res:co_status(405, "text/plain", "Method Not Allowed")
		return
	end

	send_error(app, req, res, 404, "text/plain", "Not Found")
end


local function co_run_stack(app, req, res, idx, tag, method, url_rel)
	if res._sent then
		return
	end

	local layer = app._stack[idx]
	if not layer then
		after_stack(app, req, res, method, url_rel)
		return
	end

	if "mw" == layer.kind then
		if not path_prefixed(layer.prefix, req.path) then
			co_run_stack(app, req, res, idx + 1, tag, method, url_rel)
			return
		end

		local proceeded = false
		local function nxt()
			if proceeded or res._sent then
				return
			end

			proceeded = true
			co_run_stack(app, req, res, idx + 1, tag, method, url_rel)
		end

		local ok, err = pcall(layer.fn, req, res, nxt)
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

		return
	end

	if "route" == layer.kind then
		local fns, params = match_route_layer(layer, method, url_rel)
		if not fns then
			co_run_stack(app, req, res, idx + 1, tag, method, url_rel)
			return
		end

		req.params = params or {}
		co_run_fns(app, tag, url_rel, fns, req, res, 1)
		return
	end

	if "static" == layer.kind then
		if "GET" == method or "HEAD" == method then
			local status, mime, sbody, _, extra, sfile = app._static:serve(url_rel, req.headers)
			if apply_static_hit(app, req, res, status, mime, sbody, extra, sfile) then
				return
			end
		end

		co_run_stack(app, req, res, idx + 1, tag, method, url_rel)
		return
	end

	co_run_stack(app, req, res, idx + 1, tag, method, url_rel)
end


local function parse_body_fields(app, req, headers, body)
	req.form = parser.parse_form(headers, body)
	req.files = {}
	local json, jerr = parser.parse_json(headers, body)
	req.json = json
	req.json_error = jerr

	local ctype = string.lower(headers["content-type"] or "")
	if string.find(ctype, "multipart/form-data", 1, true) then
		local form, files, err = parser.parse_multipart(headers, body, app._cfg.upload_dir or "", app._cfg.max_file or cfger.defaults.max_file)
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
		request_id = "",
		csrf_token = "",
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

	co_run_stack(app, req, res, 1, tag, method, url_rel)

	if res._streaming and not res._stream_closed then
		if "chunked" == res._stream_mode then
			res:co_chunk_end()
		else
			res:co_sse_end()
		end
	end

	if not res._sent then
		res:co_status(204, "text/plain", "")
	end

	return out
end


local function conn_tag(serve)
	if serve and serve.tls and serve:tls() then
		return "https"
	end

	return "http"
end


local function co_serve_conn(app, serve, tag)
	local msg, head, body = co_recv_until_text(serve)
	if "text" ~= msg then
		serve:disconnect()
		return
	end

	local nreq = 0
	local max_ka = app._cfg.max_keep_alive or cfger.defaults.max_keep_alive
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


local function co_accept_loop(app, listener)
	while true do
		local serve = listener:co_accept()
		if not serve then
			break
		end

		kco.fork(function ()
			co_serve_conn(app, serve, conn_tag(serve))
		end)
	end
end


local function fork_accept(app, listener)
	kco.fork(function ()
		co_accept_loop(app, listener)
	end)
end


local function collect_builtin_mw(obj, cfg)
	local list = {}

	if false ~= cfg.request_id then
		local ropt = cfg.request_id
		if true == ropt or "table" ~= type(ropt) then
			ropt = {}
		end

		list[#list + 1] = {
			prefix = "/",
			fn = mw.request_id(ropt),
		}
	end

	if false ~= cfg.log then
		local write = cfg.log
		if "function" ~= type(write) then
			write = nil
		end

		list[#list + 1] = {
			prefix = "/",
			fn = mw.log({ write = write }),
		}
	end

	if false ~= cfg.cors and nil ~= cfg.cors then
		list[#list + 1] = {
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
		list[#list + 1] = {
			prefix = "/",
			fn = mw.session({
				store = obj._sessions,
				name = sopt.name,
				ttl = sopt.ttl,
				secret = sopt.secret or cfg.session_secret,
			}),
		}
	end

	if cfg.csrf then
		local copt = cfg.csrf
		if true == copt or "table" ~= type(copt) then
			copt = {}
		end

		list[#list + 1] = {
			prefix = "/",
			fn = mw.csrf(copt),
		}
	end

	return list
end


local function apply_static_patch(static_obj, cfg)
	local d = static_obj._defaults

	if nil ~= cfg.listing then
		d.listing = true == cfg.listing
	end

	if nil ~= cfg.index then
		d.index = cfg.index
	end

	if "number" == type(cfg.max_file) then
		d.max_file = cfg.max_file
	end

	if nil ~= cfg.gzip then
		d.gzip = false ~= cfg.gzip
	end

	if nil ~= cfg.gzip_dynamic then
		d.gzip_dynamic = true == cfg.gzip_dynamic
	end
end


local function apply_site(obj)
	if obj._applied then
		return
	end

	obj._applied = true

	local cfg = obj._cfg
	local built = collect_builtin_mw(obj, cfg)
	obj._builtin_mw_n = #built
	if 0 < #built then
		local stack = {}
		for i = 1, #built do
			stack[i] = {
				kind = "mw",
				prefix = built[i].prefix,
				fn = built[i].fn,
				builtin = true,
			}
		end

		for i = 1, #obj._stack do
			stack[#stack + 1] = obj._stack[i]
		end

		obj._stack = stack
	end

	if true == cfg.healthz then
		local fns = obj._router:match("GET", "/healthz")
		if not fns then
			local hfn = function (req, res)
				res:co_json({ ok = true })
			end

			obj._router:add("GET", "/healthz", { hfn })
			local pos = (obj._builtin_mw_n or 0) + 1
			table.insert(obj._stack, pos, {
				kind = "route",
				method = "GET",
				path = "/healthz",
				fns = { hfn },
				healthz_own = true,
			})
			obj._healthz_own = true
		end
	end
end


local function unapply_site(obj)
	local stack = {}
	for i = 1, #obj._stack do
		local layer = obj._stack[i]
		if not layer.builtin and not layer.healthz_own then
			stack[#stack + 1] = layer
		end
	end

	obj._stack = stack
	obj._builtin_mw_n = 0
	if obj._healthz_own then
		obj._router:remove("GET", "/healthz")
		obj._healthz_own = false
	end

	obj._applied = false
end


local function listen_has_port(obj, port)
	for i = 1, #obj._listeners do
		if obj._listeners[i].port == port then
			return true
		end
	end

	return false
end


local function bind_listen_specs(obj)
	local specs = obj._listen_specs
	if nil == specs then
		if 0 == #obj._listeners then
			specs = {
				{ port = cfger.defaults.listen_port },
			}
		else
			return true
		end
	end

	local ok = true
	for i = 1, #specs do
		local spec = specs[i]
		if spec and not listen_has_port(obj, spec.port) then
			local lcfg = nil
			if spec.tls or spec.plain or spec.cert or spec.key then
				lcfg = {
					tls = spec.tls,
					plain = spec.plain,
					cert = spec.cert,
					key = spec.key,
				}
			end

			if not obj:listen(spec.port, lcfg) then
				ok = false
			end
		end
	end

	return ok
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

	if not self._router:add(method, path, fns) then
		return false
	end

	method = string.upper(method)
	path = compile.normalize_path(path)
	local layer = {
		kind = "route",
		method = method,
		path = path,
		fns = fns,
	}
	if compile.is_pattern(path) then
		local pat, names = compile.compile_path(path)
		if not pat then
			return false
		end

		layer.pat = pat
		layer.names = names
	end

	for i = 1, #self._stack do
		local e = self._stack[i]
		if "route" == e.kind and e.method == method and e.path == path then
			self._stack[i] = layer
			return true
		end
	end

	self._stack[#self._stack + 1] = layer
	return true
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
-- @note `fn(req, res, next)`; 不调用 next 且未发送则 204; 与 :get/:static 按注册序
function W:use(path_or_fn, fn)
	if "function" == type(path_or_fn) then
		self._stack[#self._stack + 1] = {
			kind = "mw",
			prefix = "/",
			fn = path_or_fn,
		}
		return true
	end

	if "string" == type(path_or_fn) and "function" == type(fn) then
		if "" == path_or_fn then
			return false
		end

		self._stack[#self._stack + 1] = {
			kind = "mw",
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
	if not self._static:mount(prefix, dir, opts) then
		return false
	end

	for i = 1, #self._stack do
		if "static" == self._stack[i].kind then
			return true
		end
	end

	self._stack[#self._stack + 1] = {
		kind = "static",
	}
	return true
end


-- @brief 汇总站点配置; 可多次; 后写字段覆盖
-- @param [in]      cfg[table]			[可选] 站点配置; 字段见 cfger.new
-- @return ok[boolean]					true 成功; 服务中未 stop 或参数非法为 false
-- @note 省略的键不变. cors/log/session/request_id/csrf/healthz 在 serve 时按汇总结果安装; stop 后再 serve 会重装.
--   \n `listen` 只记规格, 不 bind; serve 时再开端口. 未设且未 :listen 时默认 8000 HTTP.
function W:setup(cfg)
	-- cfg.listen = 8000
	-- cfg.listen = { port = 8000, tls, plain, cert, key }
	-- cfg.listen = { port = 8000, tls = true, plain = true, cert, key }	同端口 HTTP+HTTPS
	-- cfg.listen = { { port = 8000 }, { port = 8000, tls = true, cert, key } }	同端口合并为一个监听
	if self._applied then
		return false
	end

	if 0 < #self._listeners then
		return false
	end

	if nil ~= cfg and "table" ~= type(cfg) then
		return false
	end

	if cfg then
		if nil ~= cfg.listen then
			local specs, lok = cfger.parse_listen(cfg.listen)
			if not lok then
				return false
			end

			self._listen_specs = specs
		end

		cfger.apply(self._cfg, cfg)
		apply_static_patch(self._static, cfg)
	end

	return true
end


-- @brief 修改已生效的站点配置
-- @param [in]      patch[table]		要覆盖的字段; 省略的键不变
-- @return ok[boolean]					true 成功; 参数非法为 false
-- @note 立即影响后续请求读 `_cfg` 的项. cors/log/session/request_id/csrf/healthz/静态 listing 不重装.
function W:apply_cfg(patch)
	if "table" ~= type(patch) then
		return false
	end

	cfger.apply(self._cfg, patch)
	return true
end


-- @brief 打开监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选] 监听配置; 见内联 opts 注释
-- @return ok[boolean]					true 成功; 失败为 false
-- @note 内部使用 klbhttp.listen; 同端口 HTTP+HTTPS 一次调用 (`tls` + `plain`)
--   \n 不同端口可多次; 同一 port 已 bind 则失败
--   \n 只 bind, 不启动 accept 循环; 须再调用 :serve
--   \n cert/key 可为 PEM 原文或文件路径, 由 klbhttp 解析
--   \n 只把 tls/plain/cert/key 传给 klbhttp; 站点字段见 new/setup
function W:listen(port, cfg)
	-- cfg = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   plain[boolean]			[可选] tls 时是否同时收明文 HTTP, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM 原文或文件路径
	--   key[string]			tls 时必填, 私钥 PEM 原文或文件路径
	-- }
	if "number" ~= type(port) or port < 1 or 65535 < port then
		return false
	end

	if listen_has_port(self, port) then
		return false
	end

	local listener = klbhttp.listen(port, cfger.listen_only(cfg))
	if not listener then
		return false
	end

	apply_site(self)

	self._listeners[#self._listeners + 1] = {
		listener = listener,
		port = port,
		forked = false,
	}

	return true
end


-- @brief 按汇总 listen 规格 bind (若尚未打开), 再启动 accept 循环
-- @return ok[boolean]					true 全部 bind 成功; 任一项失败为 false
-- @note 幂等; 已启动的 listener 跳过
--   \n 未 setup listen 且未 :listen 时默认 8000 HTTP
--   \n bind 失败不回滚已成功端口, 仍对已 bind 项起 accept
function W:serve()
	apply_site(self)

	local ok = bind_listen_specs(self)
	for i = 1, #self._listeners do
		local slot = self._listeners[i]
		if slot and slot.listener and not slot.forked then
			fork_accept(self, slot.listener)
			slot.forked = true
		end
	end

	return ok
end


-- @brief 停止对外服务; 关全部监听, 许可再 setup / serve
-- @return ok[boolean]					true
-- @note 保留路由 / 静态 / 汇总 cfg 与 listen 规格. cors/log/session/request_id/csrf/healthz 在下次 serve 时按当时汇总重装.
function W:stop()
	for i = 1, #self._listeners do
		local slot = self._listeners[i]
		local l = slot and slot.listener
		if l and l.close then
			l:close()
		end
	end

	self._listeners = {}
	unapply_site(self)
	return true
end


--------------------------------------------------------------------------------------------
-- 工厂

local weber = {}

-- @brief 新建一个站点对象
-- @param [in]      cfg[table]			[可选] 初始站点配置; 字段见 cfger.new
-- @return [table]	站点对象
-- @note 可用 :setup 继续汇总; cors/log/session/request_id/csrf/healthz 在 serve 时安装, stop 后再 serve 会重装
--   \n cfg.listen 只记规格; 未设则 serve 默认 8000 HTTP
weber.new = function (cfg)
	cfg = cfg or {}
	local site = cfger.new(cfg)
	local specs, lok = cfger.parse_listen(cfg.listen)
	if not lok then
		specs = nil
	end

	local obj = {
		_cfg = site,
		_router = router.new(),
		_static = staticer.new(cfger.static_defaults(cfg)),
		_listeners = {},
		_listen_specs = specs,
		_stack = {},
		_sessions = {},
		_applied = false,
		_builtin_mw_n = 0,
		_healthz_own = false,
	}

	return setmetatable(obj, { __index = W })
end


return weber
