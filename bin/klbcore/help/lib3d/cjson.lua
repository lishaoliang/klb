--[[
-- Copyright (c) 2022, GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  cjson.lua
-- @brief   内置库 require("cjson"), 函数说明
--   \n require("cjson")
--   \n require("cjson.safe")
--   \n 内置库 require("cjson.safe"), 不抛出异常版本
--   \n C导出文件: ./klb/src_c/klua/lua-cjson-2.1.0/lua_cjson.c
-- @version 2.1.0
-- @author  Mark Pulford
--]]

local cjson = {}



-- @brief 将对象编码成json
-- @param [in] t[table] table对象
-- @return [string] json字符串
cjson.encode = function (t)
	return ''
end


-- @brief 将json字符串解码成table对象
-- @param [in] json[string] json字符串
-- @return [table] table对象
-- @note 注意此函数会抛出异常, 调用方法如下
-- local ret, obj = pcall(cjson.decode, txt)
cjson.decode = function (json)
	return {}
end


return cjson
