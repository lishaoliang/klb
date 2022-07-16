--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   krand.lua
-- @brief  C krand
--   \n require("krand")
--   \n C导出文件: ./klb/src_c/klua/klua_util/klua_krand.c
-- @version 0.1
--]]

local krand = {}



-- @brief 随机值
-- @param [in]  	max[number(int)]	[可选](默认0x7fff)值最大
-- @return [number(int)] 随机值
--			[number(int)] 范围[1, N]
-- @note eg. 1. local n, r = krand.rand(10)		1 <= n <= r, r = 10
-- 		2. local n, r = krand.rand()			1 <= n <= r, r = 0x7fff
krand.rand = function (max)
	return 5, 9
end


-- @brief 随机字符串
-- @param [in]  	size[number(int)]	字符个数
-- @return [string] 字符串
krand.rand_string = function (size)
	return '123456'
end


-- @brief 随机整型数值
-- @param [in]  	size[number(int)]	字符个数: eg. 3 ==> 123
-- @return [number(int)] 整型数值
krand.rand_integer = function (size)
	return 123456
end


return krand
