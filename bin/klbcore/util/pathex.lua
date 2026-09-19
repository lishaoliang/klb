--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   pathex.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  路径相关的帮助函数
-- @history 修改历史
--  \n [2026] 创建文件
--]]
local stringex = require("klbcore.util.stringex")

local pathex = {}


-- @brief 文件扩展名
-- @param [in]	path[string]		[必须]路径; eg. '/aaa/bbb/ccc.bmp'
-- @return [string]		eg. 'bmp'
pathex.ext = function (path)
	local _, ext = string.match(path, '(%.)([^.]*)$')

	if ext then
		return ext
	end

	return ''
end


-- @brief 去除路径前后空白, 以及末尾的'/','\'
-- @param [in]	path[string]		[必须]路径; eg. '/aaa/bbb/'
-- @return [string]		eg. '/aaa/bbb'
pathex.trim = function (path)
	local s = stringex.trim(path)

	s = (string.gsub(s, '[\\/]+$', ''))

	return s
end


-- @brief 过滤 URL 路径; 去掉 `~` `../` 等相对段, 防止越权访问
-- @param [in]      url[string]			请求 URL (可含 query)
-- @return path[string]					规范化绝对 URL 路径; 非法为 nil
-- @note 拒 `..` / `.svn` / `~`; 另拒绝 NUL
pathex.filter_path = function (url)
	local path = string.match(url or "", "^[^?]*")
	if not path then
		return nil
	end

	-- 展开 %xx; 多轮以摊平双重编码 (`%252e` 等)
	local n = 0
	repeat
		local prev = path
		path = stringex.url_decode(path)
		n = n + 1
	until prev == path or 4 <= n

	if string.find(path, "\0", 1, true) then
		return nil
	end

	-- 防止使用相对路径, 非法下载文件
	path = string.gsub(path, "~", "")
	path = string.gsub(path, "\\", "/")
	path = string.gsub(path, "[.]+/", "/")
	path = string.gsub(path, "[/]+/", "/")

	-- `[.]+/` 摊不平末尾无 `/` 的 `..`; 分段再拒
	local parts = {}
	for seg in string.gmatch(path, "[^/]+") do
		if ".." == seg or ".svn" == seg then
			return nil
		elseif "." ~= seg and "" ~= seg then
			parts[#parts + 1] = seg
		end
	end

	if 0 == #parts then
		return "/"
	end

	return "/" .. table.concat(parts, "/")
end


return pathex
