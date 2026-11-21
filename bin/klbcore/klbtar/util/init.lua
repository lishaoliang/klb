--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  klbtar 内部小工具 (失败返回 / 路径安全 / 目录 / opts)
-- @stability internal
-- @note   非对外 require 面
-- @history 修改历史
--  \n 2026 创建文件 (M1-M8)
--]]


local lfs = require("lfs")
local klbcode = require("klbcore.klbcode")
local base = klbcode.base


local util = {}


util.DEFAULT_MAX_FILE = 64 * 1024 * 1024
util.DEFAULT_MAX_TOTAL = 512 * 1024 * 1024
util.DEFAULT_MAX_ENTRIES = 100000
util.CHUNK = 8192


-- @brief 统一失败 (对象/表首返回 nil)
util.fail = function (code, errstr)
	return nil, code, errstr or ""
end


-- @brief 统一失败 (boolean 首返回)
util.fail_bool = function (code, errstr)
	return false, code, errstr or ""
end


util.ok_bool = function ()
	return true, klbcode.OK, ""
end


util.check_path = function (path)
	return "string" == type(path) and "" ~= path
end


-- @brief 规范化成员名: 正斜杠, 去前导 ./ 与多余 /
-- @return name[string]|nil
-- @return code[number(int)]
-- @return errstr[string]
util.normalize_member = function (name)
	if "string" ~= type(name) or "" == name then
		return nil, base.RANGE, "empty member name"
	end

	if nil ~= string.find(name, "\0", 1, true) then
		return nil, base.RANGE, "nul in member name"
	end

	local s = string.gsub(name, "\\", "/")

	-- 绝对路径 / Windows 盘符
	if string.match(s, "^/") or string.match(s, "^%a:[/\\]") then
		return nil, base.RANGE, "absolute member path"
	end

	local parts = {}
	for seg in string.gmatch(s, "[^/]+") do
		if ".." == seg then
			return nil, base.RANGE, "member path escapes"
		elseif "." ~= seg and "" ~= seg then
			parts[#parts + 1] = seg
		end
	end

	if 0 == #parts then
		return nil, base.RANGE, "empty member path"
	end

	return table.concat(parts, "/"), klbcode.OK, ""
end


-- @brief strip 前 N 段后规范化
util.strip_member = function (name, strip)
	local n = tonumber(strip) or 0
	if n <= 0 then
		return util.normalize_member(name)
	end

	local s = string.gsub(name or "", "\\", "/")
	local parts = {}
	for seg in string.gmatch(s, "[^/]+") do
		if "." ~= seg and "" ~= seg then
			parts[#parts + 1] = seg
		end
	end

	if n >= #parts then
		return nil, base.RANGE, "strip removes all path"
	end

	local kept = {}
	for i = n + 1, #parts do
		kept[#kept + 1] = parts[i]
	end

	return util.normalize_member(table.concat(kept, "/"))
end


-- @brief dest 下安全拼接成员路径
util.safe_join = function (dest, member)
	local name, code, err = util.normalize_member(member)
	if nil == name then
		return nil, code, err
	end

	if not util.check_path(dest) then
		return nil, base.INVAL, "bad dest"
	end

	local root = string.gsub(dest, "[\\/]+$", "")
	root = string.gsub(root, "\\", "/")
	local full = root .. "/" .. name

	-- 再确认无越界段
	local check, c2, e2 = util.normalize_member(name)
	if nil == check then
		return nil, c2, e2
	end

	return full, klbcode.OK, ""
end


util.ensure_dir = function (dir)
	if "" == (dir or "") then
		return false
	end

	if "directory" == lfs.attributes(dir, "mode") then
		return true
	end

	local parent = string.match(dir, "^(.*)[/\\][^/\\]+$")
	if nil ~= parent and "" ~= parent then
		if not util.ensure_dir(parent) then
			return false
		end
	end

	return true == lfs.mkdir(dir)
end


util.ensure_parent = function (path)
	local dir = string.match(path, "^(.*)[/\\][^/\\]+$")
	if nil == dir or "" == dir then
		return true
	end

	if string.match(dir, "^%a:$") then
		return true
	end

	return util.ensure_dir(dir)
end


util.file_exists = function (path)
	local mode = lfs.attributes(path, "mode")
	return nil ~= mode
end


-- @brief 解析 opts; 填默认上限
util.parse_opts = function (opts)
	local o = opts or {}
	if "table" ~= type(o) then
		o = {}
	end

	local out = {
		overwrite = true,
		strip = 0,
		max_file = util.DEFAULT_MAX_FILE,
		max_total = util.DEFAULT_MAX_TOTAL,
		max_entries = util.DEFAULT_MAX_ENTRIES,
	}

	if nil ~= o.overwrite then
		out.overwrite = false ~= o.overwrite
	end

	if nil ~= o.strip then
		out.strip = math.floor(tonumber(o.strip) or 0)
		if out.strip < 0 then
			out.strip = 0
		end
	end

	if nil ~= o.max_file then
		out.max_file = math.floor(tonumber(o.max_file) or util.DEFAULT_MAX_FILE)
	end

	if nil ~= o.max_total then
		out.max_total = math.floor(tonumber(o.max_total) or util.DEFAULT_MAX_TOTAL)
	end

	if nil ~= o.max_entries then
		out.max_entries = math.floor(tonumber(o.max_entries) or util.DEFAULT_MAX_ENTRIES)
	end

	return out
end


util.basename = function (path)
	local s = string.gsub(path or "", "\\", "/")
	local name = string.match(s, "([^/]+)$")
	return name or s
end


util.drop_gz_suffix = function (name)
	local n = name or ""
	n = string.gsub(n, "%.gz$", "")
	n = string.gsub(n, "%.tgz$", ".tar")
	return n
end


return util

-- end
