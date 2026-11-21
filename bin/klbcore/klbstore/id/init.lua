--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
--
-- @file   init.lua
-- @brief  klbstore 文件标识 (group/M00/xx/yy/<name>[.<ext>])
-- @stability experimental
-- @impl evolving
-- @note   id 只表达"组 + 两级散列目录 + 名称"; 不含主机与端口 (节点由 tracker 选)
--   \n 名称 16 位十六进制; 前 2 位作一级目录, 次 2 位作二级目录
--   \n 桩: 名称以时间 + 进程内序号 + 随机数拼装, 后续换 C 侧唯一 id
--]]


local klbcode = require("klbcore.klbcode")
local base = klbcode.base


local mod = {}


mod.GROUP_DEFAULT = "group1"
mod.PREFIX = "M00"


local s_seq = 0


local function NormGroup(p_group)
	if nil == p_group or "" == p_group then
		return mod.GROUP_DEFAULT, nil
	end

	if "string" ~= type(p_group) or not p_group:match("^[%w_%-]+$") then
		return nil, base.INVAL
	end

	return p_group, nil
end


local function NormExt(p_ext)
	if nil == p_ext or "" == p_ext then
		return "", nil
	end

	if "string" ~= type(p_ext) or not p_ext:match("^[%w_%-]+$") then
		return nil, base.INVAL
	end

	return p_ext, nil
end


local function NextName()
	s_seq = (s_seq + 1) % 65536

	local t = os.time() % 4294967296
	local r = math.random(0, 65535)

	return string.format("%08x%04x%04x", t, s_seq, r)
end


-- @brief 生成文件标识
-- @param [in] group[string|nil]	组名; nil 取 GROUP_DEFAULT
-- @param [in] ext[string|nil]		扩展名 (不含点)
-- @return [string] file_id; 失败 nil, code
-- @stability experimental
mod.new_id = function (p_group, p_ext)
	local group, code = NormGroup(p_group)
	if nil == group then
		return nil, code
	end

	local ext
	ext, code = NormExt(p_ext)
	if nil == ext then
		return nil, code
	end

	local name = NextName()
	local dir1 = name:sub(1, 2)
	local dir2 = name:sub(3, 4)

	if "" == ext then
		return string.format("%s/%s/%s/%s/%s", group, mod.PREFIX, dir1, dir2, name)
	end

	return string.format("%s/%s/%s/%s/%s.%s", group, mod.PREFIX, dir1, dir2, name, ext)
end


-- @brief 解析文件标识
-- @param [in] file_id[string]		文件标识
-- @return [table] { group, prefix, dir1, dir2, name, ext, file }; 失败 nil, code
-- @stability experimental
mod.parse = function (p_file_id)
	if "string" ~= type(p_file_id) then
		return nil, base.INVAL
	end

	local group, prefix, dir1, dir2, file =
		p_file_id:match("^(%w[%w_%-]*)/(%w%d%d)/(%x%x)/(%x%x)/([%w_%-%.]+)$")
	if nil == file then
		return nil, base.INVAL
	end

	local name, ext = file:match("^([%w_%-]+)%.([%w_%-]+)$")
	if nil == name then
		name = file
		ext = ""
	end

	return {
		group = group,
		prefix = prefix,
		dir1 = dir1,
		dir2 = dir2,
		name = name,
		ext = ext,
		file = file,
	}
end


-- @brief 校验文件标识
-- @param [in] file_id[string]		文件标识
-- @return [boolean] true.合法; false.非法
-- @stability experimental
mod.is_valid = function (p_file_id)
	local info = mod.parse(p_file_id)

	return nil ~= info
end


-- @brief 取两级散列目录 (xx/yy)
-- @param [in] file_id[string]		文件标识
-- @return [string] "xx/yy"; 失败 nil, code
-- @stability experimental
mod.dir_of = function (p_file_id)
	local info, code = mod.parse(p_file_id)
	if nil == info then
		return nil, code
	end

	return string.format("%s/%s", info.dir1, info.dir2)
end


-- @brief 取相对落盘路径 (group/M00/xx/yy/<name>[.<ext>])
-- @param [in] file_id[string]		文件标识
-- @return [string] 相对路径; 失败 nil, code
-- @stability experimental
mod.rel_path = function (p_file_id)
	local info, code = mod.parse(p_file_id)
	if nil == info then
		return nil, code
	end

	return string.format("%s/%s/%s/%s/%s", info.group, info.prefix, info.dir1, info.dir2, info.file)
end


return mod
