--[[
-- Copyright(c) 2022, LGPL All Rights Reserved
-- @file   kenv.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  C kenv
--   \n require("kenv")
--   \n C导出文件: ./klb/src_c/klua/klua_util/klua_kenv.c
--   \n klua_env_t 环境相关 API
-- @version 0.1
--]]

local kenv = {}


-- @brief 获取应用根路径 (可执行/工作目录基准)
-- @return [string] path
-- @note 配置 klbcore package.path 时常用: kenv.base_path() .. '/klbcore/'
kenv.base_path = function ()
	return './'
end


-- @brief 获取默认配置/偏好路径
-- @param [in]	org[string]		组织名
-- @param [in]	app[string]		应用名
-- @return [string] pref_path
kenv.pref_path = function (org, app)
	return ''
end


-- @brief 获取当前 env 全局参数 (与 ksys.get_args 相同)
-- @return [...] 任意类型
-- @note 来源于入口或 kthread.start 第4个参数起
kenv.get_args = function ()
	return ...
end


-- @brief 获取当前 lua 环境名称
-- @return [string] name
-- @note kthread.start 返回的名称
kenv.get_name = function ()
	return ''
end


-- @brief 退出当前 env (同 ksys.exit)
-- @return 无
kenv.exit = function ()
	return
end


-- @brief 是否已退出 (同 ksys.is_exit)
-- @return [boolean]
kenv.is_exit = function ()
	return false
end


-- @brief 当前 env 滴答数 (毫秒, 与系统 tick 可能有小偏差)
-- @return [number(int)]
kenv.tick_count = function ()
	return 0
end


-- @brief 立即刷新 env 滴答数
-- @return 无
kenv.update_tick_count = function ()
	return
end


-- @brief 设置 loop_once 单次 sleep 上限 (毫秒)
-- @param [in]	ms[number(int)]
-- @return 无
kenv.set_loop_sleep = function (ms)
	return
end


-- @brief 获取 loop_once 单次 sleep 上限 (毫秒)
-- @return [number(int)]
kenv.get_loop_sleep = function ()
	return 0
end


return kenv
