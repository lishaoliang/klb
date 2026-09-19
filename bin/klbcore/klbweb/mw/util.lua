--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb mw 共用小函数
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local util = {}


-- @brief 取请求头 (小写键)
-- @param [in]      headers[table]		头表
-- @param [in]      name[string]		头名
-- @return [string]						值; 无为 `""`
util.header_get = function (headers, name)
	return (headers and headers[string.lower(name)]) or ""
end


return util
