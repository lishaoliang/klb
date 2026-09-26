--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   multipart.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb multipart/form-data 解析
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local io = require("io")
local lfs = require("lfs")
local stringex = require("klbcore.util.stringex")
local query = require("klbcore.klbweb.parser.query")


local multipart = {}


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
multipart.parse_multipart = function (headers, body, upload_dir, max_part)
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
				query.put_multi(form, name, part_body)
			end
		end

		pos = nxt
	end

	return form, files, ""
end


return multipart
