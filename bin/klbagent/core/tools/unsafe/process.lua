--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   process.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  unsafe.process_list / unsafe.process_kill
-- @note   POSIX /proc; Win32 tasklist/taskkill (ppid/cmd 由 wmic 可选补充)
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; 补 Win32 分支 (tasklist/taskkill)
--]]


local lfs = require("lfs")
local util = require("klbagent.core.tools.unsafe.util")


local M = {}


local DEFAULT_PROC_MAX = 200
local MAX_PROC_MAX = 2000


local function _read_proc_cmd(pid)
	local f = io.open("/proc/" .. tostring(pid) .. "/cmdline", "rb")
	if f == nil then
		return ""
	end

	local raw = f:read("*a") or ""
	f:close()
	if raw == "" then
		return ""
	end

	local parts = {}
	for part in string.gmatch(raw, "([^%z]+)") do
		parts[#parts + 1] = part
	end

	return table.concat(parts, " ")
end


local function _read_proc_comm(pid)
	local f = io.open("/proc/" .. tostring(pid) .. "/comm", "rb")
	if f == nil then
		return ""
	end

	local s = f:read("*l") or ""
	f:close()
	return s
end


local function _read_proc_ppid(pid)
	local f = io.open("/proc/" .. tostring(pid) .. "/stat", "rb")
	if f == nil then
		return nil
	end

	local line = f:read("*l") or ""
	f:close()
	-- pid (comm) state ppid ...
	local ppid = string.match(line, "^%d+ %b() %S+ (%d+)")
	return tonumber(ppid)
end


local function _trim(s)
	return (string.gsub(s, "^%s*(.-)%s*$", "%1"))
end


-- @brief Win32 临时文件路径 (%TEMP% 下)
local function _win_tmp_path(tag)
	local dir = os.getenv("TEMP") or os.getenv("TMP") or "."
	local uniq = tostring({}):match("(%x+)$") or "0"
	return dir .. "\\alua_" .. uniq .. "_" .. tag .. ".tmp"
end


-- @brief CSV 行解析 (支持双引号字段与 "" 转义)
local function _csv_parse(line)
	local out = {}
	local i = 1
	local n = #line
	while i <= n do
		local c = string.sub(line, i, i)
		if c == '"' then
			i = i + 1
			local buf = {}
			while i <= n do
				c = string.sub(line, i, i)
				if c == '"' then
					if string.sub(line, i + 1, i + 1) == '"' then
						buf[#buf + 1] = '"'
						i = i + 2
					else
						i = i + 1
						break
					end
				else
					buf[#buf + 1] = c
					i = i + 1
				end
			end

			out[#out + 1] = table.concat(buf)
		else
			local j = string.find(line, ",", i, true)
			if j == nil then
				j = n + 1
			end

			out[#out + 1] = string.sub(line, i, j - 1)
			i = j
		end

		if i <= n and string.sub(line, i, i) == "," then
			i = i + 1
		end
	end

	return out
end


-- @brief Win32 进程详情 (wmic 可用时补 ppid/cmd; 失败返回空表)
local function _win_proc_detail()
	local map = {}
	local out_path = _win_tmp_path("wmic")
	os.execute("wmic process get ProcessId,ParentProcessId,CommandLine /format:csv > "
		.. util.win_quote(out_path) .. " 2>nul")

	local f = io.open(out_path, "rb")
	if f == nil then
		return map
	end

	local idx_pid = nil
	local idx_ppid = nil
	local idx_cmd = nil
	for raw in f:lines() do
		raw = _trim(raw)
		if raw ~= "" then
			local fields = _csv_parse(raw)
			if idx_pid == nil then
				-- 表头: Node,CommandLine,ParentProcessId,ProcessId
				for i = 1, #fields do
					local k = string.lower(_trim(fields[i]))
					if k == "processid" then
						idx_pid = i
					elseif k == "parentprocessid" then
						idx_ppid = i
					elseif k == "commandline" then
						idx_cmd = i
					end
				end

				if idx_pid == nil then
					break
				end
			else
				local pid = tonumber(_trim(fields[idx_pid] or ""))
				if pid ~= nil then
					map[pid] = {
						ppid = idx_ppid and tonumber(_trim(fields[idx_ppid] or "")) or nil,
						cmd = (idx_cmd and fields[idx_cmd]) or "",
					}
				end
			end
		end
	end

	f:close()
	os.remove(out_path)
	return map
end


-- @brief Win32 进程枚举 (tasklist CSV; 详情可选)
local function _win_process_list(filter_pid, name_substr, max_entries)
	local detail = _win_proc_detail()
	local out_path = _win_tmp_path("tasklist")
	os.execute("tasklist /FO CSV /NH > " .. util.win_quote(out_path) .. " 2>nul")

	local f = io.open(out_path, "rb")
	if f == nil then
		error("tasklist unavailable")
	end

	local entries = {}
	local truncated = false
	for raw in f:lines() do
		local fields = _csv_parse(_trim(raw))
		local comm = _trim(fields[1] or "")
		local pid = tonumber(_trim(fields[2] or ""))
		if pid ~= nil then
			if filter_pid == nil or pid == filter_pid then
				local d = detail[pid]
				local cmd = (d and d.cmd) or ""
				if cmd == "" then
					cmd = comm
				end

				local hit = true
				if type(name_substr) == "string" and name_substr ~= "" then
					hit = string.find(cmd, name_substr, 1, true) ~= nil
						or string.find(comm, name_substr, 1, true) ~= nil
				end

				if hit then
					if #entries >= max_entries then
						truncated = true
						break
					end

					entries[#entries + 1] = {
						pid = pid,
						ppid = (d and d.ppid) or nil,
						comm = comm,
						cmd = cmd,
					}
				end
			end
		end
	end

	f:close()
	os.remove(out_path)

	table.sort(entries, function (a, b)
		return a.pid < b.pid
	end)

	return {
		ok = true,
		count = #entries,
		truncated = truncated,
		entries = entries,
	}
end


local function _process_list(args)
	args = args or {}
	local max_entries = tonumber(args.max_entries) or DEFAULT_PROC_MAX
	max_entries = math.floor(max_entries)
	if max_entries < 1 then
		max_entries = DEFAULT_PROC_MAX
	end

	if max_entries > MAX_PROC_MAX then
		max_entries = MAX_PROC_MAX
	end

	local name_substr = args.name_substr
	if name_substr ~= nil and type(name_substr) ~= "string" then
		error("bad name_substr")
	end

	local filter_pid = tonumber(args.pid)
	local entries = {}
	local truncated = false

	if util.is_windows() then
		return _win_process_list(filter_pid, name_substr, max_entries)
	end

	if lfs.attributes("/proc", "mode") ~= "directory" then
		error("/proc unavailable")
	end

	for name in lfs.dir("/proc") do
		local pid = tonumber(name)
		if pid ~= nil then
			if filter_pid == nil or pid == filter_pid then
				local cmd = _read_proc_cmd(pid)
				local comm = _read_proc_comm(pid)
				if cmd == "" then
					cmd = comm
				end

				local hit = true
				if type(name_substr) == "string" and name_substr ~= "" then
					hit = string.find(cmd, name_substr, 1, true) ~= nil
						or string.find(comm, name_substr, 1, true) ~= nil
				end

				if hit then
					if #entries >= max_entries then
						truncated = true
						break
					end

					entries[#entries + 1] = {
						pid = pid,
						ppid = _read_proc_ppid(pid),
						comm = comm,
						cmd = cmd,
					}
				end
			end
		end
	end

	table.sort(entries, function (a, b)
		return a.pid < b.pid
	end)

	return {
		ok = true,
		count = #entries,
		truncated = truncated,
		entries = entries,
	}
end


local function _process_kill(args)
	args = args or {}
	local pid = tonumber(args.pid)
	if pid == nil or pid <= 1 or pid ~= math.floor(pid) then
		error("need pid > 1")
	end

	local signal = args.signal
	if signal == nil or signal == "" then
		signal = "TERM"
	end

	if type(signal) ~= "string" then
		error("bad signal")
	end

	signal = string.upper(signal)
	if signal ~= "TERM" and signal ~= "KILL" then
		error("signal must be TERM or KILL")
	end

	if util.is_windows() then
		local line = "taskkill /PID " .. tostring(math.floor(pid))
		if signal == "KILL" then
			line = line .. " /F"
		end

		line = line .. " >nul 2>&1"
		local ok_ex, what, code = os.execute(line)
		local ok = (ok_ex == true) or (what == "exit" and code == 0)
		if not ok then
			error("kill fail pid=" .. tostring(pid) .. " signal=" .. signal)
		end

		return {
			ok = true,
			pid = math.floor(pid),
			signal = signal,
		}
	end

	local line = "kill -s " .. signal .. " " .. tostring(math.floor(pid))
		.. " >/dev/null 2>&1"
	local ok_ex, what, code = os.execute(line)
	local ok = (ok_ex == true) or (what == "exit" and code == 0)
	if not ok then
		error("kill fail pid=" .. tostring(pid) .. " signal=" .. signal)
	end

	return {
		ok = true,
		pid = math.floor(pid),
		signal = signal,
	}
end


M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "unsafe.process_list",
		category = "unsafe",
		title = "list processes",
		summary = "List /proc processes; optional name_substr/pid/max_entries. Needs profile unsafe.",
		min_profile = "unsafe",
		args_schema = {
			name_substr = "string optional substring match on cmd/comm",
			pid = "number optional exact pid",
			max_entries = "number optional default 200 max 2000",
		},
		examples = {
			{ args = { name_substr = "alua", max_entries = 50 } },
			{ args = { pid = 1 } },
		},
		docs = "tools/unsafe.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("unsafe.process_list", _process_list)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "unsafe.process_kill",
		category = "unsafe",
		title = "kill process by pid",
		summary = "Send TERM or KILL to pid (>1). Needs profile unsafe.",
		min_profile = "unsafe",
		args_schema = {
			pid = "number process id (>1)",
			signal = "string optional TERM|KILL (default TERM)",
		},
		examples = {
			{ args = { pid = 12345 } },
			{ args = { pid = 12345, signal = "KILL" } },
		},
		docs = "tools/unsafe.md",
		side_effects = { "unsafe" },
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("unsafe.process_kill", _process_kill)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
