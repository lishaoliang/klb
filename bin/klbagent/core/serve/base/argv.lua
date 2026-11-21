--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   argv.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 子模块: 解析 --serve 位置参数 (port / profile)
-- @stability internal
-- @impl evolving
-- @note   勿用 --flag: C 壳 getopt 会吃掉未知长选项; 未点名档位回落 pref runtime.json
--   \n @stability internal: 实现文件, 契约面见 core/serve/init.lua
-- @history 修改历史
--  \n [2026-11] 自 proc/run.lua 迁出 (单线程 run 删除后保留解析)
--]]
local profiles = require("klbagent.core.profiles")


local M = {}


-- @brief 解析 serve argv (port / profile; 未点名档位回落 pref runtime.json)
-- @param [in] argv[table]
-- @return opts[table|nil]				{ port, port_explicit, profile, profile_explicit }
-- @return err[string|nil]
M.parse_argv = function (argv)
	local listen_port = nil
	local port_explicit = false
	local profile_name = nil
	local profile_explicit = false
	local i = 1

	if type(argv) ~= "table" then
		argv = {}
	end

	while i <= #argv do
		local a = argv[i]
		if string.match(a, "^%d+$") then
			listen_port = tonumber(a)
			port_explicit = true
			i = i + 1
		elseif a == "readonly" or a == "net" or a == "device" or a == "unsafe" then
			profile_name = a
			profile_explicit = true
			i = i + 1
		else
			return nil, "bad serve arg: " .. tostring(a)
		end
	end

	if port_explicit then
		if type(listen_port) ~= "number" or listen_port < 1 or 65535 < listen_port then
			return nil, "bad port"
		end
	end

	if not profile_explicit then
		local config = require("klbagent.core.config")
		local cfg = config.runtime.load()
		if type(cfg) == "table" and profiles.is_profile(cfg.profile) then
			profile_name = cfg.profile
		else
			profile_name = "readonly"
		end
	end

	return {
		port = listen_port,
		port_explicit = port_explicit,
		profile = profile_name,
		profile_explicit = profile_explicit,
	}
end


return M

-- end
