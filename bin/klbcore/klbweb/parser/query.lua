--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   query.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb query / cookie / urlencoded 解析
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local stringex = require("klbcore.util.stringex")


local query = {}


local function put_multi(t, k, v)
	if "" == k then
		return
	end

	local old = t[k]
	if nil == old then
		t[k] = v
	elseif "table" == type(old) then
		old[#old + 1] = v
	else
		t[k] = { old, v }
	end
end


-- 同名多值写入表; 内部供 parse_query / multipart 共用
query.put_multi = put_multi


-- @brief 解析 query string; 同名多值为数组
-- @param [in]      qs[string]			`?` 后原文
-- @return query[table]					解码后的键值; 无为 `{}`
query.parse_query = function (qs)
	local out = {}
	if "" == (qs or "") then
		return out
	end

	for pair in string.gmatch(qs, "[^&]+") do
		local k, v = string.match(pair, "^([^=]*)=?(.*)$")
		k = stringex.url_decode(k or "")
		v = stringex.url_decode(v or "")
		put_multi(out, k, v)
	end

	return out
end


-- @brief 解析 Cookie 头
-- @param [in]      headers[table]		小写键头表
-- @return cookies[table]				name -> value; 无为 `{}`
query.parse_cookies = function (headers)
	local cookies = {}
	local raw = (headers and headers["cookie"]) or ""
	if "" == raw then
		return cookies
	end

	for pair in string.gmatch(raw, "[^;]+") do
		local k, v = string.match(pair, "^%s*([^=]+)=(.*)$")
		if k then
			k = stringex.trim(k)
			v = stringex.trim(v or "")
			if "" ~= k then
				cookies[k] = v
			end
		end
	end

	return cookies
end


-- @brief 解析 application/x-www-form-urlencoded
-- @param [in]      headers[table]		小写键头表
-- @param [in]      body[string]		请求体
-- @return form[table]					同 query; 非该类型为 `{}`
query.parse_form = function (headers, body)
	local form = {}
	local ctype = string.lower((headers and headers["content-type"]) or "")
	if not string.find(ctype, "application/x-www-form-urlencoded", 1, true) then
		return form
	end

	return query.parse_query(body or "")
end


return query
