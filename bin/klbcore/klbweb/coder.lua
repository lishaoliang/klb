--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   coder.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb HTTP 状态码与 reason-phrase 对应
-- @note   状态行固定 `HTTP/1.1`; 未知码无 phrase, 仍拼合法状态行
-- @history 修改历史
--		[2026-09] 创建文件
--]]

--------------------------------------------------------------------------------------------
-- 状态码 -> reason-phrase (RFC 9110 常用子集)

local PHRASE = {
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


--------------------------------------------------------------------------------------------
-- 导出

local coder = {}

-- @brief 由 HTTP 状态码取 reason-phrase
-- @param [in]      code[number(int)]	状态码; eg. `500`
-- @return [string]						短语; 未知或非法为 `""`
coder.phrase = function (code)
	if "number" ~= type(code) then
		return ""
	end

	return PHRASE[code] or ""
end


-- @brief 由 HTTP 状态码拼状态行
-- @param [in]      code[number(int)]	状态码; eg. `500`
-- @return [string]						整行; eg. `HTTP/1.1 500 Internal Server Error`
-- @note 非法 code 按 `500`; 未知数字码无 phrase, 形如 `HTTP/1.1 418`
coder.status = function (code)
	if "number" ~= type(code) then
		code = 500
	end

	local phrase = PHRASE[code]
	if not phrase then
		return string.format("HTTP/1.1 %d", code)
	end

	return string.format("HTTP/1.1 %d %s", code, phrase)
end


return coder
