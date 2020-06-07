--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief rtsp commond
-- @author 李绍良
--]]
local table = require("table")
local string = require("string")


local rtsp_cmd = {}


local user_agent = 'KLB (KLB Streaming Media v2020)'
local user_server = 'KLB Sever (v2020)'


------------------------------------------------------------------------------
-- 命令


rtsp_cmd.OPTIONS = 'OPTIONS'
rtsp_cmd.DESCRIBE = 'DESCRIBE'
rtsp_cmd.SETUP = 'SETUP'
rtsp_cmd.TEARDOWN = 'TEARDOWN'
rtsp_cmd.PLAY = 'PLAY'
rtsp_cmd.PAUSE = 'PAUSE'
rtsp_cmd.GET_PARAMETER = 'GET_PARAMETER'


------------------------------------------------------------------------------
-- User-Agent
-- Server


-- @brief 设置'User-Agent'字符串
-- @param [in] str[string]     'User-Agent'字符串
-- @return 无
rtsp_cmd.set_user_agent = function (str)
	user_agent = str
end


-- @brief 设置'Server'字符串
-- @param [in] str[string]     'Server'字符串
-- @return 无
rtsp_cmd.set_server = function (str)
	user_server = str
end


------------------------------------------------------------------------------
-- 打包请求


-- @brief 打包请求: 'OPTIONS'
-- @param [in] cseq[number]	   序列号
-- @param [in] path[string]    请求路径: '\xxx'
-- @return [string] 打包好的请求串
rtsp_cmd.pack_req_options = function (cseq, path)
	local t = {}
	
	table.insert(t, string.format('OPTIONS %s RTSP/1.0\r\n', path))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- @brief 打包请求: 'DESCRIBE'
-- @param [in] cseq[number]	   序列号
-- @param [in] path[string]    请求路径: '\xxx'
-- @return [string] 打包好的请求串
rtsp_cmd.pack_req_describe = function (cseq, path)
	local t = {}
	
	table.insert(t, string.format('DESCRIBE %s RTSP/1.0\r\n', path))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Accept: application/sdp\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- @brief 打包请求: 'SETUP'
-- @param [in] cseq[number]	   序列号
-- @param [in] path[string]    请求路径: '\xxx'
-- @param [in] session[string] 会话字符串
-- @return [string] 打包好的请求串
-- @note  TCP方式
rtsp_cmd.pack_req_setup_tcp = function (cseq, path, session)
	local t = {}
	
	table.insert(t, string.format('SETUP %s RTSP/1.0\r\n', path))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n')

	if nil ~= session and 0 < string.len(session) then
		table.insert(t, string.format('Session: %s\r\n', session))
	end
	
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- @brief 打包请求: 'SETUP'
-- @param [in] cseq[number]	   序列号
-- @param [in] path[string]    请求路径: '\xxx'
-- @param [in] session[string] 会话字符串
-- @return [string] 打包好的请求串
-- @note  UDP方式
rtsp_cmd.pack_req_setup_udp = function (cseq, path, session)
	local t = {}
	
	table.insert(t, string.format('SETUP %s RTSP/1.0\r\n', path))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Transport: RTP/AVP;unicast;interleaved=0-1\r\n')

	if nil ~= session and 0 < string.len(session) then
		table.insert(t, string.format('Session: %s\r\n', session))
	end
	
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- @brief 打包请求: 'PLAY'
-- @param [in] cseq[number]	   序列号
-- @param [in] path[string]    请求路径: '\xxx'
-- @param [in] session[string] 会话字符串
-- @return [string] 打包好的请求串
rtsp_cmd.pack_req_play = function (cseq, path, session)
	local t = {}
	
	table.insert(t, string.format('PLAY %s RTSP/1.0\r\n', path))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))

	if nil ~= session and 0 < string.len(session) then
		table.insert(t, string.format('Session: %s\r\n', session))
	end
	
	table.insert(t, 'Range: npt=0.000-\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


------------------------------------------------------------------------------
-- 打包回复


-- @brief 打包回复: 'OPTIONS'
-- @param [in] cseq[number]	   序列号
-- @return [string] 打包好的回复串
rtsp_cmd.pack_res_options = function (cseq)
	local t = {}
	table.insert(t, 'RTSP/1.0 200 OK\r\n')
	table.insert(t, string.format('Cseq: %d\r\n', cseq))
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, 'Public: OPTIONS,DESCRIBE,SETUP,TEARDOWN,PLAY,PAUSE,GET_PARAMETER\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- @brief 打包回复: 'DESCRIBE'
-- @param [in] cseq[number]	     序列号
-- @param [in] base_path[string] 基础路径: '\xxx'
-- @param [in] sdp[string]       SDP字符串
-- @return [string] 打包好的回复串
rtsp_cmd.pack_res_describe = function (cseq, base_path, sdp)
	local t = {}
	table.insert(t, 'RTSP/1.0 200 OK\r\n')
	table.insert(t, string.format('Cseq: %d\r\n', cseq))
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, 'Content-Type: application/sdp\r\n')
	table.insert(t, string.format('Content-Base: %s\r\n', base_path))
	table.insert(t, 'Cache-Control: no-cache\r\n')
	table.insert(t, string.format('Content-Length: %d\r\n', string.len(sdp)))
	table.insert(t, '\r\n')

	-- sdp字符串
	--table.insert(t, sdp)
	
	return table.concat(t)
end


-- @brief 打包回复: 'SETUP'
-- @param [in] cseq[number]	   序列号
-- @param [in] session[string] 会话串
-- @return [string] 打包好的回复串
-- @note  TCP方式
rtsp_cmd.pack_res_setup_tcp = function (cseq, session)
	local t = {}
	table.insert(t, 'RTSP/1.0 200 OK\r\n')
	table.insert(t, string.format('Cseq: %d\r\n', cseq))
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, 'Transport: RTP/AVP/TCP;interleaved=0-1;unicast\r\n')
	table.insert(t, string.format('Session: %s;timeout=60\r\n', session))
	table.insert(t, 'Cache-Control: no-cache\r\n')	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- @brief 打包回复: 'SETUP'
-- @param [in] cseq[number]	   序列号
-- @param [in] session[string] 会话串
-- @return [string] 打包好的回复串
-- @note  UDP方式
rtsp_cmd.pack_res_setup_udp = function (cseq, session)
	local t = {}
	table.insert(t, 'RTSP/1.0 200 OK\r\n')
	table.insert(t, string.format('Cseq: %d\r\n', cseq))
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, 'Transport: RTP/AVP;interleaved=0-1;unicast\r\n')
	table.insert(t, string.format('Session: %s;timeout=60\r\n', session))
	table.insert(t, 'Cache-Control: no-cache\r\n')	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- @brief 打包回复: 'PLAY'
-- @param [in] cseq[number]	   序列号
-- @param [in] session[string] 会话串
-- @return [string] 打包好的回复串
rtsp_cmd.pack_res_play = function (cseq, session)	
	local t = {}
	table.insert(t, 'RTSP/1.0 200 OK\r\n')
	table.insert(t, string.format('Cseq: %d\r\n', cseq))	
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, 'Range: npt=0.000-\r\n')
	table.insert(t, string.format('Session: %s;timeout=60\r\n', session))
	table.insert(t, 'Cache-Control: no-cache\r\n')
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- @brief 打包回复: 'TEARDOWN'
-- @param [in] cseq[number]	   序列号
-- @param [in] session[string] 会话串
-- @return [string] 打包好的回复串
rtsp_cmd.pack_res_teardown = function (cseq, session)
	local t = {}
	table.insert(t, 'RTSP/1.0 200 OK\r\n')
	table.insert(t, string.format('Cseq: %d\r\n', cseq))	
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, string.format('Session: %s\r\n', session))
	table.insert(t, 'Cache-Control: no-cache\r\n')
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- @brief 打包回复: 'PAUSE'
-- @param [in] cseq[number]	   序列号
-- @param [in] session[string] 会话串
-- @return [string] 打包好的回复串
rtsp_cmd.pack_res_pause = function (cseq, session)
	local t = {}
	table.insert(t, 'RTSP/1.0 200 OK\r\n')
	table.insert(t, string.format('Cseq: %d\r\n', cseq))	
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, string.format('Session: %s\r\n', session))
	table.insert(t, 'Cache-Control: no-cache\r\n')	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- @brief 打包回复: 'GET_PARAMETER'
-- @param [in] cseq[number]	   序列号
-- @param [in] session[string] 会话串
-- @return [string] 打包好的回复串
rtsp_cmd.pack_res_get_parameter = function (cseq, session)
	local t = {}
	table.insert(t, 'RTSP/1.0 200 OK\r\n')
	table.insert(t, string.format('Cseq: %d\r\n', cseq))
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, string.format('Session: %s\r\n', session))
	table.insert(t, 'Cache-Control: no-cache\r\n')	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


return rtsp_cmd
