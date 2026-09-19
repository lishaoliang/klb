--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 静态目录
-- @note   默认关目录列表; 过滤见 pathex.filter_path; 304 / Range / 预压 gzip
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local lfs = require("lfs")
local pathex = require("klbcore.util.pathex")
local listing = require("klbcore.klbweb.static.listing")
local file = require("klbcore.klbweb.static.file")


--------------------------------------------------------------------------------------------
-- 内部

local function resolve_mount(url_rel, mounts)
	for i = 1, #mounts do
		local prefix = mounts[i].prefix
		if "/" ~= prefix then
			if url_rel == prefix then
				return mounts[i], "/", prefix
			end

			local head = prefix .. "/"
			if head == string.sub(url_rel, 1, #head) then
				return mounts[i], string.sub(url_rel, #prefix + 1), prefix
			end
		end
	end

	for i = 1, #mounts do
		if "/" == mounts[i].prefix then
			return mounts[i], url_rel, ""
		end
	end

	return nil, nil, nil
end


local function mount_url(prefix, fs_rel)
	if "" == prefix then
		return fs_rel
	end

	if "/" == fs_rel then
		return prefix
	end

	return prefix .. fs_rel
end


local function open_path(mount, rel, url_path, headers)
	local root = mount.dir
	local real
	if "/" == rel then
		real = root
	else
		real = root .. rel
	end

	local do_listing = mount.listing
	local index = mount.index or "index.html"
	local max_file = mount.max_file or 0
	local gzip = false ~= mount.gzip
	local gzip_dynamic = true == mount.gzip_dynamic
	local mode = lfs.attributes(real, "mode")
	if "directory" == mode then
		local index_path = real .. "/" .. index
		if "file" == lfs.attributes(index_path, "mode") then
			return file.open(index_path, "/" .. index, headers, max_file, gzip, gzip_dynamic)
		end

		if do_listing then
			local body = listing.make(url_path, real)
			local extra = file.extra_of("text/html", "listing", "index.html", nil, nil, {})
			return 200, "text/html", body, "listing", extra
		end

		return 404, "", "", "miss", {}
	end

	if "file" ~= mode then
		return 404, "", "", "miss", {}
	end

	return file.open(real, rel, headers, max_file, gzip, gzip_dynamic)
end


--------------------------------------------------------------------------------------------
-- 静态对象

local S = {}

-- @brief 挂载静态前缀
-- @param [in]      prefix[string]		URL 前缀; eg. `/`
-- @param [in]      dir[string]			本地目录
-- @param [in]      opts[table]			[可选] listing/index/max_file/gzip/gzip_dynamic
-- @return ok[boolean]					true 成功; 参数非法为 false
function S:mount(prefix, dir, opts)
	if "string" ~= type(prefix) or "string" ~= type(dir) then
		return false
	end

	if "" == prefix or "" == dir then
		return false
	end

	opts = opts or {}
	prefix = string.gsub(prefix, "\\", "/")
	dir = string.gsub(dir, "\\", "/")
	dir = string.gsub(dir, "/+$", "")

	if "/" ~= prefix then
		prefix = string.gsub(prefix, "/+$", "")
		if "" == prefix then
			prefix = "/"
		end
	end

	local do_listing = self._defaults.listing
	if nil ~= opts.listing then
		do_listing = true == opts.listing
	end

	local index = opts.index or self._defaults.index
	local max_file = opts.max_file or self._defaults.max_file
	local gzip = self._defaults.gzip
	if nil ~= opts.gzip then
		gzip = false ~= opts.gzip
	end

	local gzip_dynamic = self._defaults.gzip_dynamic
	if nil ~= opts.gzip_dynamic then
		gzip_dynamic = true == opts.gzip_dynamic
	end

	local rec = {
		prefix = prefix,
		dir = dir,
		listing = do_listing,
		index = index,
		max_file = max_file,
		gzip = gzip,
		gzip_dynamic = gzip_dynamic,
	}

	for i = 1, #self._mounts do
		if self._mounts[i].prefix == prefix then
			self._mounts[i] = rec
			return true
		end
	end

	self._mounts[#self._mounts + 1] = rec
	return true
end


-- @brief 按请求路径取本地文件
-- @param [in]      url_path[string]	请求 URL 路径 (可含 query)
-- @return body[string]					文件内容; 未命中或失败为 nil
-- @return mime[string]					MIME; 失败为 `""`
function S:read_file(url_path)
	local status, mime, body = self:serve(url_path)
	if 200 == status then
		return body, mime
	end

	return nil, ""
end


-- @brief 按请求路径提供静态内容
-- @param [in]      url_path[string]	请求 URL 路径
-- @param [in]      headers[table]		[可选] 请求头 (Range / 条件缓存 / Accept-Encoding)
-- @return status[number]				200/206/304/400/404/413/416; 0 无挂载
-- @return mime[string]
-- @return body[string]
-- @return kind[string]
-- @return extra[table]					额外响应头数组 `{ { name, value }, ... }`
function S:serve(url_path, headers)
	local path = pathex.filter_path(url_path)
	if not path then
		return 400, "text/plain", "bad path", "bad", {}
	end

	local mount, rel, prefix = resolve_mount(path, self._mounts)
	if not mount then
		return 0, "", "", "none", {}
	end

	local url = mount_url(prefix, rel)
	return open_path(mount, rel, url, headers or {})
end


--------------------------------------------------------------------------------------------
-- 工厂

local staticer = {}

-- @brief 新建静态映射表
-- @param [in]      defaults[table]		[可选] 默认 listing/index/max_file/gzip/gzip_dynamic
-- @return [table]	静态对象
staticer.new = function (defaults)
	defaults = defaults or {}
	local obj = {
		_mounts = {},
		_defaults = {
			listing = true == defaults.listing,
			index = defaults.index or "index.html",
			max_file = defaults.max_file or (2 * 1024 * 1024),
			gzip = false ~= defaults.gzip,
			gzip_dynamic = true == defaults.gzip_dynamic,
		},
	}

	return setmetatable(obj, { __index = S })
end


return staticer
