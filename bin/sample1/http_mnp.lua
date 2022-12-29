
local io = require("io")
local khttp = require("khttp")
local kco = require("kco")
local kh26x = require("kh26x")
local khttp_mnp = require("khttp_mnp")
local http_mime = require("klbcore.net.http_mime")


local root_www = './res/html'
local path_h264 = "./res/media/BGM-You KnowI'llGo Get-960x540.h264"


local filter_path = function (path)	
	-- 防止使用相对路径, 非法下载文件
	path = string.gsub(path, '~', '')			-- 将'~' 替换为 ''
	path = string.gsub(path, '\\', '/')			-- 将'\' 替换为 '/'
	path = string.gsub(path, '[.]+/', '/')		-- 将'./','../' 替换为 '/'
	path = string.gsub(path, '[/]+/', '/')		-- 将'//','///' 替换为 '/'

	local root = root_www

	if '/' == path then
		path = '/index.html'			-- 默认主页
	end
	
	-- 提取文件名
	local filename = string.match(path, '[^/]*$')	-- '/index.html'
	if nil == filename or 0 == string.len(filename) then
		return '', ''
	end

	return (root_www .. path), (http_mime(filename))
end

local normal_res_header = function (mime, size)
	local h = {}
	
	table.insert(h, 'HTTP/1.1 200 OK\r\n')
	table.insert(h, string.format('Server: %s\r\n', 'demo'))
	table.insert(h, 'Connection: close\r\n')
	table.insert(h, string.format('Content-Type: %s\r\n', mime))
	table.insert(h, string.format('Content-Length: %d\r\n', size))

	if true then
		table.insert(h, 'Cache-Control: max-age=0\r\n')		-- 缓存时间(秒)
	else
		table.insert(h, 'Cache-Control: max-age=3600\r\n')	-- 缓存时间(秒)
	end
	
	table.insert(h, 'Access-Control-Allow-Origin: *\r\n')	-- 许可跨域请求
	table.insert(h, '\r\n')
	
	local head = table.concat(h)
	return head
end

local html_file = function (conn, path)
	
	local real_path, mime = filter_path(path)
	
	local r = io.open(real_path, "rb")
	if r then
		local body = r:read("*a")
		r:close()
		
		local h = normal_res_header(mime, string.len(body))
		
		conn:send(h)
		conn:send(body)
		
		return true
	end
	
	return false
end

local send_404 = function (conn)
		local body_404 = [[<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
</head>
<body>
<div>
<h1>Not Found</h1>
<h2>404</h2>
</div>
</body>
</html>
]]

	local h = {}
	
	table.insert(h, 'HTTP/1.1 404 OK\r\n')
	table.insert(h, string.format('Server: %s\r\n', 'demo'))
	table.insert(h, 'Connection: close\r\n')
	table.insert(h, string.format('Content-Type: %s\r\n', 'text/html'))
	table.insert(h, string.format('Content-Length: %d\r\n', string.len(body_404)))

	if true then
		table.insert(h, 'Cache-Control: max-age=0\r\n')		-- 缓存时间(秒)
	else
		table.insert(h, 'Cache-Control: max-age=3600\r\n')	-- 缓存时间(秒)
	end
	
	table.insert(h, 'Access-Control-Allow-Origin: *\r\n')	-- 许可跨域请求
	table.insert(h, '\r\n')
	
	local head = table.concat(h)
	
	conn:send(head)
	conn:send(body_404)
end

local on_http_get = function (conn, url)
	if not html_file(conn, url) then
		send_404(conn)
	end	
end


local dispatch_http = function (conn)
	local url = ''
	local method = ''
	
	while true do
		local t, s1, s2 = conn:co_recv()
		--print(t, s1)
		if 'url' == t then
			url = s1
			method = string.upper(s2)
		elseif 'body' == t then
			if 'GET' == method then
				print('http', 'GET', url)
				on_http_get(conn, url)
			end	
		elseif 'disconnect' == t then
			break
		end
	end
end

------------------------------------------------------------
-- http listen
local port = 3456
local listen = khttp.listen(port)

kco.fork(function ()
	while true do
		local conn = listen:co_accept()		
		kco.fork(function ()
			--print('http connect', conn)
			
			dispatch_http(conn)
			
			--print('http disconnect', conn)
			conn:disconnect()
		end)
	end
end)


------------------------------------------------------------
-- http-mnp listen

local normal_res_header2 = function (mime, size)
	local h = {}
	
	table.insert(h, 'HTTP/1.1 200 OK\r\n')
	table.insert(h, string.format('Server: %s\r\n', 'demo'))
	--table.insert(h, 'Connection: close\r\n')
	--table.insert(h, string.format('Content-Type: %s\r\n', mime))
	table.insert(h, 'Connection: keep-alive\r\n')
	table.insert(h, 'Content-Type: video/x-flv\r\n')
	table.insert(h, 'X-Tengine-Type: live\r\n')
	
	--table.insert(h, 'Connection: Upgrade\r\n')
	--table.insert(h, 'Upgrade: http-mnp\r\n')

	--table.insert(h, string.format('Content-Length: %d\r\n', size))

	--if true then
	--	table.insert(h, 'Cache-Control: max-age=0\r\n')		-- 缓存时间(秒)
	--else
	--	table.insert(h, 'Cache-Control: max-age=3600\r\n')	-- 缓存时间(秒)
	--end
	
	table.insert(h, 'Access-Control-Allow-Origin: *\r\n')	-- 许可跨域请求
	table.insert(h, '\r\n')
	
	local head = table.concat(h)
	return head
end

local send_media_http_mnp = function (conn)
	-- head
	local h = normal_res_header2('application/octet-stream', 0)
	conn:send_text(h, '')

	local h264 = kh26x.load(path_h264)
	
	while true do
		local frame1, frame2 = h264:read()
		conn:send_media(frame1)
		conn:send_media(frame2)
		
		kco.co_sleep(25)
	end	
end


local dispatch_http_mnp = function (conn)
	local url = ''
	local method = ''
	
	while true do
		local t, s1, s2 = conn:co_recv()
		--print(t, s1)
		if 'url' == t then
			url = s1
			method = string.upper(s2)
		elseif 'body' == t then
			--if 'GET' == method then
				print('http-mnp', 'GET', url)
				send_media_http_mnp(conn)
			--end
		elseif 'disconnect' == t then
			break
		end
	end
end

local listen_http_mnp = khttp_mnp.listen(port + 1)

kco.fork(function ()
	while true do
		local conn = listen_http_mnp:co_accept()
		kco.fork(function ()
			print('http-mnp connect', conn)
			
			dispatch_http_mnp(conn)
			
			print('http-mnp disconnect', conn)
			conn:disconnect()
		end)
	end
end)


-------------------------------------------
-- test

--[[
local pack_req_get = function (path, host)
	local t = {}	
	
	table.insert(t, string.format('GET %s HTTP/1.1\r\n', path))
	table.insert(t, string.format('Host: %s\r\n', host))
	table.insert(t, string.format('User-Agent: %s\r\n', 'demo'))
	table.insert(t, 'Connection: keep-alive\r\n')
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


kco.timeout(1000, function ()
	local conn = khttp_mnp.connect('127.0.0.1', port + 1)

	

	conn:send_text(pack_req_get('/httpmnp', '127.0.0.1' .. tostring(port + 1)), '')

	while true do
		local t, s1, s2 = conn:co_recv()
		print('test', t)
	end
	
end)
--]]
