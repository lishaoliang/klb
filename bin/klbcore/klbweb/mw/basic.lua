--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   basic.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb Basic 鉴权中间件
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local util = require("klbcore.klbweb.mw.util")


local B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"


local function b64_decode(data)
	data = string.gsub(data or "", "[^" .. B64 .. "=]", "")
	local bin = data:gsub(".", function (x)
		if "=" == x then
			return ""
		end

		local f = (string.find(B64, x, 1, true) or 1) - 1
		local r = ""
		for i = 6, 1, -1 do
			if 0 < (f % (2 ^ i) - f % (2 ^ (i - 1))) then
				r = r .. "1"
			else
				r = r .. "0"
			end
		end

		return r
	end)

	return (bin:gsub("%d%d%d?%d?%d?%d?%d?%d?", function (x)
		if 8 ~= #x then
			return ""
		end

		local c = 0
		for i = 1, 8 do
			if "1" == string.sub(x, i, i) then
				c = c + 2 ^ (8 - i)
			end
		end

		return string.char(c)
	end))
end


-- @brief Basic 鉴权
-- @param [in]      opts[table]			`realm` / `users` map 或 `verify(user, pass)`
-- @return fn[function]					中间件
local function basic(opts)
	opts = opts or {}
	local realm = opts.realm or "klbweb"
	local users = opts.users or {}
	local verify = opts.verify

	return function (req, res, nxt)
		local raw = util.header_get(req.headers, "authorization")
		local b64 = string.match(raw, "^[Bb]asic%s+(%S+)")
		local user = ""
		local pass = ""
		if b64 then
			local decoded = b64_decode(b64)
			user, pass = string.match(decoded, "^([^:]*):(.*)$")
			user = user or ""
			pass = pass or ""
		end

		local ok_auth = false
		if "function" == type(verify) then
			ok_auth = true == verify(user, pass, req)
		elseif "" ~= user and users[user] == pass then
			ok_auth = true
		end

		if not ok_auth then
			res:header("WWW-Authenticate", string.format('Basic realm="%s"', realm))
			res:co_status(401, "text/plain", "Unauthorized")
			return
		end

		req.auth_user = user
		nxt()
	end
end


return basic
