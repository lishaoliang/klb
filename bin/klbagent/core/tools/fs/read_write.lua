--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   read_write.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  fs.read / fs.write
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出; encoding=base64 (WS 文本帧安全传二进制; Bug.2026-9)
--]]


local lfs = require("lfs")
local util = require("klbagent.core.tools.fs.util")


local M = {}


local function _use_b64(encoding)
	if type(encoding) ~= "string" then
		return false
	end

	local low = string.lower(encoding)
	return low == "base64" or low == "b64"
end


local function _read(args)
	local path = args.path
	if not util.path_ok(path) then
		error("need path")
	end

	local max_bytes = util.clamp_max_bytes(args.max_bytes)
	local offset = tonumber(args.offset) or 0
	if offset < 0 then
		offset = 0
	end

	offset = math.floor(offset)
	local attr = lfs.attributes(path)
	if attr == nil then
		error("path not found")
	end

	if attr.mode ~= "file" then
		error("not a file")
	end

	local f = io.open(path, "rb")
	if f == nil then
		error("open fail")
	end

	if offset > 0 then
		f:seek("set", offset)
	end

	local data = f:read(max_bytes + 1) or ""
	f:close()

	local truncated = #data > max_bytes
	if truncated then
		data = string.sub(data, 1, max_bytes)
	end

	local raw_n = #data
	local encoding = nil
	if _use_b64(args.encoding) then
		data = util.b64_encode(data)
		encoding = "base64"
	end

	return {
		path = path,
		content = data,
		bytes = raw_n,
		offset = offset,
		truncated = truncated,
		size = attr.size or 0,
		encoding = encoding,
	}
end


local function _write(args)
	local path = args.path
	if not util.path_ok(path) then
		error("need path")
	end

	if type(args.content) ~= "string" then
		error("need content string")
	end

	local content = args.content
	if _use_b64(args.encoding) then
		local decoded = util.b64_decode(content)
		if decoded == nil then
			error("bad base64 content")
		end

		content = decoded
	end

	if #content > util.MAX_MAX_BYTES then
		error("content too large")
	end

	local mode = args.mode
	if mode ~= "append" then
		mode = "overwrite"
	end

	local open_mode = "wb"
	if mode == "append" then
		open_mode = "ab"
	end

	local f = io.open(path, open_mode)
	if f == nil then
		error("open fail")
	end

	f:write(content)
	f:close()

	local attr = lfs.attributes(path)
	return {
		path = path,
		mode = mode,
		bytes = #content,
		size = (attr and attr.size) or 0,
	}
end


M.register = function (catalog, dispatch)
	local ok, err = util.reg(catalog, dispatch, {
		name = "fs.read",
		category = "fs",
		title = "read file",
		summary = "Read file bytes as string; truncated by max_bytes (default 1MiB). encoding=base64 for WS-safe binary.",
		args_schema = {
			path = "string file path",
			max_bytes = "number optional",
			offset = "number optional byte offset",
			encoding = "string optional raw|base64 (base64 → content is ASCII)",
		},
		examples = {
			{ args = { path = "/etc/hostname", max_bytes = 4096 } },
		},
		docs = "README.md",
		side_effects = {},
	}, _read)
	if not ok then
		return false, err
	end

	ok, err = util.reg(catalog, dispatch, {
		name = "fs.write",
		category = "fs",
		title = "write file",
		summary = "Write string content (overwrite|append). Needs profile>=net (write_fs). encoding=base64 decodes content first.",
		min_profile = "net",
		args_schema = {
			path = "string file path",
			content = "string bytes to write (or base64 if encoding=base64)",
			mode = "string optional overwrite|append",
			encoding = "string optional raw|base64",
		},
		examples = {
			{ args = { path = "/tmp/alua_fs_write.txt", content = "hello\n" } },
		},
		docs = "README.md",
		side_effects = { "write_fs" },
	}, _write)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
