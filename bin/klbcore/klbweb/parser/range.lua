--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   range.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb Range 头解析
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local range = {}


-- @brief 解析 Range: bytes=start-end
-- @param [in]      headers[table]		小写键头表
-- @param [in]      size[number]		资源字节数
-- @return rng[table]					`{ first, last }`; 无 Range 为 nil; 非法为 false
range.parse_range = function (headers, size)
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


return range
