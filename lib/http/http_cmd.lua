

local http_cmd = {}


local user_agent = 'KLB (KLB Streaming Media v2020)'
local user_server = 'KLB Sever (v2020)'


------------------------------------------------------------------------------
-- √¸¡Ó


http_cmd.GET = 'GET'
http_cmd.POST = 'POST'


------------------------------------------------------------------------------


--	GET /xxxxxx.flv HTTP/1.1
--	Host: xxx.xxx.com
--	User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:71.0) Gecko/20100101 Firefox/71.0
--	Accept: */*
--	Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2
--	Accept-Encoding: gzip, deflate
--	Referer: http://127.0.0.1:3456/page/testflv.html
--	Origin: http://127.0.0.1:3456
--	Connection: keep-alive

http_cmd.pack_req_get = function (path, host)
	local t = {}	
	
	table.insert(t, string.format('GET %s HTTP/1.1\r\n', path))
	table.insert(t, string.format('Host: %s\r\n', host))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Connection: keep-alive\r\n')
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end

return http_cmd
