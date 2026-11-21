--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua_web 可选包入口 (alua 配置页 www)
-- @note   alua pkg run alua_web serve @web.json; 前台阻塞
--   \n 常驻由 alua setup web yes 生成单元/任务拉起
-- @history 修改历史
--  \n [2026-10] 创建文件 (桩); serve 入口接 server.start (klbweb 端口组 + /api/v1/*)
--]]
local codes = require("klbagent.code")


local M = {}


M.help = function ()
	print("Usage: alua pkg run alua_web serve [config path|@path|web.json]")
	print("       alua pkg run alua_web help")
end


-- @brief 位置参数: 取 serve 的配置路径 (@path / *.json / 绝对路径)
-- @param [in] argv[table]
-- @return cfg_path[string|nil]
local function _cfg_path_of(argv)
	local a = argv[2]
	if type(a) ~= "string" or a == "" then
		return nil
	end

	if string.sub(a, 1, 1) == "@" and #a > 1 then
		return string.sub(a, 2)
	end

	return a
end


-- @brief 包入口
-- @param [in] argv[table]			位置参数
-- @param [in] man[table]			包清单
-- @return ok[boolean]
-- @return code[number]
-- @return msg[string]
M.entry = function (argv, man)
	argv = argv or {}
	local cmd = argv[1]

	-- 无子命令: 打印用法, 不默认阻塞
	if type(cmd) ~= "string" or cmd == "" then
		M.help()
		return false, codes.alua_web.BAD_REQUEST, "usage: alua pkg run alua_web serve [config path|@path]"
	end

	if cmd == "help" or cmd == "--help" or cmd == "-h" then
		M.help()
		return true, codes.OK, ""
	end

	if cmd ~= "serve" then
		M.help()
		return false, codes.alua_web.BAD_REQUEST, "usage: alua pkg run alua_web serve [config path|@path]"
	end

	local server = require("klbagent.packages.alua_web.server")
	local ok, err = server.start(_cfg_path_of(argv))
	if not ok then
		return false, codes.alua_web.TOOL_FAILED, tostring(err or "web serve fail")
	end

	return true, codes.OK, "alua web stopped"
end


return M

-- end
