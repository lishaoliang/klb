--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
--
-- @file   init.lua
-- @brief  klbstore 元数据索引 (SQLite 优先; 无 SQLite 退化侧写 .meta)
-- @stability experimental
-- @impl evolving
-- @note   P0: lsqlite3 建 meta 表 (file_id 主键, grp, size, mtime, sha256, ext, ref)
--   \n min-core 不含 SQLite (no-sqlite): 退化为 <db_path>.d/ 下按 file_id 侧写 .meta, 接口形状不变
--   \n 索引只存元数据, 文件本体在 storage; 二者由 file_id 关联
--]]


local lfs = require("lfs")

local klbcode = require("klbcore.klbcode")
local base = klbcode.base


local ok_sql, sqlite3 = pcall(require, "lsqlite3")
local ok_json, cjson = pcall(require, "cjson")

local mod = {}


mod.backend = (ok_sql and type(sqlite3) == "table") and "sqlite" or "file"


local function _mkdir_p(dir)
	if type(dir) ~= "string" or dir == "" then
		return false
	end

	local acc = ""
	for part in string.gmatch(dir, "[^/\\]+") do
		if acc == "" then
			acc = part
		else
			acc = acc .. "/" .. part
		end

		if lfs.attributes(acc, "mode") ~= "directory" then
			local ok = lfs.mkdir(acc)
			if not ok and lfs.attributes(acc, "mode") ~= "directory" then
				return false
			end
		end
	end

	return true
end


local function _dir_of(db_path)
	return db_path .. ".d"
end


local function _key_of(file_id)
	return (string.gsub(file_id, "[/\\]", "_")) .. ".meta"
end


local function _encode(obj)
	if ok_json then
		return cjson.encode(obj)
	end

	return nil
end


local function _decode(text)
	if ok_json then
		local ok, obj = pcall(cjson.decode, text)
		if ok then
			return obj
		end
	end

	return nil
end


local function _stat_of(list)
	local count = 0
	local bytes = 0

	for i = 1, #list do
		count = count + 1
		bytes = bytes + (tonumber(list[i].size) or 0)
	end

	return { count = count, bytes = bytes }
end


--------------------------------------------------------------------------------------------
-- SQLite 后端

local function _sqlite_open(db_path)
	local db, err = sqlite3.open(db_path)
	if not db then
		return nil, base.IO
	end

	local rc = db:exec([[
CREATE TABLE IF NOT EXISTS meta(
	file_id TEXT PRIMARY KEY,
	grp     TEXT NOT NULL,
	size    INTEGER DEFAULT 0,
	mtime   INTEGER DEFAULT 0,
	sha256  TEXT,
	ext     TEXT,
	ref     INTEGER DEFAULT 1
);
CREATE INDEX IF NOT EXISTS idx_meta_grp ON meta(grp);
]])
	if rc ~= sqlite3.OK then
		db:close()
		return nil, base.IO
	end

	return { backend = "sqlite", db = db, path = db_path }
end


local function _sqlite_put(idx, file_id, meta)
	local db = idx.db
	local stmt = db:prepare("INSERT OR REPLACE INTO meta(file_id,grp,size,mtime,sha256,ext,ref) VALUES(?,?,?,?,?,?,?)")
	if not stmt then
		return nil, base.IO
	end

	local info = require("klbcore.klbstore.id").parse(file_id)
	if info == nil then
		stmt:finalize()
		return nil, base.INVAL
	end

	stmt:bind_values(file_id, info.group, tonumber(meta.size) or 0,
		tonumber(meta.mtime) or 0, tostring(meta.sha256 or ""), tostring(meta.ext or ""),
		tonumber(meta.ref) or 1)
	local rc = stmt:step()
	stmt:finalize()

	if rc ~= sqlite3.DONE and rc ~= sqlite3.OK then
		return nil, base.IO
	end

	return true
end


local function _row_of(stmt)
	return {
		file_id = stmt:get_value(0),
		group = stmt:get_value(1),
		size = tonumber(stmt:get_value(2)) or 0,
		mtime = tonumber(stmt:get_value(3)) or 0,
		sha256 = stmt:get_value(4) or "",
		ext = stmt:get_value(5) or "",
		ref = tonumber(stmt:get_value(6)) or 1,
	}
end


--------------------------------------------------------------------------------------------
-- 文件后端 (.meta 侧写)

local function _file_put(idx, file_id, meta)
	if not _mkdir_p(idx.dir) then
		return nil, base.IO
	end

	local info = require("klbcore.klbstore.id").parse(file_id)
	if info == nil then
		return nil, base.INVAL
	end

	local rec = {
		file_id = file_id,
		group = info.group,
		size = tonumber(meta.size) or 0,
		mtime = tonumber(meta.mtime) or 0,
		sha256 = tostring(meta.sha256 or ""),
		ext = tostring(meta.ext or ""),
		ref = tonumber(meta.ref) or 1,
	}
	local text = _encode(rec)
	if text == nil then
		return nil, base.NOTIMPL
	end

	local path = idx.dir .. "/" .. _key_of(file_id)
	local fh, err = io.open(path, "wb")
	if fh == nil then
		return nil, base.IO
	end

	fh:write(text)
	fh:close()

	return true
end


local function _read_meta(path)
	local fh = io.open(path, "rb")
	if fh == nil then
		return nil
	end

	local text = fh:read("*a")
	fh:close()

	return _decode(text)
end


--------------------------------------------------------------------------------------------
-- 接口

-- @brief 打开索引库
-- @param [in] db_path[string]		索引库路径
-- @param [in] opts[table|nil]		{ sync = "NORMAL" | "FULL" }
-- @return [table] 索引对象; 失败 nil, code
-- @stability experimental
mod.open = function (db_path, opts)
	if type(db_path) ~= "string" or db_path == "" then
		return nil, base.INVAL
	end

	if "sqlite" == mod.backend then
		return _sqlite_open(db_path)
	end

	local dir = _dir_of(db_path)
	if not _mkdir_p(dir) then
		return nil, base.IO
	end

	return { backend = "file", dir = dir, path = db_path }
end


-- @brief 关闭索引库
-- @param [in] idx[table]			索引对象
-- @return [boolean] true 成功
-- @stability experimental
mod.close = function (idx)
	if type(idx) ~= "table" then
		return false
	end

	if idx.backend == "sqlite" and idx.db ~= nil then
		idx.db:close()
		idx.db = nil
	end

	return true
end


-- @brief 写入 / 更新一条元数据
-- @param [in] idx[table]			索引对象
-- @param [in] file_id[string]		文件标识
-- @param [in] meta[table]			{ size, mtime, sha256, ext, ref }
-- @return [boolean] true 成功; 失败 nil, code
-- @stability experimental
mod.put = function (idx, file_id, meta)
	if type(idx) ~= "table" or type(file_id) ~= "string" then
		return nil, base.INVAL
	end

	meta = meta or {}

	if idx.backend == "sqlite" then
		return _sqlite_put(idx, file_id, meta)
	end

	return _file_put(idx, file_id, meta)
end


-- @brief 读取一条元数据
-- @param [in] idx[table]			索引对象
-- @param [in] file_id[string]		文件标识
-- @return [table] meta; 失败 nil, code
-- @stability experimental
mod.get = function (idx, file_id)
	if type(idx) ~= "table" or type(file_id) ~= "string" then
		return nil, base.INVAL
	end

	if idx.backend == "sqlite" then
		local stmt = idx.db:prepare("SELECT file_id,grp,size,mtime,sha256,ext,ref FROM meta WHERE file_id=?")
		if not stmt then
			return nil, base.IO
		end

		stmt:bind_values(file_id)
		local rc = stmt:step()
		if rc ~= sqlite3.ROW then
			stmt:finalize()
			return nil, base.NOENT
		end

		local rec = _row_of(stmt)
		stmt:finalize()

		return rec
	end

	local rec = _read_meta(idx.dir .. "/" .. _key_of(file_id))
	if rec == nil then
		return nil, base.NOENT
	end

	return rec
end


-- @brief 删除一条元数据
-- @param [in] idx[table]			索引对象
-- @param [in] file_id[string]		文件标识
-- @return [boolean] true 成功; 失败 nil, code
-- @stability experimental
mod.del = function (idx, file_id)
	if type(idx) ~= "table" or type(file_id) ~= "string" then
		return nil, base.INVAL
	end

	if idx.backend == "sqlite" then
		local stmt = idx.db:prepare("DELETE FROM meta WHERE file_id=?")
		if not stmt then
			return nil, base.IO
		end

		stmt:bind_values(file_id)
		stmt:step()
		stmt:finalize()

		return true
	end

	os.remove(idx.dir .. "/" .. _key_of(file_id))

	return true
end


-- @brief 按组列举元数据
-- @param [in] idx[table]			索引对象
-- @param [in] group[string]		组名
-- @param [in] opts[table|nil]		{ offset = 0, limit = 100 }
-- @return [table] meta 数组; 失败 nil, code
-- @stability experimental
mod.list = function (idx, group, opts)
	if type(idx) ~= "table" then
		return nil, base.INVAL
	end

	opts = opts or {}
	local offset = tonumber(opts.offset) or 0
	local limit = tonumber(opts.limit) or 100

	if offset < 0 or limit <= 0 then
		return nil, base.INVAL
	end

	if idx.backend == "sqlite" then
		local stmt = idx.db:prepare("SELECT file_id,grp,size,mtime,sha256,ext,ref FROM meta WHERE grp=? ORDER BY file_id LIMIT ? OFFSET ?")
		if not stmt then
			return nil, base.IO
		end

		stmt:bind_values(tostring(group or ""), limit, offset)
		local out = {}
		while stmt:step() == sqlite3.ROW do
			out[#out + 1] = _row_of(stmt)
		end
		stmt:finalize()

		return out
	end

	local all = {}
	for name in lfs.dir(idx.dir) do
		if string.sub(name, -5) == ".meta" then
			local rec = _read_meta(idx.dir .. "/" .. name)
			if rec ~= nil and tostring(rec.group or "") == tostring(group or "") then
				all[#all + 1] = rec
			end
		end
	end

	table.sort(all, function (a, b)
		return tostring(a.file_id) < tostring(b.file_id)
	end)

	local out = {}
	local last = offset + limit
	for i = offset + 1, math.min(#all, last) do
		out[#out + 1] = all[i]
	end

	return out
end


-- @brief 统计条目数
-- @param [in] idx[table]			索引对象
-- @param [in] group[string|nil]	组名; nil 为全组
-- @return [table] { count, bytes }; 失败 nil, code
-- @stability experimental
mod.stat = function (idx, group)
	if type(idx) ~= "table" then
		return nil, base.INVAL
	end

	if idx.backend == "sqlite" then
		local stmt
		if group == nil or group == "" then
			stmt = idx.db:prepare("SELECT COUNT(*), COALESCE(SUM(size),0) FROM meta")
		else
			stmt = idx.db:prepare("SELECT COUNT(*), COALESCE(SUM(size),0) FROM meta WHERE grp=?")
			if stmt then
				stmt:bind_values(tostring(group))
			end
		end

		if not stmt then
			return nil, base.IO
		end

		if stmt:step() ~= sqlite3.ROW then
			stmt:finalize()
			return { count = 0, bytes = 0 }
		end

		local out = {
			count = tonumber(stmt:get_value(0)) or 0,
			bytes = tonumber(stmt:get_value(1)) or 0,
		}
		stmt:finalize()

		return out
	end

	local list = {}
	for name in lfs.dir(idx.dir) do
		if string.sub(name, -5) == ".meta" then
			local rec = _read_meta(idx.dir .. "/" .. name)
			if rec ~= nil and (group == nil or group == "" or tostring(rec.group or "") == tostring(group)) then
				list[#list + 1] = rec
			end
		end
	end

	return _stat_of(list)
end


return mod

-- end
