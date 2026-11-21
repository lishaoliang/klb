--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   onvifdiscover_listen.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ONVIF WS-Discovery 服务端 (应答 Probe)
-- @note   经 konvif.discover_listen; 须在 kco 协程内
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local konvif = require("konvif")
local wsd_soap = require("klbcore.klbonvif.discovery.wsd_soap")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- onvifdiscover_listen


local onvifdiscover_listen = {}


-- @brief 关闭
function onvifdiscover_listen:close()
	if nil ~= self._disc then
		self._disc:close()
		self._disc = nil
	end
end


-- @brief 加入组播并监听
-- @param [in]      port[number(int)]	[可选] 端口; 默认 3702
-- @return [number]	0 成功; 非0 失败
function onvifdiscover_listen:open(port)
	if nil ~= self._disc then
		return 0
	end

	local bind_port = 0
	if "number" == type(port) and 0 < port then
		bind_port = port
	end

	local disc, err = konvif.discover_listen(bind_port)
	if nil == disc then
		E.last_err = err or "konvif.discover_listen failed"
		return 1
	end

	self._disc = disc
	return 0
end


-- @brief 处理一次 Probe (协程)
-- @return msg[string]					"ok" / "ignore" / "error" / "exit"
-- @return detail[string|number|nil]	补充信息
-- @note 须在 kco 协程内; cfg 需含 xaddr
function onvifdiscover_listen:co_once()
	if nil == self._disc then
		return "error", "not open"
	end

	local cfg = self._cfg or {}
	if "string" ~= type(cfg.xaddr) or "" == cfg.xaddr then
		return "error", "cfg.xaddr required"
	end

	local msg, peer, xml = self._disc:co_recv()

	if "datagram" ~= msg then
		return msg, peer
	end

	if not wsd_soap.is_probe(xml) then
		return "ignore", peer
	end

	local relates_to = wsd_soap.probe_message_id(xml)
	local reply = wsd_soap.build_probe_matches({
		xaddr = cfg.xaddr,
		types = cfg.types,
		scopes = cfg.scopes,
		endpoint = cfg.endpoint,
		relates_to = relates_to,
	})

	local ip, port = wsd_soap.parse_peer(peer)
	if nil == ip then
		return "error", "bad peer"
	end

	local rc = self._disc:send(reply, ip, port)
	if 0 ~= rc then
		return "error", "send ProbeMatches failed"
	end

	return "ok", peer
end


--------------------------------------------------------------------------------------------
-- factory


local M = {}


-- @brief 新建 Discovery 服务模块
-- @param [in]      cfg[table]			须含 xaddr; 可选 types/scopes/endpoint
-- @return [table]	服务模块
M.new = function (cfg)
	local obj = {
		_cfg = cfg,
		_disc = nil,
	}

	setmetatable(obj, {
		__index = onvifdiscover_listen,
	})

	return obj
end


return M
