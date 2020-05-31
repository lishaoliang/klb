--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief lsm http
-- @author ������

����:
GET / HTTP/1.1
Host: 127.0.0.1:8000
Connection: Upgrade
Pragma: no-cache
Cache-Control: no-cache
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.163 Safari/537.36
Upgrade: websocket
Origin: http://127.0.0.1:8000
Sec-WebSocket-Version: 13
Accept-Encoding: gzip, deflate, br
Accept-Language: zh-CN,zh;q=0.9
Cookie: lang=zh_cn
Sec-WebSocket-Key: LDIFqu7eWcRiGMwOMxAvOg==
Sec-WebSocket-Extensions: permessage-deflate; client_max_window_bits
Sec-WebSocket-Protocol: binary

�ظ�:
HTTP/1.1 400 Bad Request
Connection: close
X-WebSocket-Reject-Reason: Client must provide a Host header.

HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: XdvX1cArD2MsC4Pf4+PvlV8DXeA=
Sec-WebSocket-Protocol: binary
--]]
local cjson = require("cjson.safe")
local kg_crypto = require("kg_crypto")
local kg_lsmfw = require("kg_lsmfw")


local printex = require("base.printex")
local http_serve = require("http.http_serve")


local lsm_ws_mnp = {}

local trim = function (s)
	return (string.gsub(s, '^%s*(.-)%s*$', '%1'))
end


local pack_101 = function (key, protocol)
	local t = {}
	
	table.insert(t, 'HTTP/1.1 101 Switching Protocols\r\n')
	table.insert(t, string.format('Server: %s\r\n', 'KLB'))
	
	table.insert(t, 'Upgrade: websocket\r\n')
	table.insert(t, 'Connection: Upgrade\r\n')
	table.insert(t, 'Sec-WebSocket-Version: 13\r\n')
	--table.insert(t, 'Sec-WebSocket-Accept: xY9zZMQTFsuY8fI6UBpaOmQ4x0Y=\r\n')
	table.insert(t, string.format('Sec-WebSocket-Accept: %s\r\n', key))
	
	-- ��ӿͻ���д��Э��, ��ֹ������
	if 0 < string.len(protocol) then
		--table.insert(t, 'Sec-WebSocket-Protocol: binary\r\n')
		table.insert(t, string.format('Sec-WebSocket-Accept: %s\r\n', protocol))
	end
	
	table.insert(t, string.format('Content-Length: %d\r\n', 0))			-- ���ݳ���
	table.insert(t, 'Access-Control-Allow-Origin: *\r\n')				-- ��ɿ�������
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


lsm_ws_mnp.on_newconn = function (name, protocol, ok, err)
	print('lsm_ws_mnp.on_newconn', name, protocol, ok, err)
	return 0
end

lsm_ws_mnp.on_receiver = function (name, protocol, sequence, uid, head, body)
	--print('lsm_ws_mnp.on_receiver', name, protocol, sequence, uid, head, body)

	local req = http_serve.parser(head)
	--printex(name, protocol, req)
	print(protocol, name, req.method, req.url, body)

	if '' ~= req.method then
		-- http�׶�
		local ws_key = http_serve.header_value(req.header, 'Sec-WebSocket-Key')
		
		local key = kg_crypto.ws_encode(ws_key)
		--print('ws keys:', ws_key, key)
		
		local res_head = pack_101(key, http_serve.header_value(req.header, 'Sec-WebSocket-Protocol'))
		--print('lsm_ws_mnp.send_txt', res_head)
		
		kg_lsmfw.send_txt(name, protocol, sequence, uid, res_head, '')
		kg_lsmfw.open_stream(name, protocol, 0, 1)
		print('open_stream', name)
	else
		-- websocket �׶�
		kg_lsmfw.send_txt(name, protocol, sequence, uid, '', body)
	end
		
	return 0
end


lsm_ws_mnp.on_disconnect = function (name, protocol, err)
	print('lsm_ws_mnp.on_disconnect', name, protocol, err)
	
	return 0
end


return lsm_ws_mnp
