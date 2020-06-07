local string = require("string")
local stringex = require("base.stringex")
local printex = require("base.printex")

local http_serve ={}

local parser_kv = function (req, k, v)
	req.header[k] = v
end

http_serve.parser = function (head)	
	local req = {
		method = '',
		url = '',
		
		header = {}
	}
	
	if 'string' ~= type(head) then
		return req
	end
	
	-- 提取请求头信息
	local first = true
	for line in string.gmatch(head, '[^\r\n]+[\r\n]*') do
		--print(line)
		if first then
			local method, url, ver = string.match(line, '([^ ]+) +([^ ]+) +([^ ]+)') -- 匹配请求头
			--print(method, url, ver)
			if nil ~= method and nil ~= url and nil ~= ver then
				req.method = string.upper(stringex.trim(method))
				req.url = stringex.trim(url)
			end
			
			first = false
		else
			local k, v = string.match(line, '([^:]+):(.*)') -- 匹配参数
			if nil ~= k and nil ~= v then
				parser_kv(req, stringex.trim(k), stringex.trim(v))
			end
		end
	end

	--printex('http_serve.parser', req)
	
	return req
end

http_serve.header_value = function (header, key)
	if 'table' ~= type(header) then
		return ''
	end
	
	local low_key = string.lower(key)
	for k, v in pairs(header) do
		--print('http_serve.header_value', k, v, key)	
		if string.lower(k) == low_key  and 'string' == type(v) then
			return v
		end
	end
	
	return ''
end


http_serve.is_upgrade = function (header)
	if 'table' ~= type(header) then
		return false, ''
	end

	-- 检查头中的上面两项之一
	-- Connection: Upgrade
	-- Upgrade: websocket
	local bup = false
	local protocol = ''
	
	local conn = http_serve.header_value(header, 'Connection')
	if stringex.find_ignore_case(conn, 'Upgrade') then
		bup = true
	end
	
	local protocol = http_serve.header_value(header, 'Upgrade')
	if 0 < string.len(protocol) then
		bup = true
	end
	
	return bup, protocol
end


http_serve.pack_404 = function ()
	
	local body = [[<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>404 NOT FOUND</title>
</head>
<body>
<div>
    <div style="text-align: center;">
        <h1>404 NOT FOUND!</h1>
    </div>
</div>
</body>
</html>]]

	local t = {}

	table.insert(t, 'HTTP/1.1 404 Not Found\r\n')
	table.insert(t, string.format('Server: %s\r\n', 'KLB'))
	table.insert(t, 'Connection: close\r\n')									-- 短连接关闭
	table.insert(t, string.format('Content-Type: %s\r\n', 'text/html'))			-- 数据类型
	table.insert(t, string.format('Content-Length: %d\r\n', string.len(body)))	-- 数据长度
	--table.insert(t, 'Cache-Control: max-age=0\r\n')							-- 缓存时间(秒)
	table.insert(t, 'Access-Control-Allow-Origin: *\r\n')						-- 许可跨域请求
	table.insert(t, '\r\n')
	
	return table.concat(t), body
end


return http_serve
