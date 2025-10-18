--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rtspclienter.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klb rtsp client
-- @note   rtsp 客户端
-- @history 修改历史
--]]
local krtsp = require("krtsp")
local kurl = require("kurl")
local rtspsdper = require("klbcore.klbrtsp.client.rtspsdper")


local rtspclienter = {}
local E = {}


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


-- 请求 OPTIONS
local req_options = function (self)
	local client = self._client
	
	local url = self.url
	local cseq = get_cseq(self)
	
	-- 发送请求
	local options = rtspsdper.pack_OPTIONS(url, cseq)
	client:send_text(options)
	
	-- 接收请求
	local msg, txt = client:co_recv()
	
	print('OPTIONS', txt)
	
	local status, header = rtspsdper.parse(txt)
	
	--print('OPTIONS', status)
end


-- 请求 DESCRIBE
local req_describe = function (self)
	local client = self._client
	
	local url = self.url
	local cseq = get_cseq(self)
	
	-- 发送请求
	local describe = rtspsdper.pack_DESCRIBE(url, cseq)
	client:send_text(describe)

	-- 接收请求
	local msg, txt = client:co_recv()
	
	print('DESCRIBE', txt)
	
	local status, header, body = rtspsdper.parse(txt)
	
	--print('DESCRIBE', status, body)
	
	local sdp, sdp_video, sdp_audio = rtspsdper.parse_sdp(body)
	
	self.video = sdp_video
	self.audio = sdp_audio
end


-- 请求 视频
local req_setup_video = function (self)	
	local client = self._client
	
	local url = self.video[1].url
	local cseq = get_cseq(self)

	local s = rtspsdper.pack_SETUP(url, cseq)
	client:send_text(s)
	
	-- 接收请求
	local msg, txt = client:co_recv()
	
	print('SETUP', txt)
	
	local status, header = rtspsdper.parse(txt)
	local session = header['session'] -- todo
	
	if '' == self.session then
		self.session = session
	end
end

-- PLAY
local req_play = function (self)
	local client = self._client
	
	local url = self.url
	local cseq = get_cseq(self)
	
	local play = rtspsdper.pack_PLAY(url, cseq, self.session)
	client:send_text(play)
	
	-- 接收请求
	local msg, txt = client:co_recv()
	
	print('PLAY', txt)
	
	rtspsdper.parse(txt)
end

-- GET_PARAMETER
local req_get_parameter = function (self)
	local client = self._client
	
	local url = self.url
	local cseq = get_cseq(self)
	
	local s = rtspsdper.pack_GET_PARAMETER(url, cseq, self.session)
	client:send_text(s)
	
	-- 接收请求
	local msg, txt = client:co_recv()
	
	print('GET_PARAMETER', txt)
	
	rtspsdper.parse(txt)
end

-- TEARDOWN
local req_teardown = function (self)
	local client = self._client
	
	local url = self.url
	local cseq = get_cseq(self)
	
	local s = rtspsdper.pack_TEARDOWN(url, cseq, self.session)
	client:send_text(s)
	
	-- 接收请求
	local msg, txt = client:co_recv()
	
	print('TEARDOWN', txt)
	
	rtspsdper.parse(txt)
end

--------------------------------------------------------------------------------------------
-- 对外接口

local rtspclient = {}


-- @brief 断开连接
function rtspclient:disconnect()
	if self._client then
		self._client:disconnect()	
		self._client = nil
	end	
end


-- @brief 连接到目标
function rtspclient:co_connect(url)	
	-- eg. http://username:password@127.0.0.1:8080/test/test.aspx?name=sviergn&x=true#stuff
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
		return 1 -- 连接失败
	end
	
	-- 更新
	self._client = client
	self.url = url
	
	-- 请求 OPTIONS
	req_options(self)
	
	-- 请求 DESCRIBE
	req_describe(self)	
	
	-- 请求 SETUP
	req_setup_video(self)
	
	--
	req_get_parameter(self)
	
	-- 请求 PLAY
	req_play(self)
	
	
	-- 
	--req_teardown(self)
	
	
	return 0
end


-- @brief 新建一个rtsp客户端
rtspclienter.new = function (cfg)
	local obj = {
		_client = nil,		-- C/C++ 提供的客户端连接
		
		cseq = 1,			-- 请求序号
		
		url = '',			-- 基础 url	
		session = '',		-- 会话标识
		
		video = {},			-- 视频列表
		audio = {},			-- 音频列表
	}
	
	setmetatable(obj, {
		__index = rtspclient,
		__tostring = function(self)
			return self._client
        end
	})
	
	return obj
end


return rtspclienter
