--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   krand.lua
-- @brief  C krand
--   \n require("krand")
-- @author 李绍良
-- @version 0.1
--]]

local krand = {}



krand.rand = function ()

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
