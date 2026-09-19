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
local httpserver = require("klbcore.klbhttp.serve.httpserver")


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
-- @return ok[boolean]					true 成功; false 失败 (PEM 无法解析 / tls 证书无效等)
function httplisten:open(port, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM 原文或文件路径
	--   key[string]			tls 时必填, 私钥 PEM 原文或文件路径
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
		return false
	end

	self._listen = khttp.listen(port, resolved)
	return nil ~= self._listen
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @note 须在 kco 协程内
function httplisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn = self._listen:co_accept()
	if not conn then
		return nil
	end

	return httpserver.new(conn)
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


return httplistener
