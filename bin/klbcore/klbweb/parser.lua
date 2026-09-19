--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   parser.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb 请求解析
-- @note   请求行 / headers / query / cookie / form / json / multipart / Range
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local cjson = require("cjson.safe")
local io = require("io")
local lfs = require("lfs")
local stringex = require("klbcore.util.stringex")


local parser = {}


-- @brief 解析请求行
-- @param [in]      head[string]		原始 HTTP 头
-- @return method[string]				大写 METHOD; 失败为 `""`
-- @return path[string]					path (含未过滤的 `..` 等)
-- @return qs[string]					query 原文; 无为 `""`
-- @return ver[string]					HTTP 版本串; 无为 `""`
parser.parse_request = function (head)
	local line = string.match(head or "", "^([^\r\n]+)")
	if not line then
		return "", "", "", ""
	end

	local method, target, ver = string.match(line, "^(%S+)%s+(%S+)%s*(%S*)")
	if not method then
		return "", "", "", ""
	end

	local path = string.match(target, "^([^?]*)") or "/"
	local qs = string.match(target, "%?(.*)$") or ""
	return string.upper(method), path, qs, ver or ""
end


-- @brief 解析请求头
-- @param [in]      head[string]		原始 HTTP 头
-- @return headers[table]				小写键; 同名后写覆盖
parser.parse_headers = function (head)
	local headers = {}
	local skip = true

	for line in string.gmatch(head or "", "[^\r\n]+") do
		if skip then
			skip = false
		else
			local k, v = string.match(line, "^([^:]+):%s*(.*)$")
			if k then
				k = string.lower(stringex.trim(k))
				headers[k] = stringex.trim(v or "")
			end
		end
	end

	return headers
end


local function put_multi(t, k, v)
	if "" == k then
		return
	end

	local old = t[k]
	if nil == old then
		t[k] = v
	elseif "table" == type(old) then
		old[#old + 1] = v
	else
		t[k] = { old, v }
	end
end


-- @brief 解析 query string; 同名多值为数组
-- @param [in]      qs[string]			`?` 后原文
-- @return query[table]					解码后的键值; 无为 `{}`
parser.parse_query = function (qs)
	local query = {}
	if "" == (qs or "") then
		return query
	end

	for pair in string.gmatch(qs, "[^&]+") do
		local k, v = string.match(pair, "^([^=]*)=?(.*)$")
		k = stringex.url_decode(k or "")
		v = stringex.url_decode(v or "")
		put_multi(query, k, v)
	end

	return query
end


-- @brief 解析 Cookie 头
-- @param [in]      headers[table]		小写键头表
-- @return cookies[table]				name -> value; 无为 `{}`
parser.parse_cookies = function (headers)
	local cookies = {}
	local raw = (headers and headers["cookie"]) or ""
	if "" == raw then
		return cookies
	end

	for pair in string.gmatch(raw, "[^;]+") do
		local k, v = string.match(pair, "^%s*([^=]+)=(.*)$")
		if k then
			k = stringex.trim(k)
			v = stringex.trim(v or "")
			if "" ~= k then
				cookies[k] = v
			end
		end
	end

	return cookies
end


-- @brief 解析 application/x-www-form-urlencoded
-- @param [in]      headers[table]		小写键头表
-- @param [in]      body[string]		请求体
-- @return form[table]					同 query; 非该类型为 `{}`
parser.parse_form = function (headers, body)
	local form = {}
	local ctype = string.lower((headers and headers["content-type"]) or "")
	if not string.find(ctype, "application/x-www-form-urlencoded", 1, true) then
		return form
	end

	return parser.parse_query(body or "")
end


-- @brief 解析 JSON 体
-- @param [in]      headers[table]		小写键头表
-- @param [in]      body[string]		请求体
-- @return obj[table]					解码为 table 时; 否则 `{}`
-- @return err[string]					Content-Type 含 json 且体非空但解码失败时为原因; 否则 `""`
parser.parse_json = function (headers, body)
	local ctype = string.lower((headers and headers["content-type"]) or "")
	if not string.find(ctype, "json", 1, true) then
		return {}, ""
	end

	if "" == (body or "") then
		return {}, ""
	end

	local obj = cjson.decode(body)
	if "table" ~= type(obj) then
		return {}, "invalid json"
	end

	return obj, ""
end


local function ensure_dir(dir)
	if "" == (dir or "") then
		return false
	end

	local mode = lfs.attributes(dir, "mode")
	if "directory" == mode then
		return true
	end

	local parent = string.match(dir, "^(.+)/[^/]+$")
	if parent and "" ~= parent then
		if not ensure_dir(parent) then
			return false
		end
	end

	return lfs.mkdir(dir)
end


local function write_file(path, data)
	local f = io.open(path, "wb")
	if not f then
		return false
	end

	f:write(data or "")
	f:close()
	return true
end


local function parse_disposition(disp)
	local name = string.match(disp or "", '[Nn]ame="([^"]*)"')
	if not name then
		name = string.match(disp or "", "[Nn]ame=([^%s;]+)") or ""
	end

	local filename = string.match(disp or "", '[Ff]ilename="([^"]*)"')
	if not filename then
		filename = string.match(disp or "", "[Ff]ilename=([^%s;]+)") or ""
	end

	return name, filename
end


-- @brief 解析 multipart/form-data; 文件落盘, 不把文件内容放入 form
-- @param [in]      headers[table]		小写键头表
-- @param [in]      body[string]		请求体
-- @param [in]      upload_dir[string]	落盘目录; 空则跳过文件部分
-- @param [in]      max_part[number]	单文件上限字节; 0 表示不限制
-- @return form[table]					文本字段
-- @return files[table]					文件描述数组; 项含 name/filename/path/size/mime
-- @return err[string]					失败原因; 成功为 `""`
-- @note 下层 khttp 仍会先收整包; 本函数避免 handler 再持有文件副本
parser.parse_multipart = function (headers, body, upload_dir, max_part)
	local form = {}
	local files = {}
	local ctype = (headers and headers["content-type"]) or ""
	if not string.find(string.lower(ctype), "multipart/form-data", 1, true) then
		return form, files, ""
	end

	local boundary = string.match(ctype, '[Bb]oundary%s*=%s*"([^"]+)"')
	if not boundary then
		boundary = string.match(ctype, "[Bb]oundary%s*=%s*([^;%s]+)")
	end

	if not boundary or "" == boundary then
		return form, files, "missing boundary"
	end

	body = body or ""
	local delim = "--" .. boundary
	local pos = 1
	local n = #body

	while pos <= n do
		local s, e = string.find(body, delim, pos, true)
		if not s then
			break
		end

		pos = e + 1
		if "\r\n" == string.sub(body, pos, pos + 1) then
			pos = pos + 2
		elseif "\n" == string.sub(body, pos, pos) then
			pos = pos + 1
		end

		if "--" == string.sub(body, pos, pos + 1) then
			break
		end

		local nxt = string.find(body, delim, pos, true)
		if not nxt then
			break
		end

		local chunk = string.sub(body, pos, nxt - 1)
		if "\r\n" == string.sub(chunk, -2) then
			chunk = string.sub(chunk, 1, -3)
		elseif "\n" == string.sub(chunk, -1) then
			chunk = string.sub(chunk, 1, -2)
		end

		local head_end = string.find(chunk, "\r\n\r\n", 1, true)
		local sep_len = 4
		if not head_end then
			head_end = string.find(chunk, "\n\n", 1, true)
			sep_len = 2
		end

		if head_end then
			local part_head = string.sub(chunk, 1, head_end - 1)
			local part_body = string.sub(chunk, head_end + sep_len)
			local disp = ""
			local part_mime = "application/octet-stream"

			for line in string.gmatch(part_head, "[^\r\n]+") do
				local lk = string.lower(line)
				if string.find(lk, "content%-disposition:", 1, true) then
					disp = line
				elseif string.find(lk, "content%-type:", 1, true) then
					part_mime = stringex.trim(string.match(line, "^[^:]+:%s*(.*)$") or part_mime)
				end
			end

			local name, filename = parse_disposition(disp)
			if "" ~= filename then
				if 0 < (max_part or 0) and max_part < #part_body then
					return form, files, "part too large"
				end

				local path
				if "" == (upload_dir or "") then
					path = os.tmpname()
				else
					if not ensure_dir(upload_dir) then
						return form, files, "upload_dir create fail"
					end

					local safe = string.gsub(filename, "[^%w%.%-_]", "_")
					if "" == safe then
						safe = "upload"
					end

					path = upload_dir .. "/" .. tostring(os.time()) .. "_" .. safe
				end

				if not write_file(path, part_body) then
					return form, files, "upload write fail"
				end

				files[#files + 1] = {
					name = name,
					filename = filename,
					path = path,
					size = #part_body,
					mime = part_mime,
				}
			elseif "" ~= name then
				put_multi(form, name, part_body)
			end
		end

		pos = nxt
	end

	return form, files, ""
end


-- @brief 解析 Range: bytes=start-end
-- @param [in]      headers[table]		小写键头表
-- @param [in]      size[number]		资源字节数
-- @return rng[table]					`{ first, last }`; 无 Range 为 nil; 非法为 false
parser.parse_range = function (headers, size)
	local raw = (headers and headers["range"]) or ""
	if "" == raw then
		return nil
	end

	local first_s, last_s = string.match(raw, "^bytes=(%d+)%-(%d*)$")
	if not first_s then
		local suffix = string.match(raw, "^bytes=-+(%d+)$")
		if not suffix then
			return false
		end

		local n = tonumber(suffix) or 0
		if n <= 0 or size <= 0 then
			return false
		end

		local first = size - n
		if first < 0 then
			first = 0
		end

		return { first = first, last = size - 1 }
	end

	local first = tonumber(first_s) or 0
	local last
	if "" == last_s then
		last = size - 1
	else
		last = tonumber(last_s) or (size - 1)
	end

	if first < 0 or last < first or size <= first then
		return false
	end

	if size <= last then
		last = size - 1
	end

	return { first = first, last = last }
end


return parser
