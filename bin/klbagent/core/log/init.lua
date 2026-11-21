--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  Agent 统一结构化日志
-- @note   JSONL; 多出口; fields.module; ring; P3 subscribe/push_poll
-- @history 修改历史
--  \n [2026-09] 创建文件; 实装 level 过滤与可选落盘
--  \n [2026-10] P0/P1 分源 module; 多出口等级; 路径与轮转; 内存 ring + recent; P3 subscribe / push_poll / unsubscribe
--]]
local config_log = require("klbagent.core.config.log")
local store = require("klbagent.core.config.store")
local cjson = require("cjson")
local lfs = require("lfs")


local M = {}


local LEVEL_ORD = {
	error = 1,
	warn = 2,
	info = 3,
	debug = 4,
}


-- ring: circular buffer of row tables (pass remote_level)
local _ring = {}
local _ring_cap = 0
local _ring_head = 1
local _ring_len = 0


-- P3 push: per-subscriber queues (producer push, debugger poll)
local MAX_SUBS = 8
local DEFAULT_SUB_MAX = 100
local _subs = {}
local _sub_seq = 0


local function _level_ok(level, threshold)
	local a = LEVEL_ORD[level]
	local b = LEVEL_ORD[threshold]
	if a == nil or b == nil then
		return false
	end

	return a <= b
end


local function _cfg()
	local ok, cfg = pcall(config_log.load)
	if ok and type(cfg) == "table" then
		return cfg
	end

	return config_log.defaults()
end


local function _default_file_path()
	local dir = store.dir()
	if type(dir) ~= "string" or dir == "" then
		return ""
	end

	local path = dir
	if string.sub(path, -1) ~= "/" and string.sub(path, -1) ~= "\\" then
		path = path .. "/"
	end

	return path .. "alua.log"
end


-- @brief 解析结构化日志文件路径
-- @param [in] cfg[table|nil]
-- @return path[string]
M.file_path = function (cfg)
	if type(cfg) ~= "table" then
		cfg = _cfg()
	end

	if type(cfg.path) == "string" and cfg.path ~= "" then
		return cfg.path
	end

	return _default_file_path()
end


local function _norm_fields(fields)
	local out = {}
	if type(fields) == "table" then
		for k, v in pairs(fields) do
			out[k] = v
		end
	end

	if type(out.module) ~= "string" or out.module == "" then
		out.module = "unknown"
	end

	return out
end


local function _line(level, msg, fields)
	local row = {
		ts = os.date("!%Y-%m-%dT%H:%M:%SZ"),
		level = level,
		msg = tostring(msg or ""),
		fields = fields,
	}

	local ok_j, text = pcall(cjson.encode, row)
	if ok_j and type(text) == "string" then
		return text, row
	end

	return string.format(
		'{"ts":"%s","level":"%s","msg":%q,"fields":{"module":%q}}',
		row.ts,
		level,
		row.msg,
		fields.module
	), row
end


local function _ring_ensure(n)
	n = math.floor(tonumber(n) or 0)
	if n <= 0 then
		_ring = {}
		_ring_cap = 0
		_ring_head = 1
		_ring_len = 0
		return
	end

	if _ring_cap == n then
		return
	end

	local old = {}
	if _ring_len > 0 and _ring_cap > 0 then
		for i = 0, _ring_len - 1 do
			local idx = ((_ring_head - 1 + i) % _ring_cap) + 1
			old[#old + 1] = _ring[idx]
		end
	end

	_ring = {}
	_ring_cap = n
	_ring_head = 1
	_ring_len = 0
	local from = #old - n + 1
	if from < 1 then
		from = 1
	end

	for i = from, #old do
		_ring_len = _ring_len + 1
		_ring[_ring_len] = old[i]
	end

	if _ring_len == n then
		_ring_head = 1
	end
end


local function _ring_push(row, n)
	_ring_ensure(n)
	if _ring_cap <= 0 then
		return
	end

	if _ring_len < _ring_cap then
		_ring_len = _ring_len + 1
		_ring[_ring_len] = row
		return
	end

	_ring[_ring_head] = row
	_ring_head = (_ring_head % _ring_cap) + 1
end


local function _rotate_if_needed(path, max_bytes)
	if type(path) ~= "string" or path == "" then
		return
	end

	local n = tonumber(max_bytes) or 0
	if n <= 0 then
		return
	end

	local size = lfs.attributes(path, "size")
	if type(size) ~= "number" or size < n then
		return
	end

	local bak = path .. ".1"
	os.remove(bak)
	os.rename(path, bak)
end


local function _append_file(text, cfg)
	local path = M.file_path(cfg)
	if path == "" then
		return
	end

	_rotate_if_needed(path, cfg.max_bytes)

	local f = io.open(path, "ab")
	if f == nil then
		return
	end

	f:write(text)
	f:write("\n")
	f:close()
end


local function _sub_match(sub, row)
	if type(sub.level) == "string" and sub.level ~= "" then
		if type(row.level) ~= "string" or not _level_ok(row.level, sub.level) then
			return false
		end
	end

	if type(sub.module) == "string" and sub.module ~= "" then
		local mod = nil
		if type(row.fields) == "table" then
			mod = row.fields.module
		end

		if mod ~= sub.module then
			return false
		end
	end

	return true
end


local function _subs_fanout(row)
	for i = 1, #_subs do
		local sub = _subs[i]
		if _sub_match(sub, row) then
			local max_n = sub.max_n
			if #sub.buf >= max_n then
				table.remove(sub.buf, 1)
				sub.dropped = sub.dropped + 1
			end

			sub.buf[#sub.buf + 1] = row
		end
	end
end


local function _find_sub(sub_id)
	if type(sub_id) ~= "string" or sub_id == "" then
		return nil, 0
	end

	for i = 1, #_subs do
		if _subs[i].id == sub_id then
			return _subs[i], i
		end
	end

	return nil, 0
end


-- @brief 写日志
-- @param [in] level[string]	error|warn|info|debug
-- @param [in] msg[string]
-- @param [in] fields[table|nil]	须含 module; 可选 role/request_id/tool
M.write = function (level, msg, fields)
	if type(level) ~= "string" or level == "" then
		return
	end

	local cfg = _cfg()
	local th = cfg.level or "warn"
	if not _level_ok(level, th) then
		return
	end

	local norm = _norm_fields(fields)
	local text, row = _line(level, msg, norm)

	io.stderr:write(text)
	io.stderr:write("\n")

	if cfg.to_file == true then
		local file_th = cfg.file_level or cfg.level or "warn"
		if _level_ok(level, file_th) then
			_append_file(text, cfg)
		end
	end

	local remote_th = cfg.remote_level or "info"
	if _level_ok(level, remote_th) then
		_ring_push(row, cfg.ring_n or 0)
		_subs_fanout(row)
	end
end


-- @brief 当前配置与文件/ring 状态摘要 (供 log.status)
-- @return info[table]
M.status = function ()
	local cfg = _cfg()
	local path = M.file_path(cfg)
	local size = 0
	local exists = false
	if path ~= "" then
		local n = lfs.attributes(path, "size")
		if type(n) == "number" then
			exists = true
			size = n
		end
	end

	local host_kind = "linux"
	local ok, hi = pcall(require, "klbagent.core.hostinfo")
	if ok and type(hi) == "table" and type(hi.probe) == "function" then
		local info = hi.probe()
		if type(info) == "table" and type(info.host_kind) == "string" then
			host_kind = info.host_kind
		end
	end

	_ring_ensure(cfg.ring_n or 0)

	local push_subs = {}
	for i = 1, #_subs do
		local s = _subs[i]
		push_subs[#push_subs + 1] = {
			sub_id = s.id,
			module = s.module,
			level = s.level,
			pending = #s.buf,
			dropped = s.dropped,
			max_n = s.max_n,
		}
	end

	if #push_subs == 0 and cjson.empty_array ~= nil then
		push_subs = cjson.empty_array
	end

	return {
		cfg = cfg,
		path = path,
		exists = exists,
		size = size,
		host_kind = host_kind,
		config_path = config_log.path(),
		ring_n = _ring_cap,
		ring_len = _ring_len,
		push_sub_count = #_subs,
		push_subs = push_subs,
	}
end


local function _parse_row(text)
	local ok, row = pcall(cjson.decode, text)
	if ok and type(row) == "table" then
		return row
	end

	return nil
end


local function _row_pass(row, opts)
	if type(row) ~= "table" then
		return false
	end

	local level = row.level
	if type(level) ~= "string" then
		return false
	end

	local remote_th = opts.remote_level or "debug"
	if not _level_ok(level, remote_th) then
		return false
	end

	if type(opts.level) == "string" and opts.level ~= "" then
		if not _level_ok(level, opts.level) then
			return false
		end
	end

	if type(opts.module) == "string" and opts.module ~= "" then
		local fields = row.fields
		local mod = nil
		if type(fields) == "table" then
			mod = fields.module
		end

		if mod ~= opts.module then
			return false
		end
	end

	return true
end


local function _empty_lines()
	if cjson.empty_array ~= nil then
		return cjson.empty_array
	end

	return {}
end


local function _read_tail_bytes(path, max_bytes)
	local f = io.open(path, "rb")
	if f == nil then
		return nil, "open failed"
	end

	local size = f:seek("end")
	if type(size) ~= "number" then
		f:close()
		return nil, "seek failed"
	end

	local start = size - max_bytes
	if start < 0 then
		start = 0
	end

	f:seek("set", start)
	local data = f:read("*a") or ""
	f:close()

	if start > 0 then
		local nl = string.find(data, "\n", 1, true)
		if nl ~= nil then
			data = string.sub(data, nl + 1)
		end
	end

	return data, nil
end


local function _collect_ring(filter, lines_n)
	local matched = {}
	if _ring_cap <= 0 or _ring_len <= 0 then
		return matched, false
	end

	for i = 0, _ring_len - 1 do
		local idx = ((_ring_head - 1 + i) % _ring_cap) + 1
		local row = _ring[idx]
		if _row_pass(row, filter) then
			matched[#matched + 1] = row
		end
	end

	local truncated = false
	local start_i = #matched - lines_n + 1
	if start_i < 1 then
		start_i = 1
	else
		truncated = true
	end

	local out = {}
	for i = start_i, #matched do
		out[#out + 1] = matched[i]
	end

	return out, truncated
end


-- @brief 截取结构化日志 (供 log.tail)
-- @param [in] opts[table]	lines/max_bytes/module/level
-- @return result[table]
M.tail = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local cfg = _cfg()
	local path = M.file_path(cfg)
	local max_bytes = tonumber(opts.max_bytes) or 65536
	if max_bytes < 256 then
		max_bytes = 256
	end

	if max_bytes > 524288 then
		max_bytes = 524288
	end

	local lines_n = tonumber(opts.lines) or 50
	lines_n = math.floor(lines_n)
	if lines_n < 1 then
		lines_n = 1
	end

	if lines_n > 500 then
		lines_n = 500
	end

	local filter = {
		module = opts.module,
		level = opts.level,
		remote_level = cfg.remote_level or "info",
	}

	local out = {
		path = path,
		source = "file",
		lines = _empty_lines(),
		truncated = false,
		exists = false,
	}

	if path == "" or lfs.attributes(path, "mode") ~= "file" then
		return out
	end

	out.exists = true
	local data, err = _read_tail_bytes(path, max_bytes)
	if data == nil then
		out.error = err or "read failed"
		return out
	end

	local raw = {}
	for line in string.gmatch(data, "[^\r\n]+") do
		raw[#raw + 1] = line
	end

	local matched = {}
	for i = 1, #raw do
		local row = _parse_row(raw[i])
		if row ~= nil and _row_pass(row, filter) then
			matched[#matched + 1] = row
		end
	end

	local start_i = #matched - lines_n + 1
	if start_i < 1 then
		start_i = 1
	else
		out.truncated = true
	end

	local lines = {}
	for i = start_i, #matched do
		lines[#lines + 1] = matched[i]
	end

	if #lines == 0 then
		out.lines = _empty_lines()
	else
		out.lines = lines
	end

	return out
end


-- @brief journald 取数 (单元 stderr 的 JSONL 文本; 无 json 解析)
-- @param [in] unit[string]
-- @param [in] n[number]		最多行数
-- @param [in] user[boolean]	true=--user 单元, false=system 单元
-- @return data[string]
local function _journal_cat(unit, n, user)
	local flag = ""
	if user then
		flag = "--user "
	end

	local cmd = "journalctl " .. flag .. "-u " .. unit .. " -n " .. tostring(n)
		.. " --no-pager -o cat -q 2>/dev/null"
	local f = io.popen(cmd, "r")
	if f == nil then
		return ""
	end

	local data = f:read("*a") or ""
	f:close()
	return data
end


-- @brief 从 journald 截取 (pref 无日志文件时的回退源; 过滤规则同 tail)
-- @param [in] opts[table]	lines/module/level
-- @param [in] unit[string]	systemd 单元名 (alua-web.service)
-- @return result[table]
M.tail_journal = function (opts, unit)
	if type(opts) ~= "table" then
		opts = {}
	end

	local out = {
		source = "journal",
		unit = type(unit) == "string" and unit or "",
		lines = _empty_lines(),
		truncated = false,
		exists = false,
	}

	if out.unit == "" then
		return out
	end

	local lines_n = tonumber(opts.lines) or 50
	lines_n = math.floor(lines_n)
	if lines_n < 1 then
		lines_n = 1
	end

	if lines_n > 500 then
		lines_n = 500
	end

	-- 模块/等级在 Lua 侧过滤, 故多取若干倍
	local read_n = lines_n * 8
	if read_n > 4000 then
		read_n = 4000
	end

	local cfg = _cfg()
	local filter = {
		module = opts.module,
		level = opts.level,
		remote_level = cfg.remote_level or "info",
	}

	local data = _journal_cat(out.unit, read_n, true)
	if data == "" then
		data = _journal_cat(out.unit, read_n, false)
	end

	if data == "" then
		return out
	end

	out.exists = true

	local raw = {}
	for line in string.gmatch(data, "[^\r\n]+") do
		raw[#raw + 1] = line
	end

	local matched = {}
	for i = 1, #raw do
		local row = _parse_row(raw[i])
		if row ~= nil and _row_pass(row, filter) then
			matched[#matched + 1] = row
		end
	end

	local start_i = #matched - lines_n + 1
	if start_i < 1 then
		start_i = 1
	else
		out.truncated = true
	end

	local lines = {}
	for i = start_i, #matched do
		lines[#lines + 1] = matched[i]
	end

	if #lines > 0 then
		out.lines = lines
	end

	return out
end


-- @brief 内存 ring 最近条目 (供 log.recent)
-- @param [in] opts[table]	lines/module/level
-- @return result[table]
M.recent = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local cfg = _cfg()
	_ring_ensure(cfg.ring_n or 0)

	local lines_n = tonumber(opts.lines) or 50
	lines_n = math.floor(lines_n)
	if lines_n < 1 then
		lines_n = 1
	end

	if lines_n > 500 then
		lines_n = 500
	end

	local filter = {
		module = opts.module,
		level = opts.level,
		remote_level = cfg.remote_level or "info",
	}

	local lines, truncated = _collect_ring(filter, lines_n)
	local out_lines = lines
	if #out_lines == 0 then
		out_lines = _empty_lines()
	end

	return {
		source = "ring",
		ring_n = _ring_cap,
		ring_len = _ring_len,
		truncated = truncated,
		lines = out_lines,
	}
end


-- @brief 订阅 push 缓冲 (进程内; 远程经 remote.call)
-- @param [in] opts[table]	module/level/max_n
-- @return result[table]
M.subscribe = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	if #_subs >= MAX_SUBS then
		return nil, "too many subscriptions (max " .. tostring(MAX_SUBS) .. ")"
	end

	local max_n = tonumber(opts.max_n) or DEFAULT_SUB_MAX
	max_n = math.floor(max_n)
	if max_n < 1 then
		max_n = 1
	end

	if max_n > 500 then
		max_n = 500
	end

	local module = nil
	if type(opts.module) == "string" and opts.module ~= "" then
		module = opts.module
	end

	local level = nil
	if type(opts.level) == "string" and opts.level ~= "" then
		if LEVEL_ORD[opts.level] == nil then
			return nil, "bad level"
		end

		level = opts.level
	end

	_sub_seq = _sub_seq + 1
	local id = "s" .. tostring(_sub_seq)
	_subs[#_subs + 1] = {
		id = id,
		module = module,
		level = level,
		max_n = max_n,
		buf = {},
		dropped = 0,
	}

	return {
		sub_id = id,
		module = module,
		level = level,
		max_n = max_n,
		push_sub_count = #_subs,
	}
end


-- @brief 取消订阅
-- @param [in] sub_id[string]
-- @return result[table]|nil
-- @return err[string|nil]
M.unsubscribe = function (sub_id)
	local sub, idx = _find_sub(sub_id)
	if sub == nil then
		return nil, "sub not found"
	end

	table.remove(_subs, idx)
	return {
		sub_id = sub_id,
		ok = true,
		push_sub_count = #_subs,
	}
end


-- @brief 取走 push 缓冲中的新行 (消费)
-- @param [in] opts[table]	sub_id 必填; lines 可选
-- @return result[table]|nil
-- @return err[string|nil]
M.push_poll = function (opts)
	if type(opts) ~= "table" then
		opts = {}
	end

	local sub = _find_sub(opts.sub_id)
	if sub == nil then
		return nil, "sub not found"
	end

	local lines_n = tonumber(opts.lines) or sub.max_n
	lines_n = math.floor(lines_n)
	if lines_n < 1 then
		lines_n = 1
	end

	if lines_n > 500 then
		lines_n = 500
	end

	local take = lines_n
	if take > #sub.buf then
		take = #sub.buf
	end

	local lines = {}
	for i = 1, take do
		lines[#lines + 1] = sub.buf[i]
	end

	if take > 0 then
		local rest = {}
		for i = take + 1, #sub.buf do
			rest[#rest + 1] = sub.buf[i]
		end

		sub.buf = rest
	end

	local out_lines = lines
	if #out_lines == 0 then
		out_lines = _empty_lines()
	end

	local dropped = sub.dropped
	sub.dropped = 0

	return {
		sub_id = sub.id,
		source = "push",
		lines = out_lines,
		pending = #sub.buf,
		dropped = dropped,
		module = sub.module,
		level = sub.level,
	}
end


-- @brief 等级序 (测试/工具)
M.level_ok = _level_ok


return M

-- end
