--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rtspserve_parser.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  RTSP Serve Parser
-- @note   RTSP 服务 解析
-- @history 修改历史
--		[2025-10] 添加基础解析
--]]
local stringex = require("klbcore.util.stringex")
local rtspcode = require("klbcore.klbrtsp.rtspcode")

local rtspserve_parser = {}

local user_server = 'klbrtspserve-v2025.10'

--------------------------------------------------------------------------------------------

-- 转换为字符串
local ToString = function (s)
	local ss = tostring(s)
	ss = stringex.trim(ss)	
	return ss
end

-- code码 转换为字符串
local ToCodeString = function (code)
	return rtspcode.code_tostring(code)
end

-- 获取支持的所有方法
local GetMethodsString = function (method_map)
	local t = {}
	
	local first = true
	for k, _ in pairs(method_map) do
		if first then
			first = false
		else
			table.insert(t, ', ')
		end
		
		table.insert(t, k)
	end
	
	return table.concat(t)
end

--------------------------------------------------------------------------------------------

-- OPTIONS 回复 字符串
rtspserve_parser.pack_res_OPTIONS = function (ver, code, cseq, method_map)	
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	table.insert(t, string.format('Public: %s\r\n', GetMethodsString(method_map)))
	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end

-- DESCRIBE 回复 字符串
rtspserve_parser.pack_res_DESCRIBE = function (ver, code, cseq, sdp_len)
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	table.insert(t, 'Content-Type: application/sdp\r\n')
	
	table.insert(t, string.format('Content-Length: %s\r\n', ToString(sdp_len)))
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end

-- sdp
rtspserve_parser.pack_res_sdp = function ()
	local t = {}
	
	-- 公共头定义
	table.insert(t, 'v=0\r\n')
	table.insert(t, 'o=- 123456789 123456789 IN IP4 127.0.0.1\r\n')
	table.insert(t, 's=rtsp serve\r\n')
	table.insert(t, 'c=IN IP4 0.0.0.0\r\n')
	table.insert(t, 't=0 0\r\n')
	
	-- 视频流
	table.insert(t, 'm=video 0 RTP/AVP 96\r\n')
	table.insert(t, 'a=rtpmap:96 H264/90000\r\n')
	table.insert(t, 'a=fmtp:96 packetization-mode=1;sprop-parameter-sets=Z2QADazZQYHflwEQAAADABAAAAMDIPFCmWA=,aOvjyyLA\r\n')
	table.insert(t, 'a=control:rtsp://127.0.0.1:3456/chnn0/idx0/video\r\n')
	
	return table.concat(t)
end

-- SETUP 回复 字符串
rtspserve_parser.pack_res_tcp_SETUP = function (ver, code, cseq, session, timeout)
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	table.insert(t, 'Transport: RTP/AVP/TCP;interleaved=0-1;unicast\r\n')
	table.insert(t, string.format('Session: %s;timeout=%s\r\n', ToString(session), ToString(timeout)))
	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end

-- PLAY 回复 字符串
rtspserve_parser.pack_res_PLAY = function (ver, code, cseq, session, timeout)
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	table.insert(t, 'Range: npt=0.000-\r\n')
	table.insert(t, string.format('Session: %s;timeout=%s\r\n', ToString(session), ToString(timeout)))
	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- GET_PARAMETER 回复 字符串
rtspserve_parser.pack_res_GET_PARAMETER = function (ver, code, cseq, session, timeout)	
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	table.insert(t, string.format('Session: %s;timeout=%s\r\n', ToString(session), ToString(timeout)))
	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- TEARDOWN 回复 字符串
rtspserve_parser.pack_res_TEARDOWN = function (ver, code, cseq, session)
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	table.insert(t, string.format('Session: %s\r\n', ToString(session)))
	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- 不支持 回复 字符串
rtspserve_parser.pack_res_unsupported = function (ver, code, cseq)
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


--------------------------------------------------------------------------------------------

-- 解析 rtsp 会话数据
rtspserve_parser.parse = function (txt)	
	local method_str, url_str, ver_str = '', '', ''
	local header = {}
	
	-- 切分
	local head, body = string.match(txt, '(.*)\r\n\r\n(.*)')
	
	-- 提取请求头信息
	if nil ~= head then
		local first = true
		for line in string.gmatch(head, '[^\r\n]+[\r\n]*') do
			if first then
				-- eg. 'OPTIONS rtsp://192.168.1.100:554/stream1 RTSP/1.0'
				local method, url, ver = string.match(line, '([^ ]+) +([^ ]+) +([^ ]+)') -- 匹配请求头
				if nil ~= method and nil ~= url and nil ~= ver then
					method_str = stringex.trim(method)
					url_str = stringex.trim(url)
					ver_str = stringex.trim(ver)
					
					method_str = string.upper(method_str)
				end
				
				first = false
			else
				local k, v = string.match(line, '([^:]+):(.*)') -- 匹配参数
				if nil ~= k and nil ~= v then
					k = stringex.trim(k)
					v = stringex.trim(v)
					
					k = string.lower(k)
					
					header[k] = v
				end
			end
		end
	end
	
	return method_str, url_str, ver_str, header
end

return rtspserve_parser
