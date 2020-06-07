--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @file   kgo.lua
-- @brief  C kgo
-- @author 李绍良
-- @version 0.1
--]]


-- @brief 默认加载的全局消息处理函数,Lua虚拟机环境
-- @param [in] msg[string]    消息
-- @param [in] msgex[string]  扩展消息
-- @param [in] lparam[string] L消息参数
-- @param [in] wparam[string] W消息参数
-- @param [in] ptr[userdata(lightuserdata)] 指针参数
-- @return 0
kgo = function (msg, msgex, lparam, wparam, ptr)
	return 0
end
