
local string = require("string")
local trim = require("base.stringex").trim

local rtsp_serve = {}



local parse_next = function (req, line)
	-- eg. 'CSeq: 2'
	-- eg. 'User-Agent: LibVLC/3.0.2 (LIVE555 Streaming Media v2016.11.28)'
	local k, v = string.match(line, '([^:]+):(.*)') -- 匹配以第一个冒号分割格式
	if nil ~= k and nil ~= v then
		k = string.lower(trim(k)) -- 注意所有key被转换为小写
		if 'cseq' == k then
			req[k] = tonumber(v)
		else
			req[k] = trim(v)
		end
	end	
end

local parse_first = function (req, line)
	-- eg. 'SETUP rtsp://192.168.1.2:8000/111 RTSP/1.0'
	local method, url, protocol = string.match(line, '([^ ]+) +([^ ]+) +([^ ]+)') -- 匹配第一行请求
	if nil ~= method and nil ~= url then
		req['method'] = trim(method)
		req['url'] = trim(url)
	end
end


-- @brief 服务端解析请求
rtsp_serve.parse = function (head, body)
	--print('rtsp_serve.parse:', head)	
	local req = {
		method = '',
		url = ''
	}
	
	local first = true
	for line in string.gmatch(head, '[^\r\n]+[\r\n]*') do
		--print('line:', line)
		if first then
			parse_first(req, line)
		else
			parse_next(req, line)
		end
		first = false
	end

	return req
end


return rtsp_serve
