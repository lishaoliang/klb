--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   json.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbweb JSON 体解析
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local cjson = require("cjson.safe")


local json = {}


-- @brief 解析 JSON 体
-- @param [in]      headers[table]		小写键头表
-- @param [in]      body[string]		请求体
-- @return obj[any]						解码值 (table/标量/cjson.null); 非 json 类型或空体为 `{}`
-- @return err[string]					Content-Type 含 json 且体非空但解码失败时为原因; 否则 `""`
json.parse_json = function (headers, body)
	local ctype = string.lower((headers and headers["content-type"]) or "")
	if not string.find(ctype, "json", 1, true) then
		return {}, ""
	end

	if "" == (body or "") then
		return {}, ""
	end

	local obj = cjson.decode(body)
	-- decode 失败为 nil; 合法 JSON 标量 (number/string/boolean/null→cjson.null) 亦接受
	if nil == obj then
		return {}, "invalid json"
	end

	return obj, ""
end


return json
