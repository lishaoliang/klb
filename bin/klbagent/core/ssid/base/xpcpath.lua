--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   xpcpath.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua.ssid XPC unix 路径拼装 (Lua 指定; C 仅透传)
-- @stability internal
-- @impl evolving
-- @note   require("klbagent.core.ssid.base.xpcpath")
--   \n 格式: /tmp/alua.ssid.xpc.unix.pxxxx (xxxx=serve 实际/候选口)
--   \n C 默认 /tmp/klb.xpc.<name> 仅作空 path 回退; alua 侧禁依赖该默认
-- @history 修改历史
--  \n [2026-11] 创建文件
--]]
local port_mod = require("klbagent.core.serve.port")


local M = {}


-- 固定前缀 (端口数字紧随 p)
M.PATH_PREFIX = "/tmp/alua.ssid.xpc.unix.p"


-- @brief 端口是否合法
-- @param [in] port[any]
-- @return ok[boolean]
-- @return n[number|nil]
local function _valid_port(port)
	local n = tonumber(port)
	if type(n) ~= "number" or n < 1 or 65535 < n then
		return false, nil
	end

	return true, n
end


-- @brief 由端口拼 unix 路径
-- @param [in] port[number|string]
-- @return path[string|nil]
M.make = function (port)
	local ok, n = _valid_port(port)
	if not ok then
		return nil
	end

	return M.PATH_PREFIX .. tostring(n)
end


-- @brief 解析 opts → 完整 unix 路径
-- @param [in] opts[table|nil]		path? / port? / ports?[1]
-- @return path[string|nil]
-- @return port[number|nil]
-- @return err[string|nil]
-- @note 显式 path 非空则原样返回; 否则 port → ports[1] → serve.port 落盘 → DEFAULT_PORT
M.resolve = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	if type(opts.path) == "string" and opts.path ~= "" then
		local ok_p, n = _valid_port(opts.port)
		if ok_p then
			return opts.path, n, nil
		end

		return opts.path, nil, nil
	end

	local n = nil
	local ok_n = false
	ok_n, n = _valid_port(opts.port)
	if not ok_n and type(opts.ports) == "table" and #opts.ports > 0 then
		ok_n, n = _valid_port(opts.ports[1])
	end

	if not ok_n then
		n = port_mod.read_listen_port()
		ok_n = (type(n) == "number")
	end

	if not ok_n then
		n = port_mod.DEFAULT_PORT
		ok_n = true
	end

	local path = M.make(n)
	if path == nil then
		return nil, nil, "alua.ssid xpc path: bad port"
	end

	return path, n, nil
end


return M

-- end
