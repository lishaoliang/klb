--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   grep.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  fs.grep
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local lfs = require("lfs")
local util = require("klbagent.core.tools.fs.util")


local M = {}


local function _grep_file(path, pattern, plain, max_matches, max_file_bytes, matches)
	local attr = lfs.attributes(path)
	if attr == nil or attr.mode ~= "file" then
		return false
	end

	local f = io.open(path, "rb")
	if f == nil then
		return false
	end

	local data = f:read(max_file_bytes + 1) or ""
	f:close()

	local truncated_file = #data > max_file_bytes
	if truncated_file then
		data = string.sub(data, 1, max_file_bytes)
	end

	local line_no = 0
	local start = 1
	local n = #data
	while start <= n + 1 do
		if #matches >= max_matches then
			return true
		end

		local i = string.find(data, "\n", start, true)
		local line
		if i == nil then
			line = string.sub(data, start)
			start = n + 2
		else
			line = string.sub(data, start, i - 1)
			start = i + 1
		end

		-- skip phantom empty line after final newline
		if i == nil and line == "" and line_no > 0 then
			break
		end

		line_no = line_no + 1
		local found = false
		if plain then
			found = string.find(line, pattern, 1, true) ~= nil
		else
			local ok_m, m = pcall(string.find, line, pattern)
			found = ok_m and m ~= nil
		end

		if found then
			local text = line
			if #text > 512 then
				text = string.sub(text, 1, 512)
			end

			matches[#matches + 1] = {
				path = path,
				line = line_no,
				text = text,
			}
		end

		if i == nil then
			break
		end
	end

	return truncated_file
end


local function _grep(args)
	local path = args.path
	local pattern = args.pattern
	if not util.path_ok(path) then
		error("need path")
	end

	if type(pattern) ~= "string" or pattern == "" then
		error("need pattern")
	end

	local max_matches = util.clamp_max_matches(args.max_matches)
	local max_file_bytes = util.clamp_max_bytes(args.max_file_bytes)
	local max_depth = util.clamp_max_depth(args.max_depth)
	local recursive = args.recursive == true
	if recursive and max_depth == 0 then
		max_depth = 3
	end

	local plain = args.plain ~= false
	if args.regex == true then
		plain = false
	end

	local attr = lfs.attributes(path)
	if attr == nil then
		error("path not found")
	end

	local matches = {}
	local truncated = false
	local files_truncated = false

	local function walk(p, depth)
		if #matches >= max_matches then
			truncated = true
			return
		end

		local a = lfs.attributes(p)
		if a == nil then
			return
		end

		if a.mode == "file" then
			local ft = _grep_file(p, pattern, plain, max_matches, max_file_bytes, matches)
			if ft then
				files_truncated = true
			end

			if #matches >= max_matches then
				truncated = true
			end
		elseif a.mode == "directory" and (recursive or depth == 0) then
			if depth == 0 and not recursive and a.mode == "directory" then
				-- non-recursive dir: only files in this dir
				for name in lfs.dir(p) do
					if name ~= "." and name ~= ".." then
						local full = util.join(p, name)
						local fa = lfs.attributes(full)
						if fa ~= nil and fa.mode == "file" then
							local ft = _grep_file(full, pattern, plain, max_matches, max_file_bytes, matches)
							if ft then
								files_truncated = true
							end

							if #matches >= max_matches then
								truncated = true
								return
							end
						end
					end
				end
			elseif recursive and depth <= max_depth then
				for name in lfs.dir(p) do
					if name ~= "." and name ~= ".." then
						walk(util.join(p, name), depth + 1)
						if truncated then
							return
						end
					end
				end
			end
		end
	end

	if attr.mode == "file" then
		local ft = _grep_file(path, pattern, plain, max_matches, max_file_bytes, matches)
		if ft then
			files_truncated = true
		end

		if #matches >= max_matches then
			truncated = true
		end
	else
		walk(path, 0)
	end

	return {
		path = path,
		pattern = pattern,
		plain = plain,
		matches = matches,
		count = #matches,
		truncated = truncated,
		files_truncated = files_truncated,
		max_matches = max_matches,
	}
end


M.register = function (catalog, dispatch)
	local ok, err = util.reg(catalog, dispatch, {
		name = "fs.grep",
		category = "fs",
		title = "search text in files",
		summary = "Substring (default) or Lua regex search; optional recursive. Caps matches/bytes.",
		args_schema = {
			path = "string file or directory",
			pattern = "string search pattern",
			plain = "boolean optional default true (literal)",
			regex = "boolean optional Lua pattern when true",
			recursive = "boolean optional",
			max_depth = "number optional when recursive",
			max_matches = "number optional",
			max_file_bytes = "number optional per file",
		},
		examples = {
			{ args = { path = "/tmp", pattern = "hello", recursive = true, max_matches = 20 } },
		},
		docs = "README.md",
		side_effects = {},
	}, _grep)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
