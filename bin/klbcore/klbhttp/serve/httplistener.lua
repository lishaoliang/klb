--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   httplistener.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  HTTP 服务 监听
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local io = require("io")
local khttp = require("khttp")
local net = require("klbcore.klbcode.netcode")
local httpserver = require("klbcore.klbhttp.serve.httpserver")
local httpserverpcer = require("klbcore.klbhttp.serve.httpserverpcer")


--------------------------------------------------------------------------------------------
-- 前置定义
local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现

-- @brief 解析 TLS PEM: 内联 PEM 原文, 或本地文件路径
-- @param [in]      s[string]			PEM 原文或文件路径
-- @return pem[string]					PEM 正文; 无法识别或读取失败为 nil
-- @note 含 `-----BEGIN` 视为原文; 否则按路径读文件
local function resolve_pem(s)
	if "string" ~= type(s) or "" == s then
		return nil
	end

	if string.find(s, "-----BEGIN", 1, true) then
		return s
	end

	local f = io.open(s, "rb")
	if not f then
		return nil
	end

	local body = f:read("*a")
	f:close()
	if not body or 0 == #body then
		return nil
	end

	return body
end


-- @brief 适配 listen opts: cert/key 可为 PEM 原文或文件路径
-- @param [in]      opts[table]			[可选] 监听配置
-- @return listen_opts[table]			交给 khttp.listen 的配置; 非 tls 时原样返回
-- @return ok[boolean]					true 可继续 listen; tls 且无法解析 PEM 为 false
local function resolve_listen_opts(opts)
	if not opts or true ~= opts.tls then
		return opts, true
	end

	local cert = resolve_pem(opts.cert)
	local key = resolve_pem(opts.key)
	if not cert or not key then
		return nil, false
	end

	local listen_opts = {}
	for k, v in pairs(opts) do
		listen_opts[k] = v
	end
	listen_opts.cert = cert
	listen_opts.key = key

	return listen_opts, true
end


--------------------------------------------------------------------------------------------
-- httplisten 对外接口

local httplisten = {}

-- @brief 关闭监听
function httplisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] 监听选项; 默认使用 new(cfg)
-- @return ok[boolean]					true 成功; 失败为 nil
-- @return code[number]					失败时 klb_netcode_e
function httplisten:open(port, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   plain[boolean]			[可选] tls 时是否同时收明文 HTTP, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM 原文或文件路径
	--   key[string]			tls 时必填, 私钥 PEM 原文或文件路径
	--   host[string]			[可选] 绑定地址; 缺省 INADDR_ANY
	--   family[string]			[可选] "ipv4"|"ipv6"|"dual"
	--   max_connect[number]	[可选] backlog, 默认 20
	--   reuseport[boolean]		[可选]
	--   nodelay[boolean]		[可选] accept 后 TCP_NODELAY
	--   keepalive[boolean]		[可选] accept 后 SO_KEEPALIVE
	-- }

	-- 关闭
	self:close()

	-- 开启监听
	local listen_opts = opts
	if not listen_opts then
		listen_opts = self._cfg
	end

	local resolved, ok = resolve_listen_opts(listen_opts)
	if not ok then
		self._last_code = net.INVAL
		return nil, net.INVAL
	end

	local listen, code = khttp.listen(port, resolved)
	self._listen = listen
	if not listen then
		self._last_code = code
		return nil, code
	end

	self._last_code = nil
	return true
end

-- @brief 监听端口 (含 port=0 回读)
-- @return port[number]
function httplisten:port()
	if not self._listen or not self._listen.port then
		return 0
	end
	return self._listen:port()
end

-- @brief getsockname 回读本地地址
-- @return ip[string|nil], port[number|nil]
function httplisten:local_addr()
	if not self._listen or not self._listen.local_addr then
		return nil, nil
	end
	return self._listen:local_addr()
end

-- @brief 监听 native fd
-- @return fd[number|nil]
function httplisten:native_fd()
	if not self._listen or not self._listen.native_fd then
		return nil
	end
	return self._listen:native_fd()
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @return [string|number]				失败时 C 侧第二值 ("ok"|"error"|"exit" 等)
-- @note 须在 kco 协程内
function httplisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn, msg = self._listen:co_accept()
	if not conn then
		return nil, msg
	end

	return httpserver.new(conn)
end


-- @brief 热换 TLS 证书 (新 accept 用新证; 旧连接仍持旧 ctx)
-- @param [in]      cfg[table]			须含 cert / key (PEM 原文或文件路径)
-- @return ok[boolean]
function httplisten:reload_tls(cfg)
	if not self._listen or "function" ~= type(self._listen.reload_tls) then
		return false
	end

	if "table" ~= type(cfg) then
		return false
	end

	local cert = resolve_pem(cfg.cert)
	local key = resolve_pem(cfg.key)
	if not cert or not key then
		return false
	end

	return self._listen:reload_tls({ cert = cert, key = key })
end


--------------------------------------------------------------------------------------------
-- httprpclisten 对外接口

local httprpclisten = {}

-- @brief 关闭监听
function httprpclisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] 监听选项; 默认使用 new_rpc(cfg)
-- @return ok[boolean]					true 成功; 失败为 nil
-- @return code[number]					失败时 klb_netcode_e
function httprpclisten:open(port, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   plain[boolean]			[可选] tls 时是否同时收明文 HTTP, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM 原文或文件路径
	--   key[string]			tls 时必填, 私钥 PEM 原文或文件路径
	--   rpctype[string]		[可选] LUA 或 JSON; 交给 accept 后的服务连接
	--   host/family/max_connect/reuseport/nodelay/keepalive 同 httplisten:open
	-- }

	self:close()

	local listen_opts = opts
	if not listen_opts then
		listen_opts = self._cfg
	end

	local resolved, ok = resolve_listen_opts(listen_opts)
	if not ok then
		self._last_code = net.INVAL
		return nil, net.INVAL
	end

	local listen, code = khttp.listen_rpc(port, resolved)
	self._listen = listen
	if not listen then
		self._last_code = code
		return nil, code
	end

	self._last_code = nil
	return true
end

function httprpclisten:port()
	if not self._listen or not self._listen.port then
		return 0
	end
	return self._listen:port()
end

function httprpclisten:local_addr()
	if not self._listen or not self._listen.local_addr then
		return nil, nil
	end
	return self._listen:local_addr()
end

function httprpclisten:native_fd()
	if not self._listen or not self._listen.native_fd then
		return nil
	end
	return self._listen:native_fd()
end

-- @brief 接收 新连接
-- @return [table]	RPC 服务连接; 无连接或退出时为 nil
-- @return [string|number]				失败时 C 侧第二值
-- @note 须在 kco 协程内
function httprpclisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn, msg = self._listen:co_accept()
	if not conn then
		return nil, msg
	end

	return httpserverpcer.new_rpc(conn, self._cfg)
end


-- @brief 热换 TLS 证书 (转发 C listen:reload_tls)
-- @param [in]      cfg[table]			须含 cert / key (PEM 原文或文件路径)
-- @return ok[boolean]
function httprpclisten:reload_tls(cfg)
	if not self._listen or "function" ~= type(self._listen.reload_tls) then
		return false
	end

	if "table" ~= type(cfg) then
		return false
	end

	local cert = resolve_pem(cfg.cert)
	local key = resolve_pem(cfg.key)
	if not cert or not key then
		return false
	end

	return self._listen:reload_tls({ cert = cert, key = key })
end


--------------------------------------------------------------------------------------------
-- httplistener

local httplistener = {}


-- @brief 新建一个监听模块
httplistener.new = function (cfg)
	local obj = {
		_listen = nil,						-- C 提供的监听模块
		_cfg = cfg,							-- 监听选项; tls/cert/key
	}

	setmetatable(obj, {
		__index = httplisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})

	return obj
end


-- @brief 新建一个监听 RPC 模块
httplistener.new_rpc = function (cfg)
	local obj = {
		_listen = nil,						-- C 提供的监听 RPC 模块
		_cfg = cfg,							-- accept 时交给 RPC 服务连接
	}

	setmetatable(obj, {
		__index = httprpclisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})

	return obj
end


return httplistener
