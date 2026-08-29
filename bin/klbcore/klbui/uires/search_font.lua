--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   search_font.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ttf 字库资源路径
-- @note   启动前须 res.configure({ font_search_dirs = {...} })
-- @history 修改历史
--  \n [2026] 创建文件
--]]
local lfs = require("lfs")
local string = require("string")
local pathex = require("klbcore.util.pathex")
local tableex = require("klbcore.util.tableex")


local search_font = {}


local font_search_dirs = {}	-- 字体检索目录 (可直接 lfs 扫描的路径)


-- @brief 在指定路径中寻找 ttf 字体文件
-- @param [in]	path[string]		[必须]路径: eg. './'
-- @return [string]	ttf字体路径
local function FindFont(path)
	for file in lfs.dir(path) do
		if file ~= '.' and file ~= '..' then
			local fullpath = table.concat({path, '/', file})	-- path .. '/' .. file

			local attr = lfs.attributes(fullpath)
			if attr.mode == 'file' then				-- 文件
				local ext = pathex.ext(fullpath)
				ext = string.lower(ext)
				if ext == 'ttf' or ext == 'ttc' or ext == 'otf' then
					return fullpath
				end
			end
		end
	end

	return ''
end


-- @brief 字体资源路径
-- @return [string]	ttf字体路径
function search_font.find()
	local path_list = font_search_dirs

	for i = 1, #path_list do
		local font_path = FindFont(path_list[i])
		if '' ~= font_path then
			return font_path	-- 找到第一个符合条件的字体文件
		end
	end

	return ''
end


-- @brief 配置字体检索目录 (可直接扫描的路径)
-- @param [in] opts[table]	键: font_search_dirs
-- @return search_font
function search_font.configure(opts)
	if 'table' ~= type(opts) then
		return search_font
	end

	if 'table' == type(opts.font_search_dirs) then
		font_search_dirs = tableex.copy(opts.font_search_dirs)
		for i = 1, #font_search_dirs do
			font_search_dirs[i] = pathex.trim(font_search_dirs[i])
		end
	end

	return search_font
end


-- @brief 追加字体检索目录 (可直接扫描的路径)
-- @param [in] path[string]	[必须]路径
-- @return search_font
function search_font.append_path(path)
	if 'string' ~= type(path) then
		return search_font
	end

	font_search_dirs[#font_search_dirs + 1] = pathex.trim(path)

	return search_font
end


setmetatable(search_font, {
	__call = function (t)
		return t.find()
	end
})

return search_font
