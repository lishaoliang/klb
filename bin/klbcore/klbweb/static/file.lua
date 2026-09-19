--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   file.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 静态文件读取 (304 / Range / gzip)
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local io = require("io")
local lfs = require("lfs")
local http_mime = require("klbcore.util.http_mime")
local parser = require("klbcore.klbweb.parser")


local file = {}


local BROWSE_TEXT = {
	lua = true,
	md = true,
	txt = true,
	c = true,
	h = true,
	cpp = true,
	hpp = true,
	json = true,
	mdc = true,
}

local WEEK = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" }
local MONTH = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }

local LONG_CACHE = {
	css = true,
	js = true,
	png = true,
	jpg = true,
	jpeg = true,
	gif = true,
	ico = true,
	svg = true,
	webp = true,
	woff = true,
	woff2 = true,
	ttf = true,
	mp4 = true,
	mp3 = true,
}


local function file_ext(filename)
	local ext = string.match(filename, "%.([^.]+)$") or ""
	return string.lower(ext)
end


local function browse_mime(filename)
	local ext = file_ext(filename)
	if BROWSE_TEXT[ext] then
		return "text/plain; charset=utf-8"
	end

	return http_mime(filename)
end


local function cache_control_of(filename, kind)
	if "listing" == kind then
		return "no-store"
	end

	local ext = file_ext(filename)
	if "html" == ext or "htm" == ext then
		return "no-cache"
	end

	if LONG_CACHE[ext] then
		return "public, max-age=86400"
	end

	return "max-age=0"
end


local function http_date(ts)
	local t = os.date("!*t", ts)
	if not t then
		return ""
	end

	return string.format("%s, %02d %s %04d %02d:%02d:%02d GMT",
		WEEK[t.wday] or "Thu", t.day, MONTH[t.month] or "Jan", t.year, t.hour, t.min, t.sec)
end


local function make_etag(size, mtime)
	return string.format("W/\"%x-%x\"", size or 0, mtime or 0)
end


local function read_file(path)
	local f = io.open(path, "rb")
	if not f then
		return nil
	end

	local body = f:read("*a")
	f:close()
	return body
end


local function read_range(path, first, last)
	local f = io.open(path, "rb")
	if not f then
		return nil
	end

	f:seek("set", first)
	local n = last - first + 1
	local body = f:read(n)
	f:close()
	return body
end


local function gzip_body(body)
	local ok, zlib = pcall(require, "zlib")
	if not ok or not zlib or not zlib.deflate then
		return nil
	end

	local ok2, stream = pcall(zlib.deflate, 6, 31)
	if not ok2 or not stream then
		return nil
	end

	local ok3, out = pcall(stream, body, "finish")
	if not ok3 or "string" ~= type(out) then
		return nil
	end

	return out
end


-- @brief 组装 Cache-Control / ETag / Last-Modified
file.extra_of = function (mime, kind, filename, etag, mtime, extra)
	extra = extra or {}
	extra[#extra + 1] = { "Cache-Control", cache_control_of(filename, kind) }
	if etag and "" ~= etag then
		extra[#extra + 1] = { "ETag", etag }
	end

	if mtime then
		local lm = http_date(mtime)
		if "" ~= lm then
			extra[#extra + 1] = { "Last-Modified", lm }
		end
	end

	return extra
end


local function cond_304(headers, etag, mtime)
	headers = headers or {}
	local inm = headers["if-none-match"] or ""
	if "" ~= inm and etag and string.find(inm, etag, 1, true) then
		return true
	end

	local ims = headers["if-modified-since"] or ""
	if "" ~= ims and mtime then
		local lm = http_date(mtime)
		if lm == ims then
			return true
		end
	end

	return false
end


local function want_gzip(headers)
	local enc = string.lower((headers and headers["accept-encoding"]) or "")
	return nil ~= string.find(enc, "gzip", 1, true)
end


-- @brief 读单个文件 (含 304 / Range / gzip)
file.open = function (real, rel, headers, max_file, gzip, gzip_dynamic)
	local attr = lfs.attributes(real)
	if not attr or "file" ~= attr.mode then
		return 404, "", "", "miss", {}
	end

	local size = attr.size or 0
	local mtime = attr.modification
	local filename = string.match(rel, "[^/]*$") or rel
	local mime = browse_mime(filename)
	local extra = file.extra_of(mime, "file", filename, make_etag(size, mtime), mtime, {})

	if gzip and want_gzip(headers) then
		local gz_path = real .. ".gz"
		local gz_attr = lfs.attributes(gz_path)
		if gz_attr and "file" == gz_attr.mode then
			if 0 < max_file and max_file < (gz_attr.size or 0) then
				return 413, "text/plain", "file too large", "large", {}
			end

			local etag = make_etag(gz_attr.size, gz_attr.modification)
			extra = file.extra_of(mime, "file", filename, etag, gz_attr.modification, {
				{ "Content-Encoding", "gzip" },
				{ "Vary", "Accept-Encoding" },
			})
			if cond_304(headers, etag, gz_attr.modification) then
				return 304, mime, "", "not-modified", extra
			end

			local body = read_file(gz_path)
			if not body then
				return 404, "", "", "miss", {}
			end

			return 200, mime, body, "file", extra
		end
	end

	if cond_304(headers, make_etag(size, mtime), mtime) then
		return 304, mime, "", "not-modified", extra
	end

	local rng = parser.parse_range(headers, size)
	if false == rng then
		extra[#extra + 1] = { "Content-Range", string.format("bytes */%d", size) }
		return 416, "text/plain", "Range Not Satisfiable", "range", extra
	end

	if rng then
		local slice = rng.last - rng.first + 1
		if 0 < max_file and max_file < slice then
			return 413, "text/plain", "file too large", "large", {}
		end

		local body = read_range(real, rng.first, rng.last)
		if not body then
			return 404, "", "", "miss", {}
		end

		extra[#extra + 1] = { "Accept-Ranges", "bytes" }
		extra[#extra + 1] = { "Content-Range", string.format("bytes %d-%d/%d", rng.first, rng.last, size) }
		return 206, mime, body, "file", extra
	end

	if 0 < max_file and max_file < size then
		return 413, "text/plain", "file too large", "large", {}
	end

	local body = read_file(real)
	if not body then
		return 404, "", "", "miss", {}
	end

	if gzip_dynamic and want_gzip(headers) and 64 < size then
		local gz = gzip_body(body)
		if gz and #gz < #body then
			extra[#extra + 1] = { "Content-Encoding", "gzip" }
			extra[#extra + 1] = { "Vary", "Accept-Encoding" }
			return 200, mime, gz, "file", extra
		end
	end

	extra[#extra + 1] = { "Accept-Ranges", "bytes" }
	return 200, mime, body, "file", extra
end


return file
