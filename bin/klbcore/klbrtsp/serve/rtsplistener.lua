--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   rtsplistener.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  RTSP服务 监听
-- @history 修改历史
--		[2025-10] 添加基础监听
--]]
local krtsp = require("krtsp")
local rtspserver = require("klbcore.klbrtsp.serve.rtspserver")

local rtsplistener = {}
local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现


--------------------------------------------------------------------------------------------
-- 对外接口

local rtsplisten = {}

-- @brief 关闭监听
function rtsplisten:close()
	if self._listen then
		self._listen:close()
	end
end

-- @brief 开始监听端口
function rtsplisten:open(port)
	-- 关闭
	rtsplisten:close()
	
	-- 开启监听
	self._listen = krtsp.listen(port)
end

-- @brief 接收 新连接
function rtsplisten:co_accept()
	
	-- C/Lua 协程里面, 暂未找到直接 返回userdata的方法
	-- 这里 先返回指针, 再通过接口函数新建
	local ptr, msg = self._listen:co_accept()
	
	local conn = krtsp.new_serve(ptr)
	
	return rtspserver.new(conn)
end

-- @brief 新建一个监听模块
rtsplistener.new = function (cfg)
	local obj = {
		_listen = nil,				-- C/C++ 提供的监听模块
	}
	
	setmetatable(obj, {
		__index = rtsplisten,
		__tostring = function(self)
			return self._listen
        end
	})
	
	return obj
end

return rtsplistener
