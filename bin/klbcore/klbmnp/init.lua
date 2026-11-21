--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbmnp init.lua
-- @note   MNP 协议脚本封装; 内部使用 kmnp
-- @history 修改历史
--		[2026-09] 创建文件
--		[2026-10] adopt / MAGIC 魔数分类 (proxy L0-1/2)
--]]
local knetcode = require("knetcode")
local kmnp = require("kmnp")
local mnpclienter = require("klbcore.klbmnp.client.mnpclienter")
local mnpclientrpcer = require("klbcore.klbmnp.client.mnpclientrpcer")
local mnplistener = require("klbcore.klbmnp.serve.mnplistener")
local mnpserver = require("klbcore.klbmnp.serve.mnpserver")


local klbmnp = {}


--------------------------------------------------------------------------------------------
-- 魔数 / 同口 demux

-- 线上 LE: 0xEB 'M' 'N' 'P'
klbmnp.MAGIC = 0x504E4DEB
klbmnp.MAGIC0 = 0xEB


local function _read_u32le(s, off)
	local b1, b2, b3, b4 = string.byte(s, off, off + 3)
	if b4 == nil then
		return nil
	end
	return b1 + b2 * 256 + b3 * 65536 + b4 * 16777216
end


-- @brief 是否 MNP 魔数首字节
-- @param [in]  first[number|string|nil]
-- @return [boolean]
klbmnp.is_magic0 = function (first)
	local b0 = nil
	if type(first) == "number" then
		b0 = math.floor(first)
	elseif type(first) == "string" and #first > 0 then
		b0 = string.byte(first, 1)
	end
	return b0 == klbmnp.MAGIC0
end


-- @brief 按 4 字节魔数分类 (窥探; 不解码载荷)
-- @param [in]  buf[string]
-- @return [string] "mnp"|"unknown"
klbmnp.classify_magic = function (buf)
	if type(buf) ~= "string" or #buf < 4 then
		return "unknown"
	end
	if _read_u32le(buf, 1) == klbmnp.MAGIC then
		return "mnp"
	end
	return "unknown"
end


-- @brief 由已 accept 的 listen_socket 接管为 MNP 服务连接
-- @param [in]  listen_sock[lightuserdata]	kmnp.new_serve 同源
-- @param [in]  opts[table|nil]				透传 kmnp.new_serve
-- @return [table|nil] serve				mnpserver; 失败 nil
-- @return [number|nil] code
klbmnp.adopt = function (listen_sock, opts)
	if listen_sock == nil or type(kmnp.new_serve) ~= "function" then
		return nil, 1
	end

	local raw, code = kmnp.new_serve(listen_sock, opts)
	if raw == nil then
		return nil, code
	end

	return mnpserver.new(raw), nil
end


--------------------------------------------------------------------------------------------
-- 客户端

-- @brief 新建一个客户端
-- @param [in]      cfg[table]			[可选] 客户端配置; 现行无消费字段
-- @return [table]	客户端模块
klbmnp.new_client = function (cfg)
	return mnpclienter.new(cfg)
end


-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table|nil]		[可选] `{ tls[boolean] }`
-- @return [table]	客户端模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbmnp.connect = function (host, port, opts)
	local client = mnpclienter.new()
	local ret = client:connect(host, port, opts)
	if 0 ~= ret then
		return nil, ret
	end

	return client
end


--------------------------------------------------------------------------------------------
-- 服务端

-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			[可选] 监听模块配置
-- @return [table]	监听模块
klbmnp.new_listen = function (cfg)
	return mnplistener.new(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置
-- @return [table]	监听模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbmnp.listen = function (port, cfg)
	local l = mnplistener.new(cfg)
	local ok, code = l:open(port)
	if not ok then
		return nil, code
	end

	return l
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC

-- @brief 新建一个RPC客户端
-- @param [in]      cfg[table]			[可选] `{ rpctype }`
-- @return [table]	客户端模块
klbmnp.new_client_rpc = function (cfg)
	return mnpclientrpcer.new_rpc(cfg)
end

-- @brief 连接到目标
-- @param [in]      host[string]		主机
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table|nil]		[可选] `{ tls[boolean] }` (mnprpcs)
-- @return [table]	客户端模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbmnp.connect_rpc = function (host, port, opts)
	local client = mnpclientrpcer.new_rpc()
	local ret = client:connect_rpc(host, port, opts)
	if 0 ~= ret then
		return nil, ret
	end

	return client
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC 一次性接口

-- @brief 发起一次 POST-RPC
-- @param [in]      url[string]			目标, eg. 'mnprpc://127.0.0.1:3456'
-- @return [string]						成功为 'success'
-- @return [number]						失败时 code (首值为 nil)
-- @return [string]						失败时 knetcode.strerror(code)
klbmnp.co_post_rpc = function (url, ...)
	local r = mnpclientrpcer.new_rpc()
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
-- @param [in]      url[string]			目标, eg. 'mnprpc://127.0.0.1:3456'
-- @return [string]						成功为 'success'
-- @return [number]						失败时 code (首值为 nil)
-- @return [string]						失败时 knetcode.strerror(code)
klbmnp.co_notify_rpc = function (url, ...)
	local r = mnpclientrpcer.new_rpc()
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
-- @param [in]      url[string]			目标, eg. 'mnprpc://127.0.0.1:3456'
-- @return [string]						成功为 'success'
-- @return [...]						成功时回应数据; 失败时 code, errstr (首值为 nil)
-- @note 须在 kco 协程内
klbmnp.co_call_rpc = function (url, ...)
	local ReturnCoCall = function (r, ...)
		r:disconnect()
		return 'success', ...
	end

	local r = mnpclientrpcer.new_rpc()
	local rc = r:connect(url)
	if 0 ~= rc then
		return nil, rc, knetcode.strerror(rc)
	end

	return ReturnCoCall(r, r:co_call(...))
end

--------------------------------------------------------------------------------------------
-- 服务端 RPC


-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			[可选] 监听模块配置; `{ rpctype }`
-- @return [table]	监听模块
klbmnp.new_listen_rpc = function (cfg)
	return mnplistener.new_rpc(cfg)
end


-- @brief 开始 监听模块
-- @param [in]      port[number(int)]	端口
-- @param [in]      cfg[table]			[可选]监听模块配置
-- @return [table]	监听模块; 失败为 nil
-- @return [number]	失败时 klb_netcode_e
klbmnp.listen_rpc = function (port, cfg)
	local l = mnplistener.new_rpc(cfg)
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
-- 线上 packtype: RPC_LUA=0x10 / RPC_JSON=0x11; RPC_EX(0x12)/USER(0x14-0x1B) 为规格保留,
-- 现行 conn 解析后静默丢弃, 不上抛 Lua
klbmnp.RPC_LUA			= 'LUA'
klbmnp.RPC_JSON			= 'JSON'


-------------------------------------------------------
-- RPC 方法 (脚本契约面; C 枚举另有 OPTIONS/GET/PUT/DELETE 为规格保留, 本层不暴露)
klbmnp.RPC_POST 		= 'POST'
klbmnp.RPC_NOTIFY 		= 'NOTIFY'
klbmnp.RPC_REQUEST 		= 'REQUEST'
klbmnp.RPC_RESPONSE 	= 'RESPONSE'


return klbmnp
