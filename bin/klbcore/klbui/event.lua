--[[
-- Copyright (c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  event.lua
-- @brief klb gui event定义
--   \n C定义文件: ./klb/inc/klbgui/klbui_event.h
-- @note 
--]]

local event = {}


-- 鼠标按键
event.MOUSE_left = 1		-- KLBUI_MOUSE_left, 鼠标左键
event.MOUSE_middle = 2		-- KLBUI_MOUSE_middle, 鼠标中键
event.MOUSE_right = 3		-- KLBUI_MOUSE_right, 鼠标右键
event.MOUSE_x1 = 4			-- KLBUI_MOUSE_x1, 扩展x1
event.MOUSE_x2 = 4			-- KLBUI_MOUSE_x2, 扩展x2


-- 鼠标拖拽事件
event.MOUSEDRAG_start = 1	-- KLBUI_MOUSEDRAG_start, 拖拽事件开始
event.MOUSEDRAG_move = 2	-- KLBUI_MOUSEDRAG_move, 拖拽移动中
event.MOUSEDRAG_end = 3		-- KLBUI_MOUSEDRAG_end, 拖拽事件结束


-- @def   KLBUI_click
-- @brief 左键单击事件: 当左键点击并释放鼠标左键时触发
event.click = 0x401			-- click

-- @def   KLBUI_dblclick
-- @brief 左键双击事件: 当左键双击并释放鼠标左键时触发
event.dblclick = 0x402		-- dblclick


-- @def   KLBUI_mousedown
-- @brief 
event.mousedown = 0x403		-- mousedown


-- @def   KLBUI_mouseenter
-- @brief 
event.mouseenter = 0x404	-- mouseenter


-- @def   KLBUI_mouseleave
-- @brief 
event.mouseleave = 0x405	-- mouseleave


-- @def   KLBUI_mousemove
-- @brief 
event.mousemove = 0x406		-- mousemove


-- @def   KLBUI_mouseout
-- @brief 
event.mouseout = 0x407		-- mouseout

-- @def   KLBUI_mouseover
-- @brief 
event.mouseover = 0x408		-- mouseover


-- @def   KLBUI_mouseup
-- @brief 
event.mouseup = 0x409		-- mouseup


-- @def   KLBUI_mousedrag
-- @brief 鼠标拖拽事件
event.mousedrag = 0x410		-- mousedrag


-- @def   KLBUI_outwindow
-- @brief 在popup窗口外点击事件
event.outwindow = 0x450		-- outwindow


-- @def   KLBUI_onabort
-- @brief 窗口加载崩溃事件: 当窗口加载失败时触发; 完全崩溃, 无法使用
event.onabort = 0x500		-- onabort


-- @def   KLBUI_onerror
-- @brief 窗口加载发生错误事件: 当窗口加载发生错误时触发; 有错误发生, 但不影响使用
event.onerror = 0x501		-- onerror


-- @def   KLBUI_onpredraw
-- @brief 预绘制事件: 在第一次窗口绘制之前(on pre-draw)
event.onpredraw = 0x518		-- onpredraw


-- @def   KLBUI_onpaint
-- @brief 绘制事件: 当需要窗口组件绘制时触发
event.onpaint = 0x520		-- onpaint


-- @def   KLBUI_onparsewindow
-- @brief on parse window: Lua解析窗口(控件), 完成后触发; 包含所有子窗口解析完成
--   部分控件或窗口, 需要在所有 CSS 属性设置完成之后, 再做调整
--   此时传统C/C流程是无此流程的, 这里特加入 KLBUI_onparsewindow/KLBUI_onparsedialog 事件来扩充
event.onparsewindow = 0x580	-- onparsewindow


-- @def   KLBUI_onparsedialog
-- @brief on parse dialog: Lua解析对话框, 完成后触发
event.onparsedialog = 0x581	-- onparsedialog


-- @def   KLBUI_onload
-- @brief 加载事件: 对话框/文档/资源 加载完成时触发
event.onload = 0x601		-- onload


-- @def   KLBUI_onunload
-- @brief 卸载事件: 对话框/文档/资源 卸载时触发
event.onunload = 0x602		-- onunload

-- @def   KLBUI_onresize
-- @brief 窗口尺寸变化事件: 当窗口尺寸变化时触发
event.onresize = 0x603		-- onresize

-- @def   KLBUI_onchange
-- @brief 内容变更事件
event.onchange = 0x604		-- onchange


-- @def   KLBUI_focusin
-- @brief 即将获得焦点事件: 窗口组件即将获得焦点时触发
event.focusin = 0x700		-- focusin

-- @def   KLBUI_focus
-- @brief 聚焦事件: 窗口组件获得焦点时触发
event.focus = 0x701			-- focus

-- @def   KLBUI_focusout
-- @brief 即将失去聚焦事件: 窗口组件即将失去焦点时触发
event.focusout = 0x702		-- focusout

-- @def   KLBUI_blur
-- @brief 失去聚焦事件: 窗口组件失去焦点时触发
event.blur = 0x703			-- blur


--------------------------------
-- @def   KLBUI_event_ctrl
-- @brief 控件自定义事件起始
event.ctrl = 0x3A00


--------------------------------
-- @def   KLBUI_event_user
-- @brief 用户自定事件起始
event.user = 0x4000


-- @brief 转换表
local transform_map = {
	['click'] = event.click,			-- 左键单击事件
	[event.click] = 'click',			-- 左键单击事件

	['dblclick'] = event.dblclick,		-- 左键双击事件
	[event.dblclick] = 'dblclick',		-- 左键双击事件
	
	['mousedown'] = event.mousedown,	-- 
	[event.mousedown] = 'mousedown',	-- 

	['mouseenter'] = event.mouseenter,	--
	[event.mouseenter] = 'mouseenter',	--

	['mouseleave'] = event.mouseleave,	--
	[event.mouseleave] = 'mouseleave',	--

	['mousemove'] = event.mousemove,	--
	[event.mousemove] = 'mousemove',	--

	['mouseout'] = event.mouseout,		--
	[event.mouseout] = 'mouseout',		--
	
	['mouseover'] = event.mouseover,	--
	[event.mouseover] = 'mouseover',	--

	['mouseup'] = event.mouseup,		-- 
	[event.mouseup] = 'mouseup',		-- 	

	['mousedrag'] = event.mousedrag,	-- 鼠标拖拽事件
	[event.mousedrag] = 'mousedrag',
	
	['outwindow'] = event.outwindow,	-- 在popup窗口外点击事件
	[event.outwindow] = 'outwindow',	-- 在popup窗口外点击事件
	
	['onabort'] = event.onabort,		-- 窗口加载崩溃事件
	[event.onabort] = 'onabort',		-- 窗口加载崩溃事件
	
	['onerror'] = event.onerror,		-- 窗口加载发生错误事件
	[event.onerror] = 'onerror',		-- 窗口加载发生错误事件

	['onpredraw'] = event.onpredraw,	-- 预绘制事件: 在第一次窗口绘制之前(on pre-draw)
	[event.onpredraw] = 'onpredraw',	-- 预绘制事件: 在第一次窗口绘制之前(on pre-draw)

	['onpaint'] = event.onpaint,		-- 绘制事件
	[event.onpaint] = 'onpaint',		-- 绘制事件
	
	['onparsewindow'] = event.onparsewindow,	-- Lua解析窗口(控件), 完成后触发
	[event.onparsewindow] = 'onparsewindow',	-- Lua解析窗口(控件), 完成后触发
	
	['onparsedialog'] = event.onparsedialog,	-- Lua解析对话框, 完成后触发
	[event.onparsedialog] = 'onparsedialog',	-- Lua解析对话框, 完成后触发
	
	['onload'] = event.onload,			-- 加载事件
	[event.onload] = 'onload',			-- 加载事件
	
	['onunload'] = event.onunload,		-- 卸载事件
	[event.onunload] = 'onunload',		-- 卸载事件

	['onresize'] = event.onresize,		-- 窗口尺寸变化事件: 当窗口尺寸变化时触发
	[event.onresize] = 'onresize',		-- 窗口尺寸变化事件: 当窗口尺寸变化时触发
	
	['onchange'] = event.onchange,		-- 内容变更事件
	[event.onchange] = 'onchange',		-- 内容变更事件
	
	['focusin'] = event.focusin,		-- 即将获得焦点事件: 窗口组件即将获得焦点时触发
	[event.focusin] = 'focusin',
	
	['focus'] = event.focus,			-- 聚焦事件: 窗口组件获得焦点时触发
	[event.focus] = 'focus',
	
	['focusout'] = event.focusout,		-- 即将失去聚焦事件: 窗口组件即将失去焦点时触发
	[event.focusout] = 'focusout',
	
	['blur'] = event.blur,				-- 失去聚焦事件: 窗口组件失去焦点时触发
	[event.blur] = 'blur',
	
	
	--------------------------------
	-- 控件自定义事件起始
	['ctrl+1'] = event.ctrl + 1,
	[event.ctrl + 1] = 'ctrl+1',
	
	['ctrl+2'] = event.ctrl + 2,
	[event.ctrl + 2] = 'ctrl+2',
	
	['ctrl+3'] = event.ctrl + 3,
	[event.ctrl + 3] = 'ctrl+3',

	['ctrl+4'] = event.ctrl + 4,
	[event.ctrl + 4] = 'ctrl+4',

	['ctrl+5'] = event.ctrl + 5,
	[event.ctrl + 5] = 'ctrl+5',

	['ctrl+6'] = event.ctrl + 6,
	[event.ctrl + 6] = 'ctrl+6',

	['ctrl+7'] = event.ctrl + 7,
	[event.ctrl + 7] = 'ctrl+7',

	['ctrl+8'] = event.ctrl + 8,
	[event.ctrl + 8] = 'ctrl+8',

	['ctrl+9'] = event.ctrl + 9,
	[event.ctrl + 9] = 'ctrl+9',
	
	
	--------------------------------
	-- 用户自定事件起始
	['user+1'] = event.user + 1,
	[event.user + 1] = 'user+1',
	
	['user+2'] = event.user + 2,
	[event.user + 2] = 'user+2',
	
	['user+3'] = event.user + 3,
	[event.user + 3] = 'user+3',

	['user+4'] = event.user + 4,
	[event.user + 4] = 'user+4',
	
	['user+5'] = event.user + 5,
	[event.user + 5] = 'user+5',
	
	['user+6'] = event.user + 6,
	[event.user + 6] = 'user+6',

	['user+7'] = event.user + 7,
	[event.user + 7] = 'user+7',
	
	['user+8'] = event.user + 8,
	[event.user + 8] = 'user+8',
	
	['user+9'] = event.user + 9,
	[event.user + 9] = 'user+9',
}


-- @brief 转换事件
-- @param [in] v[string,number(int)]	事件值/事件字符串
-- @return [string,number(int),nil] 	字符串/数值/nil
event.transform = function (v)
	return transform_map[v]
end


return event
