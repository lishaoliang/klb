--[[
-- Copyright (c) 2022, GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   httpc.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  khttp client 封装
--]]
local table = require("table")
local kurl = require("kurl")
local khttp = require("khttp")

local httpc = {}

local user_agent = 'KLB(v2022)'


-- @brief HTTP 参数
httpc.cfg = {
	['timeout'] = 20 * 1000			-- 单位毫秒
}


-----------------------------------

local function cmp_ignore_case (s1, s2)
	return (string.lower(s1) == string.lower(s2))
end

-- @brief HTTP 请求
-- @param [in] method[string]		[必须]方法: 'GET', 'POST'
--    仅协程中使用
local function http_request(method, url, req_head, req_body)	
	-- eg. http://username:password@127.0.0.1:8080/test/test.aspx?name=sviergn&x=true#stuff
	local u = kurl.parse(url)
	
	local schema = u['schema'] or 'http'
	local host = u['host'] or ''	
	local port = '80'
	local tls = false

	if cmp_ignore_case('https', schema) then
		tls = true
	end
	
	if u['port'] then
		port = u['port']
	else
		if tls then
			port = '443'
		end
	end
	
	--print(host, port, tls)
	local conn = khttp.connect(host, tonumber(port), 'tls', tls, 'timeout', httpc.cfg['timeout'])
	
	if nil == conn then
		return '', 'error'
	end
	
	local get_request_str = function ()	
		local path = u['path'] or '/'
		local query = u['query']
		local fragment = u['fragment']
		
		local t = {}
		
		table.insert(t, path)
		
		if query then
			table.insert(t, '?')
			table.insert(t, query)
		end
		
		if fragment then
			table.insert(t, '#')
			table.insert(t, fragment)
		end
		
		return table.concat(t)
	end

	local function pack_get_header()
		local t = {}
		
		table.insert(t, string.format('%s %s HTTP/1.1\r\n', method, get_request_str()))
		table.insert(t, string.format('Host: %s:%s\r\n', host, port))
		table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
		
		if 'table' == type(req_head) then
			for k, v in pairs(req_head) do
				if 'string' == type(k) and 'string' == type(v) then
					table.insert(t, string.format('%s: %s\r\n', k, v))
				end
			end
		end
		
		local req_body_len = ('string' == type(req_body) and string.len(req_body)) or 0
		
		table.insert(t, 'Connection: close\r\n')	-- close,keep-alive
		table.insert(t, string.format('Content-Length: %d\r\n', req_body_len))
		table.insert(t, '\r\n')
		
		return table.concat(t)
	end
	
	local str_header = pack_get_header()
	
	--print(host, port, str_header)
	conn:send(str_header)
	if 'string' == type(req_body) then
		conn:send(req_body)
	end

	local body = ''
	local status = 'error'
	
	while true do
		local msg, s1, s2 = conn:co_recv()
		
		if 'status' == msg then
			--print(msg, s1, s2)
		elseif 'header' == msg then
			--print(msg, s1, s2)
		elseif 'header_complete' == msg then
			--break
			--print(msg, s1, s2)
		elseif 'disconnect' == msg then
			--print(msg, s1, s2)
			break
		elseif 'body' == msg then
			body = s1
			status = 'success'
			break
		else
			--print(msg, s1, s2)
		end
	end
	
	conn:disconnect()
	
	return body, status	
end


-- @brief HTTP.POST 请求
-- @param [in] url[string]				[必须]需要请求的URL
-- @param [in] req_head[nil,table]		[可选]额外请求头; eg. {['Content-Type']='application/json'}
-- @param [in] req_body[nil,string]		[可选]附加数据体; eg. "{}"
-- @return [string]	包含来自请求的结果数据
--         [string]	状态信息: "success","notmodified","error","timeout","parsererror"
-- @note 参考 jQuery.get() https://www.runoob.com/jquery/ajax-post.html
--    仅协程中使用
httpc.post = function (url, req_head, req_body)
	return http_request('POST', url, req_head, req_body)
end


-- @brief HTTP.GET 请求
-- @param [in] url[string]				[必须]需要请求的URL
-- @param [in] req_head[nil,table]		[可选]额外请求头; eg. {['Content-Type']='application/json'}
-- @param [in] req_body[nil,string]		[可选]附加数据体; eg. "{}"
-- @return [string]	包含来自请求的结果数据
--         [string]	状态信息: "success","notmodified","error","timeout","parsererror"
-- @note 参考 jQuery.get() https://www.runoob.com/jquery/ajax-get.html
--    仅协程中使用
httpc.get = function (url, req_head, req_body)
	return http_request('GET', url, req_head, req_body)
end


return httpc
