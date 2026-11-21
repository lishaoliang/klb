--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  klbtar gzip 流封装 (基于 require("zlib"))
-- @note   解压侧; 分块 inflate
-- @history 修改历史
--  \n 2026 创建文件
--  \n 2026 M2 实现 open_read / extract / read_all
--]]


local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local util = require("klbcore.klbtar.util")


local gzip = {}


local function load_zlib()
	local ok, zlib = pcall(require, "zlib")
	if not ok or not zlib or not zlib.inflate then
		return nil
	end

	return zlib
end


-- @brief 打开 gzip 只读字节源
-- @return src[table]: read(n) / close(); 失败 nil, code, errstr
gzip.open_read = function (path)
	if not util.check_path(path) then
		return util.fail(base.INVAL, "bad path")
	end

	local zlib = load_zlib()
	if nil == zlib then
		return util.fail(base.NOTSUP, "zlib unavailable")
	end

	local f = io.open(path, "rb")
	if nil == f then
		return util.fail(base.NOENT, "open fail")
	end

	local ok_inf, inflate = pcall(zlib.inflate)
	if not ok_inf or not inflate then
		f:close()
		return util.fail(base.IO, "inflate init fail")
	end

	local buf = ""
	local src_eof = false
	local closed = false
	local err_msg = nil

	local function pull_more()
		if src_eof or nil ~= err_msg then
			return
		end

		local chunk = f:read(util.CHUNK)
		if nil == chunk or "" == chunk then
			local ok2, piece, eof = pcall(inflate, "")
			if not ok2 then
				err_msg = "inflate fail"
				return
			end

			if "string" == type(piece) and "" ~= piece then
				buf = buf .. piece
			end

			src_eof = true
			return
		end

		local ok2, piece, eof = pcall(inflate, chunk)
		if not ok2 then
			err_msg = "inflate fail"
			return
		end

		if "string" == type(piece) and "" ~= piece then
			buf = buf .. piece
		end

		if true == eof then
			src_eof = true
		end
	end


	local src = {}

	src.read = function (n)
		n = math.floor(tonumber(n) or 0)
		if n <= 0 or closed then
			return nil
		end

		while n > #buf and not src_eof and nil == err_msg do
			pull_more()
		end

		if nil ~= err_msg then
			return nil, base.IO, err_msg
		end

		if 0 == #buf then
			return nil
		end

		local take = n
		if take > #buf then
			take = #buf
		end

		local out = string.sub(buf, 1, take)
		buf = string.sub(buf, take + 1)
		return out
	end

	src.close = function ()
		if closed then
			return
		end

		closed = true
		f:close()
	end

	return src, klbcode.OK, ""
end


-- @brief 完整解压到字符串 (受 max_file)
gzip.read_all = function (path, opts)
	local o = util.parse_opts(opts)
	local src, code, err = gzip.open_read(path)
	if nil == src then
		return util.fail(code, err)
	end

	local parts = {}
	local total = 0

	while true do
		local chunk, c2, e2 = src.read(util.CHUNK)
		if nil ~= c2 then
			src.close()
			return util.fail(c2, e2)
		end

		if nil == chunk then
			break
		end

		total = total + #chunk
		if total > o.max_file then
			src.close()
			return util.fail(base.LIMIT, "max_file exceeded")
		end

		parts[#parts + 1] = chunk
	end

	src.close()
	return table.concat(parts), klbcode.OK, ""
end


-- @brief 将 gzip 解压到目标文件路径
gzip.extract = function (path, dest, opts)
	if not util.check_path(dest) then
		return util.fail_bool(base.INVAL, "bad dest")
	end

	local o = util.parse_opts(opts)
	if (not o.overwrite) and util.file_exists(dest) then
		return util.fail_bool(base.NOENT, "dest exists")
	end

	if not util.ensure_parent(dest) then
		return util.fail_bool(base.IO, "mkdir fail")
	end

	local src, code, err = gzip.open_read(path)
	if nil == src then
		return util.fail_bool(code, err)
	end

	local out = io.open(dest, "wb")
	if nil == out then
		src.close()
		return util.fail_bool(base.IO, "create dest fail")
	end

	local total = 0
	while true do
		local chunk, c2, e2 = src.read(util.CHUNK)
		if nil ~= c2 then
			out:close()
			src.close()
			return util.fail_bool(c2, e2)
		end

		if nil == chunk then
			break
		end

		total = total + #chunk
		if total > o.max_file then
			out:close()
			src.close()
			return util.fail_bool(base.LIMIT, "max_file exceeded")
		end

		out:write(chunk)
	end

	out:close()
	src.close()
	return util.ok_bool()
end


return gzip

-- end
