--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   stats.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  proxy 入口连接/拒绝计数 (观测面)
-- @note   进程内累加; 入口 janitor 周期落 runtime 快照供跨进程 status; entry/status 读取
--]]


local M = {}


-- 运行态快照文件 (跨进程 status 读取; 与 proxy.json 同目录)
local RUNTIME_FILE = "proxy_runtime.json"
-- 快照过期秒数 (入口 janitor 周期写; 超出即判入口已停)
local RUNTIME_MAX_AGE_S = 90


local _s = {
	accepts = 0,
	active = 0,
	closed = 0,
	auth_fail = 0,
	reject = 0,
	probe_noise = 0,
	bytes_c2u = 0,
	bytes_u2c = 0,
}


-- @brief 快照
-- @return [table]
M.snapshot = function ()
	return {
		accepts = _s.accepts,
		active = _s.active,
		closed = _s.closed,
		auth_fail = _s.auth_fail,
		reject = _s.reject,
		probe_noise = _s.probe_noise,
		bytes_c2u = _s.bytes_c2u,
		bytes_u2c = _s.bytes_u2c,
	}
end


M.on_accept = function ()
	_s.accepts = _s.accepts + 1
	_s.active = _s.active + 1
end


M.on_close = function ()
	if _s.active > 0 then
		_s.active = _s.active - 1
	end
	_s.closed = _s.closed + 1
end


M.on_auth_fail = function ()
	_s.auth_fail = _s.auth_fail + 1
end


M.on_reject = function ()
	_s.reject = _s.reject + 1
end


M.on_probe_noise = function ()
	_s.probe_noise = _s.probe_noise + 1
end


-- @brief 累加中继字节 (可选; 由包装泵调用)
M.add_bytes = function (c2u, u2c)
	local a = tonumber(c2u) or 0
	local b = tonumber(u2c) or 0
	if a > 0 then
		_s.bytes_c2u = _s.bytes_c2u + a
	end
	if b > 0 then
		_s.bytes_u2c = _s.bytes_u2c + b
	end
end


M.reset = function ()
	_s.accepts = 0
	_s.active = 0
	_s.closed = 0
	_s.auth_fail = 0
	_s.reject = 0
	_s.probe_noise = 0
	_s.bytes_c2u = 0
	_s.bytes_u2c = 0
end


-- @brief store 模块 (pref_path 目录 + JSON 路径)
-- @return [table|nil]
local function _store()
	local ok_s, S = pcall(require, "klbagent.core.config.store")
	if ok_s and type(S) == "table" and type(S.path) == "function" then
		return S
	end

	return nil
end


-- @brief 运行态快照落盘 (供另一进程 status 读取; 入口 janitor 周期调用)
-- @return [boolean] ok
M.save_runtime = function ()
	local S = _store()
	if S == nil then
		return false
	end

	local path = S.path(RUNTIME_FILE)
	if path == "" then
		return false
	end

	local ok_j, cjson = pcall(require, "cjson")
	if not ok_j or type(cjson) ~= "table" or type(cjson.encode) ~= "function" then
		return false
	end

	local obj = M.snapshot()
	obj.ts = os.time()

	local ok_e, text = pcall(cjson.encode, obj)
	if not ok_e or type(text) ~= "string" then
		return false
	end

	local f = io.open(path, "wb")
	if f == nil then
		return false
	end

	f:write(text)
	f:write("\n")
	f:close()
	return true
end


-- @brief 读运行态快照; 过期/缺失返回 nil
-- @return [table|nil]
M.load_runtime = function ()
	local S = _store()
	if S == nil then
		return nil
	end

	local path = S.path(RUNTIME_FILE)
	if path == "" then
		return nil
	end

	local f = io.open(path, "rb")
	if f == nil then
		return nil
	end

	local raw = f:read("*a")
	f:close()
	if type(raw) ~= "string" or raw == "" then
		return nil
	end

	local ok_j, cjson = pcall(require, "cjson")
	if not ok_j or type(cjson) ~= "table" or type(cjson.decode) ~= "function" then
		return nil
	end

	local ok_d, obj = pcall(cjson.decode, raw)
	if not ok_d or type(obj) ~= "table" then
		return nil
	end

	local ts = tonumber(obj.ts) or 0
	if (os.time() - ts) > RUNTIME_MAX_AGE_S then
		return nil
	end

	return obj
end


-- @brief 入口停止时清运行态快照
M.clear_runtime = function ()
	local S = _store()
	if S == nil then
		return
	end

	local path = S.path(RUNTIME_FILE)
	if path == "" then
		return
	end

	pcall(function ()
		os.remove(path)
	end)
end


return M

-- end
