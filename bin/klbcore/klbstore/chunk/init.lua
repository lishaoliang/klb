--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
--
-- @file   init.lua
-- @brief  klbstore 分片上传 (大文件切片与会话)
-- @stability experimental
-- @impl evolving
-- @note   分片键: file_id + index; 会话内记已收区间, 完成后校验总数并归位
--   \n 临时名带进程内序号与对象地址后缀, 禁止 os.time() 秒级单键
--   \n cfg: { storage, group, ext, total, chunk_size, name, root }; 无 storage 时按 root 新建
--]]


local lfs = require("lfs")

local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local id = require("klbcore.klbstore.id")
local storage = require("klbcore.klbstore.storage")
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


local function _temp_of(dir)
	s_seq = (s_seq + 1) % 65536
	local addr = tostring({}):match("0x(%x+)") or "0"

	return string.format("%s/chunk.%d.%04x.%s", dir, os.time(), s_seq, addr)
end


-- @brief 规划分片区间
-- @param [in] total[number]		总字节
-- @param [in] size[number|nil]		分片大小; nil 取 1 MiB
-- @return [table] { { offset, length }, ... }; 失败 nil, code
-- @stability experimental
mod.plan = function (total, size)
	if "number" ~= type(total) or total < 0 then
		return nil, base.INVAL
	end

	if "number" ~= type(size) or size <= 0 then
		size = 1024 * 1024
	end

	local list = {}
	local offset = 0

	while offset < total do
		local length = size
		if offset + length > total then
			length = total - offset
		end

		list[#list + 1] = { offset = offset, length = length }
		offset = offset + length
	end

	return list
end


-- @brief 开启上传会话
-- @param [in] cfg[table]			{ storage, group, ext, total, chunk_size, name, root }
-- @return [table] 会话对象; 失败 nil, code
-- @stability experimental
mod.new = function (cfg)
	if type(cfg) ~= "table" then
		return nil, base.INVAL
	end

	local total = tonumber(cfg.total)
	if total == nil or total < 0 then
		return nil, base.INVAL
	end

	local st = cfg.storage
	if type(st) ~= "table" then
		st = storage.new({ root = cfg.root, group = cfg.group })
		if st == nil then
			return nil, base.INVAL
		end
	end

	local file_id, code = id.new_id(cfg.group, cfg.ext)
	if file_id == nil then
		return nil, code
	end

	local plan, code_p = mod.plan(total, cfg.chunk_size)
	if plan == nil then
		return nil, code_p
	end

	local tmp_dir = st.root .. "/.tmp"
	if not _mkdir_p(tmp_dir) then
		return nil, base.IO
	end

	local tmp = _temp_of(tmp_dir)
	local fh = io.open(tmp, "wb")
	if fh == nil then
		return nil, base.IO
	end
	fh:close()

	return {
		storage = st,
		file_id = file_id,
		total = total,
		chunk_size = cfg.chunk_size,
		name = cfg.name,
		plan = plan,
		received = {},
		tmp = tmp,
	}
end


-- @brief 推入一片
-- @param [in] sess[table]			会话对象
-- @param [in] index[number]		片序号 (从 0 起)
-- @param [in] data[string]			片数据
-- @return [boolean] true 成功; 失败 nil, code
-- @stability experimental
mod.push = function (sess, index, data)
	if type(sess) ~= "table" or type(sess.plan) ~= "table" then
		return nil, base.INVAL
	end

	if type(data) ~= "string" then
		return nil, base.INVAL
	end

	local i = tonumber(index)
	if i == nil then
		return nil, base.INVAL
	end

	local item = sess.plan[i + 1]
	if item == nil then
		return nil, base.RANGE
	end

	if #data ~= item.length then
		return nil, base.RANGE
	end

	local fh = io.open(sess.tmp, "r+b")
	if fh == nil then
		return nil, base.IO
	end

	if not fh:seek("set", item.offset) then
		fh:close()
		return nil, base.RANGE
	end

	fh:write(data)
	fh:close()

	sess.received[i] = true

	return true
end


-- @brief 完成会话 (校验总数并归位)
-- @param [in] sess[table]			会话对象
-- @return [string] file_id; 失败 nil, code
-- @stability experimental
mod.finish = function (sess)
	if type(sess) ~= "table" or type(sess.storage) ~= "table" then
		return nil, base.INVAL
	end

	if sess.received == nil or #sess.plan ~= 0 then
		for i = 0, #sess.plan - 1 do
			if not sess.received[i] then
				return nil, base.RANGE
			end
		end
	end

	local dest = storage.path(sess.storage, sess.file_id)
	if dest == nil then
		return nil, base.INVAL
	end

	local dir = string.match(dest, "^(.*)/[^/]*$")
	if not _mkdir_p(dir) then
		return nil, base.IO
	end

	if lfs.attributes(dest, "mode") == "file" then
		os.remove(dest)
	end

	local ok = os.rename(sess.tmp, dest)
	if not ok then
		return nil, base.IO
	end
	sess.tmp = nil

	if type(sess.storage.index) == "table" then
		local info = id.parse(sess.file_id)
		index.put(sess.storage.index, sess.file_id, {
			size = sess.total,
			mtime = os.time(),
			ext = info and info.ext or "",
			ref = 1,
		})
	end

	return sess.file_id
end


-- @brief 放弃会话 (清理临时片)
-- @param [in] sess[table]			会话对象
-- @return [boolean] true 成功
-- @stability experimental
mod.abort = function (sess)
	if type(sess) ~= "table" then
		return false
	end

	if type(sess.tmp) == "string" then
		os.remove(sess.tmp)
		sess.tmp = nil
	end

	sess.received = {}

	return true
end


return mod

-- end
