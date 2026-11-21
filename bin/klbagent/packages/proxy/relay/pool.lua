--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   pool.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  上游出站连接池 (HTTP 明文转发 keep-alive; 直连)
-- @note   仅直连 tcp; 有链 hops 不入池; 与入口客户端 keep-alive 正交
--]]


local M = {}


local _max = 16
local _idle_ms = 30000
-- key -> { { tcp=, last_ms= }, ... }
local _buckets = {}
local _stats = {
	hit = 0,
	miss = 0,
	put = 0,
	drop = 0,
}


local function _now_ms()
	return os.time() * 1000
end


local function _key(host, port)
	return tostring(host or "") .. ":" .. tostring(tonumber(port) or 0)
end


local function _close_tcp(tcp)
	if tcp ~= nil and type(tcp.close) == "function" then
		pcall(function ()
			tcp:close()
		end)
	end
end


local function _prune_bucket(list, now)
	if type(list) ~= "table" then
		return
	end

	local i = 1
	while i <= #list do
		local it = list[i]
		local age = now - (tonumber(it.last_ms) or 0)
		if age > _idle_ms then
			_close_tcp(it.tcp)
			table.remove(list, i)
			_stats.drop = _stats.drop + 1
		else
			i = i + 1
		end
	end
end


local function _count_all()
	local n = 0
	for _, list in pairs(_buckets) do
		n = n + #list
	end
	return n
end


-- @brief 取空闲上游 (无则 nil)
-- @param [in]  host[string]
-- @param [in]  port[number]
-- @return [userdata|nil] tcp
M.take = function (host, port)
	local k = _key(host, port)
	local list = _buckets[k]
	local now = _now_ms()
	_prune_bucket(list, now)
	if type(list) ~= "table" or #list == 0 then
		_stats.miss = _stats.miss + 1
		return nil
	end

	local it = table.remove(list)
	_stats.hit = _stats.hit + 1
	return it.tcp
end


-- @brief 归还上游 (池满则关)
-- @param [in]  host[string]
-- @param [in]  port[number]
-- @param [in]  tcp[userdata]
-- @return [boolean] kept
M.put = function (host, port, tcp)
	if tcp == nil then
		return false
	end

	local now = _now_ms()
	if _count_all() >= _max then
		_close_tcp(tcp)
		_stats.drop = _stats.drop + 1
		return false
	end

	local k = _key(host, port)
	local list = _buckets[k]
	if type(list) ~= "table" then
		list = {}
		_buckets[k] = list
	end

	list[#list + 1] = { tcp = tcp, last_ms = now }
	_stats.put = _stats.put + 1
	return true
end


-- @brief 清空
M.clear = function ()
	for k, list in pairs(_buckets) do
		if type(list) == "table" then
			for i = 1, #list do
				_close_tcp(list[i].tcp)
			end
		end
		_buckets[k] = nil
	end
end


-- @brief 配置上限 / 空闲
-- @param [in]  opts[table|nil] `{ max=, idle_ms= }`
M.configure = function (opts)
	if type(opts) ~= "table" then
		return
	end
	if opts.max ~= nil then
		local n = tonumber(opts.max)
		if n ~= nil and n >= 1 and n <= 256 then
			_max = math.floor(n)
		end
	end
	if opts.idle_ms ~= nil then
		local n = tonumber(opts.idle_ms)
		if n ~= nil and n >= 1000 then
			_idle_ms = math.floor(n)
		end
	end
end


-- @brief 统计
-- @return [table]
M.stats = function ()
	return {
		hit = _stats.hit,
		miss = _stats.miss,
		put = _stats.put,
		drop = _stats.drop,
		size = _count_all(),
		max = _max,
		idle_ms = _idle_ms,
	}
end


return M

-- end
