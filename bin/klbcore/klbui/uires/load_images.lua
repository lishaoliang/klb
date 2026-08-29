--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   load_images.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  加载所有图片
-- @note   启动前须 res.configure({ image_dirs = {...} })
-- @history 修改历史
--  \n [2026] 创建文件
--]]
local lfs = require("lfs")
local klbui = require("klbcore.klbui")
local pathex = require("klbcore.util.pathex")
local tableex = require("klbcore.util.tableex")


local load_images = {}


local image_dirs = {}		-- BMP 图片检索目录 (可直接 lfs 扫描的路径)


-- @brief 加载图片资源
-- @param [in]	keypath[string]		[必须]图片资源 key (相对 image_dirs 检索根, 以'/'开头); eg. '/aaa/audio.bmp'
-- @param [in]	fullpath[string]	[必须]图片文件全路径; eg. '/mnt/mtd/aaa/audio.bmp'
-- @return 无
local function LoadImage(keypath, fullpath)
	local ret = klbui.load_image(keypath, fullpath)
	if 0 ~= ret then
		print('LoadImage error!', keypath, fullpath)
	end
end


-- @brief 从目录中查找并加载图片文件
local function FindImageFiles(path, keypath)
	for file in lfs.dir(path) do
		if file ~= '.' and file ~= '..' then
			local fullpath = table.concat({path, '/', file})	-- path .. '/' .. file
			local key = table.concat({keypath, '/', file})

			local attr = lfs.attributes(fullpath)
			if attr.mode == 'file' then				-- 文件
				if pathex.ext(fullpath) == 'bmp' then
					--print('==>>', key, fullpath)
					LoadImage(key, fullpath)
				end
			elseif attr.mode == 'directory' then	-- 目录
				FindImageFiles(fullpath, key)
			end
		end
	end
end


-- @brief 加载所有图片资源
-- @return 无
function load_images.load()
	for i = 1, #image_dirs do
		FindImageFiles(image_dirs[i], '')
	end
end


-- @brief 尝试 按绝对目录直接加载 图片
-- 若 已存在 key 对应的 图片, 则不加载
function load_images.try_load_path(key, fullpath)
	LoadImage(key, fullpath)
end


-- @brief 配置图片检索目录 (可直接扫描的路径)
-- @param [in] opts[table]	键: image_dirs
-- @return load_images
function load_images.configure(opts)
	if 'table' ~= type(opts) then
		return load_images
	end

	if 'table' == type(opts.image_dirs) then
		image_dirs = tableex.copy(opts.image_dirs)
		for i = 1, #image_dirs do
			image_dirs[i] = pathex.trim(image_dirs[i])
		end
	end

	return load_images
end


-- @brief 追加图片检索目录 (可直接扫描的路径)
-- @param [in] path[string]	[必须]路径
-- @return load_images
function load_images.append_path(path)
	if 'string' ~= type(path) then
		return load_images
	end

	image_dirs[#image_dirs + 1] = pathex.trim(path)

	return load_images
end


return load_images
