--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief live stream media framework
--  \n 实时流媒体框架: 服务端
-- @author 李绍良
--]]

local kg_lsmfw = require("kg_lsmfw")

local pname = require("base.pname")
local lsm_mnp = require("server.lsm.lsm_mnp")
local lsm_http = require("server.lsm.lsm_http")
local lsm_rtsp = require("server.lsm.lsm_rtsp")
local lsm_ws = require("server.lsm.lsm_ws")
local lsm_ws_mnp = require("server.lsm.lsm_ws_mnp")
local lsm_ws_flv = require("server.lsm.lsm_ws_flv")
local lsm_http_flv = require("server.lsm.lsm_http_flv")



local cb_funcs = {}


-- @brief 注册协议处理函数
-- @param [in] protocol[string]      协议名称
-- @param [in] newconn[function]     新连接接入
-- @param [in] receiver[function]    接收数据
-- @param [in] disconnect[function]  连接断开
-- @return 无
local reg_callback = function (protocol, newconn, receiver, disconnect)	
	local o = cb_funcs[protocol]
	if nil ~= o then
		o['on_newconn'] = newconn
		o['on_receiver'] = receiver
		o['on_disconnect'] = disconnect
	else
		o = {}
		o['on_newconn'] = newconn
		o['on_receiver'] = receiver
		o['on_disconnect'] = disconnect
		
		cb_funcs[protocol] = o
	end	
end



local on_newconn = function (name, protocol, ok, err)
	--print('lsmfw on_newconn', name, protocol, ok, err)
	
	local o = cb_funcs[protocol]
	if nil ~= o then
		local cb = o['on_newconn']
		if nil ~= cb then
			cb(name, protocol, ok, err)
		end
	end
	
	return 0
end


-- @brief 接收数据处理函数
-- @param [in] name[string]	    连接名称
-- @param [in] protocol[string] 协议名称
-- @param [in] sequence[number] 序列号: [0,2^32)
-- @param [in] uid[number]      UID: [0,2^32)
-- @param [in] extra[string]    附加数据(或头数据)
-- @param [in] txt[string]      正式文本数据
-- @return 0
local on_receiver = function (name, protocol, sequence, uid, extra, txt)
	--print('lsmfw on_receiver', name, protocol, sequence, uid, extra, txt)

	local o = cb_funcs[protocol]
	if nil ~= o then
		local cb = o['on_receiver']
		if nil ~= cb then
			cb(name, protocol, sequence, uid, extra, txt)
		end
	end

	return 0
end


-- @param [in] name[string]	    连接名称
-- @param [in] protocol[string] 协议名称
-- @param [in] err[string]      错误字符串
-- @return 0
local on_disconnect = function (name, protocol, err)
	--print('lsmfw on_disconnect', name, protocol, err)

	local o = cb_funcs[protocol]
	if nil ~= o then
		local cb = o['on_disconnect']
		if nil ~= cb then
			cb(name, protocol, err)
		end
	end
	
	return 0
end


kin = function ()
	print('kin->lsmfw.')

	-- 注册HTTP网络模块: 路径对应的子协议
	kg_lsmfw.register_protocol(pname.HTTPFLV, '/httpflv/')
	kg_lsmfw.register_protocol(pname.WSMNP, '/wsmnp/')
	kg_lsmfw.register_protocol(pname.WSFLV, '/wsflv/')

	-- 注册支持的协议处理函数
	reg_callback(pname.MNP, lsm_mnp.on_newconn, lsm_mnp.on_receiver, lsm_mnp.on_disconnect)
	reg_callback(pname.MNPS, lsm_mnp.on_newconn, lsm_mnp.on_receiver, lsm_mnp.on_disconnect)
	reg_callback(pname.HTTP, lsm_http.on_newconn, lsm_http.on_receiver, lsm_http.on_disconnect)
	reg_callback(pname.HTTPS, lsm_http.on_newconn, lsm_http.on_receiver, lsm_http.on_disconnect)
	reg_callback(pname.RTSP, lsm_rtsp.on_newconn, lsm_rtsp.on_receiver, lsm_rtsp.on_disconnect)
	reg_callback(pname.WS, lsm_ws.on_newconn, lsm_ws.on_receiver, lsm_ws.on_disconnect)
	
	reg_callback(pname.HTTPFLV, lsm_http_flv.on_newconn, lsm_http_flv.on_receiver, lsm_http_flv.on_disconnect)
	reg_callback(pname.WSMNP, lsm_ws_mnp.on_newconn, lsm_ws_mnp.on_receiver, lsm_ws_mnp.on_disconnect)
	reg_callback(pname.WSFLV, lsm_ws_flv.on_newconn, lsm_ws_flv.on_receiver, lsm_ws_flv.on_disconnect)
	

	-- 设置根回调函数
	kg_lsmfw.set_newconn(on_newconn)
	kg_lsmfw.set_receiver(on_receiver)
	kg_lsmfw.set_disconnect(on_disconnect)	
	
	return 0
end

kexit = function ()
	
	return 0
end

kgo = function (msg, msgex, lparam, wparam)
	print(msg, msgex, lparam, wparam)
	
	return 0
end

