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
local mnpserverpcer = require("klbcore.klbmnp.serve.mnpserverpcer")


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

-- @brief 是否已成功 open
-- @return ok[boolean]
function mnplisten:is_open()
	return nil ~= self._listen
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] host/family/max_connect/reuseport/nodelay/keepalive
-- @return ok[boolean]					true 成功; 失败为 nil
-- @return code[number]					失败时 klb_netcode_e
function mnplisten:open(port, opts)
	-- 关闭
	self:close()

	-- 开启监听
	local listen, code = kmnp.listen(port, opts)
	if not listen then
		self._last_code = code
		return nil, code
	end

	self._listen = listen
	self._last_code = 0
	return true
end

function mnplisten:port()
	if not self._listen or not self._listen.port then
		return 0
	end
	return self._listen:port()
end

function mnplisten:local_addr()
	if not self._listen or not self._listen.local_addr then
		return nil, nil
	end
	return self._listen:local_addr()
end

function mnplisten:native_fd()
	if not self._listen or not self._listen.native_fd then
		return nil
	end
	return self._listen:native_fd()
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @note 须在 kco 协程内; kmnp.listen:co_accept 可能回 (conn,"ok"), 本层只取首值
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
-- mnprpclisten 对外接口

local mnprpclisten = {}

-- @brief 关闭监听
function mnprpclisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 是否已成功 open
-- @return ok[boolean]
function mnprpclisten:is_open()
	return nil ~= self._listen
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] host/family/max_connect/reuseport/nodelay/keepalive
-- @return ok[boolean]					true 成功; 失败为 nil
-- @return code[number]					失败时 klb_netcode_e
function mnprpclisten:open(port, opts)
	-- 关闭
	self:close()

	-- 开启监听
	local listen, code = kmnp.listen_rpc(port, opts)
	if not listen then
		self._last_code = code
		return nil, code
	end

	self._listen = listen
	self._last_code = 0
	return true
end

function mnprpclisten:port()
	if not self._listen or not self._listen.port then
		return 0
	end
	return self._listen:port()
end

function mnprpclisten:local_addr()
	if not self._listen or not self._listen.local_addr then
		return nil, nil
	end
	return self._listen:local_addr()
end

function mnprpclisten:native_fd()
	if not self._listen or not self._listen.native_fd then
		return nil
	end
	return self._listen:native_fd()
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @note 须在 kco 协程内; kmnp.listen_rpc:co_accept 回单值 conn, 与普通 listen 脚本层统一
function mnprpclisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn = self._listen:co_accept()
	if not conn then
		return nil
	end

	return mnpserverpcer.new_rpc(conn, self._cfg)
end


--------------------------------------------------------------------------------------------
-- mnplistener

local mnplistener = {}


-- @brief 新建一个监听模块
mnplistener.new = function (cfg)
	local obj = {
		_listen = nil,						-- C 提供的监听模块
		_cfg = cfg,							-- 预留; 普通 listen 暂未使用
		_last_code = 0,						-- 最近一次 open 失败码
	}

	setmetatable(obj, {
		__index = mnplisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})

	return obj
end


-- @brief 新建一个监听 RPC 模块
mnplistener.new_rpc = function (cfg)
	local obj = {
		_listen = nil,						-- C 提供的监听 RPC 模块
		_cfg = cfg,							-- accept 时交给 RPC 服务连接
		_last_code = 0,						-- 最近一次 open 失败码
	}

	setmetatable(obj, {
		__index = mnprpclisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})

	return obj
end


return mnplistener
