--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   file.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb handler 发本地文件
-- @note   路径拒 `..` / `.svn` / `~` / NUL. 实体由 C send_file 发送, 不因体积 413
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local lfs = require("lfs")
local http_mime = require("klbcore.util.http_mime")


local file = {}


local function bad_path(path)
	if "string" ~= type(path) or "" == path then
		return true
	end

	if nil ~= string.find(path, "\0", 1, true) then
		return true
	end

	if nil ~= string.find(path, "~", 1, true) then
		return true
	end

	local norm = string.gsub(path, "\\", "/")
	for seg in string.gmatch(norm, "[^/]+") do
		if ".." == seg or ".svn" == seg then
			return true
		end
	end

	return false
end


local function base_name(path)
	local norm = string.gsub(path or "", "\\", "/")
	local name = string.match(norm, "([^/]+)$") or ""
	name = string.gsub(name, '[\r\n"]', "_")
	if "" == name or ".." == name or ".svn" == name then
		return "download"
	end

	if nil ~= string.find(name, "~", 1, true) then
		return "download"
	end

	return name
end


local function empty_src()
	return { path = "", offset = 0, length = 0 }
end


-- @brief 检查本地文件可否发送
-- @param [in]      path[string]		本地路径
-- @param [in]      mime[string]		[可选] Content-Type; 省略则按扩展名
-- @return code[number]					200 可发; 400 路径非法; 404 不是文件
-- @return mime[string]
-- @return src[table]					`{ path, offset, length }`; 失败 path 为 `""`
file.open = function (path, mime)
	if bad_path(path) then
		return 400, "text/plain", empty_src()
	end

	local attr = lfs.attributes(path)
	if not attr or "file" ~= attr.mode then
		return 404, "text/plain", empty_src()
	end

	if "string" ~= type(mime) or "" == mime then
		mime = http_mime(base_name(path))
	end

	return 200, mime, {
		path = path,
		offset = 0,
		length = attr.size or 0,
	}
end


-- @brief Content-Disposition: attachment
-- @param [in]      name[string]		文件名或路径; 只取最后一段
-- @return [string]
file.attachment = function (name)
	return string.format('attachment; filename="%s"', base_name(name))
end


return file
