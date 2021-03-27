local table = require("table")
local khttp = require("khttp")

local http = {}

local user_agent = 'KLB (KLB Streaming Media v2020)'

local cmp_ignore_case = function (s1, s2)
	return (string.lower(s1) == string.lower(s2))
end

-- @brief HTTP.GET 请求
-- @param [in] url[string]			[必须]需要请求的URL
-- @param [in] data[string/table]	[可选]规定连同请求发送到服务器的数据
-- @param [in] cb[function]			[可选]当请求成功时运行的函数
-- @return 无
-- @note 参考 jQuery.get() https://www.runoob.com/jquery/ajax-get.html
--	cb = function (data, status)
--		-- data[string]		包含来自请求的结果数据
--		-- status[string]	包含请求的状态("success","notmodified","error","timeout","parsererror")
--		return
--	end
http.get = function (url, data, cb)
	-- eg. http://username:password@127.0.0.1:8080/test/test.aspx?name=sviergn&x=true#stuff
	local u = khttp.parse_url(url)
	
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
	
	local conn = khttp.connect(host, tonumber(port), tls)
	
	if nil == conn then
		cb('', 'error')
		return
	end
	
	conn:set_on_recv(function (msg, s1, s2)
		if 'http' == msg then
			print(msg, s1, s2)
		elseif 'header' == msg then
			print(msg, s1, s2)
		elseif 'header_complete' == msg then
			print(msg, s1, s2)
		elseif 'body' == msg then
			cb(s2, 'success')
		else
			--print(msg, s1, s2)
		end
	end)
	
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

	local pack_get_header = function ()
		local t = {}
		
		table.insert(t, string.format('GET %s HTTP/1.1\r\n', get_request_str()))
		table.insert(t, string.format('Host: %s:%s\r\n', host, port))
		table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
		table.insert(t, 'Connection: close\r\n')	-- close,keep-alive
		table.insert(t, 'Content-Length: 0\r\n')
		table.insert(t, '\r\n')
		
		return table.concat(t)
	end
	
	conn:send(pack_get_header())
end


-- 参考 jQuery.post() https://www.runoob.com/jquery/ajax-post.html
http.post = function (url, data, cb)
	
end

return http
