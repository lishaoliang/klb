--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   tree_ops.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  fs.mkdir / fs.remove / fs.copy / fs.move
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local lfs = require("lfs")
local util = require("klbagent.core.tools.fs.util")


local M = {}


local function _rm_tree(path)
	local attr = lfs.attributes(path)
	if attr == nil then
		return
	end

	if attr.mode == "directory" then
		for name in lfs.dir(path) do
			if name ~= "." and name ~= ".." then
				_rm_tree(util.join(path, name))
			end
		end

		local ok, err = lfs.rmdir(path)
		if not ok then
			error(tostring(err or "rmdir fail"))
		end
	else
		local ok, err = os.remove(path)
		if not ok then
			error(tostring(err or "remove fail"))
		end
	end
end


local function _copy_file(src, dst)
	local inf = io.open(src, "rb")
	if inf == nil then
		error("open src fail")
	end

	local outf = io.open(dst, "wb")
	if outf == nil then
		inf:close()
		error("open dst fail")
	end

	while true do
		local chunk = inf:read(65536)
		if chunk == nil then
			break
		end

		outf:write(chunk)
	end

	inf:close()
	outf:close()
end


local function _copy_tree(src, dst)
	local attr = lfs.attributes(src)
	if attr == nil then
		error("src not found")
	end

	if attr.mode == "directory" then
		local ok, err = lfs.mkdir(dst)
		if not ok then
			local da = lfs.attributes(dst)
			if da == nil or da.mode ~= "directory" then
				error(tostring(err or "mkdir dst fail"))
			end
		end

		for name in lfs.dir(src) do
			if name ~= "." and name ~= ".." then
				_copy_tree(util.join(src, name), util.join(dst, name))
			end
		end
	else
		_copy_file(src, dst)
	end
end


local function _mkdir(args)
	local path = args.path
	if not util.path_ok(path) then
		error("need path")
	end

	local parents = args.parents == true
	local attr = lfs.attributes(path)
	if attr ~= nil then
		if attr.mode == "directory" then
			return {
				path = path,
				created = false,
				exists = true,
			}
		end

		error("path exists and not directory")
	end

	if not parents then
		local ok, err = lfs.mkdir(path)
		if not ok then
			error(tostring(err or "mkdir fail"))
		end

		return {
			path = path,
			created = true,
			exists = true,
		}
	end

	local parts = {}
	local rest = path
	if string.sub(rest, 1, 1) == "/" then
		parts[#parts + 1] = ""
		rest = string.sub(rest, 2)
	end

	for seg in string.gmatch(rest, "[^/]+") do
		parts[#parts + 1] = seg
	end

	if #parts == 0 then
		error("bad path")
	end

	local cur = parts[1]
	local start_i = 2
	if parts[1] == "" then
		cur = "/"
		start_i = 2
	end

	local function ensure_dir(p)
		local a = lfs.attributes(p)
		if a == nil then
			local ok, err = lfs.mkdir(p)
			if not ok then
				error(tostring(err or "mkdir fail"))
			end
		elseif a.mode ~= "directory" then
			error("path exists and not directory: " .. p)
		end
	end

	if cur ~= "/" then
		ensure_dir(cur)
	end

	for i = start_i, #parts do
		if cur == "/" then
			cur = "/" .. parts[i]
		else
			cur = cur .. "/" .. parts[i]
		end

		ensure_dir(cur)
	end

	return {
		path = path,
		created = true,
		exists = true,
		parents = true,
	}
end


local function _remove(args)
	local path = args.path
	if not util.path_ok(path) then
		error("need path")
	end

	local recursive = args.recursive == true
	local attr = lfs.attributes(path)
	if attr == nil then
		return {
			path = path,
			removed = false,
			exists = false,
		}
	end

	if attr.mode == "directory" then
		if not recursive then
			error("directory needs recursive=true")
		end

		_rm_tree(path)
	else
		local ok, err = os.remove(path)
		if not ok then
			error(tostring(err or "remove fail"))
		end
	end

	return {
		path = path,
		removed = true,
		exists = false,
		recursive = recursive,
	}
end


local function _copy(args)
	local src = args.src
	local dst = args.dst
	if not util.path_ok(src) or not util.path_ok(dst) then
		error("need src and dst")
	end

	local attr = lfs.attributes(src)
	if attr == nil then
		error("src not found")
	end

	if lfs.attributes(dst) ~= nil then
		error("dst exists")
	end

	_copy_tree(src, dst)

	return {
		src = src,
		dst = dst,
		mode = attr.mode or "",
		ok = true,
	}
end


local function _move(args)
	local src = args.src
	local dst = args.dst
	if not util.path_ok(src) or not util.path_ok(dst) then
		error("need src and dst")
	end

	local attr = lfs.attributes(src)
	if attr == nil then
		error("src not found")
	end

	if lfs.attributes(dst) ~= nil then
		error("dst exists")
	end

	local ok, err = os.rename(src, dst)
	if not ok then
		-- cross-device fallback: copy then remove
		_copy_tree(src, dst)
		_rm_tree(src)
	end

	return {
		src = src,
		dst = dst,
		mode = attr.mode or "",
		ok = true,
		renamed = ok == true,
	}
end


M.register = function (catalog, dispatch)
	local ok, err = util.reg(catalog, dispatch, {
		name = "fs.mkdir",
		category = "fs",
		title = "make directory",
		summary = "Create directory; parents=true makes intermediate dirs. Needs write_fs.",
		min_profile = "net",
		args_schema = {
			path = "string directory path",
			parents = "boolean optional",
		},
		examples = {
			{ args = { path = "/tmp/alua_fs_dir", parents = true } },
		},
		docs = "README.md",
		side_effects = { "write_fs" },
	}, _mkdir)
	if not ok then
		return false, err
	end

	ok, err = util.reg(catalog, dispatch, {
		name = "fs.remove",
		category = "fs",
		title = "remove path",
		summary = "Remove file; directory needs recursive=true. Needs write_fs.",
		min_profile = "net",
		args_schema = {
			path = "string path",
			recursive = "boolean optional for directories",
		},
		examples = {
			{ args = { path = "/tmp/alua_fs_write.txt" } },
		},
		docs = "README.md",
		side_effects = { "write_fs" },
	}, _remove)
	if not ok then
		return false, err
	end

	ok, err = util.reg(catalog, dispatch, {
		name = "fs.copy",
		category = "fs",
		title = "copy path",
		summary = "Copy file or directory tree to new dst (dst must not exist). Needs write_fs.",
		min_profile = "net",
		args_schema = {
			src = "string source path",
			dst = "string destination path",
		},
		examples = {
			{ args = { src = "/tmp/a.txt", dst = "/tmp/b.txt" } },
		},
		docs = "README.md",
		side_effects = { "write_fs" },
	}, _copy)
	if not ok then
		return false, err
	end

	ok, err = util.reg(catalog, dispatch, {
		name = "fs.move",
		category = "fs",
		title = "move/rename path",
		summary = "Rename/move src to dst (dst must not exist). Needs write_fs.",
		min_profile = "net",
		args_schema = {
			src = "string source path",
			dst = "string destination path",
		},
		examples = {
			{ args = { src = "/tmp/a.txt", dst = "/tmp/c.txt" } },
		},
		docs = "README.md",
		side_effects = { "write_fs" },
	}, _move)
	if not ok then
		return false, err
	end

	return true
end


return M

-- end
