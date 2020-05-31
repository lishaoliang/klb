
local string = require("string")
local trim = require("base.stringex").trim

local rtsp_serve = {}



local parse_next = function (req, line)
	-- eg. 'CSeq: 2'
	-- eg. 'User-Agent: LibVLC/3.0.2 (LIVE555 Streaming Media v2016.11.28)'
	local k, v = string.match(line, '([^:]+):(.*)') -- ƥ���Ե�һ��ð�ŷָ��ʽ
	if nil ~= k and nil ~= v then
		k = string.lower(trim(k)) -- ע������key��ת��ΪСд
		if 'cseq' == k then
			req[k] = tonumber(v)
		else
			req[k] = trim(v)
		end
	end	
end

local parse_first = function (req, line)
	-- eg. 'SETUP rtsp://192.168.1.2:8000/111 RTSP/1.0'
	local method, url, protocol = string.match(line, '([^ ]+) +([^ ]+) +([^ ]+)') -- ƥ���һ������
	if nil ~= method and nil ~= url then
		req['method'] = trim(method)
		req['url'] = trim(url)
	end
end


-- @brief ����˽�������
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
