--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief lsm http
-- @author 李绍良
--]]
local cjson = require("cjson.safe")
local kg_lsmfw = require("kg_lsmfw")

local conf = require("conf")
local pname = require("base.pname")
local stringex = require("base.stringex")
local http_mime = require("http.http_mime")
local http_serve = require("http.http_serve")
local http_cmd = require("http.http_cmd")
local printex = require("base.printex")


local lsm_http = {}

local root_html = conf.path_html -- '/usr/local/lib/klb1.0/html' './html'

local get_filter_path = function (url)
	local path = string.match(url, '^[^?]*')	-- '/index.html?a=1&b=2&c=3'	
	if nil == path then
		return '', ''
	end
	
	-- 防止使用相对路径, 非法下载文件
	path = string.gsub(path, '~', '')			-- 将'~' 替换为 ''
	path = string.gsub(path, '\\', '/')			-- 将'\' 替换为 '/'
	path = string.gsub(path, '[.]+/', '/')		-- 将'./','../' 替换为 '/'
	path = string.gsub(path, '[/]+/', '/')		-- 将'//','///' 替换为 '/'
	
	-- 提取文件名
	local filename = string.match(path, '[^/]*$')	-- '/index.html'
	if nil == filename or 0 == string.len(filename) then
		return '', ''
	end
	
	return (root_html .. path), (http_mime(filename)), path
end


lsm_http.on_newconn = function (name, protocol, ok, err)
	--print('lsm_http.on_newconn', name, protocol, ok, err)
	return 0
end

lsm_http.on_receiver = function (name, protocol, sequence, uid, head, body)
	--print('lsm_http.on_receiver', name, protocol, sequence, uid, head, body)

	local req = http_serve.parser(head)
	--printex(name, protocol, req)
	--print(protocol, name, req.method, req.url)

	local connnection = http_serve.header_value(req.header, 'Connection')
	local upgrade = http_serve.header_value(req.header, 'Upgrade')

	-- 更换协议
	local bup, up_protocol = http_serve.is_upgrade(req.header)	
	if bup then
		if stringex.find_ignore_case(up_protocol, 'websocket') then
			if stringex.find_ignore_case(req.url, '/wsmnp') then
				kg_lsmfw.switch_protocol(name, pname.WSMNP, head, body)
			else
				kg_lsmfw.switch_protocol(name, pname.WS, head, body)
			end
		else
			kg_lsmfw.close_conn(name, protocol) -- 不支持的, 直接关闭
		end
		
		return 0
	end

	-- HTTP
	local bres = false

	-- '/api/authorize'

	if http_cmd.GET == req.method then
		local url = req.url
		if '' == url or '/' == url then
			url = '/index.html'
		end
		
		local path, mime, url_path = get_filter_path(url)
		print(protocol, name, req.method, mime, url_path, path)
		
		bres = kg_lsmfw.send_file(name, protocol, sequence, uid, mime, '', path)
		
	end
	
	if not bres then
		local h, b = http_serve.pack_404()
		kg_lsmfw.send_txt(name, protocol, sequence, uid, h, b)
	end
	
	--kg_lsmfw.close_conn(name, protocol)
	
	return 0
end


lsm_http.on_disconnect = function (name, protocol, err)
	--print('lsm_http.on_disconnect', name, protocol, err)
	
	return 0
end


return lsm_http
