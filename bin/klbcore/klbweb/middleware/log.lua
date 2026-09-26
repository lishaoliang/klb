--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   log.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 访问日志中间件
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local ktime = require("ktime")


-- @brief 访问日志; 默认 print 一行
-- @param [in]      opts[table]			`write` 回调; 默认 print
-- @return fn[function]					中间件
local function log(opts)
	opts = opts or {}
	local write = opts.write
	if "function" ~= type(write) then
		write = function (...)
			print(...)
		end
	end

	return function (req, res, nxt)
		local t0 = ktime.tick_count()
		nxt()
		local dt = ktime.tick_count() - t0
		local code = res._status_code or 0
		write(req.method, req.tls and "https" or "http", req.path, code, dt, req.request_id or "")
	end
end


return log
