--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
--
-- @file   init.lua
-- @brief  klbstore 存储节点 (文件本体 + 本地索引)
-- @stability experimental
-- @impl evolving
-- @note   落盘: root/group/M00/xx/yy/<name>[.<ext>], 由 id.rel_path 给出相对路径
--   \n 写: 整包先写同目录临时文件, 完成后改名归位; 分片按 offset 追加
--   \n 读: 下载主路径由 klbweb static 承担 (Range/304), 本接口供脚本内取用
--   \n 一致: 索引先写后落盘, 失败回滚索引; 崩溃后由 gc 对账
--]]


local lfs = require("lfs")

local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local id = require("klbcore.klbstore.id")
local index = require("klbcore.klbstore.index")


local mod = {}


local s_seq = 0


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


local function _dirname(path)
	return (string.match(path, "^(.*)/[^/]*$"))
end


local function _temp_of(path)
	s_seq = (s_seq + 1) % 65536
	local addr = tostring({}):match("0x(%x+)") or "0"

	return string.format("%s.part.%d.%04x.%s", path, os.time(), s_seq, addr)
end


local function _valid(file_id)
	local ok_k, kstore = pcall(require, "kstore")
	if ok_k and type(kstore) == "table" and type(kstore.valid_id) == "function" then
		return kstore.valid_id(file_id) == true
	end

	return id.is_valid(file_id)
end


local function _abs_of(st, file_id)
	local ok_k, kstore = pcall(require, "kstore")
	if ok_k and type(kstore) == "table" and type(kstore.abs_path) == "function" then
		local path = kstore.abs_path(st.root, file_id)
		if type(path) == "string" then
			return path
		end
	end

	local rel = id.rel_path(file_id)
	if rel == nil then
		return nil
	end

	return st.root .. "/" .. rel
end


-- @brief 创建存储节点
-- @param [in] cfg[table]			{ root, group, prefix, index }
-- @return [table] 存储对象; 失败 nil, code
-- @stability experimental
mod.new = function (cfg)
	if type(cfg) ~= "table" or type(cfg.root) ~= "string" or cfg.root == "" then
		return nil, base.INVAL
	end

	local root = string.gsub(cfg.root, "\\", "/")
	root = string.gsub(root, "/+$", "")
	if root == "" then
		return nil, base.INVAL
	end

	local st = {
		root = root,
		group = cfg.group or id.GROUP_DEFAULT,
		prefix = cfg.prefix or id.PREFIX,
		index = cfg.index,
	}

	if not _mkdir_p(root) then
		return nil, base.IO
	end

	return st
end


-- @brief 关闭存储节点
-- @param [in] st[table]			存储对象
-- @return [boolean] true 成功
-- @stability experimental
mod.close = function (st)
	if type(st) ~= "table" then
		return false
	end

	if type(st.index) == "table" then
		index.close(st.index)
		st.index = nil
	end

	return true
end


-- @brief 写入文件 (整包或分片)
-- @param [in] st[table]			存储对象
-- @param [in] file_id[string]		文件标识
-- @param [in] data[string]			数据
-- @param [in] opts[table|nil]		{ overwrite = false, offset = 0, total = nil }
-- @return [boolean] true 成功; 失败 nil, code
-- @stability experimental
mod.put = function (st, file_id, data, opts)
	if type(st) ~= "table" or type(file_id) ~= "string" or type(data) ~= "string" then
		return nil, base.INVAL
	end

	if not _valid(file_id) then
		return nil, base.INVAL
	end

	opts = opts or {}
	local offset = tonumber(opts.offset) or 0
	local total = tonumber(opts.total)
	local overwrite = (opts.overwrite == true)

	if offset < 0 then
		return nil, base.INVAL
	end

	local path = _abs_of(st, file_id)
	if path == nil then
		return nil, base.INVAL
	end

	local dir = _dirname(path)
	if not _mkdir_p(dir) then
		return nil, base.IO
	end

	if total == nil and offset == 0 then
		-- 整包: 临时文件写完再改名归位
		local exists = (lfs.attributes(path, "mode") == "file")
		if exists and not overwrite then
			return nil, base.BUSY
		end

		local tmp = _temp_of(path)
		local fh = io.open(tmp, "wb")
		if fh == nil then
			return nil, base.IO
		end

		fh:write(data)
		fh:close()

		if exists then
			os.remove(path)
		end

		local ok, err = os.rename(tmp, path)
		if not ok then
			os.remove(tmp)
			return nil, base.IO
		end
	else
		-- 分片: 按 offset 覆盖写
		local exists = (lfs.attributes(path, "mode") == "file")
		local fh = io.open(path, exists and "r+b" or "w+b")
		if fh == nil then
			return nil, base.IO
		end

		local ok_seek = fh:seek("set", offset)
		if not ok_seek then
			fh:close()
			return nil, base.RANGE
		end

		fh:write(data)
		fh:close()
	end

	if type(st.index) == "table" then
		local attr = lfs.attributes(path)
		local size = 0
		local mtime = os.time()
		local info = id.parse(file_id)
		if type(attr) == "table" then
			size = tonumber(attr.size) or 0
			mtime = tonumber(attr.modification) or mtime
		end
		index.put(st.index, file_id, {
			size = size,
			mtime = mtime,
			ext = info and info.ext or "",
			ref = 1,
		})
	end

	return true
end


-- @brief 读取文件片段
-- @param [in] st[table]			存储对象
-- @param [in] file_id[string]		文件标识
-- @param [in] offset[number|nil]	起始偏移; nil 为 0
-- @param [in] length[number|nil]	长度; nil 到文件尾
-- @return [string] data; 失败 nil, code
-- @stability experimental
mod.get = function (st, file_id, offset, length)
	if type(st) ~= "table" or type(file_id) ~= "string" then
		return nil, base.INVAL
	end

	if not _valid(file_id) then
		return nil, base.INVAL
	end

	offset = tonumber(offset) or 0
	if offset < 0 then
		return nil, base.INVAL
	end

	local path = _abs_of(st, file_id)
	if path == nil then
		return nil, base.INVAL
	end

	if lfs.attributes(path, "mode") ~= "file" then
		return nil, base.NOENT
	end

	local fh = io.open(path, "rb")
	if fh == nil then
		return nil, base.IO
	end

	if not fh:seek("set", offset) then
		fh:close()
		return nil, base.RANGE
	end

	local data
	if tonumber(length) ~= nil and tonumber(length) >= 0 then
		data = fh:read(tonumber(length))
	else
		data = fh:read("*a")
	end
	fh:close()

	if data == nil then
		return nil, base.RANGE
	end

	return data
end


-- @brief 删除文件
-- @param [in] st[table]			存储对象
-- @param [in] file_id[string]		文件标识
-- @return [boolean] true 成功; 失败 nil, code
-- @stability experimental
mod.del = function (st, file_id)
	if type(st) ~= "table" or type(file_id) ~= "string" then
		return nil, base.INVAL
	end

	if not _valid(file_id) then
		return nil, base.INVAL
	end

	local path = _abs_of(st, file_id)
	if path == nil then
		return nil, base.INVAL
	end

	if lfs.attributes(path, "mode") ~= "file" then
		return nil, base.NOENT
	end

	local ok = os.remove(path)
	if not ok then
		return nil, base.IO
	end

	if type(st.index) == "table" then
		index.del(st.index, file_id)
	end

	return true
end


-- @brief 取文件元信息
-- @param [in] st[table]			存储对象
-- @param [in] file_id[string]		文件标识
-- @return [table] { size, mtime }; 失败 nil, code
-- @stability experimental
mod.stat = function (st, file_id)
	if type(st) ~= "table" or type(file_id) ~= "string" then
		return nil, base.INVAL
	end

	if not _valid(file_id) then
		return nil, base.INVAL
	end

	local path = _abs_of(st, file_id)
	if path == nil then
		return nil, base.INVAL
	end

	local attr = lfs.attributes(path)
	if type(attr) ~= "table" or attr.mode ~= "file" then
		return nil, base.NOENT
	end

	return {
		size = tonumber(attr.size) or 0,
		mtime = tonumber(attr.modification) or 0,
	}
end


-- @brief 取文件绝对路径
-- @param [in] st[table]			存储对象
-- @param [in] file_id[string]		文件标识
-- @return [string] path; 失败 nil, code
-- @stability experimental
mod.path = function (st, file_id)
	if type(st) ~= "table" or type(file_id) ~= "string" then
		return nil, base.INVAL
	end

	if not _valid(file_id) then
		return nil, base.INVAL
	end

	return _abs_of(st, file_id)
end


return mod

-- end
