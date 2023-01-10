--[[
-- Copyright (c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  event.lua
-- @brief klb gui event定义
--   \n C定义文件: ./klb/inc/klbgui/klbui_event.h
-- @note 
--]]

local event = {}


-- @def   KLBUI_PAINT
-- @brief 绘制事件
event.PAINT = 0x501		-- paint


-- @def   KLBUI_LOAD
-- @brief 加载事件: 对话框即将载入
event.LOAD = 0x601		-- load


-- @def   KLBUI_UNLOAD
-- @brief 卸载事件: 对话框即将关闭
event.UNLOAD = 0x602	-- unload


-- @brief 转换表
local transform_map = {
	['paint'] = event.PAINT,		-- 绘制事件
	[event.PAINT] = 'paint',		-- 绘制事件
	
	['load'] = event.LOAD,			-- 加载事件
	[event.LOAD] = 'load',			-- 加载事件
	
	['unload'] = event.UNLOAD,		-- 卸载事件
	[event.UNLOAD] = 'unload',		-- 卸载事件
}


-- @brief 转换事件
-- @param [in] v[string,number(int)]	事件值/事件字符串
-- @return [string,number(int),nil] 	字符串/数值/nil
event.transform = function (v)
	return transform_map[v]
end


return event
