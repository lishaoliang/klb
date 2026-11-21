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


--------------------------------------------------------------------------------------------
-- 前置定义

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

-- 方法 固定顺序(RFC 2326 方法习惯序); 保证 Public / Allow 输出稳定, 不随 pairs 抖动
local method_order = {
	'OPTIONS', 'DESCRIBE', 'ANNOUNCE', 'SETUP', 'PLAY', 'PAUSE',
	'RECORD', 'TEARDOWN', 'GET_PARAMETER', 'SET_PARAMETER', 'REDIRECT',
}

-- 获取支持的所有方法. 先按固定序取已支持项, 表外新增项排序后补在尾部
local GetMethodsString = function (method_map)
	local t = {}
	local used = {}
	
	for _, k in ipairs(method_order) do
		if nil ~= method_map[k] then
			table.insert(t, k)
			used[k] = true
		end
	end
	
	local rest = {}
	for k, _ in pairs(method_map) do
		if not used[k] then
			table.insert(rest, k)
		end
	end
	
	table.sort(rest)
	
	for _, k in ipairs(rest) do
		table.insert(t, k)
	end
	
	return table.concat(t, ', ')
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
rtspserve_parser.pack_res_DESCRIBE = function (ver, code, cseq, sdp_len, base_url)
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	table.insert(t, 'Content-Type: application/sdp\r\n')
	
	table.insert(t, string.format('Content-Length: %s\r\n', ToString(sdp_len)))
	if 'string' == type(base_url) and 0 < string.len(base_url) then
		local base = base_url
		if '/' ~= string.sub(base, -1) then
			base = base .. '/'
		end
		table.insert(t, string.format('Content-Base: %s\r\n', base))
	end
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end

-- 追加 一路视频 m= 段
local AppendVideoM = function (t, pt, sps, pps, track_id)
	table.insert(t, string.format('m=video 0 RTP/AVP %d\r\n', pt))
	table.insert(t, string.format('a=rtpmap:%d H264/90000\r\n', pt))
	table.insert(t, string.format('a=fmtp:%d packetization-mode=1;sprop-parameter-sets=%s,%s\r\n', pt, sps, pps))
	table.insert(t, string.format('a=control:trackID=%d\r\n', track_id))
end

-- 追加 一路音频 m= 段
local AppendAudioM = function (t, pt, rate, channels, config, track_id)
	table.insert(t, string.format('m=audio 0 RTP/AVP %d\r\n', pt))
	table.insert(t, string.format('a=rtpmap:%d mpeg4-generic/%d/%d\r\n', pt, rate, channels))
	table.insert(t, string.format(
		'a=fmtp:%d streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=%s\r\n',
		pt, config))
	table.insert(t, string.format('a=control:trackID=%d\r\n', track_id))
end

-- sdp. h264 为 {pt, sps, pps}; aac 为 {pt, rate, channels, config}. sps/pps/config 为 base64
-- tracks 为可选多轨数组(每项 {pt, is_audio, sps/pps 或 rate/channels/config}); 为空时输出单视频+可选单音频
rtspserve_parser.pack_res_sdp = function (h264, aac, tracks)
	local t = {}
	local pt = 96
	local sps = 'Z2QADazZQYHflwEQAAADABAAAAMDIPFCmWA='
	local pps = 'aOvjyyLA'
	local n = nil
	local apt = 97
	local rate = 44100
	local channels = 2
	local config = '1210'
	
	if 'table' == type(h264) then
		n = tonumber(h264.pt)
		if nil ~= n and 0 < n and 128 > n then
			pt = math.floor(n)
		end
		if 'string' == type(h264.sps) and '' ~= h264.sps then
			sps = h264.sps
		end
		if 'string' == type(h264.pps) and '' ~= h264.pps then
			pps = h264.pps
		end
	end

	if 'table' == type(aac) then
		n = tonumber(aac.pt)
		if nil ~= n and 0 < n and 128 > n then
			apt = math.floor(n)
		end
		n = tonumber(aac.rate) or tonumber(aac.samples)
		if nil ~= n and 0 < n then
			rate = math.floor(n)
		end
		n = tonumber(aac.channels) or tonumber(aac.tracks)
		if nil ~= n and 0 < n then
			channels = math.floor(n)
		end
		if 'string' == type(aac.config) and '' ~= aac.config then
			config = aac.config
		end
	end
	
	-- 公共头定义
	table.insert(t, 'v=0\r\n')
	table.insert(t, 'o=- 123456789 123456789 IN IP4 127.0.0.1\r\n')
	table.insert(t, 's=rtsp serve\r\n')
	table.insert(t, 'c=IN IP4 0.0.0.0\r\n')
	table.insert(t, 't=0 0\r\n')
	
	if 'table' == type(tracks) then
		-- 多轨: 按列表逐轨输出, trackID = 索引(0..)
		for i, item in ipairs(tracks) do
			local track_id = i - 1
			local n = tonumber(item.pt)
			local m_pt = pt
			if nil ~= n and 0 < n and 128 > n then
				m_pt = math.floor(n)
			end
			if true == item.is_audio or 'audio' == item.kind then
				local m_rate = rate
				local m_channels = channels
				local m_config = config
				n = tonumber(item.rate) or tonumber(item.samples)
				if nil ~= n and 0 < n then
					m_rate = math.floor(n)
				end
				n = tonumber(item.channels) or tonumber(item.tracks)
				if nil ~= n and 0 < n then
					m_channels = math.floor(n)
				end
				if 'string' == type(item.config) and '' ~= item.config then
					m_config = item.config
				end
				AppendAudioM(t, m_pt, m_rate, m_channels, m_config, track_id)
			else
				local m_sps = sps
				local m_pps = pps
				if 'string' == type(item.sps) and '' ~= item.sps then
					m_sps = item.sps
				end
				if 'string' == type(item.pps) and '' ~= item.pps then
					m_pps = item.pps
				end
				AppendVideoM(t, m_pt, m_sps, m_pps, track_id)
			end
		end
	else
		-- 视频流
		AppendVideoM(t, pt, sps, pps, 0)
	
		-- 音频流 (可选)
		if 'table' == type(aac) then
			AppendAudioM(t, apt, rate, channels, config, 1)
		end
	end
	
	return table.concat(t)
end

-- SETUP 回复 字符串
rtspserve_parser.pack_res_tcp_SETUP = function (ver, code, cseq, session, timeout, ch0, ch1)
	local t = {}
	local a = tonumber(ch0) or 0
	local b = tonumber(ch1) or (a + 1)
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	table.insert(t, string.format('Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n', a, b))
	table.insert(t, string.format('Session: %s;timeout=%s\r\n', ToString(session), ToString(timeout)))
	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- UDP SETUP 回复
rtspserve_parser.pack_res_udp_SETUP = function (ver, code, cseq, session, timeout, client_port, server_port)
	local t = {}
	local cport = tonumber(client_port) or 0
	local sport = tonumber(server_port) or 0

	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))

	table.insert(t, string.format('Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n', cport, cport + 1, sport, sport + 1))
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


-- PAUSE 回复 字符串
rtspserve_parser.pack_res_PAUSE = function (ver, code, cseq, session, timeout)
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	table.insert(t, string.format('Session: %s;timeout=%s\r\n', ToString(session), ToString(timeout)))
	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- SET_PARAMETER 回复 字符串
rtspserve_parser.pack_res_SET_PARAMETER = function (ver, code, cseq, session, timeout)
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
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


-- 401. scheme 为 'basic' / 'digest' / nil; nil 时同时给出 Basic 与 Digest(Digest 置后)
rtspserve_parser.pack_res_unauthorized = function (ver, cseq, realm, nonce, scheme)
	local t = {}
	local r = string.gsub(realm or 'klbrtsp', '([\\"])', '\\%1')
	local n = string.gsub(nonce or '', '([\\"])', '\\%1')
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(401), ToCodeString(401)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	if 'digest' == scheme then
		table.insert(t, string.format('WWW-Authenticate: Digest realm="%s", nonce="%s", algorithm=MD5\r\n', r, n))
	elseif 'basic' == scheme then
		table.insert(t, string.format('WWW-Authenticate: Basic realm="%s"\r\n', r))
	else
		table.insert(t, string.format('WWW-Authenticate: Basic realm="%s"\r\n', r))
		table.insert(t, string.format('WWW-Authenticate: Digest realm="%s", nonce="%s", algorithm=MD5\r\n', r, n))
	end
	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


-- 不支持 / 状态非法 回复 字符串. allow 为可支持方法列表(可选), 用于 405/455 补 Allow 头
rtspserve_parser.pack_res_unsupported = function (ver, code, cseq, allow)
	local t = {}
	
	table.insert(t, string.format('RTSP/1.0 %s %s\r\n', ToString(code), ToCodeString(code)))
	table.insert(t, string.format('CSeq: %s\r\n', ToString(cseq)))
	
	if 'string' == type(allow) and 0 < string.len(allow) then
		table.insert(t, string.format('Allow: %s\r\n', allow))
	end
	
	table.insert(t, 'Content-Length: 0\r\n')
	table.insert(t, string.format('Server: %s\r\n', user_server))
	table.insert(t, '\r\n')
	
	return table.concat(t)
end


--------------------------------------------------------------------------------------------

-- 对外暴露方法列表串(固定顺序), 供 OPTIONS Public 与 405/455 Allow 复用
rtspserve_parser.methods_string = GetMethodsString


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
				-- eg. 'OPTIONS rtsp://192.0.2.10:554/stream1 RTSP/1.0'
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
