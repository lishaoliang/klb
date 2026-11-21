--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rtspserver.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  RTSP服务 模块
-- @history 修改历史
--		[2025-10] 添加基础RTSP服务
--]]
local krand = require("krand")
local kco = require("kco")
local rtspserve_parser = require("klbcore.klbrtsp.serve.rtspserve_parser")
local rtspauth = require("klbcore.klbrtsp.auth")


--------------------------------------------------------------------------------------------
-- 前置定义
local E = {}
local method_map = {}


--------------------------------------------------------------------------------------------
-- 内部实现

-- @brief 发送文本
local SendText = function (self, head, body)
	local serve = self._serve
	
	local ret = serve:send_text(head, body)
end

-- @brief 发送 媒体数据
local SendMedia = function (self, ptr)
	local serve = self._serve
	
	local ret = serve:send_media(ptr)
end

-- @brief 读取数据
local CoRecv = function (self)
	local serve = self._serve
	
	local txt, msg = serve:co_recv()
	if 'exit' == msg then
		return nil, msg
	end
	
	return txt, msg
end

--------------------------------------------------

-- @brief 会话建立后启用服务端空闲超时回收(由 C 层 ticker 判定, 超时走 exit 拆连)
-- @note idle 值取自 Session timeout; 单位毫秒; 0 表示不启用
local InitTimeouts = function (self)
	if self._serve and self._serve.set_timeout then
		local sec = tonumber(self.timeout) or 60
		self._serve:set_timeout(0, 0, sec * 1000)
	end
end


-- @brief OPTIONS 方法
local OnMethod_OPTIONS = function (self, cseq, url, ver, head)
	-- 公共方法
	local code = 200
	local s = rtspserve_parser.pack_res_OPTIONS(ver, code, cseq, method_map)
	
	-- 回复
	SendText(self, s)	
end

-- @brief DESCRIBE 方法
local OnMethod_DESCRIBE = function (self, cseq, url, ver, head)	
	local code = 200
	
	local sdp = rtspserve_parser.pack_res_sdp(self.h264, self.aac, self.tracks)
	local s = rtspserve_parser.pack_res_DESCRIBE(ver, code, cseq, string.len(sdp), url)
	
	-- 回复
	SendText(self, s, sdp)
end

-- 按客户端列出的顺序选第一项可支持的传输
local PickTransport = function (raw)
	local pos = 1

	if 'string' ~= type(raw) or '' == raw then
		return nil
	end

	while pos <= string.len(raw) do
		local comma = string.find(raw, ',', pos, true)
		local item = ''
		local low = ''

		if nil == comma then
			item = string.sub(raw, pos)
			pos = string.len(raw) + 1
		else
			item = string.sub(raw, pos, comma - 1)
			pos = comma + 1
		end

		low = string.lower(item)

		if nil ~= string.find(low, 'tcp', 1, true) then
			local ch = tonumber(string.match(low, 'interleaved%s*=%s*(%d+)')) or 0
			return 'tcp', ch
		end

		local cport = tonumber(string.match(low, 'client_port%s*=%s*(%d+)'))
		if nil ~= cport then
			return 'udp', cport
		end
	end

	return nil
end


local next_count_tracks = function (self)
	local n = 0
	for _, _ in pairs(self.setup_tracks) do
		n = n + 1
	end
	return n
end


local RememberSetup = function (self, mode, url, arg, sport)
	self.setup_tracks[url] = {
		mode = mode,
		arg = arg,
		sport = sport or 0,
	}
end


local PackSetupOk = function (self, ver, cseq, track)
	if 'udp' == track.mode then
		return rtspserve_parser.pack_res_udp_SETUP(ver, 200, cseq, self.session, self.timeout, track.arg, track.sport)
	end

	return rtspserve_parser.pack_res_tcp_SETUP(ver, 200, cseq, self.session, self.timeout, track.arg, track.arg + 1)
end


-- 从 SETUP url 解析 trackID(0..); 无则返回 0
local ParseTrackId = function (url)
	if 'string' ~= type(url) then
		return 0
	end
	
	local n = tonumber(string.match(url, 'trackID%s*=%s*(%d+)'))
	return n or 0
end

-- @brief SETUP 方法. 按 url 区分视频/音频轨; 同轨重放; 同轨换传输回 461
local OnMethod_SETUP = function (self, cseq, url, ver, head)
	local mode, arg = PickTransport(head['transport'])
	local code = 200
	local s = ''
	local prev = self.setup_tracks[url]

	if nil ~= prev then
		if prev.mode == mode then
			s = PackSetupOk(self, ver, cseq, prev)
		else
			code = 461
			s = rtspserve_parser.pack_res_unsupported(ver, code, cseq)
		end

		SendText(self, s)
		return
	end

	if 'udp' == mode then
		local sport = nil
		if nil ~= self.tracks and nil ~= self._serve.setup_udp_track then
			-- 多轨: 按 trackID 映射轨索引
			sport = self._serve:setup_udp_track(ParseTrackId(url), arg)
		else
			sport = self._serve:setup_udp(arg)
		end
		if nil == sport then
			code = 461
			s = rtspserve_parser.pack_res_unsupported(ver, code, cseq)
		else
			RememberSetup(self, 'udp', url, arg, sport)
			s = PackSetupOk(self, ver, cseq, self.setup_tracks[url])
		end
	elseif 'tcp' == mode then
		local ch = tonumber(arg)
		if nil ~= self.tracks and nil ~= self._serve.setup_tcp_track then
			-- 多轨: 按 trackID 映射轨索引; 未给 interleaved 则按轨分配 0,2,4...
			local track = ParseTrackId(url)
			if nil == ch or 0 > ch then
				ch = track * 2
			end
			self._serve:setup_tcp_track(track, ch)
		else
			-- 旧语义: 首轨默认 0; 已有轨则用客户端 interleaved 或 2
			if 0 == next_count_tracks(self) then
				if nil == ch or 0 > ch then
					ch = 0
				end
			else
				if nil == ch or 2 > ch then
					ch = 2
				end
			end
			self._serve:setup_tcp(ch)
		end
		RememberSetup(self, 'tcp', url, ch, 0)
		s = PackSetupOk(self, ver, cseq, self.setup_tracks[url])
	else
		code = 461
		s = rtspserve_parser.pack_res_unsupported(ver, code, cseq)
	end

	SendText(self, s)

	if 200 == code and 'options' == self.state then
		self.state = 'ready'
		InitTimeouts(self)
	end
end

-- @brief PLAY 方法
local OnMethod_PLAY = function (self, cseq, url, ver, head)
	
	local code = 200
	local s = rtspserve_parser.pack_res_PLAY(ver, code, cseq, self.session, self.timeout)
	
	-- 回复
	SendText(self, s)
	
	if 200 == code then
		self.state = 'play' -- 开始进入 play, 可以开始发送码流
	end
end

-- @brief PAUSE 方法. 暂停播放; 是否停发媒体由上层按 state 决定
local OnMethod_PAUSE = function (self, cseq, url, ver, head)
	local code = 200
	local s = rtspserve_parser.pack_res_PAUSE(ver, code, cseq, self.session, self.timeout)

	-- 回复
	SendText(self, s)

	if 200 == code then
		self.state = 'pause'
	end
end

-- @brief SET_PARAMETER 方法. 仅确认, 不改变会话状态
local OnMethod_SET_PARAMETER = function (self, cseq, url, ver, head)
	local code = 200
	local s = rtspserve_parser.pack_res_SET_PARAMETER(ver, code, cseq, self.session, self.timeout)

	-- 回复
	SendText(self, s)
end

-- @brief GET_PARAMETER 方法
local OnMethod_GET_PARAMETER = function (self, cseq, url, ver, head)
	
	local code = 200
	local s = rtspserve_parser.pack_res_GET_PARAMETER(ver, code, cseq, self.session, self.timeout)

	-- 回复
	SendText(self, s)
end

-- @brief TEARDOWN 方法
local OnMethod_TEARDOWN = function (self, cseq, url, ver, head)
	
	local code = 200
	local s = rtspserve_parser.pack_res_TEARDOWN(ver, code, cseq, self.session)

	-- 回复
	SendText(self, s)
end

-- 未识别方法: 回 405, 并按固定顺序列出可支持方法(Allow)
local OnUnsupported = function (self, cseq, url, ver, head)
	local code = 405
	local s = rtspserve_parser.pack_res_unsupported(ver, code, cseq, rtspserve_parser.methods_string(method_map))

	-- 回复
	SendText(self, s)
end


-- @brief 支持的方法
method_map = {
	['OPTIONS']			= OnMethod_OPTIONS,			-- OPTIONS 方法
	['DESCRIBE']		= OnMethod_DESCRIBE,		-- DESCRIBE 方法
	['SETUP']			= OnMethod_SETUP,			-- SETUP 方法
	['PLAY']			= OnMethod_PLAY,			-- PLAY 方法
	['PAUSE']			= OnMethod_PAUSE,			-- PAUSE 方法
	['SET_PARAMETER']	= OnMethod_SET_PARAMETER,	-- SET_PARAMETER 方法
	['GET_PARAMETER']	= OnMethod_GET_PARAMETER,	-- GET_PARAMETER 方法
	['TEARDOWN']		= OnMethod_TEARDOWN,		-- TEARDOWN 方法
}

--------------------------------------------------

-- 配置了 user 时, OPTIONS 以外的方法须通过 Basic 或 Digest. 拒绝则已回 401
-- nonce 过期则轮换并重置 nc 防护, 强制客户端重新认证
local RefuseAuth = function (self, method, url, ver, cseq, head)
	if '' == self.auth_user then
		return false
	end

	if 'OPTIONS' == method then
		return false
	end

	if not rtspauth.nonce_valid(self.nonce, self.auth_ttl) then
		self.nonce = rtspauth.gen_nonce(krand.rand_string(16))
		self.nc_ctx = rtspauth.new_nc_ctx()
	end

	if rtspauth.check(method, url or '', self.auth_user, self.auth_pass,
			self.realm, self.nonce, head['authorization'], self.nc_ctx) then
		return false
	end

	-- 按客户端已用认证方案选择 401 挑战; 无则双挑战
	SendText(self, rtspserve_parser.pack_res_unauthorized(ver, cseq, self.realm, self.nonce,
		rtspauth.scheme_of(head['authorization'])))
	return true
end


-- Session 头与 self.session 比对; 允许 "id;timeout=" 形态
local MatchSession = function (self, head)
	local raw = head and head['session']
	if 'string' ~= type(raw) or '' == raw then
		return false
	end

	local sid = string.match(raw, '^%s*([^;%s]+)')
	if nil == sid or '' == sid then
		return false
	end

	return sid == self.session
end


-- PLAY / TEARDOWN / GET_PARAMETER 及 SETUP 续轨须带匹配 Session; 否则 454
local RefuseSession = function (self, method, ver, cseq, head)
	local need = false

	if 'PLAY' == method or 'TEARDOWN' == method or 'GET_PARAMETER' == method then
		need = true
	elseif 'SETUP' == method then
		if 0 < next_count_tracks(self) then
			need = true
		end
	end

	if not need then
		return false
	end

	if MatchSession(self, head) then
		return false
	end

	SendText(self, rtspserve_parser.pack_res_unsupported(ver, 454, cseq))
	return true
end


-- 方法状态机(RFC 2326): state 为 'options'(未SETUP) / 'ready'(已SETUP) / 'play' / 'pause'
local StateAllows = function (state, method)
	if 'OPTIONS' == method or 'DESCRIBE' == method or 'GET_PARAMETER' == method or 'TEARDOWN' == method then
		return true
	elseif 'SETUP' == method then
		return ('options' == state or 'ready' == state)
	elseif 'PLAY' == method then
		return ('ready' == state or 'pause' == state)
	elseif 'PAUSE' == method then
		return ('play' == state)
	elseif 'SET_PARAMETER' == method then
		return ('ready' == state or 'play' == state or 'pause' == state)
	end

	return false
end


-- 越序请求回 455; 未实现的方法不参与状态校验(交由 OnUnsupported 回 501)
local RefuseState = function (self, method, ver, cseq, head)
	if nil == method_map[method] then
		return false
	end

	if StateAllows(self.state or 'options', method) then
		return false
	end

	SendText(self, rtspserve_parser.pack_res_unsupported(ver, 455, cseq, rtspserve_parser.methods_string(method_map)))
	return true
end


local Dispatch = function (self, method, url, ver, head, cseq)
	local func = nil

	if RefuseAuth(self, method, url, ver, cseq, head) then
		return
	end

	if RefuseSession(self, method, ver, cseq, head) then
		return
	end

	if RefuseState(self, method, ver, cseq, head) then
		return
	end

	func = method_map[method]
	if func then
		func(self, cseq, url, ver, head)
	else
		OnUnsupported(self, cseq, url, ver, head)
	end
end


local CoCtrl = function (self)
	while self._run do
		local txt, msg = CoRecv(self)
		if 'exit' == msg or nil == txt or '' == txt then
			break
		end

		local method, url, ver, head = rtspserve_parser.parse(txt)
		local cseq = head['cseq'] or '1'
		Dispatch(self, method, url, ver, head, cseq)

		if 'TEARDOWN' == method then
			break
		end
	end

	self._run = false
end


local CoDoSession = function (self)
	while true do
		-- 接收数据
		local txt, msg = CoRecv(self)
		if 'exit' == msg or nil == txt or '' == txt then
			break
		end
		
		-- 解析
		local method, url, ver, head = rtspserve_parser.parse(txt)
		local cseq = head['cseq'] or '1'

		-- 执行
		Dispatch(self, method, url, ver, head, cseq)
		
		-- 进入 'play' 后继续在另一协程收信令, 本协程返回以便发送媒体
		if 'play' == self.state then
			self._run = true
			kco.fork(function ()
				CoCtrl(self)
			end)
			break
		end
	end
end


--------------------------------------------------------------------------------------------
-- rtspserve 对外接口

local rtspserve = {}


-- @brief 断开连接
function rtspserve:disconnect()
	self._run = false
	if self._serve then
		self._serve:disconnect()
		self._serve = nil
	end	
end

function rtspserve:co_do_session()
	
	return CoDoSession(self)	
end

-- @brief 发送 媒体数据
function rtspserve:send_media(ptr)
	if self._serve then
		SendMedia(self, ptr)
	end
end

-- @brief DESCRIBE 之前写入真实 H264. h264 为 {pt, sps, pps}, sps/pps 为 base64
-- @note 同时把 pt 同步到 C 层, 保证 RTP 实际载荷类型与 SDP 宣告一致
function rtspserve:set_h264(h264)
	if 'table' == type(h264) then
		self.h264 = h264
		if self._serve and self._serve.set_video_pt then
			self._serve:set_video_pt(tonumber(h264.pt) or 96)
		end
	end
end

-- @brief DESCRIBE 之前写入 AAC. aac 为 {pt, rate, channels, config}
-- @note 同时把 pt 同步到 C 层; 未配置音频时关闭 C 层音频载荷
function rtspserve:set_aac(aac)
	if 'table' == type(aac) then
		self.aac = aac
		if self._serve and self._serve.set_audio_pt then
			self._serve:set_audio_pt(tonumber(aac.pt) or 97)
		end
	elseif self._serve and self._serve.set_audio_pt then
		self.aac = nil
		self._serve:set_audio_pt(0)
	end
end


--------------------------------------------------------------------------------------------
-- rtspserver

local rtspserver = {}


-- @brief 新建一个 服务连接 
rtspserver.new = function (conn, cfg)	
	local h264 = nil
	local aac = nil
	local tracks = nil
	local auth_user = ''
	local auth_pass = ''
	local realm = 'klbrtsp'

	if 'table' == type(cfg) then
		if 'table' == type(cfg.h264) then
			h264 = cfg.h264
		end
		if 'table' == type(cfg.aac) then
			aac = cfg.aac
		end
		if 'table' == type(cfg.tracks) then
			tracks = cfg.tracks
		end
		if 'string' == type(cfg.user) then
			auth_user = cfg.user
		end
		if 'string' == type(cfg.password) then
			auth_pass = cfg.password
		elseif 'string' == type(cfg.pass) then
			auth_pass = cfg.pass
		end
		if 'string' == type(cfg.realm) and '' ~= cfg.realm then
			realm = cfg.realm
		end
	end

	local obj = {
		_serve = conn,						-- C/C++ 提供的服务连接
		
		state = 'options',					-- 状态 'options'(未SETUP) / 'ready'(已SETUP) / 'play' / 'pause'
		_run = false,						-- play 后信令协程
		
		session = krand.rand_string(16),	-- 会话ID
		timeout = 60,						-- 会话超时(秒); 声明于 Session 头, SETUP 后转为服务端空闲回收

		h264 = nil,							-- DESCRIBE 用的 SPS/PPS
		aac = nil,							-- DESCRIBE 用的 AAC (可选)
		tracks = tracks,					-- DESCRIBE 用的多轨列表 (可选)
		setup_tracks = {},					-- url -> {mode,arg,sport}

		auth_user = auth_user,
		auth_pass = auth_pass,
		realm = realm,
		nonce = rtspauth.gen_nonce(krand.rand_string(16)),	-- 带时间戳, 可轮换
		auth_ttl = 300,						-- nonce 有效期(秒)
		nc_ctx = rtspauth.new_nc_ctx(),		-- Digest nc 重放防护
	}
	
	setmetatable(obj, {
		__index = rtspserve,
		__tostring = function(self)
			return tostring(self._serve)
		end
	})
	
	-- init: 写入媒体参数并把 RTP PT 同步到 C 层
	if nil ~= h264 then
		obj:set_h264(h264)
	end
	if nil ~= aac then
		obj:set_aac(aac)
	end
	if nil ~= tracks and nil ~= obj._serve and obj._serve.set_track_pt then
		-- 多轨: 按轨索引写入 C 层载荷类型/音频属性
		for i, item in ipairs(tracks) do
			if 'table' == type(item) then
				local is_audio = (true == item.is_audio) or ('audio' == item.kind)
				obj._serve:set_track_pt(i - 1, tonumber(item.pt) or 0, is_audio)
			end
		end
	end

	return obj
end


return rtspserver
