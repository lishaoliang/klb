--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   chunked.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb Transfer-Encoding chunked 组包
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local chunked = {}


-- @brief 一节 chunk
-- @param [in]      data[string]		[可选] 载荷; 无为 `""`
-- @return [string]						`size\r\n data \r\n`
chunked.wrap = function (data)
	data = data or ""
	return string.format("%X\r\n%s\r\n", #data, data)
end


-- @brief 结束块
-- @return [string]						`0\r\n\r\n`
chunked.tail = function ()
	return "0\r\n\r\n"
end


return chunked
