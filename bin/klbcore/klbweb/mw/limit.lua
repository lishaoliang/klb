--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   limit.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 限流中间件
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local util = require("klbcore.klbweb.mw.util")


-- @brief 简单限流 (无对端 IP 时按 Authorization / Cookie / `*`)
-- @param [in]      opts[table]			`window` 秒, `max` 次数, `key` 可选函数
-- @return fn[function]					中间件
local function limit(opts)
	opts = opts or {}
	local window = opts.window or 60
	local max_n = opts.max or 60
	local key_fn = opts.key
	local buckets = {}

	return function (req, res, nxt)
		local key
		if "function" == type(key_fn) then
			key = key_fn(req) or "*"
		else
			key = util.header_get(req.headers, "authorization")
			if "" == key then
				key = (req.cookies and req.cookies["klbweb.sid"]) or "*"
			end
		end

		local now = os.time()
		local b = buckets[key]
		if not b or b.start + window <= now then
			b = { start = now, n = 0 }
			buckets[key] = b
		end

		b.n = b.n + 1
		if max_n < b.n then
			res:header("Retry-After", tostring(window))
			res:co_status(429, "text/plain", "Too Many Requests")
			return
		end

		nxt()
	end
end


return limit
