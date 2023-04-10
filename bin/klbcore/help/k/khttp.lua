--[[
-- Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   khttp.lua
-- @brief  C khttp
--   \n require("khttp")
--   \n C导出文件: ./klb/src_c/klua/klua_net/klua_khttp.c
-- @version 0.1
--]]

local khttp = {}


-- @brief 新HTTP连接
local new_http_connect = function (...)
	local conn = {}
	
	-- @brief 关闭连接
	-- @return 无
	conn:disconnect = function ()
		return
	end

	-- @brief 发送数据
	-- @param [in]	s[string]			http数据
	-- @return 	code[number(int)]		0.成功; 非0.错误码
	conn:send = function (s)
		return 0
	end
	
	-- @brief 从连接中接收数据
	-- @return 	msg[string]		消息码: "url", "header", "header_complete", "body", "disconnect"
	--			s1[string]		字符串1
	--			s2[string]		字符串2	
	-- @note 仅在协程中使用	
	conn:co_recv = function ()
		local msg = 'body'	-- "url", "header", "header_complete", "body", "disconnect"
		local s1 = ''
		local s2 = ''
			
		return msg, s1, s2
	end	
	
	return conn
end


-- @brief 连接 host:port
-- @param [in]	host[string]			主机域名或IP; eg. 'www.baidu.com'
-- @param [in]	port[number(int)]		http端口
-- @return 连接
khttp.connect = function (host, port)
	return new_http_connect(...)
end


-- @brief 一个HTTP监听模块(newmetatable)
-- @param [in]	port[number(int)]		http端口
-- @return 监听模块
-- @note 
khttp.listen = function (port)
	local listen = {}

	-- @brief 关闭
	-- @return 无
	listen:close = function ()
		return
	end

	-- @brief 获取监听到的连接
	-- @return 连接
	-- @note 仅在协程中使用	
	listen:co_accept = function ()
		return new_http_connect()
	end

	return listen
end


return khttp
