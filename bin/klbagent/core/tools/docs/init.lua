--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  docs.* 只读说明书工具
-- @note   读取 klbagent/docs/ 下 Markdown; 禁 .. 与绝对路径
-- @history 修改历史
--  \n [2026-09] 创建文件
--]]
local lfs = require("lfs")


local M = {}


local function _pkg_root()
	local info = debug.getinfo(1, "S")
	if type(info) ~= "table" or type(info.source) ~= "string" then
		return nil
	end

	local src = info.source
	if src:sub(1, 1) ~= "@" then
		return nil
	end

	local path = src:sub(2)
	local root = path:match("(.+)[/\\]core[/\\]tools[/\\]docs[/\\]init%.lua$")
	return root
end


local function _docs_root()
	local root = _pkg_root()
	if root == nil or root == "" then
		return nil
	end

	local last = root:sub(-1)
	if last == "/" or last == "\\" then
		return root .. "docs"
	end

	return root .. "/docs"
end


local function _safe_rel(rel)
	if type(rel) ~= "string" or rel == "" then
		return nil
	end

	rel = rel:gsub("\\", "/")
	if rel:sub(1, 1) == "/" or rel:match("^%a:/") then
		return nil
	end

	if rel:find("%.%.") ~= nil then
		return nil
	end

	return rel
end


local function _join(base, rel)
	local last = base:sub(-1)
	if last == "/" or last == "\\" then
		return base .. rel
	end

	return base .. "/" .. rel
end


local function _list_under(dir, prefix, out)
	for name in lfs.dir(dir) do
		if name ~= "." and name ~= ".." then
			local full = _join(dir, name)
			local rel = prefix == "" and name or (prefix .. "/" .. name)
			local mode = lfs.attributes(full, "mode")
			if mode == "file" then
				out[#out + 1] = rel
			elseif mode == "directory" then
				_list_under(full, rel, out)
			end
		end
	end
end


local function _list(_args)
	local root = _docs_root()
	if root == nil or lfs.attributes(root, "mode") ~= "directory" then
		return {
			root = "",
			files = {},
		}
	end

	local files = {}
	_list_under(root, "", files)
	table.sort(files)
	return {
		root = "docs",
		files = files,
	}
end


local function _read(args)
	local rel = _safe_rel(args.path)
	if rel == nil then
		error("bad path")
	end

	local root = _docs_root()
	if root == nil then
		error("docs root missing")
	end

	local full = _join(root, rel)
	local mode = lfs.attributes(full, "mode")
	if mode ~= "file" then
		error("not found")
	end

	local f = io.open(full, "rb")
	if f == nil then
		error("open fail")
	end

	local text = f:read("*a")
	f:close()
	if type(text) ~= "string" then
		text = ""
	end

	return {
		path = rel,
		text = text,
	}
end


-- @brief 注册 docs 工具
-- @return ok[boolean]
-- @return err[string|nil]
M.register = function (catalog, dispatch)
	local ok, err = catalog.register({
		name = "docs.list",
		category = "catalog",
		title = "list docs files",
		summary = "List Markdown files under klbagent/docs for AI help.",
		args_schema = {},
		examples = {},
		docs = "README.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("docs.list", _list)
	if not ok then
		return false, err
	end

	ok, err = catalog.register({
		name = "docs.read",
		category = "catalog",
		title = "read a docs file",
		summary = "Read one docs file by relative path (no ..).",
		args_schema = {
			path = "string relative to docs/",
		},
		examples = {
			{ args = { path = "README.md" } },
			{ args = { path = "categories.md" } },
		},
		docs = "README.md",
		side_effects = {},
	})
	if not ok then
		return false, err
	end

	ok, err = dispatch.register("docs.read", _read)
	if not ok then
		return false, err
	end

	return true
end


return M
