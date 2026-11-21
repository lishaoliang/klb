--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   onvifdiscoverer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  ONVIF WS-Discovery 搜索端
-- @note   经 konvif.discover 发 Probe, 收 ProbeMatches; 须在 kco 协程内
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local net = klbcode.net

local konvif = require("konvif")
local kco = require("kco")
local wsd_soap = require("klbcore.klbonvif.discovery.wsd_soap")


--------------------------------------------------------------------------------------------
-- 前置定义

local E = {}


--------------------------------------------------------------------------------------------
-- onvifdiscover


local onvifdiscover = {}


-- @brief 关闭搜索
function onvifdiscover:close()
	if nil ~= self._disc then
		self._disc:close()
		self._disc = nil
	end

	self._closed = true
end


-- @brief 打开组播搜索通道
-- @return ok[boolean]					true 成功
-- @return [number|nil] code			失败错误码
-- @return [string|nil] err				失败描述
function onvifdiscover:open()
	if nil ~= self._disc then
		return true
	end

	local disc, err = konvif.discover()
	if nil == disc then
		E.last_err = err or "konvif.discover failed"
		return nil, ((("number" == type(E.last_err)) and E.last_err) or base.IO), E.last_err
	end

	self._disc = disc
	self._closed = false
	return true
end


-- @brief 组播搜索设备 (协程)
-- @param [in]      opts[table]			[可选] timeout_ms / types / scopes
-- @return list[table]					设备列表; 失败 nil
-- @return [number|nil] code			失败错误码
-- @return [string|nil] err				失败描述
-- @note 须在 kco 协程内; 每项含 peer/xaddr/xaddrs/types/scopes/endpoint/xml
function onvifdiscover:co_search(opts)
	opts = opts or {}

	local ok_o, code_o, err_o = self:open()
	if not ok_o then
		return nil, code_o, err_o
	end

	local timeout_ms = tonumber(opts.timeout_ms) or 3000
	if timeout_ms < 100 then
		timeout_ms = 100
	end

	local probe = wsd_soap.build_probe({
		types = opts.types,
		scopes = opts.scopes,
		message_id = opts.message_id,
	})

	local rc = self._disc:send(probe)
	if 0 ~= rc then
		return nil, base.IO, "send probe failed"
	end

	local results = {}
	local stop = false

	kco.timeout(timeout_ms, function ()
		stop = true
		if nil ~= self._disc then
			self._disc:close()
			self._disc = nil
		end
	end)

	while not stop do
		if nil == self._disc then
			break
		end

		local msg, peer, xml = self._disc:co_recv()

		if "datagram" == msg then
			local item = wsd_soap.parse_probe_matches(xml, peer)
			if nil ~= item then
				results[#results + 1] = item
			end
		elseif "error" == msg then
			return nil, base.IO, "recv error"
		elseif "exit" == msg and not stop then
			return nil, net.CLOSED, "recv exit"
		else
			break
		end
	end

	-- 超时结束: 空表表示无匹配设备 (与传输错误区分)
	return results
end


--------------------------------------------------------------------------------------------
-- onvifdiscoverer


local onvifdiscoverer = {}


-- @brief 新建搜索模块
-- @param [in]      cfg[table]			[可选] 默认搜索配置
-- @return [table]	搜索模块
onvifdiscoverer.new = function (cfg)
	local obj = {
		_cfg = cfg,
		_disc = nil,
		_closed = false,
	}

	setmetatable(obj, {
		__index = onvifdiscover,
	})

	return obj
end


return onvifdiscoverer
