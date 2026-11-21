--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   smpclientrpcer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  SMP 客户端
-- @note   SMP协议 - RPC
-- @history 修改历史
--		[2025-10] 添加基础
--]]
local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local net = klbcode.net
local ksmp = require("ksmp")
local kurl = require("kurl")
local cjson = require("cjson.safe")
local stringex = require("klbcore.util.stringex")
local deadline = require("klbcore.klbsmp.deadline")


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


-- @brief 拆分 URL userinfo 为 `{ user, pass }`
-- @param [in]      info[string|nil]		kurl 的 userinfo
-- @return [table|nil]					无 userinfo 为 nil
local SplitUserinfo = function (info)
	if "string" ~= type(info) or "" == info then
		return nil
	end

	local user, pass = string.match(info, "^(.-):(.*)$")
	if nil == user then
		user = info
		pass = ""
	end

	return { user = user, pass = pass }
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

	local rets
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

		rets = DecodeJsonArgs(raw)
	else
		-- CONST_RPC_LUA
		rets = client:co_call(rpctype, ...)
	end

	if "table" == type(rets) and true == rets.__error then
		return nil, rets.code, rets.message
	end

	return rets
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
-- smpclientrpc

local smpclientrpc = {}


-- @brief 断开连接
function smpclientrpc:disconnect()
	if self._client then
		self._client:disconnect()
		self._client = nil
	end	
end


-- @brief 设置单次调用 deadline
-- @param [in]      ms[number]		毫秒; nil 或 <=0 不限
function smpclientrpc:set_timeout(ms)
	self._timeout_ms = tonumber(ms)
end


-- @brief 取消当前调用并断开
function smpclientrpc:cancel()
	self._cancelled = true
	self:disconnect()
end


-- @brief 连接到目标
-- @param [in]      host[string]
-- @param [in]      port[number]
-- @param [in]      opts[table|nil]		[可选] `{ tls[boolean] }`
-- @return [number]	0 成功; 非0 为 klb_netcode_e
function smpclientrpc:connect_rpc(host, port, opts)
	local client, code = ksmp.connect_rpc(tostring(host), tonumber(port), opts)
	if not client then
		self._last_code = code or 1
		return self._last_code
	end

	-- 更新
	self._client = client
	self._last_code = 0
	self._cancelled = false

	return 0
end

-- @brief 是否已连接
-- @return ok[boolean]
function smpclientrpc:is_open()
	return nil ~= self._client
end

-- @brief 连接到目标
-- @param [in]      url[string]			eg. 'smprpcs://127.0.0.1:3457' (smprpcs 启用 tls)
-- @return [number]	0 成功; 非0 为 klb_netcode_e
-- @note userinfo(`user:pass@`) 记为 `_auth`, 供 klbrpc 鉴权通道取用(见 P1-4)
function smpclientrpc:connect(url)
	-- eg. smprpc://username:password@127.0.0.1:3457
	local u = kurl.parse(url)
	
	local schema = u['schema'] or 'smprpc'
	local host = u['host'] or ''	
	local port = '3457'
	local opts = nil

	if stringex.cmp_ignore_case('smprpcs', schema) then
		opts = { tls = true }
	end
	
	if u['port'] then
		port = u['port']
	end

	local auth = SplitUserinfo(u['userinfo'])
	if auth then
		self._auth = auth
	end
	
	return self:connect_rpc(host, port, opts)
end


-- @brief URL userinfo 解析出的凭据
-- @return [table|nil]	`{ user, pass }`; 无 userinfo 为 nil
function smpclientrpc:get_auth()
	return self._auth
end


-- @brief 发送文本数据
function smpclientrpc:post(...)
	return PostRpc(self, ...)
end


-- @brief notify - RPC 数据
function smpclientrpc:notify(...)
	return NotifyRpc(self, ...)
end


-- @brief 接收数据
function smpclientrpc:co_recv()
	if not self._client then
		return
	end

	if CONST_RPC_JSON == RpcType(self.rpctype) then
		return DecodeJsonArgs(self._client:co_recv())
	end

	return self._client:co_recv()
end


-- @brief 调用远程函数
function smpclientrpc:co_call(...)
	return CoCall(self, ...)
end

-- @brief 等待发送完成
function smpclientrpc:co_wait()
	CoWait(self)
end

--------------------------------------------------------------------------------------------
-- smpclientrpcer

local smpclientrpcer = {}


-- @brief 新建一个 SMP-RPC 客户端
smpclientrpcer.new_rpc = function (cfg)
	local rpctype = CONST_RPC_LUA
	if cfg and cfg.rpctype then
		rpctype = RpcType(cfg.rpctype)
	end

	local obj = {
		_client = nil,						-- C/C++ 提供的客户端连接
		
		rpctype = rpctype,					-- RPC 数据组织方式
		_last_code = 0,						-- 最近一次 connect 失败码
		_timeout_ms = nil,					-- 单次调用 deadline (毫秒); nil 不限
		_cancelled = false,					-- 已取消标记
		_auth = nil,						-- URL userinfo 凭据 `{ user, pass }`; 供 klbrpc 鉴权
	}

	if cfg and cfg.timeout_ms then
		obj._timeout_ms = tonumber(cfg.timeout_ms)
	end
	
	setmetatable(obj, {
		__index = smpclientrpc,
		__tostring = function(self)
			return tostring(self._client)
		end
	})
	
	return obj
end


return smpclientrpcer
