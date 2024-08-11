--[[
-- Copyright(c) 2021, LGPL v3 All Rights Reserved
--
-- @file    ktime.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  	C ktime
--   \n require("ktime")
--   \n C导出文件: ./klb/src_c/klua/klua_platform/klua_ktime.c
-- @version 0.1
-- @history 修改历史
-- @warning 没有警告
--]]
local ktime = {}


-- @brief 休眠毫秒
-- @param [in]  	ms[number(int)]	休眠毫秒
-- @return 无
-- @note 直接休眠C线程
ktime.sleep = function (ms)
	return
end


-- @brief 休眠纳秒[暂未实现]
-- @param [in]  	ns[number(int)]	休眠纳秒
-- @return 无
-- @note 直接休眠C线程
ktime.sleep_ns = function (ns)
	return
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
-- @note 仅非协程使用
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
-- @note 仅非协程使用
ktime.ticker = function (name, interval, cb)
	return true
end


-- @brief 关闭长期调用定时器
-- @param [in]  	name[string]			名称
-- @return [boolean] true, false
ktime.stop_ticker = function (name)
	return true
end

return ktime
