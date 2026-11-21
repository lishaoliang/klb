--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @brief  klbtar 格式探测 (魔数优先, 后缀兜底)
-- @note   目标: tar / gzip / tar.gz|tgz
-- @history 修改历史
--  \n 2026 创建文件
--  \n 2026 M1 实现 probe
--]]


local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local util = require("klbcore.klbtar.util")


local detect = {}


detect.FMT_TAR = "tar"
detect.FMT_GZIP = "gzip"
detect.FMT_TAR_GZ = "tar.gz"
detect.FMT_UNKNOWN = "unknown"


local function is_gzip_magic(buf)
	return "string" == type(buf) and 2 <= #buf
		and 0x1F == string.byte(buf, 1)
		and 0x8B == string.byte(buf, 2)
end


local function is_ustar_magic(buf)
	if "string" ~= type(buf) or 262 > #buf then
		return false
	end

	local magic = string.sub(buf, 258, 262)
	return "ustar" == magic
end


local function tar_checksum_ok(block)
	if "string" ~= type(block) or 512 ~= #block then
		return false
	end

	local sum = 0
	for i = 1, 512 do
		if 149 <= i and i <= 156 then
			sum = sum + 32
		else
			sum = sum + string.byte(block, i)
		end
	end

	local field = string.sub(block, 149, 156)
	local oct = string.match(field, "^%s*(%d+)")
	if nil == oct then
		return false
	end

	return sum == tonumber(oct, 8)
end


local function suffix_hint(path)
	local lower = string.lower(path or "")
	if string.match(lower, "%.tar%.gz$") or string.match(lower, "%.tgz$") then
		return detect.FMT_TAR_GZ
	end

	if string.match(lower, "%.tar$") then
		return detect.FMT_TAR
	end

	if string.match(lower, "%.gz$") then
		return detect.FMT_GZIP
	end

	return nil
end


local function peek_inflated_block(path)
	local ok, zlib = pcall(require, "zlib")
	if not ok or not zlib or not zlib.inflate then
		return nil
	end

	local f = io.open(path, "rb")
	if nil == f then
		return nil
	end

	local inflate = zlib.inflate()
	local out = ""
	local done = false

	while 512 > #out and not done do
		local chunk = f:read(util.CHUNK)
		if nil == chunk or "" == chunk then
			local piece, eof = inflate("")
			if "string" == type(piece) then
				out = out .. piece
			end
			done = true == eof
			break
		end

		local piece, eof = inflate(chunk)
		if "string" == type(piece) then
			out = out .. piece
		end
		done = true == eof
	end

	f:close()

	if 512 <= #out then
		return string.sub(out, 1, 512)
	end

	return out
end


-- @brief 探测归档格式
-- @param [in]  path[string]			文件路径
-- @return fmt[string]					格式名; 失败为 nil
-- @return code[number(int)]			0 成功; 非0 为 E_*
-- @return errstr[string]				描述; 成功为 ""
detect.probe = function (path)
	if not util.check_path(path) then
		return util.fail(base.INVAL, "bad path")
	end

	local f = io.open(path, "rb")
	if nil == f then
		return util.fail(base.NOENT, "open fail")
	end

	local head = f:read(512) or ""
	f:close()

	if "" == head then
		return util.fail(base.PACKAGE_BAD, "empty file")
	end

	local hint = suffix_hint(path)

	if is_gzip_magic(head) then
		if detect.FMT_TAR_GZ == hint then
			return detect.FMT_TAR_GZ, klbcode.OK, ""
		end

		local block = peek_inflated_block(path)
		if nil ~= block and (is_ustar_magic(block) or tar_checksum_ok(block)) then
			return detect.FMT_TAR_GZ, klbcode.OK, ""
		end

		if detect.FMT_GZIP == hint or nil == hint then
			return detect.FMT_GZIP, klbcode.OK, ""
		end

		return detect.FMT_GZIP, klbcode.OK, ""
	end

	if is_ustar_magic(head) or tar_checksum_ok(head) then
		return detect.FMT_TAR, klbcode.OK, ""
	end

	if detect.FMT_TAR == hint then
		-- 后缀兜底: 无魔数仍当 tar (旧 v7 可能无 ustar)
		if tar_checksum_ok(head) or 512 == #head then
			return detect.FMT_TAR, klbcode.OK, ""
		end
	end

	return util.fail(base.PACKAGE_BAD, "unknown archive format")
end


return detect

-- end
