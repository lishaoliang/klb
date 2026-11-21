--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  klbtar 归档对象 (open 后 list/extract/read/open_entry)
-- @stability internal
-- @history 修改历史
--  \n 2026 M7-M8 创建
--  \n 2026 移入 archive/ 子目录
--]]


local lfs = require("lfs")
local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local util = require("klbcore.klbtar.util")
local detect = require("klbcore.klbtar.detect")
local gzip = require("klbcore.klbtar.gzip")
local reader = require("klbcore.klbtar.tar.reader")


local archive = {}


local function open_src(path, fmt)
	if detect.FMT_TAR == fmt then
		return reader.open_file_src(path)
	end

	if detect.FMT_TAR_GZ == fmt or detect.FMT_GZIP == fmt then
		return gzip.open_read(path)
	end

	return util.fail(base.NOTSUP, "unsupported format")
end


local function with_src(self, fn)
	if self._closed then
		return util.fail(base.IO, "archive closed")
	end

	local src, code, err = open_src(self.path, self.fmt)
	if nil == src then
		return nil, code, err
	end

	local a, b, c = fn(src)
	src.close()
	return a, b, c
end


local function with_src_bool(self, fn)
	if self._closed then
		return util.fail_bool(base.IO, "archive closed")
	end

	local src, code, err = open_src(self.path, self.fmt)
	if nil == src then
		return util.fail_bool(code, err)
	end

	local a, b, c = fn(src)
	src.close()
	return a, b, c
end


-- @brief 打开归档
-- @return arch[table]|nil, code, errstr
archive.open = function (path, opts)
	if not util.check_path(path) then
		return util.fail(base.INVAL, "bad path")
	end

	local fmt, code, err = detect.probe(path)
	if nil == fmt then
		return util.fail(code, err)
	end

	local o = util.parse_opts(opts)
	local self = {
		path = path,
		fmt = fmt,
		_opts = o,
		_closed = false,
	}

	self.format = function ()
		return self.fmt
	end

	self.is_open = function ()
		return not self._closed
	end

	self.close = function ()
		self._closed = true
	end

	self.list = function (opts2)
		if detect.FMT_GZIP == self.fmt then
			local name = util.drop_gz_suffix(util.basename(self.path))
			return {
				{ name = name, size = -1, kind = "file", typeflag = "0" },
			}, klbcode.OK, ""
		end

		return with_src(self, function (src)
			return reader.list_src(src, opts2 or self._opts)
		end)
	end

	self.extract = function (dest, opts2)
		local opt = opts2 or self._opts
		if detect.FMT_GZIP == self.fmt then
			local out = dest
			if "directory" == lfs.attributes(dest, "mode") then
				out = dest .. "/" .. util.drop_gz_suffix(util.basename(self.path))
			end

			return gzip.extract(self.path, out, opt)
		end

		return with_src_bool(self, function (src)
			return reader.extract_src(src, dest, opt)
		end)
	end

	self.extract_file = function (name, dest, opts2)
		local opt = opts2 or self._opts
		if detect.FMT_GZIP == self.fmt then
			return gzip.extract(self.path, dest, opt)
		end

		return with_src_bool(self, function (src)
			return reader.extract_file_src(src, name, dest, opt)
		end)
	end

	self.read = function (name, opts2)
		local opt = opts2 or self._opts
		if detect.FMT_GZIP == self.fmt then
			return gzip.read_all(self.path, opt)
		end

		return with_src(self, function (src)
			return reader.read_src(src, name, opt)
		end)
	end

	self.open_entry = function (name, opts2)
		if self._closed then
			return util.fail(base.IO, "archive closed")
		end

		local opt = opts2 or self._opts
		if detect.FMT_GZIP == self.fmt then
			local src, c1, e1 = gzip.open_read(self.path)
			if nil == src then
				return util.fail(c1, e1)
			end

			local closed = false
			local entry = {
				name = util.drop_gz_suffix(util.basename(self.path)),
				size = -1,
				kind = "file",
			}

			entry.read = function (n)
				if closed then
					return nil
				end

				return src.read(n)
			end

			entry.readall = function ()
				if closed then
					return util.fail(base.IO, "entry closed")
				end

				local parts = {}
				local total = 0
				while true do
					local chunk, c2, e2 = src.read(util.CHUNK)
					if nil ~= c2 then
						return util.fail(c2, e2)
					end

					if nil == chunk then
						break
					end

					total = total + #chunk
					if total > opt.max_file then
						return util.fail(base.LIMIT, "max_file exceeded")
					end

					parts[#parts + 1] = chunk
				end

				return table.concat(parts), klbcode.OK, ""
			end

			entry.close = function ()
				if not closed then
					closed = true
					src.close()
				end
			end

			return entry, klbcode.OK, ""
		end

		local src, c1, e1 = open_src(self.path, self.fmt)
		if nil == src then
			return util.fail(c1, e1)
		end

		local entry, ec, ee = reader.open_entry_src(src, name, opt)
		if nil == entry then
			src.close()
			return util.fail(ec, ee)
		end

		local user_close = entry.close
		entry.close = function ()
			user_close()
			src.close()
		end

		return entry, klbcode.OK, ""
	end

	return self, klbcode.OK, ""
end


return archive

-- end
