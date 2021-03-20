--[[
-- Copyright(c) 2021, LGPL v3 All Rights Reserved
--
-- @file    ktime.lua
-- @brief   k time
-- @author  李绍良
-- @version 0.1
-- @history 修改历史
-- @warning 没有警告
--]]
local ktime = {}


-- @brief 休眠毫秒
-- @param [in]  	ms[number(int)]	休眠毫秒
-- @return 无
ktime.sleep = function (ms)
	
end


-- @brief 休眠纳秒
-- @param [in]  	ns[number(int)]	休眠纳秒
-- @return 无
ktime.sleep_ns = function (ns)
	
end


-- @brief 获取系统滴答数(毫秒)
-- @return [number(int)] 系统滴答数
ktime.tick_count = function ()
	return 123456
end


-- @brief 一次调用定时器
-- @param [in]  	interval[number(int)]	等待间隔(毫秒)
-- @param [in]  	cb[function]			回调函数
-- @return [boolean] true, false
--  cb = function (tc)
--		-- tc[number(int)]	-- 当前系统滴答数(毫秒)
--		return				-- 无返回值
--  end
ktime.timer = function (interval, cb)
	return true
end


-- @brief 长期调用定时器
-- @param [in]  	name[string]			名称
-- @param [in]  	interval[number(int)]	等待间隔(毫秒)
-- @param [in]  	cb[function]			回调函数
-- @return [boolean] true, false
--  cb = function (tc)
--		-- tc[number(int)]	-- 当前系统滴答数(毫秒)
--		return				-- 无返回值
--  end
ktime.ticker = function (name, interval, cb)
	return true
end


return ktime
