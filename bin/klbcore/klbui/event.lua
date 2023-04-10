--[[
-- Copyright (c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  event.lua
-- @brief klb gui event定义
--   \n C定义文件: ./klb/inc/klbgui/klbui_event.h
-- @note 
--]]

local event = {}


-- @def   KLBUI_click
-- @brief 左键单击事件: 当左键点击并释放鼠标左键时触发
event.click = 0x401			-- click

-- @def   KLBUI_dblclick
-- @brief 左键双击事件: 当左键双击并释放鼠标左键时触发
event.dblclick = 0x402		-- dblclick


-- @def   KLBUI_mousedown
-- @brief 
event.mousedown = 0x403		-- mousedown


-- @def   KLBUI_mouseup
-- @brief 
event.mouseup = 0x409		-- mouseup



-- @def   KLBUI_onabort
-- @brief 窗口加载崩溃事件: 当窗口加载失败时触发; 完全崩溃, 无法使用
event.onabort = 0x500		-- onabort


-- @def   KLBUI_onerror
-- @brief 窗口加载发生错误事件: 当窗口加载发生错误时触发; 有错误发生, 但不影响使用
event.onerror = 0x501		-- onerror


-- @def   KLBUI_onpaint
-- @brief 绘制事件: 当需要窗口组件绘制时触发
event.onpaint = 0x520		-- onpaint


-- @def   KLBUI_onload
-- @brief 加载事件: 对话框/文档/资源 加载完成时触发
event.onload = 0x601		-- onload


-- @def   KLBUI_onunload
-- @brief 卸载事件: 对话框/文档/资源 卸载时触发
event.onunload = 0x602		-- onunload


-- @brief 转换表
local transform_map = {
	['click'] = event.click,			-- 左键单击事件
	[event.click] = 'click',			-- 左键单击事件

	['dblclick'] = event.dblclick,		-- 左键双击事件
	[event.dblclick] = 'dblclick',		-- 左键双击事件
	
	['mousedown'] = event.mousedown,	-- 
	[event.mousedown] = 'mousedown',	-- 

	['mouseup'] = event.mouseup,		-- 
	[event.mouseup] = 'event.mouseup',	-- 
	
	['onabort'] = event.onabort,		-- 窗口加载崩溃事件
	[event.onabort] = 'onabort',		-- 窗口加载崩溃事件
	
	['onerror'] = event.onerror,		-- 窗口加载发生错误事件
	[event.onerror] = 'onerror',		-- 窗口加载发生错误事件

	['onpaint'] = event.onpaint,		-- 绘制事件
	[event.onpaint] = 'onpaint',		-- 绘制事件
	
	['onload'] = event.onload,			-- 加载事件
	[event.onload] = 'onload',			-- 加载事件
	
	['onunload'] = event.onunload,		-- 卸载事件
	[event.onunload] = 'onunload',		-- 卸载事件
}


-- @brief 转换事件
-- @param [in] v[string,number(int)]	事件值/事件字符串
-- @return [string,number(int),nil] 	字符串/数值/nil
event.transform = function (v)
	return transform_map[v]
end


return event
