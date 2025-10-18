local stringex = require("klbcore.util.stringex")

local rtspsdper = {}


local user_agent = 'klbrtsp-v2025.10'


-- OPTIONS, 查询服务端支持的命令集合
--[[
请求:
OPTIONS rtsp://192.168.1.123:554/aaa RTSP/1.0
CSeq: 1
User-Agent: klbrtsp-v2025.10

回复200:
RTSP/1.0 200 OK
CSeq: 1
Public: DESCRIBE, ANNOUNCE, SETUP, PLAY, RECORD, PAUSE, GET_PARAMETER, TEARDOWN
Server: gortsplib

--]]
rtspsdper.pack_OPTIONS = function (url, cseq)	
	local t = {}
	
	table.insert(t, string.format('OPTIONS %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


rtspsdper.pack_DESCRIBE = function (url, cseq)
	local t = {}
	
	table.insert(t, string.format('DESCRIBE %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Accept: application/sdp\r\n')
	table.insert(t, '\r\n')
	
	return table.concat(t)
end



local try_insert_session = function (t, session)
	if 'string' == type(session) and 0 < string.len(session) then
		table.insert(t, string.format('Session: %s\r\n', session))
	end
end


rtspsdper.pack_SETUP = function (url, cseq, session)
	local t = {}
	
	table.insert(t, string.format('SETUP %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n')
	
	try_insert_session(t, session)
	
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


rtspsdper.pack_PLAY = function (url, cseq, session)
	local t = {}
	
	table.insert(t, string.format('PLAY %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Range: npt=0.000-\r\n')

	try_insert_session(t, session)
	
	table.insert(t, '\r\n')
	
	return table.concat(t)	
end

-- GET_PARAMETER
rtspsdper.pack_GET_PARAMETER = function (url, cseq, session)
	local t = {}
	
	table.insert(t, string.format('GET_PARAMETER %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))

	try_insert_session(t, session)
	
	table.insert(t, '\r\n')
	
	return table.concat(t)	
end

-- TEARDOWN
rtspsdper.pack_TEARDOWN = function (url, cseq, session)
	local t = {}
	
	table.insert(t, string.format('TEARDOWN %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	
	try_insert_session(t, session)
	
	table.insert(t, '\r\n')
	
	return table.concat(t)	
end


-- 解析 rtsp 会话数据
rtspsdper.parse = function (txt)	
	local status = 1
	local header = {}
	local body_str = ''
	
	-- 切分
	local head, body = string.match(txt, '(.*)\r\n\r\n(.*)')
	
	-- 提取请求头信息
	if nil ~= head then
		local first = true
		for line in string.gmatch(head, '[^\r\n]+[\r\n]*') do
			if first then
				-- eg. 'RTSP/1.0 200 OK'
				local ver, state, state_str = string.match(line, '([^ ]+) +([^ ]+) +([^ ]+)') -- 匹配请求头
				if nil ~= ver and nil ~= state and nil ~= state_str then
					state = stringex.trim(state)
					status = tonumber(state)
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
	
	if nil ~= body then
		body_str = body
	end
	
	return status, header, body
end

-- 解析 m=*
-- m=<媒体类型> <端口> <传输协议> <格式列表>
local parse_sdp_m = function (t, s)
	-- eg. m=video 0 RTP/AVP 96
	-- eg. m=audio 0 RTP/AVP 0
	-- eg. m=video 5004 RTP/AVP 96 (视频流, UDP 端口 5004, 格式编号 96)
	local media_type, port, protocal, fmt_id = string.match(s, '([^ ]+) +([^ ]+) +([^ ]+) +([^ ]+)') -- 匹配请求头
	
	media_type = stringex.trim(media_type)
	port = stringex.trim(port)
	protocal = stringex.trim(protocal)
	fmt_id = stringex.trim(fmt_id)
	
	t['media_type'] = media_type
	t['port'] = tonumber(port)
	t['protocal'] = protocal
	t['fmt_id'] = tonumber(fmt_id)
end

-- 解析 a=*
local parse_sdp_a = function (t, s)
	
	local k, v = string.match(s, '([^:]+):(.*)') -- 匹配参数
	
	k = stringex.trim(k)
	v = stringex.trim(v)
	
	k = string.lower(k)
	
	if 'control' == k then
		t['url'] = v
	elseif 'fmtp' == k then
		
	elseif 'rtpmap' == k then
		
	end
end

-- 解析 rtsp 的sdp 数据
rtspsdper.parse_sdp = function (txt)	
	--[[ eg.
		v=0
		o=- 1620000000 1 IN IP4 192.168.1.100
		s=IP Camera Stream
		c=IN IP4 192.168.1.100
		t=0 0
		
		m=video 5004 RTP/AVP 96
		a=rtpmap:96 H264/90000
		a=fmtp:96 profile-level-id=42e01f; sprop-parameter-sets=Z0LAHtkDxWhAAAADAEAAAAwDxYuS,aM48gA==
		a=control:trackID=0
		
		m=audio 5006 RTP/AVP 0
		a=rtpmap:0 PCMU/8000/1
		a=control:trackID=1
	--]]
	
	local sdp, sdp_video, sdp_audio = {}, {}, {}
	
	local first = true
	local media = {}
	
	for line in string.gmatch(txt, '[^\r\n]+[\r\n]*') do
		local k, v = string.match(line, '([^=]+)=(.*)') -- 匹配参数
		
		if nil ~= k and nil ~= v then
			k = stringex.trim(k)
			v = stringex.trim(v)
			
			k = string.lower(k)
			
			if 'm' == k then
				if first then
					first = false
				else
					local media_type = media['media_type'] or ''
					
					if 'video' == media_type then
						table.insert(sdp_video, media)
					elseif 'audio' == media_type then
						table.insert(sdp_audio, media)
					end
					
					media = {}
				end
				
				parse_sdp_m(media, v)
			elseif 'a' == k then
				parse_sdp_a(media, v)
			end
		end
	end
	
	local media_type = media['media_type'] or ''	
	if 'video' == media_type then
		table.insert(sdp_video, media)
	elseif 'audio' == media_type then
		table.insert(sdp_audio, media)
	end
	
	return sdp, sdp_video, sdp_audio
end

return rtspsdper
