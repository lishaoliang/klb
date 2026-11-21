--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   stat_list.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  fs.stat / fs.list
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local lfs = require("lfs")
local util = require("klbagent.core.tools.fs.util")


local M = {}


local function _stat(args)
	local path = args.path
	if not util.path_ok(path) then
		error("need path")
	end

	local attr = lfs.attributes(path)
	if attr == nil then
		return {
			exists = false,
			path = path,
			mode = "",
			size = 0,
		}
	end

	return {
		exists = true,
		path = path,
		mode = attr.mode or "",
		size = attr.size or 0,
		modification = attr.modification or 0,
	}
end


local function _list(args)
	local path = args.path
	if not util.path_ok(path) then
		error("need path")
	end

	local max_entries = util.clamp_max_entries(args.max_entries)
	local max_depth = util.clamp_max_depth(args.max_depth)
	local attr = lfs.attributes(path)
	if attr == nil then
		error("path not found")
	end

	if attr.mode ~= "directory" then
		error("not a directory")
	end

	local entries = {}
	local truncated = false

	local function walk(dir, depth)
		if truncated then
			return
		end

		for name in lfs.dir(dir) do
			if name ~= "." and name ~= ".." then
				if #entries >= max_entries then
					truncated = true
					return
				end

				local full = util.join(dir, name)
				local a = lfs.attributes(full)
				local mode = ""
				local size = 0
				if a ~= nil then
					mode = a.mode or ""
					size = a.size or 0
				end

				entries[#entries + 1] = {
					name = name,
					path = full,
					mode = mode,
					size = size,
				}

				if mode == "directory" and depth < max_depth then
					walk(full, depth + 1)
				end
			end
		end
	end

	walk(path, 0)

	return {
		path = path,
		entries = entries,
		count = #entries,
		truncated = truncated,
		max_entries = max_entries,
		max_depth = max_depth,
	}
end


M.register = function (catalog, dispatch)
	local ok, err = util.reg(catalog, dispatch, {
		name = "fs.stat",
		category = "fs",
		title = "stat path",
		summary = "Stat a filesystem path (exists/mode/size).",
		args_schema = {
			path = "string absolute or relative path",
		},
		examples = {
			{ args = { path = "/tmp" } },
		},
		docs = "README.md",
		side_effects = {},
	}, _stat)
	if not ok then
		return false, err
	end

	ok, err = util.reg(catalog, dispatch, {
		name = "fs.list",
		category = "fs",
		title = "list directory",
		summary = "List directory entries. Optional max_depth for shallow recursion.",
		args_schema = {
			path = "string directory path",
			max_entries = "number optional cap (default 1000)",
			max_depth = "number optional recurse depth (default 0)",
		},
		examples = {
			{ args = { path = "/tmp", max_entries = 100 } },
		},
		docs = "README.md",
		side_effects = {},
	}, _list)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
