--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   ksys.lua
-- @brief  C ksys
--   \n require("ksys")
--   \n C导出文件: ./klb/src_c/klua/klua_util/klua_ksys.c
-- @version 0.1
--]]

local ksys = {}


-- @brief 退出当前环境 lua_State(klua_env_t)
-- @return 无
ksys.exit = function ()
	return
end


-- @brief 将函数参数打包成一个字符串(二进制)
-- @param [in] [...]		任意类型
-- @return [string]	打包后的字符串(二进制)
-- @note eg. local s = ksys.pack_string('a', true, {a=1})
ksys.pack_string = function (...)
	return ''
end


-- @brief 将打包的字符串(二进制) 解包
-- @param [in] str[string]	打包的字符串
-- @return [...] 任意类型
-- @note eg. local a, b, c = ksys.unpack('')
ksys.unpack = function (str)
	return ...
end


-- @brief 获取 当前环境 lua_State(klua_env_t) 的全局参数
-- @return [...] 任意类型
-- @note 来源于 kthread.start() 的第4个参数开始
--  eg. local a, b, c = ksys.get_arg()
ksys.get_arg = function ()
	return ...
end


return ksys
