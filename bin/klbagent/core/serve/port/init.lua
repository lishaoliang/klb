--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve/port 子模块: 监听口池/随机段常量与 pref/serve.port 落盘
-- @note   未显式端口: 池 12817→12810; 全占则随机 11000–12000 (最多 3 次); 见 docs/rules/ports.md
--   \n 正式 --serve 选口/bind 在 worker/listen (候选口由 base.start 组装); 本模块只供池常量与落盘口读写
--   \n @stability internal: 实现文件, 契约面见 core/serve/init.lua
-- @history 修改历史
--  \n [2026-11] 创建文件; 从 core/serve/init.lua 拆出 port_pool / serve.port / 选口
--  \n [2026-11] 增 clear_listen_port (ssid XPC 等 listen 落口前清脏口)
--]]
local store = require("klbagent.core.config.store")


local M = {}


-- @brief pref 下 serve.port 完整路径
-- @return path[string|nil]
local function _serve_port_path()
	local dir = store.dir()
	if type(dir) ~= "string" or dir == "" then
		return nil
	end

	dir = string.gsub(dir, "[/\\]+$", "")
	return dir .. "/serve.port"
end


-- 默认监听端口 (局域网 P0; 亦为自动选口池首选)
M.DEFAULT_PORT = 12817

-- serve 自动选口池下沿 (向前递减至本值, 含; 共 8 口: 12817..12810)
M.PORT_POOL_LO = 12810

-- 池全冲突后随机段 (含两端) 与重试次数
M.RANDOM_PORT_MIN = 11000
M.RANDOM_PORT_MAX = 12000
M.RANDOM_PORT_TRIES = 3


-- @brief serve 自动选口池 (首选 DEFAULT_PORT 起向前递减至 PORT_POOL_LO)
-- @return ports[table]					number 数组
M.port_pool = function ()
	local ports = {}
	local p = M.DEFAULT_PORT

	while p >= M.PORT_POOL_LO do
		ports[#ports + 1] = p
		p = p - 1
	end

	return ports
end


-- @brief 写出实际监听口到 pref/serve.port (供客户端读取)
-- @param [in] port[number]
-- @return ok[boolean]
M.write_listen_port = function (port)
	local path = _serve_port_path()
	if path == nil then
		return false
	end

	local f = io.open(path, "wb")
	if f == nil then
		return false
	end

	f:write(tostring(port))
	f:write("\n")
	f:close()
	return true
end


-- @brief 清除 pref/serve.port (起 serve 前去脏口; 幂等)
-- @return ok[boolean]					无文件亦 true
M.clear_listen_port = function ()
	local path = _serve_port_path()
	if path == nil then
		return false
	end

	os.remove(path)
	return true
end


-- @brief 读 pref/serve.port (上次/当前 serve 落盘口; 无则 nil)
-- @return port[number|nil]
M.read_listen_port = function ()
	local path = _serve_port_path()
	if path == nil then
		return nil
	end

	local f = io.open(path, "rb")
	if f == nil then
		return nil
	end

	local data = f:read("*a") or ""
	f:close()
	local n = tonumber(string.match(data, "(%d+)"))
	if type(n) ~= "number" or n < 1 or 65535 < n then
		return nil
	end

	return n
end


return M

-- end
