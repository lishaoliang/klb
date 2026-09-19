--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   listing.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 静态目录列表 HTML
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local lfs = require("lfs")


local listing = {}


local function html_escape(s)
	s = string.gsub(s, "&", "&amp;")
	s = string.gsub(s, "<", "&lt;")
	s = string.gsub(s, ">", "&gt;")
	s = string.gsub(s, '"', "&quot;")
	return s
end


local function parent_url(url_path)
	local parent = string.match(url_path, "^(.+)/[^/]+$")
	if not parent or "" == parent then
		return "/"
	end

	return parent
end


local function child_url(url_path, name)
	if "/" == url_path then
		return "/" .. name
	end

	return url_path .. "/" .. name
end


-- @brief 生成目录列表 HTML
-- @param [in]      url_path[string]	URL 路径
-- @param [in]      dir_path[string]	本地目录
-- @return [string]						HTML
listing.make = function (url_path, dir_path)
	local names = {}

	for name in lfs.dir(dir_path) do
		if "." ~= name and ".." ~= name and ".svn" ~= name then
			names[#names + 1] = name
		end
	end

	table.sort(names)

	local t = {}
	t[#t + 1] = "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"><title>Index of "
	t[#t + 1] = html_escape(url_path)
	t[#t + 1] = "</title></head><body><h1>Index of "
	t[#t + 1] = html_escape(url_path)
	t[#t + 1] = "</h1><ul>"

	if "/" ~= url_path then
		t[#t + 1] = string.format("<li><a href=\"%s\">../</a></li>", html_escape(parent_url(url_path)))
	end

	for i = 1, #names do
		local name = names[i]
		local mode = lfs.attributes(dir_path .. "/" .. name, "mode")
		local href = child_url(url_path, name)
		local label = name
		if "directory" == mode then
			href = href .. "/"
			label = name .. "/"
		end
		t[#t + 1] = string.format("<li><a href=\"%s\">%s</a></li>", html_escape(href), html_escape(label))
	end

	t[#t + 1] = "</ul></body></html>"
	return table.concat(t)
end


return listing
