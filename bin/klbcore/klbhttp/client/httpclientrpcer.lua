--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   httpclientrpcer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP 客户端
-- @note   HTTP 协议 - RPC; 内部使用 khttp.connect_rpc
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local khttp = require("khttp")
local kurl = require("kurl")
local cjson = require("cjson.safe")
local stringex = require("klbcore.util.stringex")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}

local CONST_RPC_LUA		= 0x10		-- KLB_MNP_RPC_LUA
local CONST_RPC_JSON	= 0x11		-- KLB_MNP_RPC_JSON


-- @brief 公开常量是字符串, 线上类型是整数
local RpcType = function (v)
	if CONST_RPC_JSON == v or 'JSON' == v then
		return CONST_RPC_JSON
	end

	return CONST_RPC_LUA
end


-- @brief JSON 数组还原成多个返回值
local DecodeJsonArgs = function (s)
	if 'string' ~= type(s) then
		return s
	end

	local args = cjson.decode(s)
	if 'table' ~= type(args) then
		return args
	end

	return table.unpack(args)
end


--------------------------------------------------------------------------------------------
-- 内部实现


-- @brief POST-RPC
local PostRpc = function (self, ...)
	local client = self._client
	if not client then
		return 1
	end

	local rpctype = RpcType(self.rpctype)

	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)

		return client:post(rpctype, s)
	end

	-- CONST_RPC_LUA
	return client:post(rpctype, ...)
end

-- @brief NOTIFY-RPC
local NotifyRpc = function (self, ...)
	local client = self._client
	if not client then
		return 1
	end

	local rpctype = RpcType(self.rpctype)

	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)

		return client:notify(rpctype, s)
	end

	-- CONST_RPC_LUA
	return client:notify(rpctype, ...)
end


-- @brief call
local CoCall = function (self, ...)
	local client = self._client
	if not client then
		return
	end

	local rpctype = RpcType(self.rpctype)

	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)

		return DecodeJsonArgs(client:co_call(rpctype, s))
	end

	-- CONST_RPC_LUA
	return client:co_call(rpctype, ...)
end

-- @brief wait
local CoWait = function (self)
	local client = self._client
	if not client then
		return
	end

	client:co_wait()
end

--------------------------------------------------------------------------------------------
-- httpclientrpc

local httpclientrpc = {}


-- @brief 断开连接
function httpclientrpc:disconnect()
	if self._client then
		self._client:disconnect()
		self._client = nil
	end
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] `{ tls }`
-- @return [number]	0 成功; 非0 失败
function httpclientrpc:connect_rpc(host, port, opts)
	local client, code = khttp.connect_rpc(tostring(host), tonumber(port), opts)
	if not client then
		self._last_code = code
		return code or 1
	end

	self._last_code = nil
	self._client = client

	return 0
end

-- @brief 连接到目标
-- @param [in]      url[string]			eg. `httprpc://127.0.0.1:3458`
-- @return [number]	0 成功; 非0 失败
-- @note 只取 host, port, schema. httprpcs 走 TLS. 缺省端口 3458
function httpclientrpc:connect(url)
	-- eg. httprpc://127.0.0.1:3458
	local u = kurl.parse(url)

	local schema = u['schema'] or 'httprpc'
	local host = u['host'] or ''
	local port = '3458'
	local tls = false

	if stringex.cmp_ignore_case('httprpcs', schema) then
		tls = true
	end

	if u['port'] then
		port = u['port']
	end

	return self:connect_rpc(host, port, {
		tls = tls,
	})
end


-- @brief 发送文本数据
function httpclientrpc:post(...)
	return PostRpc(self, ...)
end


-- @brief notify - RPC 数据
function httpclientrpc:notify(...)
	return NotifyRpc(self, ...)
end


-- @brief 接收数据
function httpclientrpc:co_recv()
	if not self._client then
		return
	end

	if CONST_RPC_JSON == RpcType(self.rpctype) then
		return DecodeJsonArgs(self._client:co_recv())
	end

	return self._client:co_recv()
end


-- @brief 调用远程函数
function httpclientrpc:co_call(...)
	return CoCall(self, ...)
end

-- @brief 等待发送完成
function httpclientrpc:co_wait()
	CoWait(self)
end

--------------------------------------------------------------------------------------------
-- httpclientrpcer

local httpclientrpcer = {}


-- @brief 新建一个 HTTP-RPC 客户端
httpclientrpcer.new_rpc = function (cfg)
	local rpctype = CONST_RPC_LUA
	if cfg and cfg.rpctype then
		rpctype = RpcType(cfg.rpctype)
	end

	local obj = {
		_client = nil,						-- C 提供的客户端连接

		rpctype = rpctype,					-- RPC 数据组织方式
	}

	setmetatable(obj, {
		__index = httpclientrpc,
		__tostring = function(self)
			return tostring(self._client)
		end
	})

	return obj
end


return httpclientrpcer
