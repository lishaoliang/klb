--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   cors.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb CORS 中间件
-- @history 修改历史
--		[2026-09] 创建文件
--]]


-- @brief CORS; `true` 表示 `Origin: *`; `false` 关闭
-- @param [in]      opts[table|boolean]	配置; 见内联注释
-- @return fn[function]					中间件
local function cors(opts)
	-- opts = {
	--   origin[string]			默认 `*`
	--   methods[string]		默认 GET,HEAD,POST,PUT,PATCH,DELETE,OPTIONS
	--   headers[string]		默认 Content-Type,Authorization
	--   max_age[number]		默认 600
	--   credentials[boolean]	默认关; true 时回显 Origin + Allow-Credentials + Vary
	-- }
	if false == opts then
		return function (req, res, nxt)
			nxt()
		end
	end

	if true == opts then
		opts = {}
	end

	opts = opts or {}
	local origin = opts.origin or "*"
	local methods = opts.methods or "GET, HEAD, POST, PUT, PATCH, DELETE, OPTIONS"
	local allow_headers = opts.headers or "Content-Type, Authorization"
	local max_age = tostring(opts.max_age or 600)
	local credentials = true == opts.credentials

	return function (req, res, nxt)
		local allow_origin = origin
		if credentials then
			local req_origin = (req.headers and req.headers["origin"]) or ""
			if "" ~= req_origin then
				allow_origin = req_origin
			elseif "*" == origin then
				allow_origin = ""
			end

			if "" ~= allow_origin then
				res:header("Access-Control-Allow-Origin", allow_origin)
				res:header("Vary", "Origin")
			end

			res:header("Access-Control-Allow-Credentials", "true")
		else
			res:header("Access-Control-Allow-Origin", allow_origin)
		end

		res:header("Access-Control-Allow-Methods", methods)
		res:header("Access-Control-Allow-Headers", allow_headers)
		res:header("Access-Control-Max-Age", max_age)
		if "OPTIONS" == req.method then
			res:co_status(204, "text/plain", "")
			return
		end

		nxt()
	end
end


return cors
