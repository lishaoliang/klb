--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   krand.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C krand
--   \n require("krand")
--   \n C导出文件: ./klb/src_c/klua/klua_util/klua_krand.c
-- @version 0.2
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
-- @param [in]  	len[number(int)]	字符个数
-- @return [string] 字符串
krand.rand_string = function (len)
	return '123456'
end


-- @brief 随机整型数值
-- @param [in]  	len[number(int)]	字符个数: eg. 3 ==> 123
-- @return [number(int)] 整型数值
krand.rand_integer = function (len)
	return 123456
end


-- @brief 随机字节串 (CSPRNG)
-- @param [in]  	len[number(int)]	[可选] 字节数; 缺省内部默认; 0 返回空串
-- @return [string] 随机字节(二进制)
-- @return nil, err[string] 熵源不可用: "csprng unavailable" (禁伪随机回退)
-- @note 输出为二进制, 可能含 '\0'; eg. local s = krand.rand_bytes(16)
krand.rand_bytes = function (len)
	return ''
end


return krand
