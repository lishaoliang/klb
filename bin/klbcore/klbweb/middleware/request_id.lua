--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   request_id.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb Request-ID 中间件
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local krand = require("krand")
local util = require("klbcore.klbweb.util.header")


local function sanitize_id(raw)
	raw = raw or ""
	if 128 < #raw then
		raw = string.sub(raw, 1, 128)
	end

	if "" == raw then
		return ""
	end

	if not string.match(raw, "^[%w%._%-]+$") then
		return ""
	end

	return raw
end


-- @brief Request-ID; 沿用请求头或新生成, 写入 req 与响应头
-- @param [in]      opts[table]			[可选] `header` 请求/响应头名, 默认 `x-request-id`
-- @return fn[function]					中间件
local function request_id(opts)
	opts = opts or {}
	local header = string.lower(opts.header or "x-request-id")
	local out_name = opts.out or "X-Request-ID"

	return function (req, res, nxt)
		local id = sanitize_id(util.header_get(req.headers, header))
		if "" == id then
			id = krand.rand_string(16)
		end

		req.request_id = id
		res:header(out_name, id)
		nxt()
	end
end


return request_id
