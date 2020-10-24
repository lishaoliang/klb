--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief   kg环境(go提供的环境)
-- @author  李绍良
-- @version 0.1
--]]

local kg_ctx = {}


-- @brief 获取Lua预加载库函数
-- @param [in] name[string]	   预加载库名称
-- @return [userdata(lightuserdata)] C函数指针
kg_ctx.get_preload = function (name)
	return ptr
end


-- @brief 获取加载lua脚本文件函数
-- @param [in] name[string]	   加载函数名称
-- @return [userdata(lightuserdata)] C函数指针
kg_ctx.get_loader = function (name)
	return ptr
end

return kg_ctx
