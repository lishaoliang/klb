--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   onviflistener.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ONVIF 服务端监听
-- @note   经 klbhttp.new_listen; 会话见 onvifserver
-- @history 修改历史
--		[2026-09] 创建文件; 对接 klbhttp
--]]
local klbhttp = require("klbcore.klbhttp")
local errcode = require("klbcore.klbonvif.errcode")
local onvifserver = require("klbcore.klbonvif.serve.onvifserver")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- onviflisten 对外接口

local onviflisten = {}


-- @brief 关闭监听
function onviflisten:close()
	if nil ~= self._listen then
		self._listen:close()
		self._listen = nil
	end
end


-- @brief 开始监听端口
-- @param [in]      port[number(int)]	端口
-- @param [in]      opts[table]			[可选] tls/cert/key; 默认用 new(cfg) 中 listen 相关字段
-- @return ok[boolean]					true 成功
-- @return [number|nil] code			失败错误码 (下层 code)
-- @return [string|nil] err				失败描述
function onviflisten:open(port, opts)
	if "number" ~= type(port) or 0 >= port then
		return nil, errcode.E_ARG, "invalid port"
	end

	self:close()

	local listen_opts = opts
	if nil == listen_opts then
		listen_opts = self._listen_cfg
	end

	local l = klbhttp.new_listen(listen_opts)
	local ok, code = l:open(port, listen_opts)
	if not ok then
		self._last_code = code
		E.last_err = "klbhttp listen open failed"
		return nil, code or errcode.E_IO, E.last_err
	end

	self._listen = l
	self._port = port
	self._last_code = nil
	return true
end


-- @brief 接收新连接并包装为 ONVIF 会话
-- @return [table|nil]	会话模块; 失败 nil
-- @return [number|nil] code			失败错误码
-- @return [string|nil]	失败描述
-- @note 须在 kco 协程内
function onviflisten:co_accept()
	if nil == self._listen then
		return nil, errcode.E_STATE, "not open"
	end

	local http_conn = self._listen:co_accept()
	if nil == http_conn then
		return nil, errcode.E_CLOSED, "accept nil"
	end

	return onvifserver.new(http_conn, self._cfg)
end


--------------------------------------------------------------------------------------------
-- onviflistener

local onviflistener = {}


-- @brief 新建一个监听模块
-- @param [in]      cfg[table]			业务配置 + 可选 listen 选项
-- @return [table]	监听模块
-- @note cfg = {
--   device[table]			manufacturer/model/...
--   xaddrs[table]			device/media URL
--   profiles[table]		{ {token,name,stream_uri}, ... }
--   tls/cert/key/...		交给 klbhttp.listen
-- }
onviflistener.new = function (cfg)
	cfg = cfg or {}

	local listen_cfg = nil
	if true == cfg.tls or nil ~= cfg.cert or nil ~= cfg.key or true == cfg.plain then
		listen_cfg = {
			tls = cfg.tls,
			plain = cfg.plain,
			cert = cfg.cert,
			key = cfg.key,
		}
	end

	local obj = {
		_listen = nil,
		_cfg = cfg,
		_listen_cfg = listen_cfg,
		_port = nil,
	}

	setmetatable(obj, {
		__index = onviflisten,
	})

	return obj
end


return onviflistener
