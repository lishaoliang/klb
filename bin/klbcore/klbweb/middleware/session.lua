--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   session.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 内存 Session 中间件
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local krand = require("krand")


local function sign_sid(secret, sid)
	local s = (secret or "") .. "\0" .. (sid or "")
	local h = 5381
	for i = 1, #s do
		h = (h * 33 + string.byte(s, i)) % 2147483647
	end

	return string.format("%08x", h)
end


-- @brief 内存 Session; Cookie + TTL; 校验和防篡改 (非密码学 HMAC)
-- @param [in]      opts[table]			store / name / ttl / secret
-- @return fn[function]					中间件
local function session(opts)
	opts = opts or {}
	local store = opts.store or {}
	local name = opts.name or "klbweb.sid"
	local ttl = opts.ttl or 1800
	local secret = opts.secret or "klbweb"

	return function (req, res, nxt)
		local raw = (req.cookies and req.cookies[name]) or ""
		local sid, sig = string.match(raw, "^([^%.]+)%.(%w+)$")
		local sess
		if sid and sig == sign_sid(secret, sid) then
			sess = store[sid]
			if sess and sess.exp < os.time() then
				store[sid] = nil
				sess = nil
			end
		end

		if not sess then
			sid = krand.rand_string(24)
			sess = {
				id = sid,
				data = {},
				exp = os.time() + ttl,
			}
			store[sid] = sess
			res:set_cookie(name, sid .. "." .. sign_sid(secret, sid), {
				path = "/",
				http_only = true,
				max_age = ttl,
				same_site = "Lax",
			})
		else
			sess.exp = os.time() + ttl
		end

		req.session = sess.data
		req.session_id = sess.id
		nxt()
	end
end


return session
