--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   kurl.lua
-- @brief  C kurl
--   \n require("kurl")
--   \n C导出文件: ./klb/src_c/klua/klua_net/klua_kurl.c
-- @version 0.1
--]]

local kurl = {}


-- @brief 解析url
-- @param [in] url[string]	URL字符串
-- @return [table] {}
-- @note eg. http://username:password@127.0.0.1:8080/test/test.aspx?name=sviergn&x=true#stuff
kurl.parse = function (url)
	return {
		['schema'] = 'http',				-- http
		['host'] = '127.0.0.1',				-- 
		['port'] = '8080',					-- 
		['path'] = '/test/test.aspx',		-- 
		['query'] = 'name=sviergn&x=true',	-- ?*
		['fragment'] = 'stuff',				-- #*
		['userinfo'] = 'username:password'	-- 
	}
end


return kurl
