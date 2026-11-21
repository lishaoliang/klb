--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   uart.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  device.uart.* 本机串口调试 (list/open/read/write/close/flush/handles/transact)
-- @note   依赖 require("kuart"); handle 为进程内字符串 id; 同 path 独占;
--         命名会话 open.name → handle=uart-<name>; 错误消息稳定 uart:<tag>;
--         serve/stdio 长活进程内跨 call 保活; 一次性 alua call 退出即丢
-- @history 修改历史
--  \n [2026-10] 创建文件 (U-P0)
--]]


local log = require("klbagent.core.log")


local M = {}


local DEFAULT_MAX_BYTES = 4096
local MAX_MAX_BYTES = 256 * 1024
local DEFAULT_LIST_MAX = 64
local MAX_LIST_MAX = 256
local DEFAULT_TIMEOUT_MS = 200
local DEFAULT_TRANSACT_TIMEOUT_MS = 2000
local TRANSACT_SLICE_MS = 50


local g_kuart = nil
local g_ports = {}
local g_by_path = {}
local g_next_id = 1


-- @brief 稳定错误: message = uart:<tag>[: detail]; level=0 避免路径前缀污染跨 hop
local function _fail(tag, detail)
	local msg = "uart:" .. tostring(tag or "error")
	if type(detail) == "string" and detail ~= "" then
		msg = msg .. ": " .. detail
	elseif detail ~= nil and type(detail) ~= "string" then
		msg = msg .. ": " .. tostring(detail)
	end
	log.write("warn", "uart fail", {
		module = "device.uart",
		uart_err = tostring(tag or "error"),
		detail = detail ~= nil and tostring(detail) or "",
	})
	error(msg, 0)
end


local function _log_info(msg, fields)
	local f = fields or {}
	f.module = "device.uart"
	log.write("info", msg, f)
end


-- @brief 列出当前进程内已打开的 UART handle (供 status / handles)
-- @return list[table]
M.snapshot_handles = function ()
	local list = {}
	for id, ent in pairs(g_ports) do
		if type(ent) == "table" and ent.port ~= nil then
			list[#list + 1] = {
				handle = id,
				path = ent.path,
				name = ent.name,
				baud = ent.opts and ent.opts.baud or nil,
			}
		end
	end
	table.sort(list, function (a, b)
		return tostring(a.handle) < tostring(b.handle)
	end)
	return list
end


local function _load_kuart()
	if g_kuart ~= nil then
		return g_kuart
	end

	local ok, mod = pcall(require, "kuart")
	if not ok or type(mod) ~= "table" then
		return nil
	end

	g_kuart = mod
	return g_kuart
end


local function _need_kuart()
	local k = _load_kuart()
	if k == nil then
		_fail("kuart_unavailable")
	end
	return k
end


local function _norm_path(path)
	if type(path) ~= "string" or path == "" then
		_fail("need_path")
	end
	return path
end


local function _norm_name(name)
	if name == nil then
		return nil
	end
	if type(name) ~= "string" or name == "" then
		_fail("bad_name", "empty")
	end
	if #name > 32 or string.match(name, "^[%w_%-]+$") == nil then
		_fail("bad_name", "need [A-Za-z0-9_-]{1,32}")
	end
	return name
end


local function _clamp_max_bytes(v)
	local n = tonumber(v) or DEFAULT_MAX_BYTES
	n = math.floor(n)
	if n < 1 then
		n = DEFAULT_MAX_BYTES
	end
	if n > MAX_MAX_BYTES then
		n = MAX_MAX_BYTES
	end
	return n
end


local function _get_port(handle)
	if type(handle) ~= "string" or handle == "" then
		_fail("need_handle")
	end

	local ent = g_ports[handle]
	if ent == nil or ent.port == nil then
		_fail("unknown_handle", handle)
	end

	return ent
end


local function _list(args)
	args = args or {}
	local kuart = _need_kuart()
	local max_entries = tonumber(args.max_entries) or DEFAULT_LIST_MAX
	max_entries = math.floor(max_entries)
	if max_entries < 1 then
		max_entries = DEFAULT_LIST_MAX
	end
	if max_entries > MAX_LIST_MAX then
		max_entries = MAX_LIST_MAX
	end

	local include_legacy = args.include_legacy == true
	local raw = kuart.list(max_entries, include_legacy) or {}
	local ports = {}
	for i = 1, #raw do
		local p = raw[i]
		if type(p) == "string" then
			ports[#ports + 1] = {
				path = p,
				busy = (g_by_path[p] ~= nil),
			}
		end
	end

	return {
		ports = ports,
		count = #ports,
		max_entries = max_entries,
		include_legacy = include_legacy,
		platform = select(2, kuart.available()),
	}
end


local function _close_ent(id, ent)
	if ent == nil then
		return
	end
	if ent.port ~= nil then
		pcall(function()
			ent.port:close()
		end)
	end
	if ent.path ~= nil and g_by_path[ent.path] == id then
		g_by_path[ent.path] = nil
	end
	g_ports[id] = nil
end


local function _open(args)
	args = args or {}
	local path = _norm_path(args.path)
	local steal = args.steal == true
	local sess_name = _norm_name(args.name)
	local kuart = _need_kuart()

	local id
	if sess_name ~= nil then
		id = "uart-" .. sess_name
		local named = g_ports[id]
		if named ~= nil then
			if not steal then
				_fail("name_busy", id)
			end
			_close_ent(id, named)
		end
	end

	local old_id = g_by_path[path]
	if old_id ~= nil then
		if not steal then
			_fail("path_busy", path)
		end
		_close_ent(old_id, g_ports[old_id])
	end

	local opts = {
		baud = tonumber(args.baud) or 115200,
		databits = tonumber(args.databits) or 8,
		stop = tonumber(args.stop) or 1,
		parity = args.parity or "none",
		flow = args.flow or "none",
	}

	local port, err = kuart.open(path, opts)
	if port == nil then
		_fail("open_fail", err or path)
	end

	if id == nil then
		id = string.format("uart-%d", g_next_id)
		g_next_id = g_next_id + 1
	end

	g_ports[id] = {
		path = path,
		port = port,
		opts = opts,
		name = sess_name,
	}
	g_by_path[path] = id

	_log_info("uart open", {
		handle = id,
		path = path,
		name = sess_name or "",
		baud = opts.baud,
	})

	return {
		handle = id,
		path = path,
		name = sess_name,
		baud = opts.baud,
		databits = opts.databits,
		stop = opts.stop,
		parity = opts.parity,
		flow = opts.flow,
	}
end


local function _write(args)
	args = args or {}
	local ent = _get_port(args.handle)
	local data = args.data
	if type(data) ~= "string" then
		_fail("need_data")
	end

	local n, err = ent.port:write(data)
	if n == nil then
		_fail("write_fail", err)
	end

	_log_info("uart write", {
		handle = args.handle,
		path = ent.path,
		written = n,
	})

	return {
		handle = args.handle,
		path = ent.path,
		written = n,
	}
end


local function _read(args)
	args = args or {}
	local ent = _get_port(args.handle)
	local max_bytes = _clamp_max_bytes(args.max_bytes)
	local timeout_ms = tonumber(args.timeout_ms)
	if timeout_ms == nil then
		timeout_ms = DEFAULT_TIMEOUT_MS
	end
	timeout_ms = math.floor(timeout_ms)
	if timeout_ms < 0 then
		timeout_ms = 0
	end

	local data, err = ent.port:read(max_bytes, timeout_ms)
	if data == nil then
		_fail("read_fail", err)
	end

	return {
		handle = args.handle,
		path = ent.path,
		data = data,
		n = #data,
		max_bytes = max_bytes,
		timeout_ms = timeout_ms,
	}
end


local function _flush(args)
	args = args or {}
	local ent = _get_port(args.handle)
	local ok, err = ent.port:flush()
	if not ok then
		_fail("flush_fail", err)
	end

	return {
		handle = args.handle,
		path = ent.path,
		ok = true,
	}
end


local function _now_ms()
	local ok, ktime = pcall(require, "ktime")
	if ok and type(ktime) == "table" and type(ktime.tick_count) == "function" then
		local t = ktime.tick_count()
		if type(t) == "number" then
			return t
		end
	end
	return math.floor(os.clock() * 1000)
end


-- @brief 写后读直到 until/pattern/min_bytes 或超时/满缓冲
local function _transact(args)
	args = args or {}
	local ent = _get_port(args.handle)
	local max_bytes = _clamp_max_bytes(args.max_bytes)

	local timeout_ms = tonumber(args.timeout_ms)
	if timeout_ms == nil then
		timeout_ms = DEFAULT_TRANSACT_TIMEOUT_MS
	end
	timeout_ms = math.floor(timeout_ms)
	if timeout_ms < 0 then
		timeout_ms = 0
	end

	local until_s = args["until"]
	if until_s ~= nil and type(until_s) ~= "string" then
		_fail("bad_until")
	end
	if until_s == "" then
		until_s = nil
	end

	local pattern = args.pattern
	if pattern ~= nil and type(pattern) ~= "string" then
		_fail("bad_pattern")
	end
	if pattern == "" then
		pattern = nil
	end

	local min_bytes = tonumber(args.min_bytes)
	if min_bytes ~= nil then
		min_bytes = math.floor(min_bytes)
		if min_bytes < 1 then
			min_bytes = nil
		end
	end

	if until_s == nil and pattern == nil and min_bytes == nil then
		_fail("need_wait", "until|pattern|min_bytes")
	end

	if args.flush_before == true then
		local ok_f, err_f = ent.port:flush()
		if not ok_f then
			_fail("flush_fail", err_f)
		end
	end

	local written = 0
	local data_w = args.data
	if data_w ~= nil then
		if type(data_w) ~= "string" then
			_fail("need_data")
		end
		local n, err_w = ent.port:write(data_w)
		if n == nil then
			_fail("write_fail", err_w)
		end
		written = n
	end

	local buf = ""
	local matched = false
	local reason = "timeout"
	local truncated = false
	local t0 = _now_ms()

	while true do
		local elapsed = _now_ms() - t0
		if elapsed < 0 then
			elapsed = 0
		end
		local remain = timeout_ms - elapsed
		if remain < 0 then
			remain = 0
		end

		if #buf > 0 then
			if until_s ~= nil and string.find(buf, until_s, 1, true) ~= nil then
				matched = true
				reason = "until"
				break
			end
			if pattern ~= nil then
				local ok_m, mpos = pcall(string.find, buf, pattern)
				if ok_m and mpos ~= nil then
					matched = true
					reason = "pattern"
					break
				end
			end
			if min_bytes ~= nil and #buf >= min_bytes then
				matched = true
				reason = "min_bytes"
				break
			end
		end

		if #buf >= max_bytes then
			truncated = true
			reason = "max_bytes"
			break
		end

		if remain <= 0 then
			reason = "timeout"
			break
		end

		local slice = TRANSACT_SLICE_MS
		if slice > remain then
			slice = remain
		end
		local want = max_bytes - #buf
		if want < 1 then
			truncated = true
			reason = "max_bytes"
			break
		end

		local chunk, err_r = ent.port:read(want, slice)
		if chunk == nil then
			_fail("read_fail", err_r)
		end
		if #chunk > 0 then
			buf = buf .. chunk
			if #buf > max_bytes then
				buf = string.sub(buf, 1, max_bytes)
				truncated = true
				reason = "max_bytes"
				break
			end
		end
	end

	_log_info("uart transact", {
		handle = args.handle,
		path = ent.path,
		written = written,
		n = #buf,
		matched = matched and true or false,
		reason = reason,
		truncated = truncated and true or false,
	})

	return {
		handle = args.handle,
		path = ent.path,
		written = written,
		data = buf,
		n = #buf,
		matched = matched,
		reason = reason,
		truncated = truncated,
		max_bytes = max_bytes,
		timeout_ms = timeout_ms,
		["until"] = until_s,
		pattern = pattern,
		min_bytes = min_bytes,
	}
end


local function _close(args)
	args = args or {}
	local handle = args.handle
	if type(handle) ~= "string" or handle == "" then
		_fail("need_handle")
	end

	local ent = g_ports[handle]
	if ent == nil then
		return {
			handle = handle,
			closed = false,
			message = "uart:unknown_handle",
			uart_err = "unknown_handle",
		}
	end

	local path = ent.path
	_close_ent(handle, ent)

	_log_info("uart close", {
		handle = handle,
		path = path or "",
	})

	return {
		handle = handle,
		path = path,
		closed = true,
	}
end


local function _handles(_args)
	local list = M.snapshot_handles()
	local n = #list
	if n == 0 then
		local ok_cj, cjson = pcall(require, "cjson")
		if ok_cj and type(cjson) == "table" and cjson.empty_array ~= nil then
			list = cjson.empty_array
		end
	end
	return {
		handles = list,
		count = n,
		process_local = true,
		keep_alive = "serve_or_stdio",
	}
end


local function _reg(catalog, dispatch, desc, fn)
	local ok, err = catalog.register(desc)
	if not ok then
		return false, err
	end

	ok, err = dispatch.register(desc.name, fn)
	if not ok then
		return false, err
	end

	return true
end


M.register = function (catalog, dispatch)
	local ok, err

	ok, err = _reg(catalog, dispatch, {
		name = "device.uart.list",
		category = "device",
		title = "list serial ports",
		summary = "List candidate UART paths (Linux /dev/ttyUSB* etc; Windows COMn).",
		min_profile = "device",
		args_schema = {
			max_entries = "number optional max ports (default 64)",
			include_legacy = "boolean optional include ttyS* (Linux)",
		},
		examples = {
			'alua call device.uart.list {}',
		},
		docs = "tools/device.md",
		side_effects = {},
	}, _list)
	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "device.uart.open",
		category = "device",
		title = "open serial port",
		summary = "Open UART; returns handle id. Same path exclusive unless steal=true.",
		min_profile = "device",
		args_schema = {
			-- optional in schema so handler can emit uart:need_path (Bug.2026-8)
			path = "string optional device path e.g. /dev/ttyUSB0 or COM3",
			name = "string optional session name -> handle uart-<name>",
			baud = "number optional default 115200; custom high rates ok (e.g. 1500000)",
			databits = "number optional 5-8 default 8",
			parity = "string optional none|even|odd default none",
			stop = "number optional 1|2 default 1",
			flow = "string optional none|rtscts|xonxoff default none",
			steal = "boolean optional close existing same path or same name",
		},
		examples = {
			'alua call device.uart.open {"path":"/dev/ttyUSB0","baud":115200}',
			'alua call device.uart.open {"path":"/dev/ttyUSB0","name":"console","baud":115200}',
		},
		docs = "tools/device.md",
		side_effects = {},
	}, _open)
	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "device.uart.write",
		category = "device",
		title = "write serial",
		summary = "Write bytes/string to an open UART handle.",
		min_profile = "device",
		args_schema = {
			handle = "string from device.uart.open",
			data = "string bytes to write",
		},
		examples = {
			'alua call device.uart.write {"handle":"uart-1","data":"help\\r\\n"}',
		},
		docs = "tools/device.md",
		side_effects = {},
	}, _write)
	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "device.uart.read",
		category = "device",
		title = "read serial",
		summary = "Read up to max_bytes with timeout_ms (default 200).",
		min_profile = "device",
		args_schema = {
			handle = "string from device.uart.open",
			max_bytes = "number optional default 4096",
			timeout_ms = "number optional default 200; 0=nonblock",
		},
		examples = {
			'alua call device.uart.read {"handle":"uart-1","max_bytes":4096,"timeout_ms":500}',
		},
		docs = "tools/device.md",
		side_effects = {},
	}, _read)
	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "device.uart.flush",
		category = "device",
		title = "flush serial buffers",
		summary = "Flush RX/TX buffers of an open UART handle.",
		min_profile = "device",
		args_schema = {
			handle = "string from device.uart.open",
		},
		examples = {},
		docs = "tools/device.md",
		side_effects = {},
	}, _flush)
	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "device.uart.close",
		category = "device",
		title = "close serial port",
		summary = "Close UART handle and release path exclusive lock.",
		min_profile = "device",
		args_schema = {
			handle = "string from device.uart.open",
		},
		examples = {
			'alua call device.uart.close {"handle":"uart-1"}',
		},
		docs = "tools/device.md",
		side_effects = {},
	}, _close)
	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "device.uart.handles",
		category = "device",
		title = "list open UART handles",
		summary = "List process-local open UART handles (alive across calls in serve/stdio).",
		min_profile = "device",
		args_schema = {},
		examples = {
			'alua call device.uart.handles {}',
		},
		docs = "tools/device.md",
		side_effects = {},
	}, _handles)
	if not ok then
		return false, err
	end

	ok, err = _reg(catalog, dispatch, {
		name = "device.uart.transact",
		category = "device",
		title = "UART write then wait",
		summary = "Optional write, then read until until/pattern/min_bytes or timeout/max_bytes.",
		min_profile = "device",
		args_schema = {
			handle = "string from device.uart.open",
			data = "string optional bytes to write first",
			["until"] = "string optional plain substring to wait for",
			pattern = "string optional Lua pattern to wait for",
			min_bytes = "number optional wait until at least N bytes",
			max_bytes = "number optional buffer cap default 4096",
			timeout_ms = "number optional overall timeout default 2000",
			flush_before = "boolean optional flush RX/TX before write",
		},
		examples = {
			'alua call device.uart.transact {"handle":"uart-1","data":"help\\r\\n","until":">","timeout_ms":2000}',
		},
		docs = "tools/device.md",
		side_effects = {},
	}, _transact)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
