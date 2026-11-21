--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  klbtar: 常用归档解包入口 (tar / gzip / tar.gz|tgz)
-- @note   require("klbcore.klbtar"); M1-M8: probe/list/extract/read/open_entry
--   \n P0 格式: .tar / .gz / .tar.gz / .tgz; zip/xz/bz2 非本范围
-- @history 修改历史
--  \n 2026 创建文件 (桩)
--  \n 2026 M1-M8 读侧实现
--]]


local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local detect = require("klbcore.klbtar.detect")
local archive = require("klbcore.klbtar.archive")


local klbtar = {}


--------------------------------------------------------------------------------------------
-- 返回码 (真源 klbcore.klbcode; 与 C klbtar_errcode_e / klb_basecode 同值)

klbtar.OK = klbcode.OK
klbtar.ERR = base.ERR
klbtar.E_ARG = base.INVAL
klbtar.E_OPEN = base.NOENT
klbtar.E_FORMAT = base.PACKAGE_BAD
klbtar.E_IO = base.IO
klbtar.E_PATH = base.RANGE
klbtar.E_LIMIT = base.LIMIT
klbtar.E_UNSUP = base.NOTSUP


-- 格式名常量
klbtar.FMT_TAR = detect.FMT_TAR
klbtar.FMT_GZIP = detect.FMT_GZIP
klbtar.FMT_TAR_GZ = detect.FMT_TAR_GZ


--------------------------------------------------------------------------------------------
-- 模块级入口

-- @brief 探测路径对应格式
-- @param [in]  path[string]			归档路径
-- @return fmt[string]|nil
-- @return code[number(int)]
-- @return errstr[string]
klbtar.probe = function (path)
	return detect.probe(path)
end


-- @brief 打开归档只读对象
-- @param [in]  path[string]			归档路径
-- @param [in]  opts[table]				[可选] max_file / max_total / max_entries / overwrite / strip
-- @return [table]|nil					对象; 失败为 nil
-- @return code[number(int)]
-- @return errstr[string]
-- @note 方法: list / extract / extract_file / read / open_entry / format / close / is_open
klbtar.open = function (path, opts)
	return archive.open(path, opts)
end


-- @brief 列出归档条目
-- @param [in]  path[string]			归档路径
-- @param [in]  opts[table]				[可选]
-- @return entries[table]|nil
-- @return code[number(int)]
-- @return errstr[string]
klbtar.list = function (path, opts)
	local arch, code, err = archive.open(path, opts)
	if nil == arch then
		return nil, code, err
	end

	local list, c2, e2 = arch.list(opts)
	arch.close()
	return list, c2, e2
end


-- @brief 解压归档到目录 (或 .gz 到文件/目录)
-- @param [in]  path[string]			归档路径
-- @param [in]  dest[string]			目标目录或文件
-- @param [in]  opts[table]				[可选] overwrite / strip / max_file / max_total
-- @return ok[boolean]
-- @return code[number(int)]
-- @return errstr[string]
klbtar.extract = function (path, dest, opts)
	local arch, code, err = archive.open(path, opts)
	if nil == arch then
		return false, code, err
	end

	local ok, c2, e2 = arch.extract(dest, opts)
	arch.close()
	return ok, c2, e2
end


-- @brief 单文件解压到路径
-- @param [in]  path[string]			归档路径
-- @param [in]  name[string]			成员名 (.gz 可忽略)
-- @param [in]  dest[string]			输出文件路径
-- @param [in]  opts[table]				[可选]
-- @return ok[boolean]
-- @return code[number(int)]
-- @return errstr[string]
klbtar.extract_file = function (path, name, dest, opts)
	local arch, code, err = archive.open(path, opts)
	if nil == arch then
		return false, code, err
	end

	local ok, c2, e2 = arch.extract_file(name, dest, opts)
	arch.close()
	return ok, c2, e2
end


-- @brief 完整读取成员到 string
-- @param [in]  path[string]			归档路径
-- @param [in]  name[string]			成员名 (.gz 可忽略)
-- @param [in]  opts[table]				[可选] max_file
-- @return data[string]|nil
-- @return code[number(int)]
-- @return errstr[string]
klbtar.read = function (path, name, opts)
	local arch, code, err = archive.open(path, opts)
	if nil == arch then
		return nil, code, err
	end

	local data, c2, e2 = arch.read(name, opts)
	arch.close()
	return data, c2, e2
end


-- @brief 流式打开成员
-- @param [in]  path[string]			归档路径
-- @param [in]  name[string]			成员名 (.gz 可忽略)
-- @param [in]  opts[table]				[可选]
-- @return entry[table]|nil				read(n) / readall() / close()
-- @return code[number(int)]
-- @return errstr[string]
klbtar.open_entry = function (path, name, opts)
	local arch, code, err = archive.open(path, opts)
	if nil == arch then
		return nil, code, err
	end

	local entry, c2, e2 = arch.open_entry(name, opts)
	if nil == entry then
		arch.close()
		return nil, c2, e2
	end

	arch.close()
	return entry, c2, e2
end


return klbtar

-- end
