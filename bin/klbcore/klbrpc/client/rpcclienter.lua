--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client/rpcclienter.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbrpc 长期 RPC 客户端
-- @note   按 URL schema 转到协议包 new_client_rpc / connect; 连接对象可复用
-- @history 修改历史
--		[2026-10] 创建文件
--]]
local kurl = require("kurl")
local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local net = klbcode.net
local deadline = require("klbcore.klbrpc.deadline")


local MOD_BY_SCHEMA = {
	smprpc = "klbcore.klbsmp",
	smprpcs = "klbcore.klbsmp",
	mnprpc = "klbcore.klbmnp",
	httprpc = "klbcore.klbhttp",
	httprpcs = "klbcore.klbhttp",
	wsrpc = "klbcore.klbws",
	wsrpcs = "klbcore.klbws",
}


local rpcclienter = {}


-- @brief 按 URL schema 加载协议包
-- @param [in]      url[string]			eg. `smprpc://127.0.0.1:3457`
-- @return [table]						协议包模块; 失败为 nil
local function ApiForUrl(url)
	local u = kurl.parse(tostring(url or "")) or {}
	local schema = u["schema"]
	if "string" ~= type(schema) then
		return nil
	end

	local name = MOD_BY_SCHEMA[string.lower(schema)]
	if not name then
		return nil
	end

	local ok, api = pcall(require, name)
	if not ok or "table" ~= type(api) then
		return nil
	end

	if "function" ~= type(api.new_client_rpc) then
		return nil
	end

	return api
end


-- @brief 捕获多返回值
-- @param [in]      ...[any]
-- @return [table]					`{ n, ... }`
local function capture(...)
	return { n = select("#", ...), ... }
end


-- @brief URL userinfo → 凭据
-- @param [in]      info[string|nil]
-- @return [table|nil]				`{ user, pass }`; 空为 nil
local function AuthFromUserinfo(info)
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


-- @brief 取鉴权令牌
-- @param [in]      auth[table|nil]	`{ token|user, pass }`
-- @return [string|nil]
local function AuthToken(auth)
	if "table" ~= type(auth) then
		return nil
	end

	local token = auth.token
	if "string" == type(token) and "" ~= token then
		return token
	end

	local user = auth.user
	if "string" == type(user) and "" ~= user then
		return user .. ":" .. tostring(auth.pass or "")
	end

	return nil
end


-- @brief 连接后内置鉴权 (`__auth` 保留方法)
-- @param [in]      client[table]			协议包 RPC 客户端
-- @param [in]      auth[table|nil]			`{ token|user, pass }`
-- @return ok[boolean]
-- @return [number]							失败时错误码
-- @return [string]							失败时 errstr
local function Authenticate(client, auth)
	local token = AuthToken(auth)
	if nil == token then
		return true
	end

	if "function" ~= type(client.co_call) then
		return true
	end

	local rets = capture(client:co_call("__auth", token))
	if 0 == rets.n then
		return false, base.ACCES, "auth failed"
	end

	if nil ~= rets[1] and false ~= rets[1] then
		return true
	end

	return false, rets[2] or base.ACCES, tostring(rets[3] or "auth failed")
end


-- @brief 带 deadline 的连接
-- @param [in]      client[table]			协议包 RPC 客户端
-- @param [in]      url[string]
-- @param [in]      timeout_ms[number|nil]	毫秒; nil 或 <=0 不限
-- @return [number]							0 成功; 非0 为错误码
local function ConnectWithDeadline(client, url, timeout_ms)
	local state = deadline.arm(timeout_ms, function ()
		if "function" == type(client.disconnect) then
			pcall(function () client:disconnect() end)
		end
	end)

	local rc = client:connect(url)
	if deadline.finish(state) then
		return net.TIMEOUT
	end

	return rc
end


-- @brief 为长期客户端挂 deadline 与取消能力
-- @param [in,out]  client[table]			协议包 RPC 客户端
-- @param [in]      timeout_ms[number|nil]	毫秒; nil 或 <=0 不限
-- @return [table]							原 client
-- @note 到期以 disconnect 唤醒阻塞的 co_call; 之后返回 `nil, net.TIMEOUT`
local function AttachControl(client, timeout_ms)
	local raw_call = client.co_call
	if "function" ~= type(raw_call) then
		return client
	end

	client._timeout_ms = tonumber(timeout_ms)
	client._cancelled = false

	client.set_timeout = function (self, ms)
		self._timeout_ms = tonumber(ms)
	end

	client.cancel = function (self)
		self._cancelled = true

		if "function" == type(self.disconnect) then
			pcall(function () self:disconnect() end)
		end
	end

	client.co_call = function (self, ...)
		if self._cancelled then
			return nil, base.CANCELED, "cancelled"
		end

		local state = deadline.arm(self._timeout_ms, function ()
			if "function" == type(self.disconnect) then
				pcall(function () self:disconnect() end)
			end
		end)

		local rets = capture(raw_call(self, ...))
		local expired = deadline.finish(state)

		if self._cancelled then
			return nil, base.CANCELED, "cancelled"
		end

		if expired then
			return nil, net.TIMEOUT, "timeout"
		end

		return table.unpack(rets, 1, rets.n)
	end

	return client
end


-- @brief 新建未连接的 RPC 客户端 (须已知 URL schema 所属协议包)
-- @param [in]      url[string]			用于选定协议包; 不在此连接
-- @param [in]      cfg[table]			[可选] 交给协议包 `new_client_rpc`
-- @return [table]						协议包 RPC 客户端; 失败为 nil
-- @return [number]						失败时 knetcode
-- @return [string]						失败时 errstr
-- @note 连接请再调 `:connect(url)` 或本模块 `connect`
rpcclienter.new_rpc = function (url, cfg)
	local api = ApiForUrl(url)
	if not api then
		return nil, net.INVAL, "unsupported rpc schema"
	end

	return api.new_client_rpc(cfg)
end


-- @brief 按 URL 连接并返回可长期复用的 RPC 客户端
-- @param [in]      url[string]			eg. `httprpc://127.0.0.1:3458`
-- @param [in]      cfg[table]			[可选] 交给协议包 `new_client_rpc`; `timeout_ms` 为单次调用 deadline
-- @return [table]						已连接客户端; 失败为 nil
-- @return [number]						失败时 knetcode
-- @return [string]						失败时 errstr
-- @note 对象方法同协议包: `:co_call` / `:post` / `:notify` / `:disconnect`; 另挂 `:set_timeout` / `:cancel`
rpcclienter.connect = function (url, cfg)
	local api = ApiForUrl(url)
	if not api then
		return nil, net.INVAL, "unsupported rpc schema"
	end

	local client = api.new_client_rpc(cfg)
	if not client then
		return nil, net.INVAL, "new_client_rpc failed"
	end

	if "function" ~= type(client.connect) then
		return nil, net.INVAL, "connect not supported"
	end

	local timeout_ms = nil
	if "table" == type(cfg) then
		timeout_ms = cfg.timeout_ms
	end

	local rc = ConnectWithDeadline(client, url, timeout_ms)
	if 0 ~= rc then
		return nil, rc or net.INVAL, "connect failed"
	end

	-- step1. 连接后内置鉴权 (URL userinfo 或 cfg.auth)
	local auth = nil
	if "table" == type(cfg) and nil ~= cfg.auth then
		auth = cfg.auth
	else
		local u = kurl.parse(tostring(url or "")) or {}
		auth = AuthFromUserinfo(u["userinfo"])
	end

	local ok_a, code_a, err_a = Authenticate(client, auth)
	if not ok_a then
		if "function" == type(client.disconnect) then
			client:disconnect()
		end

		return nil, code_a, err_a
	end

	return AttachControl(client, timeout_ms)
end


return rpcclienter
