--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   csrf.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb CSRF helper 中间件
-- @note   安全方法发 token; 写方法校验头或 form. 宜挂在 session 之后
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local krand = require("krand")
local util = require("klbcore.klbweb.util.header")


local function is_safe(method)
	return "GET" == method or "HEAD" == method or "OPTIONS" == method
end


local function pick_token(req, field)
	local h = util.header_get(req.headers, "x-csrf-token")
	if "" ~= h then
		return h
	end

	h = util.header_get(req.headers, "x-xsrf-token")
	if "" ~= h then
		return h
	end

	local form = req.form or {}
	local v = form[field]
	if "string" == type(v) then
		return v
	end

	return ""
end


local function ensure_token(req, res, cookie_name)
	local sess = req.session
	local token = ""
	if "table" == type(sess) and "" ~= (req.session_id or "") then
		token = sess._csrf or ""
		if "" == token then
			token = krand.rand_string(24)
			sess._csrf = token
		end
	else
		token = (req.cookies and req.cookies[cookie_name]) or ""
		if "" == token then
			token = krand.rand_string(24)
		end
	end

	req.csrf_token = token
	res:set_cookie(cookie_name, token, {
		path = "/",
		http_only = false,
		max_age = 1800,
		same_site = "Lax",
	})

	return token
end


-- @brief CSRF; GET/HEAD/OPTIONS 发 token, 其它方法须匹配
-- @param [in]      opts[table]			[可选] `cookie` / `field`
-- @return fn[function]					中间件
-- @note token 在 `req.csrf_token`; 可写入 session._csrf 或可读 Cookie
local function csrf(opts)
	opts = opts or {}
	local cookie_name = opts.cookie or "klbweb.csrf"
	local field = opts.field or "_csrf"

	return function (req, res, nxt)
		local token = ensure_token(req, res, cookie_name)
		if is_safe(req.method) then
			nxt()
			return
		end

		local got = pick_token(req, field)
		if got ~= token or "" == got then
			res:co_status(403, "text/plain", "CSRF")
			return
		end

		nxt()
	end
end


return csrf
