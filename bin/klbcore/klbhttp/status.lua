--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   status.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP 常用状态码常量与 reason-phrase (供 klbhttp / proxy 等共用)
-- @note   常量别名自 klbcore.klbcode.netcode.HTTP_*; 短语为 RFC 9110 常用子集
-- @history 修改历史
--  \n [2026-10] 创建文件
--  \n [2026-11] 常量对齐 klbcode.net HTTP 别名
--]]


local net = require("klbcore.klbcode.netcode")


local status = {}


--------------------------------------------------------------------------------------------
-- 常用状态码常量 (数值同 net.HTTP_*)

-- 1xx
status.CONTINUE = net.HTTP_CONTINUE
status.SWITCHING_PROTOCOLS = net.HTTP_SWITCHING_PROTOCOLS

-- 2xx
status.OK = net.HTTP_OK
status.CREATED = net.HTTP_CREATED
status.ACCEPTED = net.HTTP_ACCEPTED
status.NO_CONTENT = net.HTTP_NO_CONTENT
status.PARTIAL_CONTENT = net.HTTP_PARTIAL_CONTENT

-- 3xx
status.MOVED_PERMANENTLY = net.HTTP_MOVED_PERMANENTLY
status.FOUND = net.HTTP_FOUND
status.SEE_OTHER = net.HTTP_SEE_OTHER
status.NOT_MODIFIED = net.HTTP_NOT_MODIFIED
status.TEMPORARY_REDIRECT = net.HTTP_TEMPORARY_REDIRECT
status.PERMANENT_REDIRECT = net.HTTP_PERMANENT_REDIRECT

-- 4xx
status.BAD_REQUEST = net.HTTP_BAD_REQUEST
status.UNAUTHORIZED = net.HTTP_UNAUTHORIZED
status.FORBIDDEN = net.HTTP_FORBIDDEN
status.NOT_FOUND = net.HTTP_NOT_FOUND
status.METHOD_NOT_ALLOWED = net.HTTP_METHOD_NOT_ALLOWED
status.PROXY_AUTH_REQUIRED = net.HTTP_PROXY_AUTHENTICATION_REQUIRED
status.REQUEST_TIMEOUT = net.HTTP_REQUEST_TIMEOUT
status.CONFLICT = net.HTTP_CONFLICT
status.GONE = net.HTTP_GONE
status.CONTENT_TOO_LARGE = net.HTTP_PAYLOAD_TOO_LARGE
status.UNSUPPORTED_MEDIA_TYPE = net.HTTP_UNSUPPORTED_MEDIA_TYPE
status.RANGE_NOT_SATISFIABLE = net.HTTP_RANGE_NOT_SATISFIABLE
status.TOO_MANY_REQUESTS = net.HTTP_TOO_MANY_REQUESTS

-- 5xx
status.INTERNAL_SERVER_ERROR = net.HTTP_INTERNAL_SERVER_ERROR
status.NOT_IMPLEMENTED = net.HTTP_NOT_IMPLEMENTED
status.BAD_GATEWAY = net.HTTP_BAD_GATEWAY
status.SERVICE_UNAVAILABLE = net.HTTP_SERVICE_UNAVAILABLE
status.GATEWAY_TIMEOUT = net.HTTP_GATEWAY_TIMEOUT


--------------------------------------------------------------------------------------------
-- 状态码 -> reason-phrase

local PHRASE = {
	[100] = "Continue",
	[101] = "Switching Protocols",

	[200] = "OK",
	[201] = "Created",
	[202] = "Accepted",
	[204] = "No Content",
	[206] = "Partial Content",

	[301] = "Moved Permanently",
	[302] = "Found",
	[303] = "See Other",
	[304] = "Not Modified",
	[307] = "Temporary Redirect",
	[308] = "Permanent Redirect",

	[400] = "Bad Request",
	[401] = "Unauthorized",
	[403] = "Forbidden",
	[404] = "Not Found",
	[405] = "Method Not Allowed",
	[407] = "Proxy Authentication Required",
	[408] = "Request Timeout",
	[409] = "Conflict",
	[410] = "Gone",
	[413] = "Content Too Large",
	[415] = "Unsupported Media Type",
	[416] = "Range Not Satisfiable",
	[429] = "Too Many Requests",

	[500] = "Internal Server Error",
	[501] = "Not Implemented",
	[502] = "Bad Gateway",
	[503] = "Service Unavailable",
	[504] = "Gateway Timeout",
}


-- @brief 由 HTTP 状态码取 reason-phrase
-- @param [in]      code[number|string]	状态码; eg. 404 / "404"
-- @return [string]						短语; 未知或非法为 ""
status.phrase = function (code)
	local n = tonumber(code)
	if n == nil then
		return ""
	end

	return PHRASE[n] or ""
end


return status

-- end
