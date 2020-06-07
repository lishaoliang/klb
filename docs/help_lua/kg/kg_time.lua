--[[
-- Copyright(c) 2019, All Rights Reserved
-- Created: 2019/8/10
--
-- @file    kg_time.lua
-- @brief   go time
-- @author  李绍良
-- @version 0.1
-- @history 修改历史
--  \n 2019/8/11 0.1 创建文件
-- @warning 没有警告
--]]

local kg_time = {}


-- @brief 休眠毫秒
-- @param [in]  	ms[number(int)]	休眠毫秒
-- @return 无
kg_time.sleep = function (ms)
	
end


-- @brief 休眠纳秒
-- @param [in]  	ns[number(int)]	休眠纳秒
-- @return 无
kg_time.sleep_ns = function (ns)
	
end


-- @brief 获取系统滴答数(毫秒)
-- @return [number(int)] 系统滴答数
kg_time.tick_count = function ()
	return 123456
end


return kg_time
