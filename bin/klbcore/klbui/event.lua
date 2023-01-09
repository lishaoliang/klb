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
event.PAINT		= 0x501		-- paint


-- @def   KLBUI_LOAD
-- @brief 加载事件: 对话框即将载入
event.LOAD		= 0x601		-- load


-- @def   KLBUI_UNLOAD
-- @brief 卸载事件: 对话框即将关闭
event.UNLOAD	= 0x602		-- load



return event
