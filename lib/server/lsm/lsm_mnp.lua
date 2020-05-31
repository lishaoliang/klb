--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief lsm mnp
-- @author ¿Ó…‹¡º
--]]
local cjson = require("cjson.safe")
local kg_lsmfw = require("kg_lsmfw")

local printex = require("base.printex")


local lsm_mnp = {}


lsm_mnp.on_newconn = function (name, protocol, ok, err)
	--print('lsm_mnp.on_newconn', name, protocol, ok, err)
	return 0
end

lsm_mnp.on_receiver = function (name, protocol, sequence, uid, extra, txt)
	--print('lsm_mnp.on_receiver', name, protocol, sequence, uid, extra, txt)

	local req = {
		name = name,
		protocol = protocol,
		sequence = sequence,
		uid = uid,
		head = cjson.decode(extra),
		body = cjson.decode(txt),
		
		ssid = '123456',
		auth = '123456',
		cmd = ''
	}

	req.cmd = req.body.cmd
	
	--printex(sequence, uid, req)
	
	local res = {}
	if 'login' == req.cmd  then
		res['cmd'] = req.cmd
		res['code'] = 0
		res['login'] = {
			llssid = '123456',
			llauth = '123456'
		}
	elseif 'open_stream' == req.cmd then
		res['cmd'] = req.cmd
		res['code'] = 0
		
		kg_lsmfw.open_stream(name, protocol, req.body.open_stream.chnn, req.body.open_stream.idx)
	end	

	kg_lsmfw.send_txt(name, protocol, sequence, uid, '123456', cjson.encode(res))
	
	return 0
end


lsm_mnp.on_disconnect = function (name, protocol, err)
	--print('lsm_mnp.on_disconnect', name, protocol, err)
	
	return 0
end


return lsm_mnp
