--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  klbmp4 内部小工具 (失败返回 / 路径校验 / 存在性 / 目录创建)
-- @stability internal
-- @note   无业务状态; read/ write/ 共用; 非对外 require 面
-- @history 修改历史
--  \n 2026 创建文件
--  \n 2026 增加 ensure_dir / ensure_parent (路径策略 P1-11)
--]]


local lfs = require("lfs")


local util = {}


-- @brief 统一失败返回
-- @param [in]  code[number(int)]		kmp4 返回码
-- @param [in]  errstr[string]			描述
-- @return nil
-- @return code[number(int)]
-- @return errstr[string]
util.fail = function (code, errstr)
	return nil, code, errstr
end


-- @brief 路径参数校验
-- @param [in]  path[string]			文件路径
-- @return [boolean]					true 合法
util.check_path = function (path)
	return 'string' == type(path) and '' ~= path
end


-- @brief 目标是否存在
-- @param [in]  path[string]			文件路径
-- @return [boolean]					true 存在
util.file_exists = function (path)
	local f = io.open(path, 'rb')

	if nil == f then
		return false
	end

	f:close()
	return true
end


-- @brief 递归创建目录
-- @param [in]  dir[string]			目录路径
-- @return [boolean]					true 已存在或创建成功
-- @note 支持 '/' 与 '\\' 分隔; 空串/失败回 false
util.ensure_dir = function (dir)
	if '' == (dir or '') then
		return false
	end

	if 'directory' == lfs.attributes(dir, 'mode') then
		return true
	end

	local parent = string.match(dir, '^(.*)[/\\][^/\\]+$')

	if nil ~= parent and '' ~= parent then
		if not util.ensure_dir(parent) then
			return false
		end
	end

	return lfs.mkdir(dir)
end


-- @brief 递归创建目标文件的父目录
-- @param [in]  path[string]			文件路径
-- @return [boolean]					true 父目录可用(当前目录 / 盘符根视为可用)
util.ensure_parent = function (path)
	local dir = string.match(path, '^(.*)[/\\][^/\\]+$')

	if nil == dir or '' == dir then
		return true
	end

	-- Windows 盘符根 (如 D:)
	if string.match(dir, '^%a:$') then
		return true
	end

	return util.ensure_dir(dir)
end


return util

-- end
