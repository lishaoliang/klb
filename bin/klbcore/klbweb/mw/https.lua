--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   https.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb HTTP 跳转 HTTPS 中间件
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local util = require("klbcore.klbweb.mw.util")


-- @brief HTTP -> HTTPS 重定向
-- @param [in]      opts[table]			`port` 可选 HTTPS 端口
-- @return fn[function]					中间件
local function https(opts)
	opts = opts or {}
	local https_port = opts.port

	return function (req, res, nxt)
		if req.tls then
			nxt()
			return
		end

		local host = util.header_get(req.headers, "host")
		if "" == host then
			host = "127.0.0.1"
		else
			host = string.gsub(host, ":%d+$", "")
		end

		local loc
		if "number" == type(https_port) and 443 ~= https_port then
			loc = string.format("https://%s:%d%s", host, https_port, req.path)
		else
			loc = string.format("https://%s%s", host, req.path)
		end

		if "" ~= (req.query_string or "") then
			loc = loc .. "?" .. req.query_string
		end

		res:co_redirect(loc, 301)
	end
end


return https
