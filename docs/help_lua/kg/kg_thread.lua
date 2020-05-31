--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @file    kg_thread.lua
-- @brief   go thread
-- @author  李绍良
-- @version 0.1
-- @history 修改历史
--  \n 2019/8/11 0.1 创建文件
-- @warning 没有警告
--]]

local kg_thread = {}


-- @brief 创建线程
-- @param [in] name[string]	   线程名称
-- @param [in] entry[string]   加载入口脚本; eg.'xxx.lua'
-- @param [in] preload[string] 预加载库函数名称; 决定线程创建后使用那些预加载库
-- @param [in] loader[string]  Lua脚本加载函数; 从自定义函数加载Lua脚本; 默认使用fopen从文件加载
-- @return [boolean] true.成功; false.失败
kg_thread.create = function (name, entry, preload, loader)
	return true
	--return false
end


-- @brief 销毁线程
-- @param [in] name[string]   线程名称
-- @return [boolean] true.成功; false.失败
kg_thread.destroy = function (name)
	return true
	--return false
end


-- @brief 向某线程Post消息
-- @param [in] name[string]	  线程名称
-- @param [in] msg[string]    消息
-- @param [in] msgex[string]  扩展消息
-- @param [in] lparam[string] L消息参数
-- @param [in] wparam[string] W消息参数
-- @return [boolean] true.成功; false.失败
kg_thread.post = function (name, msg, msgex, lparam, wparam)
	return true
	--return false
end


return kg_thread
