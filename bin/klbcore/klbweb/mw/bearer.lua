--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   bearer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb Bearer 鉴权骨架中间件
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local util = require("klbcore.klbweb.mw.util")


-- @brief Bearer 骨架; 校验函数留给产品
-- @param [in]      verify[function]	`verify(token, req)` 返回 true 通过
-- @return fn[function]					中间件
local function bearer(verify)
	return function (req, res, nxt)
		local raw = util.header_get(req.headers, "authorization")
		local token = string.match(raw, "^[Bb]earer%s+(%S+)") or ""
		req.token = token
		if "function" ~= type(verify) then
			res:co_status(401, "text/plain", "Unauthorized")
			return
		end

		if true ~= verify(token, req) then
			res:header("WWW-Authenticate", "Bearer")
			res:co_status(401, "text/plain", "Unauthorized")
			return
		end

		nxt()
	end
end


return bearer
