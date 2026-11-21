--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   handler.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  MCP Streamable HTTP 实现 (POST|GET|DELETE /mcp)
-- @stability internal
-- @impl evolving
-- @note   包外勿直 require; 对外面见 http/init.lua
--   \n POST JSON 或 SSE; GET SSE; DELETE 结束会话
-- @history 修改历史
--  \n [2026-10] 创建文件
--  \n [2026-11] 迁入 http/; 自 init 拆为 handler
--]]


local cjson = require("cjson")
local kco = require("kco")
local mcp_server = require("klbagent.packages.mcp.server")


local M = {}


local function _hdr(headers, name)
	if type(headers) ~= "table" or type(name) ~= "string" then
		return nil
	end

	local v = headers[name]
	if v == nil then
		v = headers[string.lower(name)]
	end

	if type(v) == "string" then
		return v
	end

	return nil
end


-- @brief Origin 防 DNS rebinding: 仅放行本机或空
local function _origin_ok(origin)
	if type(origin) ~= "string" or origin == "" then
		return true
	end

	local host = string.match(origin, "^https?://([^/]+)")
	if host == nil then
		return false
	end

	host = string.lower(host)
	if host == "localhost" or host == "127.0.0.1" or host == "[::1]" then
		return true
	end

	if string.match(host, "^localhost:") or string.match(host, "^127%.0%.0%.1:")
		or string.match(host, "^%[::1%]:") then
		return true
	end

	return false
end


local function _set_session_header(res, sid)
	if type(res.header) == "function" and type(sid) == "string" and sid ~= "" then
		res:header("Mcp-Session-Id", sid)
	end
end


local function _set_protocol_header(res, ver)
	if type(res.header) == "function" and type(ver) == "string" and ver ~= "" then
		res:header("MCP-Protocol-Version", ver)
	end
end


local function _accept_has(accept, needle)
	if type(accept) ~= "string" or accept == "" then
		return false
	end

	return string.find(accept, needle, 1, true) ~= nil
end


-- @brief 是否用 SSE 回 POST (多帧或 Accept 偏好 event-stream)
local function _prefer_sse(accept, frames)
	local has_sse = _accept_has(accept, "text/event-stream")
	if not has_sse then
		return false
	end

	local has_json = _accept_has(accept, "application/json") or accept == ""
	local multi = type(frames) == "table" and #frames > 1
	if multi then
		return true
	end

	-- 仅 SSE、无 json → SSE
	if has_sse and not has_json then
		return true
	end

	return false
end


local function _co_sse_write_frame(res, frame)
	local ok_e, text = pcall(cjson.encode, frame)
	if not ok_e or type(text) ~= "string" then
		return
	end

	if type(res.co_sse) == "function" then
		res:co_sse(text, "message")
	end
end


-- @brief GET: 打开 SSE 听会话出站通知 (须在 kco 协程内)
local function _co_handle_get(req, res)
	local sid = _hdr(req.headers, "Mcp-Session-Id")
	if type(sid) ~= "string" or sid == "" then
		res:co_status(400, "text/plain; charset=utf-8", "need Mcp-Session-Id")
		return true
	end

	local ok_act, err_act = mcp_server.activate_http_session(sid)
	if ok_act == nil then
		res:co_status(400, "text/plain; charset=utf-8", err_act or "unknown session")
		return true
	end

	local accept = _hdr(req.headers, "Accept") or ""
	if accept ~= "" and not _accept_has(accept, "text/event-stream")
		and not _accept_has(accept, "*/*") then
		res:co_status(406, "text/plain; charset=utf-8", "need Accept text/event-stream")
		return true
	end

	if type(res.co_sse_open) ~= "function" then
		res:co_status(405, "text/plain; charset=utf-8", "SSE unsupported")
		return true
	end

	_set_session_header(res, sid)
	local pv = _hdr(req.headers, "MCP-Protocol-Version")
	_set_protocol_header(res, pv or mcp_server.PROTOCOL_VERSION)
	res:co_sse_open()

	-- 客户端可通过 MCP-SSE-Timeout 对齐 listen 超时 (秒; 默认 15, 上限 300)
	local timeout_sec = tonumber(_hdr(req.headers, "MCP-SSE-Timeout")) or 15
	if timeout_sec < 1 then
		timeout_sec = 1
	elseif timeout_sec > 300 then
		timeout_sec = 300
	end

	local idle_ms = 1000
	local max_ms = timeout_sec * 1000
	local waited = 0
	while mcp_server.has_http_session(sid) and waited < max_ms do
		local events = mcp_server.take_sse_events(sid, 32)
		if type(events) == "table" and #events > 0 then
			for i = 1, #events do
				_co_sse_write_frame(res, events[i])
			end

			waited = 0
		else
			if type(res.co_sse_comment) == "function" then
				res:co_sse_comment("keepalive")
			end

			kco.co_sleep(idle_ms)
			waited = waited + idle_ms
		end
	end

	if type(res.co_sse_end) == "function" then
		res:co_sse_end()
	end

	return true
end


-- @brief 处理 Streamable HTTP /mcp 请求 (auth 由调用方先校验; 须在 kco 协程内)
-- @param [in] req[table]
-- @param [in] res[table]
-- @return handled[boolean]
M.co_handle = function (req, res)
	local method = string.upper(tostring(req.method or "GET"))
	local origin = _hdr(req.headers, "Origin")
	if not _origin_ok(origin) then
		res:co_status(403, "text/plain; charset=utf-8", "bad origin")
		return true
	end

	if method == "GET" then
		return _co_handle_get(req, res)
	end

	if method == "DELETE" then
		local sid = _hdr(req.headers, "Mcp-Session-Id")
		if type(sid) == "string" and sid ~= "" then
			mcp_server.end_http_session(sid)
		end

		res:co_status(200, "text/plain; charset=utf-8", "")
		return true
	end

	if method ~= "POST" then
		res:co_status(405, "text/plain; charset=utf-8", "method not allowed")
		return true
	end

	local accept = _hdr(req.headers, "Accept") or ""
	if accept ~= "" and not _accept_has(accept, "application/json")
		and not _accept_has(accept, "text/event-stream")
		and not _accept_has(accept, "*/*") then
		res:co_status(406, "text/plain; charset=utf-8", "need Accept application/json")
		return true
	end

	local raw = req.body or ""
	if type(req.json) == "table" then
		local ok_e, text = pcall(cjson.encode, req.json)
		if ok_e and type(text) == "string" then
			raw = text
		end
	end

	if type(raw) ~= "string" or raw == "" then
		res:co_status(400, "application/json; charset=utf-8",
			'{"jsonrpc":"2.0","error":{"code":-32700,"message":"parse error"},"id":null}')
		return true
	end

	local ok_j, msg = pcall(cjson.decode, raw)
	if not ok_j or type(msg) ~= "table" then
		res:co_status(400, "application/json; charset=utf-8",
			'{"jsonrpc":"2.0","error":{"code":-32700,"message":"parse error"},"id":null}')
		return true
	end

	local is_notify = (msg.id == nil) and type(msg.method) == "string"
	local want_method = msg.method
	local had_sid = _hdr(req.headers, "Mcp-Session-Id")

	local out_sid, frames, err = mcp_server.co_with_http_lock(function ()
		local sid = had_sid
		local err_act = nil
		if type(want_method) == "string" and want_method == "initialize" then
			if type(sid) == "string" and sid ~= "" then
				sid, err_act = mcp_server.activate_http_session(sid)
				if sid == nil then
					sid, err_act = mcp_server.activate_http_session(nil)
				end
			else
				sid, err_act = mcp_server.activate_http_session(nil)
			end
		else
			if type(sid) ~= "string" or sid == "" then
				return nil, nil, "need Mcp-Session-Id"
			end

			sid, err_act = mcp_server.activate_http_session(sid)
		end

		if sid == nil then
			return nil, nil, err_act or "session fail"
		end

		local captured = mcp_server.handle_line_captured(raw)
		return sid, captured, nil
	end)

	if out_sid == nil and frames == nil then
		local e = err or "mcp http fail"
		if e == "need Mcp-Session-Id" or e == "unknown session" then
			res:co_status(400, "text/plain; charset=utf-8", e)
			return true
		end

		res:co_status(500, "text/plain; charset=utf-8", tostring(e))
		return true
	end

	if is_notify then
		_set_session_header(res, out_sid)
		res:co_status(202, "text/plain; charset=utf-8", "")
		return true
	end

	-- 取带 id 的应答帧
	local reply = nil
	if type(frames) == "table" then
		for i = 1, #frames do
			local f = frames[i]
			if type(f) == "table" and f.id ~= nil then
				reply = f
			end
		end

		if reply == nil and #frames > 0 then
			reply = frames[#frames]
		end
	end

	if reply == nil then
		reply = {
			jsonrpc = "2.0",
			id = msg.id,
			error = {
				code = -32603,
				message = "empty response",
			},
		}
		frames = { reply }
	end

	local ver = nil
	if type(reply.result) == "table" then
		ver = reply.result.protocolVersion
	end

	if type(ver) ~= "string" or ver == "" then
		ver = _hdr(req.headers, "MCP-Protocol-Version")
	end

	_set_session_header(res, out_sid)
	_set_protocol_header(res, ver)

	if _prefer_sse(accept, frames) and type(res.co_sse_open) == "function" then
		res:co_sse_open()
		for i = 1, #frames do
			_co_sse_write_frame(res, frames[i])
		end

		if type(res.co_sse_end) == "function" then
			res:co_sse_end()
		end

		return true
	end

	local ok_e, body = pcall(cjson.encode, reply)
	if not ok_e or type(body) ~= "string" then
		body = '{"jsonrpc":"2.0","error":{"code":-32603,"message":"encode fail"},"id":null}'
	end

	res:co_status(200, "application/json; charset=utf-8", body)
	return true
end


-- @brief 挂到 klbweb app: POST/GET/DELETE /mcp
-- @param [in] app[table]
-- @param [in] auth_fn[function]		function(req,res)->ok; false 时已写响应
M.mount = function (app, auth_fn)
	if type(app) ~= "table" then
		return false, "need app"
	end

	local function _co_gate(req, res)
		if type(auth_fn) == "function" then
			local ok = auth_fn(req, res)
			if not ok then
				return
			end
		end

		M.co_handle(req, res)
	end

	if type(app.post) == "function" then
		app:post("/mcp", _co_gate)
	end

	if type(app.get) == "function" then
		app:get("/mcp", _co_gate)
	end

	if type(app.delete) == "function" then
		app:delete("/mcp", _co_gate)
	elseif type(app.del) == "function" then
		app:del("/mcp", _co_gate)
	end

	return true
end


return M

-- end
