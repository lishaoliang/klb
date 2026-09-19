--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   compile.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 路由 path 编译
-- @note   规范化 / `:name` / `*` 转 Lua pattern
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local compile = {}


-- @brief 规范化路由 path: 补前导 `/`, 去掉尾 `/` (根除外)
compile.normalize_path = function (path)
	path = string.gsub(path, "\\", "/")
	if "/" ~= string.sub(path, 1, 1) then
		path = "/" .. path
	end

	if "/" ~= path then
		path = string.gsub(path, "/+$", "")
		if "" == path then
			path = "/"
		end
	end

	return path
end


-- @brief 字面量转 Lua pattern
local function escape_lua_pat(s)
	return (string.gsub(s, "([%^%$%(%)%%%.%[%]%*%+%-%?])", "%%%1"))
end


-- @brief 编译 path 为 Lua pattern
-- @return pat[string]		失败为 nil
-- @return names[table]		捕获名数组; `*` 名为 `"*"`
compile.compile_path = function (path)
	local names = {}
	local buf = { "^" }
	local i = 1
	local n = #path

	while i <= n do
		local c = string.sub(path, i, i)
		if ":" == c then
			local name = string.match(string.sub(path, i + 1), "^([%a_][%w_]*)")
			if not name then
				return nil, {}
			end

			names[#names + 1] = name
			buf[#buf + 1] = "([^/]+)"
			i = i + 1 + #name
		elseif "*" == c then
			if i ~= n then
				return nil, {}
			end

			names[#names + 1] = "*"
			buf[#buf + 1] = "(.*)"
			i = n + 1
		else
			local nxt = string.find(path, "[:*]", i)
			local lit
			if nxt then
				lit = string.sub(path, i, nxt - 1)
				i = nxt
			else
				lit = string.sub(path, i)
				i = n + 1
			end

			buf[#buf + 1] = escape_lua_pat(lit)
		end
	end

	buf[#buf + 1] = "$"
	return table.concat(buf), names
end


compile.is_pattern = function (path)
	if nil ~= string.find(path, ":", 1, true) then
		return true
	end

	if nil ~= string.find(path, "*", 1, true) then
		return true
	end

	return false
end


compile.pack_fns = function (fn, extra)
	if "table" == type(fn) then
		return fn
	end

	if "function" ~= type(fn) then
		return nil
	end

	if extra and 0 < #extra then
		local fns = { fn }
		for i = 1, #extra do
			if "function" ~= type(extra[i]) then
				return nil
			end

			fns[#fns + 1] = extra[i]
		end

		return fns
	end

	return { fn }
end


return compile
