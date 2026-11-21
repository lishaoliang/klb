--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  log.* 工具 (状态 / 截取 / ring / push)
-- @note   readonly; 远程经 remote.call 复用; 见 docs/tools/log.md
-- @history 修改历史
--  \n [2026-10] 创建文件 (P0); P2 log.recent; P3 subscribe / push_poll / unsubscribe
--]]
local log = require("klbagent.core.log")
local lfs = require("lfs")
local cjson = require("cjson")


local M = {}


local function _serve_log_path()
	local env = os.getenv("ALUA_SERVE_LOG")
	if type(env) == "string" and env ~= "" then
		return env
	end

	local home = os.getenv("HOME")
	if type(home) == "string" and home ~= "" then
		local p = home .. "/debug/alua_serve.log"
		if lfs.attributes(p, "mode") == "file" then
			return p
		end
	end

	return ""
end


local function _tail_text_file(path, lines_n, max_bytes)
	local empty = {}
	if cjson.empty_array ~= nil then
		empty = cjson.empty_array
	end

	local f = io.open(path, "rb")
	if f == nil then
		return {
			path = path,
			source = "serve",
			exists = false,
			lines = empty,
		}
	end

	local size = f:seek("end")
	if type(size) ~= "number" then
		f:close()
		return {
			path = path,
			source = "serve",
			exists = true,
			error = "seek failed",
			lines = empty,
		}
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

	local raw = {}
	for line in string.gmatch(data, "[^\r\n]+") do
		raw[#raw + 1] = line
	end

	local out_lines = {}
	local from = #raw - lines_n + 1
	if from < 1 then
		from = 1
	end

	for i = from, #raw do
		out_lines[#out_lines + 1] = { text = raw[i] }
	end

	if #out_lines == 0 and cjson.empty_array ~= nil then
		out_lines = cjson.empty_array
	end

	return {
		path = path,
		source = "serve",
		exists = true,
		size = size,
		truncated = (from > 1),
		lines = out_lines,
	}
end


local function _status(_args)
	return log.status()
end


local function _tail(args)
	if type(args) ~= "table" then
		args = {}
	end

	local source = args.source
	if type(source) ~= "string" or source == "" then
		source = "file"
	end

	local lines_n = tonumber(args.lines) or 50
	local max_bytes = tonumber(args.max_bytes) or 65536
	if max_bytes < 256 then
		max_bytes = 256
	end

	if max_bytes > 524288 then
		max_bytes = 524288
	end

	if source == "serve" then
		local path = _serve_log_path()
		if path == "" then
			local empty = {}
			if cjson.empty_array ~= nil then
				empty = cjson.empty_array
			end

			return {
				path = "",
				source = "serve",
				exists = false,
				lines = empty,
				error = "serve log not found",
			}
		end

		return _tail_text_file(path, lines_n, max_bytes)
	end

	if source ~= "file" and source ~= "alua" then
		error("bad source")
	end

	return log.tail({
		lines = lines_n,
		max_bytes = max_bytes,
		module = args.module,
		level = args.level,
	})
end


local function _recent(args)
	if type(args) ~= "table" then
		args = {}
	end

	return log.recent({
		lines = args.lines,
		module = args.module,
		level = args.level,
	})
end


local function _subscribe(args)
	if type(args) ~= "table" then
		args = {}
	end

	local result, err = log.subscribe(args)
	if result == nil then
		error(err or "subscribe fail")
	end

	return result
end


local function _unsubscribe(args)
	if type(args) ~= "table" then
		args = {}
	end

	local result, err = log.unsubscribe(args.sub_id)
	if result == nil then
		error(err or "unsubscribe fail")
	end

	return result
end


local function _push_poll(args)
	if type(args) ~= "table" then
		args = {}
	end

	if type(args.sub_id) ~= "string" or args.sub_id == "" then
		error("need sub_id")
	end

	local result, err = log.push_poll(args)
	if result == nil then
		error(err or "push_poll fail")
	end

	return result
end


-- @brief 注册 log 工具
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "log.status",
		category = "log",
		title = "log status",
		summary = "Structured log cfg/path/size/host_kind.",
		min_profile = "readonly",
		args_schema = {},
		examples = {
			{ args = {} },
		},
		docs = "tools/log.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("log.status", _status)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "log.tail",
		category = "log",
		title = "tail log",
		summary = "Tail alua JSONL or serve text log; filter module/level.",
		min_profile = "readonly",
		args_schema = {
			source = "string optional file|serve",
			lines = "number optional",
			max_bytes = "number optional",
			module = "string optional",
			level = "string optional error|warn|info|debug",
		},
		examples = {
			{ args = { lines = 20 } },
			{ args = { module = "dispatch", level = "error" } },
			{ args = { source = "serve", lines = 30 } },
		},
		docs = "tools/log.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("log.tail", _tail)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "log.recent",
		category = "log",
		title = "recent ring log",
		summary = "Memory ring recent rows (remote_level); filter module/level.",
		min_profile = "readonly",
		args_schema = {
			lines = "number optional",
			module = "string optional",
			level = "string optional error|warn|info|debug",
		},
		examples = {
			{ args = { lines = 20 } },
			{ args = { module = "remote", level = "warn" } },
		},
		docs = "tools/log.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("log.recent", _recent)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "log.subscribe",
		category = "log",
		title = "subscribe push buffer",
		summary = "Begin push queue for matching rows; returns sub_id (process-local).",
		min_profile = "readonly",
		args_schema = {
			module = "string optional",
			level = "string optional error|warn|info|debug",
			max_n = "number optional queue cap (default 100)",
		},
		examples = {
			{ args = { module = "dispatch", level = "error" } },
			{ args = {} },
		},
		docs = "tools/log.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("log.subscribe", _subscribe)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "log.push_poll",
		category = "log",
		title = "poll push buffer",
		summary = "Fetch and consume new push rows for sub_id.",
		min_profile = "readonly",
		args_schema = {
			sub_id = "string from log.subscribe",
			lines = "number optional max rows to take",
		},
		examples = {
			{ args = { sub_id = "s1", lines = 20 } },
		},
		docs = "tools/log.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("log.push_poll", _push_poll)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "log.unsubscribe",
		category = "log",
		title = "unsubscribe push buffer",
		summary = "Drop push subscription by sub_id.",
		min_profile = "readonly",
		args_schema = {
			sub_id = "string from log.subscribe",
		},
		examples = {
			{ args = { sub_id = "s1" } },
		},
		docs = "tools/log.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("log.unsubscribe", _unsubscribe)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
