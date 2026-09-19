--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   mnplistener.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  MNP 服务 监听
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kmnp = require("kmnp")
local mnpserver = require("klbcore.klbmnp.serve.mnpserver")


--------------------------------------------------------------------------------------------
-- 前置定义
local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现




--------------------------------------------------------------------------------------------
-- mnplisten 对外接口

local mnplisten = {}

-- @brief 关闭监听
function mnplisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
function mnplisten:open(port)
	-- 关闭
	self:close()

	-- 开启监听
	self._listen = kmnp.listen(port)
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @note 须在 kco 协程内
function mnplisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn = self._listen:co_accept()
	if not conn then
		return nil
	end

	return mnpserver.new(conn)
end


--------------------------------------------------------------------------------------------
-- mnplistener

local mnplistener = {}


-- @brief 新建一个监听模块
mnplistener.new = function (cfg)
	local obj = {
		_listen = nil,						-- C 提供的监听模块
	}

	setmetatable(obj, {
		__index = mnplisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})

	return obj
end


return mnplistener
