--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rtspclienter.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klb rtsp client
-- @note   rtsp 客户端
-- @history 修改历史
--		[2025-10] 添加基础
--]]
local krtsp = require("krtsp")
local kurl = require("kurl")
local kco = require("kco")
local rtspsdper = require("klbcore.klbrtsp.client.rtspsdper")
local rtspauth = require("klbcore.klbrtsp.auth")


--------------------------------------------------------------------------------------------
-- 
local E = {}

-- TEARDOWN 应答等待上限(毫秒); 超时强制关闭
local TEARDOWN_WAIT_MS = 1000


--------------------------------------------------------------------------------------------
-- 内部函数

local get_cseq = function (self)
	local cseq = self.cseq
	
	if 0x8FFF0000 <= cseq then
		self.cseq = 1
	else
		self.cseq = self.cseq + 1
	end
	
	return cseq
end


-- 相对 control 拼到 DESCRIBE 的 URL 上
local join_control = function (base, ctrl)
	if 'string' ~= type(ctrl) or 0 == string.len(ctrl) then
		return base
	end

	if nil ~= string.find(ctrl, '://', 1, true) then
		return ctrl
	end

	local scheme, rest = string.match(base, '^([^:]+://)(.*)$')
	if nil == scheme then
		return ctrl
	end

	if '/' == string.sub(ctrl, 1, 1) then
		local host = string.match(rest, '^([^/]*)') or ''
		return scheme .. host .. ctrl
	end

	if '/' ~= string.sub(base, -1) then
		return base .. '/' .. ctrl
	end

	return base .. ctrl
end


-- Session 头可能带 ;timeout=
local apply_session = function (self, raw)
	if 'string' ~= type(raw) or '' == raw then
		return
	end

	local session = string.match(raw, '^%s*([^;%s]+)')
	if nil ~= session and '' ~= session and '' == self.session then
		self.session = session
	end

	local timeout = tonumber(string.match(raw, 'timeout%s*=%s*(%d+)'))
	if nil ~= timeout and 0 < timeout then
		local ms = timeout * 500
		if ms < 5000 then
			ms = 5000
		end
		self.timeout_ms = ms
	end
end


-- 收一条 RTSP 文本. 非文本返回 nil
local recv_text = function (self)
	local msg, txt = self._client:co_recv()

	if 'text' ~= msg or 'string' ~= type(txt) then
		return nil
	end

	return txt
end


local pct_decode = function (s)
	if 'string' ~= type(s) then
		return ''
	end

	return (string.gsub(s, '%%(%x%x)', function (h)
		return string.char(tonumber(h, 16))
	end))
end


local split_userinfo = function (info)
	local user = ''
	local pass = ''

	if 'string' ~= type(info) or '' == info then
		return '', ''
	end

	info = pct_decode(info)
	user, pass = string.match(info, '^(.-):(.*)$')

	if nil == user then
		return info, ''
	end

	return user, pass or ''
end


local strip_userinfo = function (url, info)
	local mark = ''
	local at = nil

	if 'string' ~= type(info) or '' == info then
		return url
	end

	mark = info .. '@'
	at = string.find(url, mark, 1, true)

	if nil == at then
		return url
	end

	return string.sub(url, 1, at - 1) .. string.sub(url, at + string.len(mark))
end


local auth_value = function (self, method, url)
	return rtspauth.authorization(method, url, self.user, self.pass, self.auth_www, self)
end


-- 发一条请求. 401 且带了用户名时, 用 WWW-Authenticate 重试一次
local exchange = function (self, method, url, build)
	local function once()
		local cseq = get_cseq(self)
		local az = auth_value(self, method, url)
		local s = build(cseq, az)
		local txt = nil

		self._client:send_text(s)
		txt = recv_text(self)

		if nil == txt then
			return nil
		end

		return rtspsdper.parse(txt)
	end

	local status, header, body = once()
	local www = nil

	if nil == status then
		return nil
	end

	if 401 == status and 'string' == type(self.user) and '' ~= self.user and 'table' == type(header) then
		www = header['www-authenticate']
		if 'string' == type(www) and '' ~= www then
			self.auth_www = www
			status, header, body = once()
		end
	end

	return status, header, body
end


local server_port_of = function (header)
	local raw = nil

	if 'table' ~= type(header) then
		return nil
	end

	raw = header['transport']
	if 'string' ~= type(raw) then
		return nil
	end

	return tonumber(string.match(string.lower(raw), 'server_port%s*=%s*(%d+)'))
end


-- 请求 OPTIONS. 失败不中断会话
local req_options = function (self)
	exchange(self, 'OPTIONS', self.url, function (cseq, az)
		return rtspsdper.pack_OPTIONS(self.url, cseq, az)
	end)
end


-- 请求 DESCRIBE
local req_describe = function (self)
	local client = self._client
	local url = self.url
	local status, _, body = exchange(self, 'DESCRIBE', url, function (cseq, az)
		return rtspsdper.pack_DESCRIBE(url, cseq, az)
	end)

	if nil == status or 200 ~= status or 'string' ~= type(body) then
		return status or 2
	end
	
	local sdp, sdp_video, sdp_audio = rtspsdper.parse_sdp(body)
	
	self.video = sdp_video
	self.audio = sdp_audio

	local track = self.video[1]
	if nil == track then
		return 2
	end

	track.url = join_control(self.url, track.url or '')
	if 'string' ~= type(track.url) or '' == track.url then
		return 2
	end

	local pt = track.fmt_id or 96
	if 0 < pt then
		client:set_video_pt(pt)
	end

	return 0
end


-- ANNOUNCE (推流端声明媒体). 常在 SETUP 之前调用
local req_announce = function (self, sdp)
	local url = self.url
	local status, header = exchange(self, 'ANNOUNCE', url, function (cseq, az)
		return rtspsdper.pack_ANNOUNCE(url, cseq, sdp, az)
	end)

	if nil == status then
		return 2
	end

	if 'table' == type(header) then
		apply_session(self, header['session'])
	end

	if 200 ~= status then
		return status
	end

	return 0
end


-- @brief 是否支持的 AAC 音频轨 (mpeg4-generic / AAC-hbr)
local audio_track_ok = function (track)
	if nil == track then
		return false
	end

	local enc = track.encoding or ''
	local mode = track.mode or 'aac-hbr'

	if string.find(enc, 'mpeg4-generic', 1, true) or string.find(enc, 'aac', 1, true) then
		if 'aac-hbr' == mode or '' == mode then
			return true
		end
	end

	return false
end


-- 请求音频. mode 为 'udp' 或 'tcp'; 失败不阻断视频会话
local req_setup_audio = function (self, mode)
	local client = self._client
	local track = self.audio[1]
	local url = ''
	local status = 0
	local header = nil

	if not audio_track_ok(track) then
		return 0
	end

	url = join_control(self.url, track.url or '')
	if 'string' ~= type(url) or '' == url then
		return 0
	end

	track.url = url

	local pt = track.rtpmap_pt or track.fmt_id or 97
	if 0 < pt then
		client:set_audio_pt(pt)
	end

	local samples = track.clock_rate or 44100
	local channels = track.channels or 2
	client:set_audio_info(samples, channels, 2)

	if 'udp' == mode then
		local port = client:open_udp()
		if nil == port then
			return 0
		end

		status, header = exchange(self, 'SETUP', url, function (cseq, az)
			return rtspsdper.pack_SETUP_udp(url, cseq, self.session, port, az)
		end)
	else
		status, header = exchange(self, 'SETUP', url, function (cseq, az)
			return rtspsdper.pack_SETUP(url, cseq, self.session, az, 2)
		end)
	end

	if nil == status or 200 ~= status then
		client:set_audio_pt(0)
		return 0
	end

	apply_session(self, header['session'])

	if 'udp' == mode then
		local sport = server_port_of(header)
		if nil ~= sport and 'string' == type(self.host) and '' ~= self.host then
			client:udp_punch(self.host, sport)
		end
	end

	return 0
end

-- 请求视频. mode 为 'udp' 或 'tcp'
local req_setup_video = function (self, mode)
	local client = self._client
	local track = self.video[1]
	local url = ''
	local status = 0
	local header = nil

	if nil == track or 'string' ~= type(track.url) or '' == track.url then
		return 2
	end

	url = track.url

	if 'udp' == mode then
		local port = client:open_udp()
		if nil == port then
			return 2
		end

		status, header = exchange(self, 'SETUP', url, function (cseq, az)
			return rtspsdper.pack_SETUP_udp(url, cseq, self.session, port, az)
		end)
	else
		client:close_udp()
		status, header = exchange(self, 'SETUP', url, function (cseq, az)
			return rtspsdper.pack_SETUP(url, cseq, self.session, az, 0)
		end)
	end

	if nil == status or 200 ~= status then
		return status or 2
	end

	apply_session(self, header['session'])
	self.transport = mode

	if 'udp' == mode then
		local sport = server_port_of(header)
		if nil ~= sport and 'string' == type(self.host) and '' ~= self.host then
			client:udp_punch(self.host, sport)
		end
	end

	return 0
end

-- PLAY
local req_play = function (self)
	local url = self.url
	local status = exchange(self, 'PLAY', url, function (cseq, az)
		return rtspsdper.pack_PLAY(url, cseq, self.session, az)
	end)

	if nil == status or 200 ~= status then
		return status or 2
	end

	return 0
end

-- RECORD (推流端开始发送媒体)
local req_record = function (self)
	local url = self.url
	local status = exchange(self, 'RECORD', url, function (cseq, az)
		return rtspsdper.pack_RECORD(url, cseq, self.session, az)
	end)

	if nil == status or 200 ~= status then
		return status or 2
	end

	return 0
end

-- PAUSE
local req_pause = function (self)
	local url = self.url
	local status = exchange(self, 'PAUSE', url, function (cseq, az)
		return rtspsdper.pack_PAUSE(url, cseq, self.session, az)
	end)

	if nil == status or 200 ~= status then
		return status or 2
	end

	return 0
end

-- GET_PARAMETER
local req_get_parameter = function (self)
	exchange(self, 'GET_PARAMETER', self.url, function (cseq, az)
		return rtspsdper.pack_GET_PARAMETER(self.url, cseq, self.session, az)
	end)
end

-- TEARDOWN
local req_teardown = function (self)
	exchange(self, 'TEARDOWN', self.url, function (cseq, az)
		return rtspsdper.pack_TEARDOWN(self.url, cseq, self.session, az)
	end)
end

--------------------------------------------------------------------------------------------
-- rtspclient 对外接口

local rtspclient = {}


-- @brief 断开连接. 立即关闭, 不等待服务端; 需要释放服务端会话请用 co_disconnect
function rtspclient:disconnect()
	self._run = false
	if self._client then
		self._client:disconnect()	
		self._client = nil
	end	
	self.session = ''
end


-- @brief 协程断开连接. 先发 TEARDOWN 释放服务端会话, 收到应答或超时后再关闭
-- @return code[number]		0 成功; 2 无连接
function rtspclient:co_disconnect()
	self._run = false

	local client = self._client
	if nil == client then
		return 2
	end

	local session = self.session
	self._client = nil		-- 防止保活协程与重入再使用
	self.session = ''

	if 'string' ~= type(session) or '' == session then
		client:disconnect()
		return 0
	end

	-- TEARDOWN 入队, 由底层事件循环写出
	local cseq = get_cseq(self)
	local az = auth_value(self, 'TEARDOWN', self.url)
	client:send_text(rtspsdper.pack_TEARDOWN(self.url, cseq, session, az))

	-- 等应答; 超时强制关闭(关闭会唤醒 co_recv)
	kco.timeout(TEARDOWN_WAIT_MS, function ()
		client:disconnect()
	end)

	while true do
		local msg, data = client:co_recv()

		if nil == msg or 'exit' == msg then
			break
		end

		if 'media' == msg then
			client:free_media(data)
		elseif 'text' == msg then
			break
		end
	end

	client:disconnect()
	return 0
end


-- @brief 连接到目标
-- @return ok[boolean]					true 成功
-- @return code[number]					失败时 1=连接失败 / RTSP 状态码等
-- @return errstr[string]				失败说明
function rtspclient:co_connect(url)	
	-- eg. rtsp://username:password@127.0.0.1:554/stream
	local u = kurl.parse(url)	
	
	local schema = u['schema'] or 'rtsp'
	local host = u['host'] or ''
	local port = '554'
	
	if u['port'] then
		port = u['port']
	end

	-- 发起连接
	local client = krtsp.connect(host, tonumber(port))
	if not client then
		return nil, 1, "connect failed"
	end
	
	-- 更新. 请求行去掉 userinfo, 凭据只放进 Authorization
	self._client = client
	self.user, self.pass = split_userinfo(u['userinfo'])
	self.host = host
	self.auth_www = nil
	self.auth_nc = 0
	self.url = strip_userinfo(url, u['userinfo'])
	
	-- 请求 OPTIONS
	req_options(self)
	
	-- 请求 DESCRIBE
	local st = req_describe(self)
	if 0 ~= st then
		return nil, st, "describe failed"
	end
	
	-- 请求 SETUP. udp 被拒时改走 tcp
	st = req_setup_video(self, self.transport)
	if 0 ~= st and 'udp' == self.transport then
		st = req_setup_video(self, 'tcp')
	end
	if 0 ~= st then
		return nil, st, "setup failed"
	end

	-- 可选音频 SETUP; 失败不影响视频
	req_setup_audio(self, self.transport)
	
	--
	req_get_parameter(self)
	
	-- 请求 PLAY
	st = req_play(self)
	if 0 ~= st then
		return nil, st, "play failed"
	end

	self._run = true
	kco.fork(function ()
		while self._run do
			kco.co_sleep(self.timeout_ms or 20000)
			if self._run and self._client and '' ~= self.session then
				local cseq = get_cseq(self)
				local az = auth_value(self, 'GET_PARAMETER', self.url)
				local s = rtspsdper.pack_GET_PARAMETER(self.url, cseq, self.session, az)
				self._client:send_text(s)
			end
		end
	end)

	return true
end

-- @brief 接收数据
-- @return msg, data  msg 为 'text' / 'media' / 'exit'. media 用完后须 free_media
function rtspclient:co_recv()
	if self._client then
		return self._client:co_recv()
	end
end

-- @brief 暂停拉流(PAUSE). 须在 PLAY 成功后调用
-- @return code[number]		0 成功; 非0 失败(2 无会话 / RTSP 状态码等)
function rtspclient:co_pause()
	if nil == self._client or nil == self.session or '' == self.session then
		return 2
	end

	return req_pause(self)
end

-- @brief 声明媒体(ANNOUNCE). 推流端在 SETUP 前发送 SDP 描述
-- @param [in] sdp[string]		会话 SDP 描述
-- @return code[number]		0 成功; 非0 失败(2 无连接/无 SDP / RTSP 状态码等)
function rtspclient:co_announce(sdp)
	if nil == self._client or 'string' ~= type(sdp) or '' == sdp then
		return 2
	end

	return req_announce(self, sdp)
end

-- @brief 开始推流(RECORD). 须在 SETUP/ANNOUNCE 成功后调用
-- @return code[number]		0 成功; 非0 失败(2 无会话 / RTSP 状态码等)
function rtspclient:co_record()
	if nil == self._client or nil == self.session or '' == self.session then
		return 2
	end

	return req_record(self)
end

-- @brief 以推流方式发送一帧媒体. ptr 为 C 层媒体数据指针; 送出后由底层释放
-- @param [in] ptr[lightuserdata]		媒体数据指针
-- @return code[number]		0 成功; 非0 失败
function rtspclient:send_media(ptr)
	if self._client and ptr then
		return self._client:send_media(ptr)
	end
end

-- @brief 释放 co_recv 返回的媒体指针
function rtspclient:free_media(ptr)
	if self._client and ptr then
		self._client:free_media(ptr)
	end
end

-- @brief 设置底层连接读写/空闲超时(毫秒). 0 表示不启用
-- @param [in] read_ms[number] [可选] 读超时
-- @param [in] write_ms[number] [可选] 写超时
-- @param [in] idle_ms[number] [可选] 空闲回收超时
function rtspclient:set_timeout(read_ms, write_ms, idle_ms)
	if self._client then
		self._client:set_timeout(read_ms or 0, write_ms or 0, idle_ms or 0)
	end
end


--------------------------------------------------------------------------------------------
-- rtspclienter

local rtspclienter = {}


-- @brief 新建一个rtsp客户端
rtspclienter.new = function (cfg)
	local transport = 'tcp'
	if 'table' == type(cfg) and 'udp' == cfg.transport then
		transport = 'udp'
	end

	local obj = {
		_client = nil,			-- C/C++ 提供的客户端连接
		
		cseq = 1,				-- 请求序号
		timeout_ms = 20000,		-- 保活间隔(毫秒)
		_run = false,			-- 保活协程开关
		transport = transport,	-- 'tcp' 或 'udp'
		
		url = '',				-- 基础 url	
		session = '',			-- 会话标识
		user = '',				-- URL userinfo
		pass = '',
		host = '',
		auth_www = nil,			-- 最近一次 WWW-Authenticate
		auth_nc = 0,
		
		video = {},			-- 视频列表
		audio = {},			-- 音频列表
	}
	
	setmetatable(obj, {
		__index = rtspclient,
		__tostring = function(self)
			return tostring(self._client)
        end
	})
	
	return obj
end


return rtspclienter
