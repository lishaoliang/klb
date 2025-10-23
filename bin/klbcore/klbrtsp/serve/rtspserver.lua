--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rtspserver.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  RTSP服务 模块
-- @history 修改历史
--		[2025-10] 添加基础RTSP服务
--]]
local krand = require("krand")
local rtspserve_parser = require("klbcore.klbrtsp.serve.rtspserve_parser")


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
	
	return txt
end

--------------------------------------------------

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
	
	local sdp = rtspserve_parser.pack_res_sdp()
	local s = rtspserve_parser.pack_res_DESCRIBE(ver, code, cseq, string.len(sdp))
	
	-- 回复
	SendText(self, s, sdp)
end

-- @brief SETUP 方法
local OnMethod_SETUP = function (self, cseq, url, ver, head)
	
	local code = 200
	
	local s = rtspserve_parser.pack_res_tcp_SETUP(ver, code, cseq, self.session, self.timeout)

	-- 回复
	SendText(self, s)
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

-- rtspserve_parser.pack_res_unsupported
local OnUnsupported = function (self, cseq, url, ver, head)
	local code = 501
	local s = rtspserve_parser.pack_res_TEARDOWN(ver, code, cseq, self.session)

	-- 回复
	SendText(self, s)
end


-- @brief 支持的方法
method_map = {
	['OPTIONS']			= OnMethod_OPTIONS,			-- OPTIONS 方法
	['DESCRIBE']		= OnMethod_DESCRIBE,		-- DESCRIBE 方法
	['SETUP']			= OnMethod_SETUP,			-- SETUP 方法
	['PLAY']			= OnMethod_PLAY,			-- PLAY 方法
	['GET_PARAMETER']	= OnMethod_GET_PARAMETER,	-- GET_PARAMETER 方法
	['TEARDOWN']		= OnMethod_TEARDOWN,		-- TEARDOWN 方法
}

--------------------------------------------------

local CoDoSession = function (self)
	local serve = self._serve

	while true do
		-- 接收数据
		local txt = CoRecv(self)	
		
		-- 解析
		local method, url, ver, head = rtspserve_parser.parse(txt)
		local cseq = head['cseq'] or '1'
		print('rtsp serve', method, url, ver, cseq)
		
		-- 执行
		local func = method_map[method]
		if func then
			func(self, cseq, url, ver, head)
		else
			OnUnsupported(self, cseq, url, ver, head)
		end
		
		-- 进入 'play' 状态后, 退出
		if 'play' == self.state then
			break
		end
	end
	
	-- 配置连接参数
	
	-- 可以开始发送媒体数据
	
end


--------------------------------------------------------------------------------------------
-- rtspserve 对外接口

local rtspserve = {}


-- @brief 断开连接
function rtspserve:disconnect()
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


--------------------------------------------------------------------------------------------
-- rtspserver

local rtspserver = {}


-- @brief 新建一个 服务连接 
rtspserver.new = function (conn, cfg)	
	local obj = {
		_serve = conn,						-- C/C++ 提供的服务连接
		
		state = 'options',					-- 状态 'options'(会话交互阶段), 'play'(开启了play)
		
		session = krand.rand_string(16),	-- 会话ID
		timeout = 60,						-- 超时时间
	}
	
	setmetatable(obj, {
		__index = rtspserve,
		__tostring = function(self)
			return tostring(self._serve)
		end
	})
	
	-- init
	
	return obj
end


return rtspserver
