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

-- @brief 是否已成功 open
-- @return ok[boolean]
function smplisten:is_open()
	return nil ~= self._listen
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] host/family/max_connect/reuseport/nodelay/keepalive
-- @return ok[boolean]					true 成功; 失败为 nil
-- @return code[number]					失败时 klb_netcode_e
function smplisten:open(port, opts)
	-- 关闭
	self:close()

	-- 开启监听
	local listen, code = ksmp.listen(port, opts)
	if not listen then
		self._last_code = code
		return nil, code
	end

	self._listen = listen
	self._last_code = 0
	return true
end

function smplisten:port()
	if not self._listen or not self._listen.port then
		return 0
	end
	return self._listen:port()
end

function smplisten:local_addr()
	if not self._listen or not self._listen.local_addr then
		return nil, nil
	end
	return self._listen:local_addr()
end

function smplisten:native_fd()
	if not self._listen or not self._listen.native_fd then
		return nil
	end
	return self._listen:native_fd()
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @note 须在 kco 协程内
function smplisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn = self._listen:co_accept()
	if not conn then
		return nil
	end

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

-- @brief 是否已成功 open
-- @return ok[boolean]
function smprpclisten:is_open()
	return nil ~= self._listen
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] host/family/max_connect/reuseport/nodelay/keepalive
-- @return ok[boolean]					true 成功; 失败为 nil
-- @return code[number]					失败时 klb_netcode_e
function smprpclisten:open(port, opts)
	-- 关闭
	self:close()

	-- 开启监听
	local listen, code = ksmp.listen_rpc(port, opts)
	if not listen then
		self._last_code = code
		return nil, code
	end

	self._listen = listen
	self._last_code = 0
	return true
end

function smprpclisten:port()
	if not self._listen or not self._listen.port then
		return 0
	end
	return self._listen:port()
end

function smprpclisten:local_addr()
	if not self._listen or not self._listen.local_addr then
		return nil, nil
	end
	return self._listen:local_addr()
end

function smprpclisten:native_fd()
	if not self._listen or not self._listen.native_fd then
		return nil
	end
	return self._listen:native_fd()
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @note 须在 kco 协程内
function smprpclisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn = self._listen:co_accept()
	if not conn then
		return nil
	end

	return smpserverpcer.new_rpc(conn, self._cfg)
end


--------------------------------------------------------------------------------------------
-- smplistener

local smplistener = {}


-- @brief 新建一个监听模块
smplistener.new = function (cfg)
	local obj = {
		_listen = nil,						-- C/C++ 提供的监听模块
		_cfg = cfg,							-- 预留; 普通 listen 暂未使用
		_last_code = 0,						-- 最近一次 open 失败码
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
		_cfg = cfg,							-- accept 时交给 RPC 服务连接
		_last_code = 0,						-- 最近一次 open 失败码
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
