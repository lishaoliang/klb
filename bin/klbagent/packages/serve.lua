--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   serve.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  packages 侧 serve 分流: 分辨 --serve 首参是包名还是 core.serve 参数
-- @stability evolving
-- @impl evolving
-- @note   供 klbagent.entry_serve 调用; 首参命中已安装包 → packages.entry; 否则交 core.serve
--   \n 数字首参视为端口 (core.serve); 非包名字符串亦交 core.serve (如 profile)
-- @history 修改历史
--  \n [2026-11] 创建文件
--]]
local packages = require("klbagent.packages")


local M = {}


-- @brief 尝试按包名加载清单 (合法名 + 已安装)
-- @param [in] name[string]
-- @return man[table|nil]
local function _try_manifest(name)
	if type(name) ~= "string" or name == "" then
		return nil
	end

	if string.find(name, "[^%w_%-]") ~= nil then
		return nil
	end

	local mod_name = "klbagent.packages." .. name .. ".package"
	local ok, man = pcall(require, mod_name)
	if not ok or type(man) ~= "table" then
		return nil
	end

	return man
end


-- @brief 分辨 --serve 位置参数: 包服务 vs core.serve
-- @param [in] ...					与 entry_serve 相同的位置参数
-- @return name[string|nil]			命中包名; nil=走 core.serve
-- @return argv[table]				交给包 entry 的剩余参数 (未命中时为完整 argv)
M.resolve = function (...)
	local argv = { ... }
	local first = argv[1]

	-- 无参 / 非字符串 → core.serve
	if type(first) ~= "string" or first == "" then
		return nil, argv
	end

	-- 数字端口 → core.serve
	if tonumber(first) ~= nil then
		return nil, argv
	end

	local man = _try_manifest(first)
	if man == nil then
		return nil, argv
	end

	local rest = {}
	for i = 2, #argv do
		rest[#rest + 1] = argv[i]
	end

	return first, rest
end


-- @brief 启动 packages 注册的服务 (转发 packages.entry)
-- @param [in] name[string]			包名
-- @param [in] argv[table|nil]		剩余参数
-- @return ok[boolean]
-- @return code[number]
-- @return hold[any]
M.entry = function (name, argv)
	return packages.entry(name, argv)
end


return M

-- end
