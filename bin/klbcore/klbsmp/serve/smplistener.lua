--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   smplistener.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  SMP服务 监听
-- @history 修改历史
--		[2025-10] 添加基础监听
--]]
local ksmp = require("ksmp")
local smpserver = require("klbcore.klbsmp.serve.smpserver")
local smpserverpcer = require("klbcore.klbsmp.serve.smpserverpcer")


--------------------------------------------------------------------------------------------
-- 前置定义
local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现





--------------------------------------------------------------------------------------------
-- smplisten 对外接口

local smplisten = {}

-- @brief 关闭监听
function smplisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 开始监听端口
function smplisten:open(port)
	-- 关闭
	smplisten:close()
	
	-- 开启监听
	self._listen = ksmp.listen(port)
end

-- @brief 接收 新连接
function smplisten:co_accept()
	if not self._listen then
		return nil
	end
	
	local conn = self._listen:co_accept()
	return smpserver.new(conn)
end

--------------------------------------------------------------------------------------------
-- smprpclisten 对外接口

local smprpclisten = {}

-- @brief 关闭监听
function smprpclisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 开始监听端口
function smprpclisten:open(port)
	-- 关闭
	smprpclisten:close()
	
	-- 开启监听
	self._listen = ksmp.listen_rpc(port)
end

-- @brief 接收 新连接
function smprpclisten:co_accept()
	if not self._listen then
		return nil
	end
	
	local conn = self._listen:co_accept()
	return smpserverpcer.new_rpc(conn)
end


--------------------------------------------------------------------------------------------
-- smplistener

local smplistener = {}


-- @brief 新建一个监听模块
smplistener.new = function (cfg)
	local obj = {
		_listen = nil,						-- C/C++ 提供的监听模块
	}
	
	setmetatable(obj, {
		__index = smplisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})
	
	return obj
end


-- @brief 新建一个监听 RPC 模块
smplistener.new_rpc = function (cfg)
	local obj = {
		_listen = nil,						-- C/C++ 提供的监听 RPC 模块
	}
	
	setmetatable(obj, {
		__index = smprpclisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})
	
	return obj
end


return smplistener
