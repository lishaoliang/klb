--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbrpc init.lua
-- @note   RPC 业务流程; 服务在 serve/; 客户端在 client/; 连接在协议包
--   \n 返回码: 直接用 klbcore.klbcode.base / net; 不本地再映射 E_*
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local rpcer = require("klbcore.klbrpc.serve.rpcer")
local rpcclienter = require("klbcore.klbrpc.client.rpcclienter")
local rpcclient = require("klbcore.klbrpc.client.rpcclient")


local klbrpc = {}


--------------------------------------------------------------------------------------------
-- 服务端 (内置一份监听; 与 new_serve 互不影响)

local builtin = nil


local function EnsureBuiltin()
	if not builtin then
		builtin = rpcer.new()
	end

	return builtin
end


-- @brief 将扁平 listen 规格收成 `{ listen, rpctype }` (对齐 setup_client 写法)
-- @param [in]      cfg[table]			扁平或嵌套
-- @return [table]
local function NormalizeServeCfg(cfg)
	if nil == cfg then
		return {}
	end

	if "table" ~= type(cfg) then
		return cfg
	end

	if nil ~= cfg.listen then
		return cfg
	end

	if nil ~= cfg.port or nil ~= cfg.schemes or nil ~= cfg.scheme then
		return {
			listen = {
				port = cfg.port,
				schemes = cfg.schemes,
				scheme = cfg.scheme,
				rpctype = cfg.rpctype,
			},
			rpctype = cfg.rpctype,
			auth = cfg.auth,
		}
	end

	return cfg
end


-- @brief 新建一个独立 RPC 服务
-- @param [in]      cfg[table]			[可选] 同 setup_serve
-- @return [table]						服务对象
klbrpc.new_serve = function (cfg)
	return rpcer.new(NormalizeServeCfg(cfg))
end


-- @brief 配置内置服务监听; 可多次; 后写覆盖
-- @param [in]      cfg[table]			`{ port, schemes|scheme, rpctype, auth }` 或 `{ listen, rpctype, auth }`
-- @return ok[boolean]					服务中未 stop, 或 listen 非法时为 false
-- @note eg. `{ port = 3457, schemes = { "smp", "mnp" } }` (写法对齐 setup_client)
-- @note auth `{ mode="token"|"none", token|user+pass, expire_ms }`; 缺省 token
klbrpc.setup_serve = function (cfg)
	return EnsureBuiltin():setup(NormalizeServeCfg(cfg))
end


-- @brief 同 setup_serve (兼容旧名)
klbrpc.setup = function (cfg)
	return klbrpc.setup_serve(cfg)
end


-- @brief 按已配置 listen bind, 并启动 accept 循环
-- @return ok[boolean]					任一项协议包加载失败为 false
-- @note 须先 setup_serve (可省略则缺省单口 3457 多协议). 幂等.
klbrpc.serve = function ()
	return EnsureBuiltin():serve()
end


-- @brief 关闭内置服务的监听; 保留方法表
-- @return ok[boolean]					true
klbrpc.stop = function ()
	if builtin then
		builtin:stop()
	end

	return true
end


-- @brief 注册 REQUEST 方法到内置服务
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`, 返回值作为 response 载荷
-- @return ok[boolean]
klbrpc.call = function (name, fn)
	return EnsureBuiltin():call(name, fn)
end


-- @brief 注册 POST 方法到内置服务; 无回应
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`
-- @return ok[boolean]
klbrpc.post = function (name, fn)
	return EnsureBuiltin():post(name, fn)
end


-- @brief 注册 NOTIFY 方法到内置服务; 无回应
-- @param [in]      name[string]		方法名
-- @param [in]      fn[function]		`fn(req)`
-- @return ok[boolean]
klbrpc.notify = function (name, fn)
	return EnsureBuiltin():notify(name, fn)
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC (长期)

-- @brief 新建未连接的 RPC 客户端; url 仅用于选定协议包
-- @param [in]      url[string]			eg. `smprpc://127.0.0.1:3457`
-- @param [in]      cfg[table]			[可选] 交给协议包
-- @return [table]						客户端; 失败为 nil
klbrpc.new_client_rpc = function (url, cfg)
	return rpcclienter.new_rpc(url, cfg)
end


-- @brief 按 URL 连接, 返回可长期复用的 RPC 客户端
-- @param [in]      url[string]			eg. `httprpc://127.0.0.1:3458`
-- @param [in]      cfg[table]			[可选] 交给协议包
-- @return [table]						已连接客户端; 失败为 nil
-- @note 对象方法: `:co_call` / `:post` / `:notify` / `:disconnect`
klbrpc.connect = function (url, cfg)
	return rpcclienter.connect(url, cfg)
end


--------------------------------------------------------------------------------------------
-- 客户端 RPC (一次性; 内置一份目标)

-- @brief 配置内置客户端目标; 可多次; 后写覆盖
-- @param [in]      cfg[table]			`{ url }` 或 `{ scheme, host|ip, port, path, rpctype, tls }`
-- @return ok[boolean]					非法为 false
-- @note eg. `{ url = "smprpc://127.0.0.1:3457" }` 或 `{ scheme = "smp", host = "127.0.0.1", port = 3457 }`
klbrpc.setup_client = function (cfg)
	return rpcclient.setup(cfg)
end


-- @brief 一次性 CALL; 须在 kco 协程内; 须先 setup_client
-- @return [string]						成功为 `success`, 之后为回应值; 失败无返回
klbrpc.co_call = function (...)
	return rpcclient.co_call(...)
end


-- @brief 一次性 POST; 须在 kco 协程内; 须先 setup_client
-- @return [string]						成功为 `success`; 失败无返回
klbrpc.co_post = function (...)
	return rpcclient.co_post(...)
end


-- @brief 一次性 NOTIFY; 须在 kco 协程内; 须先 setup_client
-- @return [string]						成功为 `success`; 失败无返回
klbrpc.co_notify = function (...)
	return rpcclient.co_notify(...)
end


return klbrpc
