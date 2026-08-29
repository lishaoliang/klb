--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   pathex.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  路径相关的帮助函数
-- @history 修改历史
--  \n [2026] 创建文件
--]]
local stringex = require("klbcore.util.stringex")

local pathex = {}


-- @brief 文件扩展名
-- @param [in]	path[string]		[必须]路径; eg. '/aaa/bbb/ccc.bmp'
-- @return [string]		eg. 'bmp'
pathex.ext = function (path)
	local _, ext = string.match(path, '(%.)([^.]*)$')

	if ext then
		return ext
	end

	return ''
end


-- @brief 去除路径前后空白, 以及末尾的'/','\'
-- @param [in]	path[string]		[必须]路径; eg. '/aaa/bbb/'
-- @return [string]		eg. '/aaa/bbb'
pathex.trim = function (path)
	local s = stringex.trim(path)

	s = (string.gsub(s, '[\\/]+$', ''))

	return s
end


return pathex
