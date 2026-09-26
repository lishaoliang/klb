--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   sse.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb Server-Sent Events 组包
-- @history 修改历史
--		[2026-09] 创建文件
--]]


local sse = {}


-- @brief 打包一条 SSE 事件
-- @param [in]      data[string]		载荷; 可含换行, 拆成多行 `data:`
-- @param [in]      event[string]		[可选] 事件名
-- @param [in]      id[string]			[可选] 事件 id
-- @return [string]						含结尾空行
sse.pack = function (data, event, id)
	local t = {}
	if "string" == type(event) and "" ~= event then
		t[#t + 1] = "event: " .. event .. "\n"
	end

	if "string" == type(id) and "" ~= id then
		t[#t + 1] = "id: " .. id .. "\n"
	end

	data = string.gsub(data or "", "\r\n", "\n")
	data = string.gsub(data, "\r", "\n")
	if "" == data then
		t[#t + 1] = "data: \n"
	else
		local start = 1
		while true do
			local nxt = string.find(data, "\n", start, true)
			if not nxt then
				t[#t + 1] = "data: " .. string.sub(data, start) .. "\n"
				break
			end

			t[#t + 1] = "data: " .. string.sub(data, start, nxt - 1) .. "\n"
			start = nxt + 1
			if start > #data then
				break
			end
		end
	end

	t[#t + 1] = "\n"
	return table.concat(t)
end


-- @brief 注释行 (心跳)
-- @param [in]      text[string]		[可选] 注释正文
-- @return [string]
sse.comment = function (text)
	return ":" .. (text or "") .. "\n\n"
end


return sse
