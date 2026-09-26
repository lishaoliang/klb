--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   header.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 请求头辅助
-- @note   跨子目录小函数; 非站点公开 API; 非 klbcore.util
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local header = {}


-- @brief 取请求头 (小写键)
-- @param [in]      headers[table]		头表
-- @param [in]      name[string]		头名
-- @return [string]						值; 无为 `""`
header.header_get = function (headers, name)
	return (headers and headers[string.lower(name)]) or ""
end


return header
