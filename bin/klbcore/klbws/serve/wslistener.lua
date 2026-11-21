--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   wslistener.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  WebSocket 服务 监听
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kws = require("kws")
local wsserver = require("klbcore.klbws.serve.wsserver")
local wsserverpcer = require("klbcore.klbws.serve.wsserverpcer")


--------------------------------------------------------------------------------------------
-- 前置定义
local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现





--------------------------------------------------------------------------------------------
-- wslisten 对外接口

local wslisten = {}

-- @brief 关闭监听
function wslisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 是否已成功 open
-- @return ok[boolean]
function wslisten:is_open()
	return nil ~= self._listen
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] 监听选项; 默认使用 new(cfg)
-- @return ok[boolean]					true 成功; 失败为 nil
-- @return code[number]					失败时 klb_netcode_e
function wslisten:open(port, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM
	--   key[string]			tls 时必填, 私钥 PEM
	--   path[string]			[可选] 只接受该路径, 忽略 query
	--   protocol[string]		[可选] 选定的子协议
	--   host/family/max_connect/reuseport/nodelay/keepalive 同 klisten
	-- }

	-- 关闭
	self:close()

	-- 开启监听
	local listen_opts = opts
	if not listen_opts then
		listen_opts = self._cfg
	end

	local listen, code = kws.listen(port, listen_opts)
	if not listen then
		self._last_code = code
		return nil, code
	end

	self._listen = listen
	self._last_code = 0
	return true
end

function wslisten:port()
	if not self._listen or not self._listen.port then
		return 0
	end
	return self._listen:port()
end

function wslisten:local_addr()
	if not self._listen or not self._listen.local_addr then
		return nil, nil
	end
	return self._listen:local_addr()
end

function wslisten:native_fd()
	if not self._listen or not self._listen.native_fd then
		return nil
	end
	return self._listen:native_fd()
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @note 须在 kco 协程内
function wslisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn = self._listen:co_accept()
	if not conn then
		return nil
	end

	return wsserver.new(conn)
end


--------------------------------------------------------------------------------------------
-- wsrpclisten 对外接口

local wsrpclisten = {}

-- @brief 关闭监听
function wsrpclisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 是否已成功 open
-- @return ok[boolean]
function wsrpclisten:is_open()
	return nil ~= self._listen
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] 监听选项; 默认使用 new_rpc(cfg)
-- @return ok[boolean]					true 成功; 失败为 nil
-- @return code[number]					失败时 klb_netcode_e
function wsrpclisten:open(port, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM
	--   key[string]			tls 时必填, 私钥 PEM
	--   path[string]			[可选] 只接受该路径; 省略为 `/rpc`; 空串不限制
	--   protocol[string]		[可选] 选定的子协议
	--   rpctype[string]		[可选] LUA 或 JSON; 交给 accept 后的服务连接
	-- }

	self:close()

	local listen_opts = opts
	if not listen_opts then
		listen_opts = self._cfg
	end

	local resolved = {}
	if "table" == type(listen_opts) then
		for k, v in pairs(listen_opts) do
			resolved[k] = v
		end
	end

	if nil == resolved.path then
		resolved.path = "/rpc"
	end

	local listen, code = kws.listen_rpc(port, resolved)
	if not listen then
		self._last_code = code
		return nil, code
	end

	self._listen = listen
	self._last_code = 0
	return true
end

function wsrpclisten:port()
	if not self._listen or not self._listen.port then
		return 0
	end
	return self._listen:port()
end

function wsrpclisten:local_addr()
	if not self._listen or not self._listen.local_addr then
		return nil, nil
	end
	return self._listen:local_addr()
end

function wsrpclisten:native_fd()
	if not self._listen or not self._listen.native_fd then
		return nil
	end
	return self._listen:native_fd()
end

-- @brief 接收 新连接
-- @return [table]	RPC 服务连接; 无连接或退出时为 nil
-- @note 须在 kco 协程内
function wsrpclisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn = self._listen:co_accept()
	if not conn then
		return nil
	end

	return wsserverpcer.new_rpc(conn, self._cfg)
end


--------------------------------------------------------------------------------------------
-- wslistener

local wslistener = {}


-- @brief 新建一个监听模块
wslistener.new = function (cfg)
	local obj = {
		_listen = nil,						-- C 提供的监听模块
		_cfg = cfg,							-- 监听选项; tls/cert/key
		_last_code = 0,						-- 最近一次 open 失败码
	}

	setmetatable(obj, {
		__index = wslisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})

	return obj
end


-- @brief 新建一个监听 RPC 模块
wslistener.new_rpc = function (cfg)
	local obj = {
		_listen = nil,						-- C 提供的监听 RPC 模块
		_cfg = cfg,							-- accept 时交给 RPC 服务连接
		_last_code = 0,						-- 最近一次 open 失败码
	}

	setmetatable(obj, {
		__index = wsrpclisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})

	return obj
end


return wslistener
