--[[
-- Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   kmcache.lua
-- @author 随风(https://gitee.com/klua/klb)
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


-- @brief 获取条目总数
-- @return [number(int)] 总数
kmcache.size = function ()
	return 0
end

-- @brief 清除所有
-- @return 无
kmcache.clear = function ()
	return 
end


return kmcache

