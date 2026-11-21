--[[
-- Copyright (c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbsmp init.lua
-- @note   SMP协议
--   \n 服务端心跳默认开 (C/Lua 默认 interval 15s / timeout 30s); `ksmp.new_serve` 可传 ping_interval_ms/ping_timeout_ms (0 关)
--   \n `klbsmp.listen` 路径 accept 后沿用 C 默认; 客户端无自动心跳
-- @history 修改历史
--		[2025-10] 添加基础
--		[2026-10] adopt / MAGIC 魔数分类 (proxy L0-1/2)
--]]
local knetcode = require("knetcode")
local ksmp = require("ksmp")
local smpclienter = require("klbcore.klbsmp.client.smpclienter")
local smpclientrpcer = require("klbcore.klbsmp.client.smpclientrpcer")
local smplistener = require("klbcore.klbsmp.serve.smplistener")
local smpserver = require("klbcore.klbsmp.serve.smpserver")


local klbsmp = {}


--------------------------------------------------------------------------------------------
-- 魔数 / 同口 demux

-- 线上 LE: 0xEB 'S' 'M' 'P'
klbsmp.MAGIC = 0x504D53EB
klbsmp.MAGIC0 = 0xEB


local function _read_u32le(s, off)
	local b1, b2, b3, b4 = string.byte(s, off, off + 3)
	if b4 == nil then
		return nil
	end
	return b1 + b2 * 256 + b3 * 65536 + b4 * 16777216
end


-- @brief 是否 SMP 魔数首字节
-- @param [in]  first[number|string|nil]
-- @return [boolean]
klbsmp.is_magic0 = function (first)
	local b0 = nil
	if type(first) == "number" then
		b0 = math.floor(first)
	elseif type(first) == "string" and #first > 0 then
		b0 = string.byte(first, 1)
	end
	return b0 == klbsmp.MAGIC0
end


-- @brief 按 4 字节魔数分类 (窥探; 不解码载荷)
-- @param [in]  buf[string]
-- @return [string] "smp"|"unknown"
klbsmp.classify_magic = function (buf)
	if type(buf) ~= "string" or #buf < 4 then
		return "unknown"
	end
	if _read_u32le(buf, 1) == klbsmp.MAGIC then
		return "smp"
	end
	return "unknown"
end


-- @brief 由已 accept 的 listen_socket 接管为 SMP 服务连接
-- @param [in]  listen_sock[lightuserdata]	ksmp.new_serve 同源
-- @param [in]  opts[table|nil]				透传 ksmp.new_serve
-- @return [table|nil] serve				smpserver; 失败 nil
-- @return [number|nil] code
klbsmp.adopt = function (listen_sock, opts)
	if listen_sock == nil or type(ksmp.new_serve) ~= "function" then
		return nil, 1
	end

	local raw, code = ksmp.new_serve(listen_sock, opts)
	if raw == nil then
		return nil, code
	end

	return smpserver.new(raw), nil
end


--------------------------------------------------------------------------------------------
-- 客户端

-- @brief 新建一个客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
klbsmp.new_client = function (cfg)
	return smpclienter.new(cfg)
end


-- @brief 连接到目标
-- @param [in]      host[string]
-- @param [in]      port[number]
-- @param [in]      opts[table|nil]		[可选] `{ tls[boolean] }`
-- @return [table]	客户端模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbsmp.connect = function (host, port, opts)
	local client = smpclienter.new()
	local ret = client:connect(host, port, opts)
	if 0 ~= ret then
		return nil, ret
	end

	return client
end


--------------------------------------------------------------------------------------------
-- 服务端

-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			监听模块配置
-- @return [table]	监听模块
klbsmp.new_listen = function (cfg)
	return smplistener.new(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置
-- @return [table]	监听模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbsmp.listen = function (port, cfg)
	local l = smplistener.new(cfg)
	local ok, code = l:open(port)
	if not ok then
		return nil, code
	end

	return l
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC

-- @brief 新建一个RPC客户端
-- @param [in]      cfg[table]			客户端配置
-- @return [table]	客户端模块
klbsmp.new_client_rpc = function (cfg)
	return smpclientrpcer.new_rpc(cfg)
end

-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table|nil]		[可选] `{ tls[boolean] }` (smprpcs)
-- @return [table]	客户端模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbsmp.connect_rpc = function (host, port, opts)
	local client = smpclientrpcer.new_rpc()
	local ret = client:connect_rpc(host, port, opts)
	if 0 ~= ret then
		return nil, ret
	end

	return client
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC 一次性接口

-- @brief 发起一次 POST-RPC
-- @param [in]      url[string]			目标, eg. 'smprpc://username:password@127.0.0.1:3457'
-- @return [string]						成功为 'success'
-- @return [number]						失败时 code (首值为 nil)
-- @return [string]						失败时 knetcode.strerror(code)
klbsmp.co_post_rpc = function (url, ...)
	local r = smpclientrpcer.new_rpc()
	local rc = r:connect(url)
	if 0 ~= rc then
		return nil, rc, knetcode.strerror(rc)
	end

	r:post(...)
	r:co_wait()
	r:disconnect()
	return 'success'
end

-- @brief 发起一次 NOTIFY-RPC
-- @param [in]      url[string]			目标, eg. 'smprpc://username:password@127.0.0.1:3457'
-- @return [string]						成功为 'success'
-- @return [number]						失败时 code (首值为 nil)
-- @return [string]						失败时 knetcode.strerror(code)
klbsmp.co_notify_rpc = function (url, ...)
	local r = smpclientrpcer.new_rpc()
	local rc = r:connect(url)
	if 0 ~= rc then
		return nil, rc, knetcode.strerror(rc)
	end

	r:notify(...)
	r:co_wait()
	r:disconnect()
	return 'success'
end

-- @brief 发起一次 CALL-RPC
-- @param [in]      url[string]			目标, eg. 'smprpc://username:password@127.0.0.1:3457'
-- @return [string]						成功为 'success'
-- @return [...]						成功时回应数据; 失败时 code, errstr (首值为 nil)
-- @note 须在 kco 协程内
klbsmp.co_call_rpc = function (url, ...)
	local ReturnCoCall = function (r, ...)
		r:disconnect()
		return 'success', ...
	end

	local r = smpclientrpcer.new_rpc()
	local rc = r:connect(url)
	if 0 ~= rc then
		return nil, rc, knetcode.strerror(rc)
	end

	return ReturnCoCall(r, r:co_call(...))
end

--------------------------------------------------------------------------------------------
-- 服务端 RPC


-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			监听模块配置
-- @return [table]	监听模块
klbsmp.new_listen_rpc = function (cfg)
	return smplistener.new_rpc(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置
-- @return [table]	监听模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbsmp.listen_rpc = function (port, cfg)
	local l = smplistener.new_rpc(cfg)
	local ok, code = l:open(port)
	if not ok then
		return nil, code
	end

	return l
end


--------------------------------------------------------------------------------------------
-- 一些常量

-------------------------------------------------------
-- 数据组织方式
klbsmp.RPC_LUA			= 'LUA'
klbsmp.RPC_JSON			= 'JSON'


-------------------------------------------------------
-- RPC 方法
klbsmp.RPC_POST 		= 'POST'
klbsmp.RPC_NOTIFY 		= 'NOTIFY'
klbsmp.RPC_REQUEST 		= 'REQUEST'
klbsmp.RPC_RESPONSE 	= 'RESPONSE'


return klbsmp
