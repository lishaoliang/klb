--[[
-- Copyright(c) 2019, All Rights Reserved
-- Created: 2019/8/10
--
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
-- @param [in]  	name[string]	线程名称; ""表示不记名线程
-- @param [in]		loader[string]	加载入口: require(loader)
-- @return [boolean] true.成功; false.失败
kg_thread.create = function (name, loader)
	return true
	--return false
end


-- @brief 销毁线程
-- @param [in]  	name[string]	线程名称
-- @return 无
kg_thread.destroy = function (name)
	
end


-- @brief 向某线程Post消息
-- @param [in]  	name[string]	线程名称
-- @param [in]  	msg[string]		消息
-- @param [in]  	lparam[string]	参数1
-- @param [in]  	wparam[string]	参数2
-- @param [in]  	ptr[string]		指针参数: void*
-- @return [boolean] true.成功; false.失败
kg_thread.post = function (name, msg, lparam, wparam, ptr)
	return true
	--return false
end


return kg_thread
