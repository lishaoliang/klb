--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   mnpclientrpcer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  MNP 客户端
-- @note   MNP协议 - RPC
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local net = klbcode.net
local kmnp = require("kmnp")
local kurl = require("kurl")
local cjson = require("cjson.safe")
local deadline = require("klbcore.klbmnp.deadline")


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


-- @brief 捕获多返回值
-- @param [in]      ...[any]
-- @return [table]					`{ n, ... }`
local capture = function (...)
	return { n = select("#", ...), ... }
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


-- @brief call (单次)
local CallOnce = function (self, ...)
	local client = self._client
	if not client then
		return nil, net.CLOSED, "closed"
	end

	local rpctype = RpcType(self.rpctype)

	if CONST_RPC_JSON == rpctype then
		local args = {...}
		local s = cjson.encode(args)
		local raw = client:co_call(rpctype, s)

		if "string" == type(raw) then
			local obj = cjson.decode(raw)
			if "table" == type(obj) and true == obj.__error then
				return nil, obj.code, obj.message
			end
		end

		return DecodeJsonArgs(raw)
	end

	-- CONST_RPC_LUA
	local rets = capture(client:co_call(rpctype, ...))
	if 0 < rets.n and "table" == type(rets[1]) and true == rets[1].__error then
		return nil, rets[1].code, rets[1].message
	end

	return table.unpack(rets, 1, rets.n)
end


-- @brief call (带 deadline / 取消)
-- @note 到期以 disconnect 唤醒阻塞的 co_call; 返回 `nil, E_TIMEOUT`
local CoCall = function (self, ...)
	if self._cancelled then
		return nil, base.CANCELED, "cancelled"
	end

	local state = deadline.arm(self._timeout_ms, function ()
		if "function" == type(self.disconnect) then
			pcall(function () self:disconnect() end)
		end
	end)

	local rets = capture(CallOnce(self, ...))
	local expired = deadline.finish(state)

	if self._cancelled then
		return nil, base.CANCELED, "cancelled"
	end

	if expired then
		return nil, net.TIMEOUT, "timeout"
	end

	return table.unpack(rets, 1, rets.n)
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
-- mnpclientrpc

local mnpclientrpc = {}


-- @brief 断开连接
function mnpclientrpc:disconnect()
	if self._client then
		self._client:disconnect()
		self._client = nil
	end
end


-- @brief 设置单次调用 deadline
-- @param [in]      ms[number]		毫秒; nil 或 <=0 不限
function mnpclientrpc:set_timeout(ms)
	self._timeout_ms = tonumber(ms)
end


-- @brief 取消当前调用并断开
function mnpclientrpc:cancel()
	self._cancelled = true
	self:disconnect()
end


-- @brief 连接到目标
-- @param [in]      host[string]
-- @param [in]      port[number]
-- @param [in]      opts[table|nil]		[可选] `{ tls[boolean] }` (mnprpcs)
-- @return [number]	0 成功; 非0 为 klb_netcode_e
function mnpclientrpc:connect_rpc(host, port, opts)
	local client, code = kmnp.connect_rpc(tostring(host), tonumber(port), opts)
	if not client then
		self._last_code = code or 1
		self._last_error = klbcode.strerror(code or base.ERR)
		return self._last_code
	end

	self._client = client
	self._last_code = 0
	self._last_error = nil
	self._cancelled = false

	return 0
end


-- @brief 最近一次错误码 / 描述
-- @return [number|nil] code
-- @return [string|nil] err
function mnpclientrpc:last_error_of()
	return self._last_code, self._last_error
end

-- @brief 是否已连接
-- @return ok[boolean]
function mnpclientrpc:is_open()
	return nil ~= self._client
end

-- @brief 连接到目标
-- @param [in]      url[string]			eg. 'mnprpc://127.0.0.1:3456' (mnprpcs 启用 tls)
-- @note 只取 host 与 port; 默认端口 3456
function mnpclientrpc:connect(url)
	-- eg. mnprpc://127.0.0.1:3456
	local u = kurl.parse(url)
	local host = u['host'] or ''
	local port = '3456'
	local opts = nil

	local schema = string.lower(tostring(u['schema'] or ''))
	if 'mnprpcs' == schema then
		opts = { tls = true }
	end

	if u['port'] then
		port = u['port']
	end

	return self:connect_rpc(host, port, opts)
end


-- @brief 发送文本数据
function mnpclientrpc:post(...)
	return PostRpc(self, ...)
end


-- @brief notify - RPC 数据
function mnpclientrpc:notify(...)
	return NotifyRpc(self, ...)
end


-- @brief 接收数据
function mnpclientrpc:co_recv()
	if not self._client then
		return "error", self._last_code or 1, self._last_error
	end

	if CONST_RPC_JSON == RpcType(self.rpctype) then
		return DecodeJsonArgs(self._client:co_recv())
	end

	local msg, head, body = self._client:co_recv()
	if "error" == msg then
		self._last_code = head or 1
		self._last_error = klbcode.strerror(base.IO)
		return "error", self._last_code, self._last_error
	end

	return msg, head, body
end


-- @brief 调用远程函数
function mnpclientrpc:co_call(...)
	return CoCall(self, ...)
end

-- @brief 等待发送完成
function mnpclientrpc:co_wait()
	CoWait(self)
end

-- @brief 当前 RPC 状态 (含最近请求/回应头)
-- @return [table|nil]					无连接时为 nil
function mnpclientrpc:status()
	if not self._client then
		return nil
	end

	return self._client:status()
end

--------------------------------------------------------------------------------------------
-- mnpclientrpcer

local mnpclientrpcer = {}


-- @brief 新建一个 MNP-RPC 客户端
-- @param [in]      cfg[table]			[可选] `{ rpctype, timeout_ms }`
mnpclientrpcer.new_rpc = function (cfg)
	local rpctype = CONST_RPC_LUA
	if cfg and cfg.rpctype then
		rpctype = RpcType(cfg.rpctype)
	end

	local obj = {
		_client = nil,						-- C 提供的客户端连接
		_cfg = cfg,							-- 构造配置; rpctype 已展开到字段
		_last_code = 0,						-- 最近一次 connect 失败码
		_last_error = nil,					-- 最近一次错误描述
		_timeout_ms = nil,					-- 单次调用 deadline (毫秒); nil 不限
		_cancelled = false,					-- 已取消标记

		rpctype = rpctype,					-- RPC 数据组织方式
	}

	if cfg and cfg.timeout_ms then
		obj._timeout_ms = tonumber(cfg.timeout_ms)
	end

	setmetatable(obj, {
		__index = mnpclientrpc,
		__tostring = function(self)
			return tostring(self._client)
		end
	})

	return obj
end


return mnpclientrpcer
