--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   load_images.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  加载所有图片
-- @note   启动前须 res.configure({ image_dirs = {...} })
--  \n key 省略扩展名; 同目录同 stem 优先 bmp 再 png
-- @history 修改历史
--  \n [2026] 创建文件
--]]
local lfs = require("lfs")
local klbui = require("klbcore.klbui")
local pathex = require("klbcore.util.pathex")
local tableex = require("klbcore.util.tableex")


local load_images = {}


local image_dirs = {}		-- 图片检索目录 (可直接 lfs 扫描的路径)

-- 同 stem 优先级: 数值越小越优先
local IMAGE_EXT_RANK = {
	['bmp'] = 1,
	['png'] = 2,
}


-- @brief 去掉 key 末尾的 bmp/png 扩展名
-- @param [in]	key[string]		[必须]资源 key; 可带或不带扩展名
-- @return [string]		无扩展名 key; eg. '/aaa/audio'
local function NormalizeKey(key)
	if 'string' ~= type(key) then
		return key
	end

	local ext_raw = pathex.ext(key)
	local ext = string.lower(ext_raw)
	if nil == IMAGE_EXT_RANK[ext] then
		return key
	end

	return string.sub(key, 1, #key - #ext_raw - 1)
end


-- @brief 从文件名取 stem 与小写扩展名
-- @param [in]	file[string]	[必须]文件名 (不含目录)
-- @return stem[string]		无扩展名; 非 bmp/png 为 nil
-- @return ext[string]		小写扩展名; 非 bmp/png 为 nil
local function StemOfFile(file)
	local ext_raw = pathex.ext(file)
	local ext = string.lower(ext_raw)
	if nil == IMAGE_EXT_RANK[ext] then
		return nil, nil
	end

	local stem = string.sub(file, 1, #file - #ext_raw - 1)
	if '' == stem then
		return nil, nil
	end

	return stem, ext
end


-- @brief 加载图片资源
-- @param [in]	keypath[string]		[必须]图片资源 key (相对 image_dirs 检索根, 以'/'开头, 无扩展名); eg. '/aaa/audio'
-- @param [in]	fullpath[string]	[必须]图片文件全路径; eg. '/mnt/mtd/aaa/audio.bmp'
-- @return 无
local function LoadImage(keypath, fullpath)
	local ret = klbui.load_image(keypath, fullpath)
	if 0 ~= ret then
		print('LoadImage error!', keypath, fullpath)
	end
end


-- @brief 从目录中查找并加载图片文件
-- @note 同目录同 stem: 优先 bmp, 无则 png; 登记 key 不含扩展名
local function FindImageFiles(path, keypath)
	local stems = {}
	local subdirs = {}

	for file in lfs.dir(path) do
		if file ~= '.' and file ~= '..' then
			local fullpath = table.concat({path, '/', file})	-- path .. '/' .. file
			local attr = lfs.attributes(fullpath)
			if attr then
				if attr.mode == 'file' then				-- 文件
					local stem, ext = StemOfFile(file)
					if nil ~= stem then
						local rank = IMAGE_EXT_RANK[ext]
						local old = stems[stem]
						if nil == old or rank < old.rank then
							stems[stem] = {
								['rank'] = rank,
								['fullpath'] = fullpath,
								['key'] = table.concat({keypath, '/', stem}),
							}
						end
					end
				elseif attr.mode == 'directory' then	-- 目录
					subdirs[#subdirs + 1] = {
						['fullpath'] = fullpath,
						['key'] = table.concat({keypath, '/', file}),
					}
				end
			end
		end
	end

	for _, item in pairs(stems) do
		LoadImage(item.key, item.fullpath)
	end

	for i = 1, #subdirs do
		FindImageFiles(subdirs[i].fullpath, subdirs[i].key)
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
	LoadImage(NormalizeKey(key), fullpath)
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
