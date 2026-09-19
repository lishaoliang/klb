--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   httplistener.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP 服务 监听
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local khttp = require("khttp")
local httpserver = require("klbcore.klbhttp.serve.httpserver")


--------------------------------------------------------------------------------------------
-- 前置定义
local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现





--------------------------------------------------------------------------------------------
-- httplisten 对外接口

local httplisten = {}

-- @brief 关闭监听
function httplisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
function httplisten:open(port)
	-- 关闭
	self:close()

	-- 开启监听
	self._listen = khttp.listen(port)
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @note 须在 kco 协程内
function httplisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn = self._listen:co_accept()
	if not conn then
		return nil
	end

	return httpserver.new(conn)
end


--------------------------------------------------------------------------------------------
-- httplistener

local httplistener = {}


-- @brief 新建一个监听模块
httplistener.new = function (cfg)
	local obj = {
		_listen = nil,						-- C 提供的监听模块
	}

	setmetatable(obj, {
		__index = httplisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})

	return obj
end


return httplistener
