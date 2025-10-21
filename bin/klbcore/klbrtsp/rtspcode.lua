--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rtspcode.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  RTSP 错误码
-- @note   错误码定义
-- @history 修改历史
--		[2025-10] 添加基础
--]]
local stringex = require("klbcore.util.stringex")

local rtspcode = {}



-- 字符对应码
local rtspcode_map = {
	
	-- 2xx (成功)
	[200] = 'OK',					-- 成功响应
	[201] = 'Created',				-- 表示请求已导致资源创建
	[250] = 'Low on Storage Space',	-- 服务器接受请求, 但存储空间不足
	
	-- 4xx (客户端错误码, 客户端请求存在问题)
	[400] = 'Bad Request',			-- 服务器无法理解请求格式
	[401] = 'Unauthorized',			-- 请求需要身份验证(用户名/密码), 但客户端未提供或提供错误
	[403] = 'Forbidden',			-- 服务器拒绝请求(即使提供认证也不允许访问,通常是权限不足)
	[404] = 'Not Found',			-- 请求的 RTSP 资源 不存在
	[405] = 'Method Not Allowed',	-- 服务器不支持请求中使用的 RTSP 方法
	[406] = 'Not Acceptable',		-- 服务器无法满足客户端Accept头中指定的格式
	[408] = 'Request Timeout',		-- 服务器等待客户端请求超时
	[415] = 'Unsupported Media Type',	-- 客户端请求的媒体类型或编码格式服务器不支持
	[451] = 'Parameter Not Understood',	-- 服务器无法理解请求中的某个参数
	[452] = 'Conference Not Found',	-- 请求的会议 / 会话不存在
	[453] = 'Not Enough Bandwidth',	-- 服务器带宽不足, 无法满足客户端请求的传输速率
	[454] = 'Session Not Found',	-- 客户端请求中携带的Session ID无效
	[455] = 'Method Not Valid in This State',	-- 请求的方法在当前会话状态下不允许
	[456] = 'Header Field Not Valid for Resource',	-- 请求中的头字段对当前资源无效
	[457] = 'Invalid Range',					-- 客户端请求的时间范围无效
	[458] = 'Parameter Is Read-Only',			-- 客户端尝试修改服务器的只读参数
	[459] = 'Aggregate Operation Not Allowed',	-- 服务器不支持批量操作
	[460] = 'Only Aggregate Operation Allowed',	-- 操作必须批量执行
	[461] = 'Unsupported Transport',			-- 服务器不支持客户端Transport字段指定的传输方式(如客户端要求 UDP, 但服务器仅支持 TCP)
	[462] = 'Destination Unreachable',			-- 服务器无法到达客户端指定的传输地址(如SETUP中client_port=5004,但客户端未开放该端口, 服务器发送数据失败)


	-- 5xx (服务器错误码 : 服务器处理请求时出错)
	[500] = 'Internal Server Error',	-- 服务器内部错误
	[501] = 'Not Implemented',			-- 服务器不支持请求的方法(比 405 更基础, 如服务器未实现PAUSE方法)
	[503] = 'Service Unavailable',		-- 服务器暂时无法处理请求
	[551] = 'Option Not Supported',		-- 服务器不支持请求中的扩展选项(如客户端使用 RTSP 2.0 扩展字段，但服务器仅支持 RTSP 1.0)

}



-- @brief 将code码 转换为 字符串
-- @param [in] code[number(int)]	状态码
-- @return [string] 	字符串
rtspcode.code_tostring = function (code)
	local no = tonumber(code)
	
	local s = rtspcode_map[no]
	if s then
		return s
	else
		return 'Not Found'
	end	
end



return rtspcode
