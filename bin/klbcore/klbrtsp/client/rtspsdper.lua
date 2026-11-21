local stringex = require("klbcore.util.stringex")

local rtspsdper = {}


local user_agent = 'klbrtsp-v2025.10'


local try_insert_auth = function (t, authorization)
	if 'string' == type(authorization) and 0 < string.len(authorization) then
		table.insert(t, string.format('Authorization: %s\r\n', authorization))
	end
end


-- OPTIONS, 查询服务端支持的命令集合
--[[
请求:
OPTIONS rtsp://192.0.2.10:554/aaa RTSP/1.0
CSeq: 1
User-Agent: klbrtsp-v2025.10

回复200:
RTSP/1.0 200 OK
CSeq: 1
Public: DESCRIBE, ANNOUNCE, SETUP, PLAY, RECORD, PAUSE, GET_PARAMETER, TEARDOWN
Server: gortsplib

--]]
rtspsdper.pack_OPTIONS = function (url, cseq, authorization)	
	local t = {}
	
	table.insert(t, string.format('OPTIONS %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	try_insert_auth(t, authorization)
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


rtspsdper.pack_DESCRIBE = function (url, cseq, authorization)
	local t = {}
	
	table.insert(t, string.format('DESCRIBE %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Accept: application/sdp\r\n')
	try_insert_auth(t, authorization)
	table.insert(t, '\r\n')
	
	return table.concat(t)
end



-- ANNOUNCE, 推流端向服务端声明媒体描述(SDP 作为 body)
--[[
请求:
ANNOUNCE rtsp://192.0.2.10:554/aaa RTSP/1.0
CSeq: 1
User-Agent: klbrtsp-v2025.10
Content-Type: application/sdp
Content-Length: 123

<sdp body>
--]]
rtspsdper.pack_ANNOUNCE = function (url, cseq, sdp, authorization)
	local t = {}
	local body = ('string' == type(sdp)) and sdp or ''

	table.insert(t, string.format('ANNOUNCE %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Content-Type: application/sdp\r\n')
	table.insert(t, string.format('Content-Length: %d\r\n', string.len(body)))

	try_insert_auth(t, authorization)

	table.insert(t, '\r\n')
	table.insert(t, body)

	return table.concat(t)
end


local try_insert_session = function (t, session)
	if 'string' == type(session) and 0 < string.len(session) then
		table.insert(t, string.format('Session: %s\r\n', session))
	end
end


rtspsdper.pack_SETUP = function (url, cseq, session, authorization, interleaved)	
	local t = {}
	local ch = tonumber(interleaved) or 0

	if 0 > ch or 254 < ch then
		ch = 0
	end
	
	table.insert(t, string.format('SETUP %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, string.format('Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n', ch, ch + 1))
	
	try_insert_session(t, session)
	try_insert_auth(t, authorization)
	
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


rtspsdper.pack_SETUP_udp = function (url, cseq, session, rtp_port, authorization)
	local t = {}
	local port = tonumber(rtp_port) or 0

	table.insert(t, string.format('SETUP %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, string.format('Transport: RTP/AVP;unicast;client_port=%d-%d\r\n', port, port + 1))

	try_insert_session(t, session)
	try_insert_auth(t, authorization)

	table.insert(t, '\r\n')

	return table.concat(t)
end


rtspsdper.pack_PLAY = function (url, cseq, session, authorization)
	local t = {}
	
	table.insert(t, string.format('PLAY %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Range: npt=0.000-\r\n')

	try_insert_session(t, session)
	try_insert_auth(t, authorization)
	
	table.insert(t, '\r\n')
	
	return table.concat(t)	
end

-- RECORD, 推流端开始向服务端发送媒体
rtspsdper.pack_RECORD = function (url, cseq, session, authorization)
	local t = {}
	
	table.insert(t, string.format('RECORD %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	table.insert(t, 'Range: npt=0.000-\r\n')

	try_insert_session(t, session)
	try_insert_auth(t, authorization)
	
	table.insert(t, '\r\n')
	
	return table.concat(t)	
end

-- PAUSE
rtspsdper.pack_PAUSE = function (url, cseq, session, authorization)
	local t = {}
	
	table.insert(t, string.format('PAUSE %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))

	try_insert_session(t, session)
	try_insert_auth(t, authorization)
	
	table.insert(t, '\r\n')
	
	return table.concat(t)	
end

-- GET_PARAMETER
rtspsdper.pack_GET_PARAMETER = function (url, cseq, session, authorization)
	local t = {}
	
	table.insert(t, string.format('GET_PARAMETER %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))

	try_insert_session(t, session)
	try_insert_auth(t, authorization)
	
	table.insert(t, '\r\n')
	
	return table.concat(t)	
end

-- TEARDOWN
rtspsdper.pack_TEARDOWN = function (url, cseq, session, authorization)
	local t = {}
	
	table.insert(t, string.format('TEARDOWN %s RTSP/1.0\r\n', url))
	table.insert(t, string.format('CSeq: %d\r\n', cseq))
	table.insert(t, string.format('User-Agent: %s\r\n', user_agent))
	
	try_insert_session(t, session)
	try_insert_auth(t, authorization)
	
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
-- m=<媒体类型> <端口> <传输协议> <格式列表...>
local parse_sdp_m = function (t, s)
	-- eg. m=video 0 RTP/AVP 96
	-- eg. m=audio 0 RTP/AVP 0
	-- eg. m=video 5004 RTP/AVP 96 97 (视频流, UDP 端口 5004, 格式编号 96/97)
	local part = {}
	local n = 0
	local fmt = nil

	for item in string.gmatch(s, '%S+') do
		n = n + 1
		part[n] = item
	end

	if 0 == n then
		return
	end

	t['media_type'] = stringex.trim(part[1])
	t['port'] = tonumber(part[2])
	t['protocal'] = stringex.trim(part[3] or '')

	-- 多格式: m= 行第 4 个 token 起均为格式编号
	fmt = {}
	local i = 4
	while i <= n do
		local id = tonumber(part[i])
		if nil ~= id then
			fmt[#fmt + 1] = id
		end
		i = i + 1
	end

	t['fmt_ids'] = fmt
	if 0 < #fmt then
		t['fmt_id'] = fmt[1]		-- 兼容: 首个格式编号
	end
end

-- 解析 a=*
local parse_sdp_a = function (t, s)
	local k, v = string.match(s, '([^:]+):(.*)')	-- 匹配 k:v

	if nil == k then
		-- 无冒号行, eg. a=sendonly / a=recvonly / a=sendrecv
		k = stringex.trim(s)
		v = ''
	end

	k = stringex.trim(k)
	v = stringex.trim(v or '')
	k = string.lower(k)

	if '' == k then
		return
	end

	if 'control' == k then
		t['url'] = v
		-- control 可为相对路径(trackID=0)或绝对 URL(rtsp://...)
		if nil ~= string.find(v, '://', 1, true) or '/' == string.sub(v, 1, 1) then
			t['control_absolute'] = true
		else
			t['control_absolute'] = false
		end
	elseif 'fmtp' == k then
		t['fmtp'] = v
		local mode = string.match(string.lower(v), 'mode%s*=%s*([^;%s]+)')
		if nil ~= mode then
			t['mode'] = mode
		end
	elseif 'rtpmap' == k then
		local pt, enc, rate, ch = string.match(v, '(%d+)%s+([^/]+)/(%d+)/?(%d*)')
		t['rtpmap_pt'] = tonumber(pt)
		if nil ~= enc then
			t['encoding'] = string.lower(stringex.trim(enc))
		end
		t['clock_rate'] = tonumber(rate)
		t['channels'] = tonumber(ch) or 1
	else
		-- 其它属性: 统一收进 attrs(含 ONVIF x-dimensions 等)
		if nil == t['attrs'] then
			t['attrs'] = {}
		end
		t['attrs'][k] = v
	end
end

-- 解析 rtsp 的sdp 数据
-- 返回: 会话级 sdp 表(v/o/s/c/t/b + 会话级 a=), 视频轨列表, 音频轨列表(均含 fmt_id/fmt_ids/url/attrs 等)
rtspsdper.parse_sdp = function (txt)	
	--[[ eg.
		v=0
		o=- 1620000000 1 IN IP4 192.0.2.10
		s=IP Camera Stream
		c=IN IP4 192.0.2.10
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
	local media = nil

	-- 收尾: 依媒体类型归入 视频/音频 轨列表
	local flush = function (m)
		if nil == m then
			return
		end

		local media_type = m['media_type'] or ''

		if 'video' == media_type then
			table.insert(sdp_video, m)
		elseif 'audio' == media_type then
			table.insert(sdp_audio, m)
		end
	end

	for line in string.gmatch(txt, '[^\r\n]+[\r\n]*') do
		local k, v = string.match(line, '([^=]+)=(.*)') -- 匹配参数

		if nil ~= k and nil ~= v then
			k = stringex.trim(k)
			v = stringex.trim(v)

			k = string.lower(k)

			if 'm' == k then
				-- 遇到新 m= 行, 归置上一轨
				flush(media)
				media = {}
				parse_sdp_m(media, v)
			elseif 'a' == k then
				-- 会话级(media 未开始) or 媒体级属性
				parse_sdp_a(media or sdp, v)
			elseif 'b' == k then
				-- 带宽: b=<类型>:<值>, eg. b=AS:500
				local target = media or sdp
				local bw_type, bw = string.match(v, '([^:]+):(.*)')

				if nil ~= bw_type then
					target['bandwidth_type'] = string.upper(stringex.trim(bw_type))
					target['bandwidth'] = tonumber(bw) or stringex.trim(bw)
				end
			elseif nil == media then
				-- 会话级: v= o= s= c= t= 等
				sdp[k] = v
			end
		end
	end

	flush(media)

	return sdp, sdp_video, sdp_audio
end

return rtspsdper
