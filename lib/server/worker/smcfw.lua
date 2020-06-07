--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief stream media client framework
--  \n 实时流媒体框架: 客户端
-- @author 李绍良
--]]
local kg_rand = require("kg_rand")
local kg_smcfw = require("kg_smcfw")
local cjson = require("cjson.safe")

local rtsp_cmd = require("rtsp.rtsp_cmd")
local http_cmd = require("http.http_cmd")

local printex = require("base.printex")


local to_json = function (o)
	local t = type(o)
	
	if 'string' == t then
		return o
	elseif 'table' == t then
		local txt = cjson.encode(o)
		return txt
	else
		assert(false)
	end
	
	return '{}'
end

local req_system = {
	cmd = 'hello_0,hello_1,support_1,support_2,encrypt,login,system,ipv4'
}

local req_stream = {
	cmd = 'open_stream',
	llssid = '123456',
	llauth = '123456',
	open_stream = {
		chnn = 0,
		idx = 1
	}
}


local req_login = {
	cmd = 'login',
	llssid = '123456',
	llauth = '123456',
	login = {
		username = 'admin',
		passwd = '123456'
	}
}

local on_newconn = function (name, protocol, ok, err)
	--print('smcfw on_newconn', name, protocol, ok, err)

	if 'MNP-4-1' == name then
		kg_smcfw.send_txt(name, protocol, 101, 1001, '', to_json(req_stream))
		kg_smcfw.bind(name, 4, 1)
	elseif 'MNP-4-2' == name then
		kg_smcfw.send_txt(name, protocol, 101, 1001, '', to_json(req_stream))
		kg_smcfw.bind(name, 4, 2)
	elseif 'MNP-5-1' == name then
		kg_smcfw.send_txt(name, protocol, 101, 1001, '', to_json(req_stream))
		kg_smcfw.bind(name, 5, 1)
	elseif 'MNP-5-2' == name then
		kg_smcfw.send_txt(name, protocol, 101, 1001, '', to_json(req_stream))
		kg_smcfw.bind(name, 5, 2)
	elseif 'MNP-6-1' == name then
		kg_smcfw.send_txt(name, protocol, 101, 1001, '', to_json(req_stream))
		kg_smcfw.bind(name, 6, 1)
	elseif 'MNP-6-2' == name then
		kg_smcfw.send_txt(name, protocol, 101, 1001, '', to_json(req_stream))
		kg_smcfw.bind(name, 6, 2)
	elseif 'MNP-7-1' == name then
		kg_smcfw.send_txt(name, protocol, 101, 1001, '', to_json(req_stream))
		kg_smcfw.bind(name, 7, 1)
	elseif 'MNP-7-2' == name then
		kg_smcfw.send_txt(name, protocol, 101, 1001, '', to_json(req_stream))
		kg_smcfw.bind(name, 7, 2)
	elseif 'HTTPS' == name then
		--kg_smcfw.send_txt(name, protocol, 102, 1002, http_cmd.pack_req_get('/index.html', '127.0.0.1'), '')
	end

	
	return 0
end

local first = true

-- @param [in] name[string]	    连接名称
-- @param [in] protocol[string] 协议名称
-- @param [in] sequence[number] 序列号: [0,2^32)
-- @param [in] uid[number]      UID: [0,2^32)
-- @param [in] extra[string]    附加数据(或头数据)
-- @param [in] txt[string]      正式文本数据
-- @return 0
local on_receiver = function (name, protocol, sequence, uid, extra, txt)
	--print('smcfw on_receiver', name, protocol, sequence, uid)
	--print(extra)
	--print(txt)

	if first then
		
		--if 'MNP' == name then
		--	kg_smcfw.send_txt(name, protocol, 200, 2000, kg_rand.str(1024 * 1024), to_json(req_stream))
		--end
		
		first = false
	end
	
	
	return 0
end


-- @param [in] name[string]	    连接名称
-- @param [in] protocol[string] 协议名称
-- @param [in] err[string]      错误字符串
-- @return 0
local on_disconnect = function (name, protocol, err)
	print('smcfw on_disconnect', name, protocol, err)
	
	return 0
end


kin = function ()
	print('kin->smcfw.')
	kg_smcfw.set_newconn(on_newconn)
	kg_smcfw.set_receiver(on_receiver)
	kg_smcfw.set_disconnect(on_disconnect)

	--kg_smcfw.dial('MNP-4-1', 'MNP', 'tcp', '127.0.0.1:8000')
	--kg_smcfw.dial('MNP-4-2', 'MNP', 'tcp', '127.0.0.1:8000')
	
	--kg_smcfw.dial('MNP-5-1', 'MNP', 'tcp', '127.0.0.1:8000')
	--kg_smcfw.dial('MNP-5-2', 'MNP', 'tcp', '127.0.0.1:8000')

	--kg_smcfw.dial('MNP-6-1', 'MNP', 'tcp', '127.0.0.1:8000')
	--kg_smcfw.dial('MNP-6-2', 'MNP', 'tcp', '127.0.0.1:8000')
	
	--kg_smcfw.dial('MNP-7-1', 'MNP', 'tcp', '127.0.0.1:8000')
	--kg_smcfw.dial('MNP-7-2', 'MNP', 'tcp', '127.0.0.1:8000')

	--kg_smcfw.dial('HTTPS', 'HTTPS', 'tcp', '127.0.0.1:8001')
	
	return 0
end

kexit = function ()
	
	return 0
end

kgo = function (msg, msgex, lparam, wparam)
	print(msg, msgex, lparam, wparam)
	
	return 0
end

