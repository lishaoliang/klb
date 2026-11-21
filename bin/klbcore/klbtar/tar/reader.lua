--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   reader.lua
-- @brief  klbtar tar 只读解析 / 解压 (ustar); 字节源接口
-- @note   src.read(n) / src.close(); 裸 tar 或 gzip 管道源
-- @history 修改历史
--  \n 2026 创建文件
--  \n 2026 M3-M8 list / extract / read / open_entry
--]]


local klbcode = require("klbcore.klbcode")
local base = klbcode.base
local util = require("klbcore.klbtar.util")


local reader = {}


-- 成功只返回 1 个 string; 失败 nil, code, err; 干净 EOF 为 nil (无 code)
local function read_exact(src, n)
	n = math.floor(tonumber(n) or 0)
	if n <= 0 then
		return ""
	end

	local parts = {}
	local left = n

	while left > 0 do
		local chunk, code, err = src.read(left)
		if nil ~= code then
			return nil, code, err
		end

		if nil == chunk or "" == chunk then
			if 0 == #parts then
				return nil
			end

			return nil, base.PACKAGE_BAD, "unexpected eof in tar"
		end

		parts[#parts + 1] = chunk
		left = left - #chunk
	end

	return table.concat(parts)
end


local function skip_bytes(src, n)
	local left = n
	while left > 0 do
		local take = left
		if take > util.CHUNK then
			take = util.CHUNK
		end

		local chunk, code, err = src.read(take)
		if nil ~= code then
			return code, err
		end

		if nil == chunk or "" == chunk then
			return base.PACKAGE_BAD, "unexpected eof skip"
		end

		left = left - #chunk
	end

	return klbcode.OK, ""
end


local function parse_octal(s)
	local digits = string.match(s or "", "([0-7]+)")
	if nil == digits then
		return 0
	end

	return tonumber(digits, 8) or 0
end


local function is_zero_block(block)
	return 512 == #block and nil == string.find(block, "[^\0]")
end


local function parse_header(block)
	if 512 ~= #block then
		return nil, base.PACKAGE_BAD, "bad header size"
	end

	if is_zero_block(block) then
		return nil, klbcode.OK, "eof"
	end

	local sum = 0
	for i = 1, 512 do
		if 149 <= i and i <= 156 then
			sum = sum + 32
		else
			sum = sum + string.byte(block, i)
		end
	end

	local chk = parse_octal(string.sub(block, 149, 156))
	if sum ~= chk then
		return nil, base.PACKAGE_BAD, "bad tar checksum"
	end

	local name = string.match(string.sub(block, 1, 100), "^([^\0]*)") or ""
	local prefix = string.match(string.sub(block, 346, 500), "^([^\0]*)") or ""
	local size = parse_octal(string.sub(block, 125, 136))
	local typeflag = string.sub(block, 157, 157)
	if "" == typeflag then
		typeflag = "0"
	end

	if "" ~= prefix then
		if "" ~= name then
			name = prefix .. "/" .. name
		else
			name = prefix
		end
	end

	local kind = "other"
	if "5" == typeflag then
		kind = "dir"
	elseif "2" == typeflag then
		kind = "symlink"
	elseif "1" == typeflag then
		kind = "hardlink"
	elseif "\0" == typeflag or "0" == typeflag then
		kind = "file"
	end

	local pad = 0
	local rem = size % 512
	if 0 ~= rem then
		pad = 512 - rem
	end

	return {
		name = name,
		size = size,
		pad = pad,
		typeflag = typeflag,
		kind = kind,
	}, klbcode.OK, ""
end


local function match_name(hdr_name, want)
	local a = select(1, util.normalize_member(hdr_name))
	local b = select(1, util.normalize_member(want))
	if nil ~= a and nil ~= b then
		return a == b
	end

	return hdr_name == want
end


-- @brief 打开本地文件为字节源
reader.open_file_src = function (path)
	local f = io.open(path, "rb")
	if nil == f then
		return util.fail(base.NOENT, "open fail")
	end

	local closed = false
	local src = {}

	src.read = function (n)
		if closed then
			return nil
		end

		n = math.floor(tonumber(n) or 0)
		if n <= 0 then
			return nil
		end

		local chunk = f:read(n)
		if nil == chunk or "" == chunk then
			return nil
		end

		return chunk
	end

	src.close = function ()
		if not closed then
			closed = true
			f:close()
		end
	end

	return src, klbcode.OK, ""
end


-- @brief 列出条目
reader.list_src = function (src, opts)
	local o = util.parse_opts(opts)
	local list = {}
	local n = 0

	while true do
		local block, code, err = read_exact(src, 512)
		if nil == block and nil == code then
			break
		end

		if nil ~= code then
			return util.fail(code, err)
		end

		local hdr, hc, he = parse_header(block)
		if nil == hdr then
			if "eof" == he then
				break
			end

			return util.fail(hc, he)
		end

		n = n + 1
		if n > o.max_entries then
			return util.fail(base.LIMIT, "max_entries exceeded")
		end

		list[#list + 1] = {
			name = hdr.name,
			size = hdr.size,
			kind = hdr.kind,
			typeflag = hdr.typeflag,
		}

		local skip_n = hdr.size + hdr.pad
		if skip_n > 0 then
			local sc, se = skip_bytes(src, skip_n)
			if klbcode.OK ~= sc then
				return util.fail(sc, se)
			end
		end
	end

	return list, klbcode.OK, ""
end


-- @brief 完整读取成员
reader.read_src = function (src, name, opts)
	local o = util.parse_opts(opts)
	local n = 0

	while true do
		local block, code, err = read_exact(src, 512)
		if nil == block and nil == code then
			break
		end

		if nil ~= code then
			return util.fail(code, err)
		end

		local hdr, hc, he = parse_header(block)
		if nil == hdr then
			if "eof" == he then
				break
			end

			return util.fail(hc, he)
		end

		n = n + 1
		if n > o.max_entries then
			return util.fail(base.LIMIT, "max_entries exceeded")
		end

		if match_name(hdr.name, name) then
			if "dir" == hdr.kind then
				return util.fail(base.INVAL, "member is directory")
			end

			if hdr.size > o.max_file then
				return util.fail(base.LIMIT, "max_file exceeded")
			end

			local body, bc, be = read_exact(src, hdr.size)
			if nil == body then
				return util.fail(bc or base.IO, be or "read fail")
			end

			if hdr.pad > 0 then
				skip_bytes(src, hdr.pad)
			end

			return body, klbcode.OK, ""
		end

		local skip_n = hdr.size + hdr.pad
		if skip_n > 0 then
			local sc, se = skip_bytes(src, skip_n)
			if klbcode.OK ~= sc then
				return util.fail(sc, se)
			end
		end
	end

	return util.fail(base.NOENT, "member not found")
end


-- @brief 解压全部到目录
reader.extract_src = function (src, dest, opts)
	if not util.check_path(dest) then
		return util.fail_bool(base.INVAL, "bad dest")
	end

	local o = util.parse_opts(opts)
	if not util.ensure_dir(dest) then
		return util.fail_bool(base.IO, "mkdir dest fail")
	end

	local n = 0
	local total = 0

	while true do
		local block, code, err = read_exact(src, 512)
		if nil == block and nil == code then
			break
		end

		if nil ~= code then
			return util.fail_bool(code, err)
		end

		local hdr, hc, he = parse_header(block)
		if nil == hdr then
			if "eof" == he then
				break
			end

			return util.fail_bool(hc, he)
		end

		n = n + 1
		if n > o.max_entries then
			return util.fail_bool(base.LIMIT, "max_entries exceeded")
		end

		local skip_payload = false
		local member, mc, me = util.strip_member(hdr.name, o.strip)
		if nil == member then
			-- ./ 或 strip 后为空: 跳过; 真正越界仍失败
			if "strip removes all path" == me or "empty member path" == me then
				skip_payload = true
			else
				return util.fail_bool(mc, me)
			end
		elseif "symlink" == hdr.kind or "hardlink" == hdr.kind then
			skip_payload = true
		else
			local full, jc, je = util.safe_join(dest, member)
			if nil == full then
				return util.fail_bool(jc, je)
			end

			if "dir" == hdr.kind then
				if not util.ensure_dir(full) then
					return util.fail_bool(base.IO, "mkdir fail")
				end

				skip_payload = true
			else
				if hdr.size > o.max_file then
					return util.fail_bool(base.LIMIT, "max_file exceeded")
				end

				if total + hdr.size > o.max_total then
					return util.fail_bool(base.LIMIT, "max_total exceeded")
				end

				if (not o.overwrite) and util.file_exists(full) then
					return util.fail_bool(base.NOENT, "dest exists")
				end

				if not util.ensure_parent(full) then
					return util.fail_bool(base.IO, "mkdir parent fail")
				end

				local out = io.open(full, "wb")
				if nil == out then
					return util.fail_bool(base.IO, "create file fail")
				end

				local left = hdr.size
				while left > 0 do
					local take = left
					if take > util.CHUNK then
						take = util.CHUNK
					end

					local chunk, c2, e2 = read_exact(src, take)
					if nil == chunk then
						out:close()
						return util.fail_bool(c2 or base.IO, e2 or "read fail")
					end

					out:write(chunk)
					left = left - #chunk
					total = total + #chunk
				end

				out:close()

				if hdr.pad > 0 then
					local sc, se = skip_bytes(src, hdr.pad)
					if klbcode.OK ~= sc then
						return util.fail_bool(sc, se)
					end
				end
			end
		end

		if skip_payload then
			local skip_n = hdr.size + hdr.pad
			if skip_n > 0 then
				local sc, se = skip_bytes(src, skip_n)
				if klbcode.OK ~= sc then
					return util.fail_bool(sc, se)
				end
			end
		end
	end

	return util.ok_bool()
end


-- @brief 单文件解压到路径
reader.extract_file_src = function (src, name, dest, opts)
	if not util.check_path(dest) then
		return util.fail_bool(base.INVAL, "bad dest")
	end

	local o = util.parse_opts(opts)
	local n = 0

	while true do
		local block, code, err = read_exact(src, 512)
		if nil == block and nil == code then
			break
		end

		if nil ~= code then
			return util.fail_bool(code, err)
		end

		local hdr, hc, he = parse_header(block)
		if nil == hdr then
			if "eof" == he then
				break
			end

			return util.fail_bool(hc, he)
		end

		n = n + 1
		if n > o.max_entries then
			return util.fail_bool(base.LIMIT, "max_entries exceeded")
		end

		if match_name(hdr.name, name) then
			if "dir" == hdr.kind then
				return util.fail_bool(base.INVAL, "member is directory")
			end

			if hdr.size > o.max_file then
				return util.fail_bool(base.LIMIT, "max_file exceeded")
			end

			if (not o.overwrite) and util.file_exists(dest) then
				return util.fail_bool(base.NOENT, "dest exists")
			end

			if not util.ensure_parent(dest) then
				return util.fail_bool(base.IO, "mkdir parent fail")
			end

			local out = io.open(dest, "wb")
			if nil == out then
				return util.fail_bool(base.IO, "create file fail")
			end

			local left = hdr.size
			while left > 0 do
				local take = left
				if take > util.CHUNK then
					take = util.CHUNK
				end

				local chunk, c2, e2 = read_exact(src, take)
				if nil == chunk then
					out:close()
					return util.fail_bool(c2 or base.IO, e2 or "read fail")
				end

				out:write(chunk)
				left = left - #chunk
			end

			out:close()

			if hdr.pad > 0 then
				skip_bytes(src, hdr.pad)
			end

			return util.ok_bool()
		end

		local skip_n = hdr.size + hdr.pad
		if skip_n > 0 then
			local sc, se = skip_bytes(src, skip_n)
			if klbcode.OK ~= sc then
				return util.fail_bool(sc, se)
			end
		end
	end

	return util.fail_bool(base.NOENT, "member not found")
end


-- @brief 打开流式成员 (顺序扫描; 关闭前勿再用同一 src 做其它操作)
reader.open_entry_src = function (src, name, opts)
	local o = util.parse_opts(opts)
	local n = 0

	while true do
		local block, code, err = read_exact(src, 512)
		if nil == block and nil == code then
			break
		end

		if nil ~= code then
			return util.fail(code, err)
		end

		local hdr, hc, he = parse_header(block)
		if nil == hdr then
			if "eof" == he then
				break
			end

			return util.fail(hc, he)
		end

		n = n + 1
		if n > o.max_entries then
			return util.fail(base.LIMIT, "max_entries exceeded")
		end

		if match_name(hdr.name, name) then
			if "dir" == hdr.kind then
				return util.fail(base.INVAL, "member is directory")
			end

			local left = hdr.size
			local pad = hdr.pad
			local closed = false

			local entry = {
				name = hdr.name,
				size = hdr.size,
				kind = hdr.kind,
			}

			entry.read = function (want)
				if closed then
					return nil
				end

				want = math.floor(tonumber(want) or 0)
				if want <= 0 then
					return nil
				end

				if left <= 0 then
					return nil
				end

				local take = want
				if take > left then
					take = left
				end

				local chunk, c2, e2 = read_exact(src, take)
				if nil == chunk then
					return nil, c2 or base.IO, e2 or "read fail"
				end

				left = left - #chunk
				return chunk
			end

			entry.readall = function ()
				if closed then
					return util.fail(base.IO, "entry closed")
				end

				if hdr.size > o.max_file then
					return util.fail(base.LIMIT, "max_file exceeded")
				end

				local parts = {}
				while left > 0 do
					local take = left
					if take > util.CHUNK then
						take = util.CHUNK
					end

					local chunk, c2, e2 = entry.read(take)
					if nil ~= c2 then
						return util.fail(c2, e2)
					end

					if nil == chunk then
						break
					end

					parts[#parts + 1] = chunk
				end

				return table.concat(parts), klbcode.OK, ""
			end

			entry.close = function ()
				if closed then
					return
				end

				closed = true
				if left > 0 then
					skip_bytes(src, left)
					left = 0
				end

				if pad > 0 then
					skip_bytes(src, pad)
					pad = 0
				end
			end

			return entry, klbcode.OK, ""
		end

		local skip_n = hdr.size + hdr.pad
		if skip_n > 0 then
			local sc, se = skip_bytes(src, skip_n)
			if klbcode.OK ~= sc then
				return util.fail(sc, se)
			end
		end
	end

	return util.fail(base.NOENT, "member not found")
end


return reader

-- end
