--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   util.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  unsafe.* 公共辅助
-- @note   shell 引号 / 超时与输出截断 / Win32 判定
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local M = {}


M.DEFAULT_TIMEOUT_SEC = 60
M.MAX_TIMEOUT_SEC = 300
M.DEFAULT_MAX_BYTES = 1048576
M.MAX_MAX_BYTES = 8 * 1048576


function M.shell_quote(s)
	return "'" .. string.gsub(tostring(s), "'", "'\\''") .. "'"
end


-- @brief 是否 Win32 宿主 (与 klbagent.core.hostinfo 判定同源, 免子进程)
function M.is_windows()
	return package.config:sub(1, 1) == "\\"
end


-- @brief cmd.exe 语义引号: 含空白/元字符才加双引号, 内部 " 转义为 \"
function M.win_quote(s)
	s = tostring(s)
	if string.find(s, '[%s"&|<>%^]') == nil then
		return s
	end

	return '"' .. string.gsub(s, '"', '\\"') .. '"'
end


function M.clamp_timeout(v)
	local n = tonumber(v)
	if n == nil or n <= 0 then
		return M.DEFAULT_TIMEOUT_SEC
	end

	n = math.floor(n)
	if n > M.MAX_TIMEOUT_SEC then
		return M.MAX_TIMEOUT_SEC
	end

	return n
end


function M.clamp_max_bytes(v)
	local n = tonumber(v)
	if n == nil or n <= 0 then
		return M.DEFAULT_MAX_BYTES
	end

	n = math.floor(n)
	if n > M.MAX_MAX_BYTES then
		return M.MAX_MAX_BYTES
	end

	return n
end


function M.has_timeout()
	if M.is_windows() then
		return false
	end

	local f = io.popen("command -v timeout 2>/dev/null")
	if f == nil then
		return false
	end

	local path = f:read("*l")
	f:close()
	return type(path) == "string" and path ~= ""
end


function M.read_trunc(path, max_n)
	local f = io.open(path, "rb")
	if f == nil then
		return "", 0, false
	end

	local data = f:read("*a") or ""
	f:close()
	local n = #data
	local truncated = false
	if n > max_n then
		data = string.sub(data, 1, max_n)
		truncated = true
	end

	return data, n, truncated
end


return M

-- end
