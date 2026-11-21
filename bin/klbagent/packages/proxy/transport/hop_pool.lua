--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   hop_pool.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy↔alua hop 长连池 (鉴权会话保活; 空闲默认 1h 客户端自断)
-- @note   仅 mnp/smp/mnps/smps 预 CONNECT 会话; C Ping/Pong 保活; 隧道消耗后不归还
--]]


local M = {}


local _max = 2
local _idle_ms = 3600000
local _enabled = true
-- key -> { { session=, last_ms= }, ... }
local _buckets = {}
local _last_activity_ms = 0
local _refilling = {}
local _stats = {
	hit = 0,
	miss = 0,
	put = 0,
	drop = 0,
	refill = 0,
}


local function _now_ms()
	local ok_e, kenv = pcall(require, "kenv")
	if ok_e and type(kenv) == "table" and type(kenv.tick_count) == "function" then
		return kenv.tick_count()
	end

	return os.time() * 1000
end


local function _close_session(sess)
	if type(sess) == "table" and type(sess.close) == "function" then
		pcall(function ()
			sess.close()
		end)
	end
end


local function _count_all()
	local n = 0
	for _, list in pairs(_buckets) do
		n = n + #list
	end
	return n
end


local function _prune_bucket(list, now)
	if type(list) ~= "table" then
		return
	end

	local i = 1
	while i <= #list do
		local it = list[i]
		local age = now - (tonumber(it.last_ms) or 0)
		if _idle_ms > 0 and age > _idle_ms then
			_close_session(it.session)
			table.remove(list, i)
			_stats.drop = _stats.drop + 1
		else
			i = i + 1
		end
	end
end


local function _prune_all(now)
	now = now or _now_ms()
	-- 全局无入口请求超过 hop_idle: 整池客户端自断
	if _idle_ms > 0 and _last_activity_ms > 0 and (now - _last_activity_ms) > _idle_ms then
		M.clear()
		return
	end

	for k, list in pairs(_buckets) do
		_prune_bucket(list, now)
		if type(list) == "table" and #list == 0 then
			_buckets[k] = nil
		end
	end
end


-- @brief hop 指纹 key (不含原始 token)
-- @param [in]  htype[string]
-- @param [in]  hop[table]
-- @param [in]  hop_opts[table|nil]
-- @return [string]
M.key = function (htype, hop, hop_opts)
	local host = ""
	local port = 0
	if type(hop) == "table" then
		host = tostring(hop.host or "")
		port = tonumber(hop.port) or 0
	end

	local tls = "0"
	local crypto = "-"
	local tok = "-"
	if type(hop_opts) == "table" then
		if hop_opts.tls == true or hop_opts.mnps == true or hop_opts.smps == true then
			tls = "1"
		end
		local c = hop_opts.crypto
		if c == false then
			crypto = "off"
		elseif type(c) == "table" then
			crypto = tostring(c.format or "on") .. ":" .. tostring(c.key_size or "")
		elseif type(c) == "string" then
			crypto = c
		elseif c == true then
			crypto = "on"
		end
	end

	-- ssid 维度优先取建链时解析结果 (hop 表本身可能尚未绑定)
	if type(hop_opts) == "table" and type(hop_opts.alua_ssid) == "string"
		and hop_opts.alua_ssid ~= "" then
		tok = "ssid:" .. hop_opts.alua_ssid
	end

	if tok == "-" and type(hop) == "table" then
		if type(hop.alua_ssid) == "string" and hop.alua_ssid ~= "" then
			tok = "ssid:" .. hop.alua_ssid
		elseif type(hop.client_id) == "string" and hop.client_id ~= "" then
			tok = "ssid:" .. hop.client_id
		elseif type(hop.token) == "string" and hop.token ~= "" then
			tok = "raw"
		end
	end

	return string.format("%s|%s|%d|t%s|c%s|k%s",
		tostring(htype or ""), host, port, tls, crypto, tok)
end


-- @brief 配置上限 / 空闲自断
-- @param [in]  opts[table|nil] `{ max=, idle_ms=, enabled= }`
M.configure = function (opts)
	if type(opts) ~= "table" then
		return
	end

	if opts.enabled ~= nil then
		_enabled = (opts.enabled == true)
	end

	if opts.max ~= nil then
		local n = tonumber(opts.max)
		if n ~= nil and n >= 0 and n <= 64 then
			_max = math.floor(n)
		end
	end

	if opts.idle_ms ~= nil then
		local n = tonumber(opts.idle_ms)
		if n ~= nil and n >= 0 then
			_idle_ms = math.floor(n)
		end
	end

	if _max <= 0 or _idle_ms == 0 then
		_enabled = false
		M.clear()
	end
end


-- @brief 标记入口/链活动 (重置 1h 空闲时钟)
M.note_activity = function ()
	_last_activity_ms = _now_ms()
end


-- @brief 取空闲鉴权会话 (无则 nil)
-- @param [in]  key[string]
-- @return [table|nil] session
M.take = function (key)
	if not _enabled or type(key) ~= "string" or key == "" then
		_stats.miss = _stats.miss + 1
		return nil
	end

	local now = _now_ms()
	_prune_all(now)
	_last_activity_ms = now

	local list = _buckets[key]
	if type(list) ~= "table" or #list == 0 then
		_stats.miss = _stats.miss + 1
		return nil
	end

	local it = table.remove(list)
	_stats.hit = _stats.hit + 1
	return it.session
end


-- @brief 归还鉴权会话 (隧道未开; 池满则关)
-- @param [in]  key[string]
-- @param [in]  session[table]
-- @return [boolean] kept
M.put = function (key, session)
	if not _enabled or type(key) ~= "string" or key == "" or type(session) ~= "table" then
		_close_session(session)
		return false
	end

	if session._client == nil then
		return false
	end

	local now = _now_ms()
	_prune_all(now)

	local list = _buckets[key]
	if type(list) ~= "table" then
		list = {}
		_buckets[key] = list
	end

	if #list >= _max or _count_all() >= (_max * 8) then
		_close_session(session)
		_stats.drop = _stats.drop + 1
		return false
	end

	list[#list + 1] = { session = session, last_ms = now }
	_stats.put = _stats.put + 1
	return true
end


-- @brief 异步补满一槽 (factory 须在协程内调用 open_session)
-- @param [in]  key[string]
-- @param [in]  factory[function]	`() -> ok, session|err`
M.ensure_async = function (key, factory)
	if not _enabled or type(key) ~= "string" or key == "" or type(factory) ~= "function" then
		return
	end

	if _refilling[key] == true then
		return
	end

	local list = _buckets[key]
	local n = (type(list) == "table") and #list or 0
	if n >= _max then
		return
	end

	local ok_c, kco = pcall(require, "kco")
	if not ok_c or type(kco) ~= "table" or type(kco.fork) ~= "function" then
		return
	end

	_refilling[key] = true
	kco.fork(function ()
		local ok_s, sess = factory()
		if ok_s and type(sess) == "table" then
			if M.put(key, sess) then
				_stats.refill = _stats.refill + 1
			end
		elseif type(sess) == "table" then
			_close_session(sess)
		end
		-- 补池结束才清标记: wait() 据此判「已无补池在途」
		_refilling[key] = nil
	end)
end


-- @brief 当前 key 可用会话数 (先剔除过期)
-- @param [in]  key[string]
-- @return [number]
M.count = function (key)
	if type(key) ~= "string" or key == "" then
		return 0
	end

	_prune_all(_now_ms())
	local list = _buckets[key]
	if type(list) ~= "table" then
		return 0
	end

	return #list
end


-- @brief 等待 key 补池 (须在 kco 协程内; 供 bench 轮间衔接)
-- @note  有货 或 已无补池在途 即返回; 不自行发起补池
-- @param [in]  key[string]
-- @param [in]  timeout_ms[number|nil]	默认 5000
-- @return [boolean] 是否已有可用会话
M.wait = function (key, timeout_ms)
	if type(key) ~= "string" or key == "" then
		return false
	end

	local ok_c, kco = pcall(require, "kco")
	if not ok_c or type(kco) ~= "table" or type(kco.co_sleep) ~= "function" then
		return false
	end

	local limit = tonumber(timeout_ms) or 5000
	local t0 = _now_ms()
	while true do
		if M.count(key) > 0 then
			return true
		end

		if _refilling[key] ~= true then
			return false
		end

		if (_now_ms() - t0) > limit then
			return false
		end

		kco.co_sleep(10)
	end
end


-- @brief 剔除并关闭某一 key 下全部暖会话 (死连接 / hit_fail 后用)
-- @param [in]  key[string]
-- @return [number] 关闭条数
M.drop_key = function (key)
	if type(key) ~= "string" or key == "" then
		return 0
	end

	local list = _buckets[key]
	local n = 0
	if type(list) == "table" then
		for i = 1, #list do
			_close_session(list[i].session)
			n = n + 1
			_stats.drop = _stats.drop + 1
		end
	end

	_buckets[key] = nil
	_refilling[key] = nil
	return n
end


-- @brief 清空并断开全部暖会话
M.clear = function ()
	for k, list in pairs(_buckets) do
		if type(list) == "table" then
			for i = 1, #list do
				_close_session(list[i].session)
			end
		end
		_buckets[k] = nil
	end
	_refilling = {}
end


-- @brief 扫空闲 (供入口 janitor; 满 hop_idle 无活动则整池自断)
M.sweep = function ()
	if not _enabled then
		return
	end

	_prune_all(_now_ms())
end


-- @brief 统计
-- @return [table]
M.stats = function ()
	return {
		hit = _stats.hit,
		miss = _stats.miss,
		put = _stats.put,
		drop = _stats.drop,
		refill = _stats.refill,
		size = _count_all(),
		max = _max,
		idle_ms = _idle_ms,
		enabled = _enabled,
		last_activity_ms = _last_activity_ms,
	}
end


return M

-- end
