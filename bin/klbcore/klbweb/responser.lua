--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   responser.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 响应对象
-- @note   header / cookie / redirect / send; 组包走 pack_head
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local cjson = require("cjson.safe")
local coder = require("klbcore.klbweb.coder")


local responser = {}


local function header_has(list, name)
	name = string.lower(name)
	for i = 1, #list do
		if string.lower(list[i][1]) == name then
			return true
		end
	end

	return false
end


-- @brief 拼响应头
-- @param [in]      app[table]			站点对象
-- @param [in]      status[string]		状态行
-- @param [in]      mime[string]		Content-Type
-- @param [in]      size[number]		Content-Length (按原 body)
-- @param [in]      keep_alive[boolean]	是否 keep-alive
-- @param [in]      extra[table]		额外头 `{ { name, value }, ... }`
-- @return [string]						头块 (含结尾空行)
responser.pack_head = function (app, status, mime, size, keep_alive, extra)
	local server = "klbweb"
	if app and app._cfg and "string" == type(app._cfg.server) and "" ~= app._cfg.server then
		server = app._cfg.server
	end

	extra = extra or {}
	local t = {}
	t[#t + 1] = status .. "\r\n"
	t[#t + 1] = string.format("Server: %s\r\n", server)
	if keep_alive then
		t[#t + 1] = "Connection: keep-alive\r\n"
	else
		t[#t + 1] = "Connection: close\r\n"
	end

	if mime and "" ~= mime then
		t[#t + 1] = string.format("Content-Type: %s\r\n", mime)
	end

	t[#t + 1] = string.format("Content-Length: %d\r\n", size or 0)

	if not header_has(extra, "Cache-Control") then
		t[#t + 1] = "Cache-Control: max-age=0\r\n"
	end

	for i = 1, #extra do
		local name = extra[i][1]
		local value = extra[i][2]
		if "string" == type(name) and "" ~= name then
			t[#t + 1] = string.format("%s: %s\r\n", name, tostring(value or ""))
		end
	end

	t[#t + 1] = "\r\n"
	return table.concat(t)
end


-- @brief 构造 res; finish 由 weber 注入
-- @param [in]      app[table]			站点对象
-- @param [in]      finish[function]	`finish(status_line, mime, body)`
-- @return res[table]					响应对象
responser.make = function (app, finish)
	local res = {
		_app = app,
		_sent = false,
		_headers = {},
		_status_code = 0,
	}

	function res:header(name, value)
		if "string" ~= type(name) or "" == name then
			return self
		end

		self._headers[#self._headers + 1] = { name, tostring(value or "") }
		return self
	end

	function res:set_cookie(name, value, opts)
		if "string" ~= type(name) or "" == name then
			return self
		end

		opts = opts or {}
		local parts = { string.format("%s=%s", name, tostring(value or "")) }
		local path = opts.path or "/"
		parts[#parts + 1] = "Path=" .. path

		if "number" == type(opts.max_age) then
			parts[#parts + 1] = "Max-Age=" .. tostring(opts.max_age)
		end

		if false ~= opts.http_only then
			parts[#parts + 1] = "HttpOnly"
		end

		if opts.secure then
			parts[#parts + 1] = "Secure"
		end

		if "string" == type(opts.same_site) and "" ~= opts.same_site then
			parts[#parts + 1] = "SameSite=" .. opts.same_site
		end

		self._headers[#self._headers + 1] = { "Set-Cookie", table.concat(parts, "; ") }
		return self
	end

	function res:co_send(body, mime)
		self._status_code = 200
		finish(coder.status(200), mime or "text/html", body or "")
	end

	function res:co_status(status, mime, body)
		if "number" == type(status) then
			self._status_code = status
			status = coder.status(status)
		end

		finish(status, mime or "text/plain", body or "")
	end

	function res:co_json(obj, code)
		local body = "null"
		if nil ~= obj then
			local s = cjson.encode(obj)
			if not s then
				self._status_code = 500
				finish(coder.status(500), "text/plain", "json encode error")
				return
			end

			body = s
		end

		self._status_code = code or 200
		finish(coder.status(code or 200), "application/json; charset=utf-8", body)
	end

	function res:co_redirect(url, code)
		code = code or 302
		self._status_code = code
		self:header("Location", url or "/")
		finish(coder.status(code), "text/plain", "")
	end

	return res
end


return responser
