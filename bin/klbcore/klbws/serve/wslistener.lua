--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   wslistener.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  WebSocket 服务 监听
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local kws = require("kws")
local wsserver = require("klbcore.klbws.serve.wsserver")


--------------------------------------------------------------------------------------------
-- 前置定义
local E = {}


--------------------------------------------------------------------------------------------
-- 内部实现





--------------------------------------------------------------------------------------------
-- wslisten 对外接口

local wslisten = {}

-- @brief 关闭监听
function wslisten:close()
	if self._listen then
		self._listen:close()
		self._listen = nil
	end
end

-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] 监听选项; 默认使用 new(cfg)
function wslisten:open(port, opts)
	-- opts = {
	--   tls[boolean]			[可选] 是否 TLS, 默认 `false`
	--   cert[string]			tls 时必填, 证书 PEM
	--   key[string]			tls 时必填, 私钥 PEM
	-- }

	-- 关闭
	self:close()

	-- 开启监听
	local listen_opts = opts
	if not listen_opts then
		listen_opts = self._cfg
	end
	self._listen = kws.listen(port, listen_opts)
end

-- @brief 接收 新连接
-- @return [table]	服务连接模块; 无连接或退出时为 nil
-- @note 须在 kco 协程内
function wslisten:co_accept()
	if not self._listen then
		return nil
	end

	local conn = self._listen:co_accept()
	if not conn then
		return nil
	end

	return wsserver.new(conn)
end


--------------------------------------------------------------------------------------------
-- wslistener

local wslistener = {}


-- @brief 新建一个监听模块
wslistener.new = function (cfg)
	local obj = {
		_listen = nil,						-- C 提供的监听模块
		_cfg = cfg,							-- 监听选项; tls/cert/key
	}

	setmetatable(obj, {
		__index = wslisten,
		__tostring = function(self)
			return tostring(self._listen)
		end
	})

	return obj
end


return wslistener
