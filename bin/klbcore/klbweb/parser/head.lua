--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   head.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 请求行与请求头解析
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local stringex = require("klbcore.util.stringex")


local head = {}


-- @brief 解析请求行
-- @param [in]      head[string]		原始 HTTP 头
-- @return method[string]				大写 METHOD; 失败为 `""`
-- @return path[string]					path (含未过滤的 `..` 等)
-- @return qs[string]					query 原文; 无为 `""`
-- @return ver[string]					HTTP 版本串; 无为 `""`
head.parse_request = function (head)
	local line = string.match(head or "", "^([^\r\n]+)")
	if not line then
		return "", "", "", ""
	end

	local method, target, ver = string.match(line, "^(%S+)%s+(%S+)%s*(%S*)")
	if not method then
		return "", "", "", ""
	end

	local path = string.match(target, "^([^?]*)") or "/"
	local qs = string.match(target, "%?(.*)$") or ""
	return string.upper(method), path, qs, ver or ""
end


-- @brief 解析请求头
-- @param [in]      head[string]		原始 HTTP 头
-- @return headers[table]				小写键; 同名后写覆盖
head.parse_headers = function (head)
	local headers = {}
	local skip = true

	for line in string.gmatch(head or "", "[^\r\n]+") do
		if skip then
			skip = false
		else
			local k, v = string.match(line, "^([^:]+):%s*(.*)$")
			if k then
				k = string.lower(stringex.trim(k))
				headers[k] = stringex.trim(v or "")
			end
		end
	end

	return headers
end


return head
