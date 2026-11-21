--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  alua serve 子模块: 进程/文件/字符串小工具 (无状态)
-- @note   由 core.serve 各子模块共用; 不做业务判断
--   \n @stability internal: 实现文件, 勿 require 子路径
-- @history 修改历史
--  \n [2026-11] 从 core/serve/init.lua 拆出
--]]
local M = {}


-- @brief shell 单引号转义
-- @param [in] s[string]
-- @return quoted[string]
M.sh_quote = function (s)
	return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end


-- @brief 是否 Windows
-- @return is_win[boolean]
M.is_windows = function ()
	return package.config:sub(1, 1) == "\\"
end


-- @brief 本进程 pid (/proc/self/stat 首字段; 非 Linux 为 nil)
-- @return pid[number|nil]
M.self_pid = function ()
	local f = io.open("/proc/self/stat", "r")
	if f == nil then
		return nil
	end

	local line = f:read("*l") or ""
	f:close()
	return tonumber(string.match(line, "^(%d+)"))
end


-- @brief 当前 alua 可执行文件路径
-- @return path[string]
M.alua_bin = function ()
	if M.is_windows() then
		return "alua.exe"
	end

	local f = io.popen("readlink -f /proc/self/exe 2>/dev/null", "r")
	if f ~= nil then
		local path = f:read("*l") or ""
		f:close()
		if type(path) == "string" and path ~= "" then
			return path
		end
	end

	return "/usr/local/bin/alua"
end


-- @brief 读整个文件
-- @param [in] path[string]
-- @return data[string|nil]
M.read_all = function (path)
	if type(path) ~= "string" or path == "" then
		return nil
	end

	local f = io.open(path, "rb")
	if f == nil then
		return nil
	end

	local data = f:read("*a")
	f:close()
	if type(data) ~= "string" or data == "" then
		return nil
	end

	return data
end


-- @brief 文件是否可读
-- @param [in] path[string]
-- @return ok[boolean]
M.file_readable = function (path)
	if type(path) ~= "string" or path == "" then
		return false
	end

	local f = io.open(path, "rb")
	if f == nil then
		return false
	end

	f:close()
	return true
end


-- @brief 环境变量是否真值 (1/true/on/yes)
-- @param [in] name[string]
-- @return truthy[boolean]
M.env_truthy = function (name)
	local v = os.getenv(name)
	if type(v) ~= "string" or v == "" then
		return false
	end

	local low = string.lower(v)
	return low == "1" or low == "true" or low == "on" or low == "yes"
end


return M

-- end
