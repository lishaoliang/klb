--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   kmcache.lua
-- @brief  C kmcache, memory cache
--   \n require("kmcache")
--   \n C导出文件: ./klb/src_c/klua/klua_multithread/klua_kmcache.c
--   \n 进程内部所有共享数据
--   \n 注意: 勿扩大使用范围,最适用于版本等固定内容但又需要全局共享的信息
-- @version 0.1
--]]


local kmcache = {}


-- @brief 设置K/V
-- @param [in] key[string]	key值
-- @param [in] [...] 		任意类型
-- @return 无
kmcache.set = function (key, ...)
	return
end


-- @brief 获取K/V
-- @param [in] key[string]	key值
-- @return [...] 任意类型
kmcache.get = function (key)
	return ...
end


-- @brief 获取数目
-- @return [number(int)] 数目
kmcache.size = function (key)
	return 0
end

-- @brief 清除所有
-- @return 无
kmcache.clear_all = function ()
	return 
end


return kmcache

