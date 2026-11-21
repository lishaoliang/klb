--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   client/rpcclient.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbrpc 一次性 RPC 客户端
-- @note   先 setup 目标; co_* 按配置转到协议包 co_*_rpc; 须在 kco 协程内
-- @history 修改历史
--		[2026-10] 创建文件
--]]
local kurl = require("kurl")
local net = require("klbcore.klbcode.netcode")
local errcode = require("klbcore.klbrpc.errcode")
local deadline = require("klbcore.klbrpc.deadline")
local cfger = require("klbcore.klbrpc.cfger")


local MOD_BY_SCHEMA = {
	smprpc = "klbcore.klbsmp",
	smprpcs = "klbcore.klbsmp",
	mnprpc = "klbcore.klbmnp",
	httprpc = "klbcore.klbhttp",
	httprpcs = "klbcore.klbhttp",
	wsrpc = "klbcore.klbws",
	wsrpcs = "klbcore.klbws",
}


local rpcclient = {}


-- 内置一份客户端目标 (setup 写入)
local client_spec = nil
local client_url = nil


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

	return api
end


-- @brief 捕获多返回值
-- @param [in]      ...[any]
-- @return [table]					`{ n, ... }`
local function capture(...)
	return { n = select("#", ...), ... }
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
		return false, errcode.E_AUTH, "auth failed"
	end

	if nil ~= rets[1] and false ~= rets[1] then
		return true
	end

	return false, rets[2] or errcode.E_AUTH, tostring(rets[3] or "auth failed")
end


-- @brief 一次性调用 (优先走可控客户端以支持 deadline)
-- @param [in]      api[table]				协议包模块
-- @param [in]      url[string]
-- @param [in]      spec[table]			`cfger.parse_client` 结果
-- @param [in]      name[string]			`call` / `post` / `notify`
-- @param [in]      ...[any]				调用参数
-- @return [string]						成功为 `success`, 之后为回应值; 失败 `nil, code, errstr`
local function OneShot(api, url, spec, name, ...)
	local cfg = {
		rpctype = spec.rpctype,
		timeout_ms = spec.timeout_ms,
	}

	local client = nil
	if "function" == type(api.new_client_rpc) then
		client = api.new_client_rpc(cfg)
	end

	if not client or "function" ~= type(client[name]) then
		local fn = api["co_" .. name .. "_rpc"]
		if "function" ~= type(fn) then
			return nil, errcode.E_ARG, "unsupported rpc schema"
		end

		return fn(url, ...)
	end

	local state = deadline.arm(spec.timeout_ms, function ()
		if "function" == type(client.disconnect) then
			pcall(function () client:disconnect() end)
		end
	end)

	local rc = client:connect(url)
	if 0 ~= rc then
		deadline.finish(state)
		client:disconnect()
		return nil, rc, "connect failed"
	end

	-- step1. 连接后内置鉴权
	local ok_a, code_a, err_a = Authenticate(client, spec.auth)
	if not ok_a then
		deadline.finish(state)
		client:disconnect()
		return nil, code_a, err_a
	end

	local rets = capture(client[name](client, ...))

	if "call" ~= name and "function" == type(client.co_wait) then
		client:co_wait()
	end

	client:disconnect()
	local expired = deadline.finish(state)

	if expired then
		return nil, errcode.E_TIMEOUT, "timeout"
	end

	if "call" == name then
		return "success", table.unpack(rets, 1, rets.n)
	end

	return "success"
end


-- @brief 取已配置的目标 URL
-- @return [string]						URL; 未配置为 nil
local function ConfiguredUrl()
	return client_url
end


-- @brief 配置一次性客户端目标; 可多次; 后写覆盖
-- @param [in]      cfg[table]			见 cfger.parse_client
-- @return ok[boolean]					非法为 false
rpcclient.setup = function (cfg)
	local spec, ok = cfger.parse_client(cfg)
	if not ok or not spec then
		return false
	end

	local url = cfger.build_client_url(spec)
	if not url then
		return false
	end

	client_spec = spec
	client_url = url
	return true
end


-- @brief 当前客户端规格 (只读视图)
-- @return [table]						规范化 spec; 未配置为 nil
rpcclient.get_spec = function ()
	return client_spec
end


-- @brief 当前目标 URL
-- @return [string]						URL; 未配置为 nil
rpcclient.get_url = function ()
	return client_url
end


-- @brief 一次性 CALL; 须在 kco 协程内; 目标取自 setup
-- @return [string]						成功为 `success`, 之后为回应值; 失败 `nil, code, errstr`
-- @note co_post/co_notify 成功仅 `success` (无载荷); 与协议包 co_*_rpc 一致
-- @note setup 配置 `timeout_ms` 后, 连接与调用受同一 deadline 约束
rpcclient.co_call = function (...)
	local spec = client_spec
	local url = ConfiguredUrl()
	if not url or not spec then
		return nil, net.INVAL, "rpcclient not setup"
	end

	local api = ApiForUrl(url)
	if not api then
		return nil, net.INVAL, "unsupported rpc schema"
	end

	return OneShot(api, url, spec, "call", ...)
end


-- @brief 一次性 POST; 须在 kco 协程内; 目标取自 setup
-- @return [string]						成功为 `success`
-- @return [number]						失败时 knetcode
-- @return [string]						失败时 errstr
rpcclient.co_post = function (...)
	local spec = client_spec
	local url = ConfiguredUrl()
	if not url or not spec then
		return nil, net.INVAL, "rpcclient not setup"
	end

	local api = ApiForUrl(url)
	if not api then
		return nil, net.INVAL, "unsupported rpc schema"
	end

	return OneShot(api, url, spec, "post", ...)
end


-- @brief 一次性 NOTIFY; 须在 kco 协程内; 目标取自 setup
-- @return [string]						成功为 `success`
-- @return [number]						失败时 knetcode
-- @return [string]						失败时 errstr
rpcclient.co_notify = function (...)
	local spec = client_spec
	local url = ConfiguredUrl()
	if not url or not spec then
		return nil, net.INVAL, "rpcclient not setup"
	end

	local api = ApiForUrl(url)
	if not api then
		return nil, net.INVAL, "unsupported rpc schema"
	end

	return OneShot(api, url, spec, "notify", ...)
end


return rpcclient
