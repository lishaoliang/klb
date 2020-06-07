--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief lsm http flv
-- @author 李绍良
--]]
local cjson = require("cjson.safe")
local kg_lsmfw = require("kg_lsmfw")

local pname = require("base.pname")
local stringex = require("base.stringex")
local http_mime = require("http.http_mime")
local http_serve = require("http.http_serve")
local http_cmd = require("http.http_cmd")
local printex = require("base.printex")


local lsm_http_flv = {}


local pack_200 = function ()
	local t = {}
	
	table.insert(t, 'HTTP/1.1 200 OK\r\n')
	table.insert(t, string.format('Server: %s\r\n', 'KLB'))
	table.insert(t, 'Connection: close\r\n')				-- 短连接关闭
	table.insert(t, 'Content-Type: video/x-flv\r\n')		-- 数据类型
	table.insert(t, 'Access-Control-Allow-Origin: *\r\n')	-- 许可跨域请求
	table.insert(t, '\r\n')
	
	return table.concat(t), ''
end


lsm_http_flv.on_newconn = function (name, protocol, ok, err)
	--print('lsm_http_flv.on_newconn', name, protocol, ok, err)
	return 0
end

lsm_http_flv.on_receiver = function (name, protocol, sequence, uid, head, body)
	--print('lsm_http_flv.on_receiver', name, protocol, sequence, uid, head, body)

	local req = http_serve.parser(head)
	--printex(name, protocol, req)
	print(protocol, name, req.method, req.url)

	-- HTTP
	local bres = false

	if http_cmd.GET == req.method then
		local h, b = pack_200()
		bres = kg_lsmfw.send_txt(name, protocol, sequence, uid, h, b)
		
		kg_lsmfw.open_stream(name, protocol, 0, 1)
		print('open_stream', name)
	end
	
	if not bres then
		local h, b = http_serve.pack_404()
		kg_lsmfw.send_txt(name, protocol, sequence, uid, h, b)
	end
	
	return 0
end


lsm_http_flv.on_disconnect = function (name, protocol, err)
	print('lsm_http_flv.on_disconnect', name, protocol, err)
	
	return 0
end


return lsm_http_flv
